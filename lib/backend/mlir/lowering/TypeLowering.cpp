#include "moksha/Backend/MLIR/TypeLowering.h"
#include "mlir/Dialect/LLVMIR/LLVMTypes.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "moksha/Dialect/MokshaDialect.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRValue.h"
#include "llvm/ADT/DenseSet.h"

namespace moksha {
namespace backend {
namespace mlir {

static uint64_t getMLIRTypeSize(::mlir::Type ty) {
  if (ty.isIntOrFloat()) {
    return (ty.getIntOrFloatBitWidth() + 7) / 8;
  }

  // FIXED: Explicitly size Fat Pointers (Data + Metadata/VTable/Length)
  if (::llvm::isa<::moksha::IR::AnyType>(ty) ||
      ::llvm::isa<::moksha::IR::ClosureType>(ty)) {
    return 16;
  }

  if (::llvm::isa<::moksha::IR::PointerType>(ty) ||
      ::llvm::isa<::moksha::IR::SliceType>(ty) ||
      ::llvm::isa<::mlir::LLVM::LLVMPointerType>(ty)) {
    return 8; // 64-bit pointers
  }
  if (auto arrTy = ::llvm::dyn_cast<::moksha::IR::ArrayType>(ty)) {
    return getMLIRTypeSize(arrTy.getElementType()) * arrTy.getSize();
  }
  if (auto stTy = ::llvm::dyn_cast<::mlir::LLVM::LLVMStructType>(ty)) {
    uint64_t size = 0;
    for (auto elem : stTy.getBody()) {
      size += getMLIRTypeSize(elem);
    }
    return size;
  }
  return 8; // Default fallback for closures/slices
}

TypeLowering::TypeLowering(::mlir::MLIRContext &ctx) : context(ctx) {}

::mlir::Type TypeLowering::lowerHIRType(const hir::HIRType &type) const {
  switch (type.getKind()) {
  case hir::TypeKind::Void:
    return ::mlir::NoneType::get(&context);

  case hir::TypeKind::Bool:
    // Booleans should be 1-bit integers in MLIR to prevent wasted memory
    return ::mlir::IntegerType::get(&context, 1);

  case hir::TypeKind::Int: {
    auto &intType = static_cast<const hir::HIRIntType &>(type);

    // Explicitly use the global ::mlir namespace
    ::mlir::IntegerType::SignednessSemantics signedness =
        intType.isSigned() ? ::mlir::IntegerType::Signed
                           : ::mlir::IntegerType::Unsigned;

    return ::mlir::IntegerType::get(&context, intType.getWidth(), signedness);
  }

  case hir::TypeKind::Float: {
    auto &floatType = static_cast<const hir::HIRFloatType &>(type);
    switch (floatType.getWidth()) {
    case 8:
      // Quarter precision (Requires modern MLIR/LLVM)
      return ::mlir::Float8E5M2Type::get(&context);
    case 16:
      // Half precision
      return ::mlir::Float16Type::get(&context);
    case 32:
      // Single precision
      return ::mlir::Float32Type::get(&context);
    case 64:
      // Double precision
      return ::mlir::Float64Type::get(&context);
    default:
      return ::mlir::Float32Type::get(&context);
    }
  }
  case hir::TypeKind::Decimal: {
    auto &decType = static_cast<const hir::HIRDecimalType &>(type);
    return ::moksha::IR::DecimalType::get(&context, decType.getPrecision(),
                                          decType.getScale());
  }
  case hir::TypeKind::Pointer: {
    auto &ptrType = static_cast<const hir::PointerType &>(type);
    ::mlir::Type pointeeTy = lowerHIRType(*ptrType.getPointee());
    return ::moksha::IR::PointerType::get(&context, pointeeTy);
  }
  case hir::TypeKind::Reference: {
    auto &refType = static_cast<const hir::ReferenceType &>(type);
    ::mlir::Type innerTy = lowerHIRType(*refType.getInner());
    return ::moksha::IR::PointerType::get(&context, innerTy);
  }
  case hir::TypeKind::Function: {
    auto &funcType = static_cast<const hir::FunctionType &>(type);
    llvm::SmallVector<::mlir::Type, 4> paramTys;
    for (const auto *p : funcType.getParamTypes()) {
      paramTys.push_back(lowerHIRType(*p));
    }

    llvm::SmallVector<::mlir::Type, 1> retTys;
    if (funcType.getReturnType() &&
        funcType.getReturnType()->getKind() != hir::TypeKind::Void) {
      retTys.push_back(lowerHIRType(*funcType.getReturnType()));
    }
    return ::mlir::FunctionType::get(&context, paramTys, retTys);
  }
  case hir::TypeKind::Closure: {
    auto &closureType = static_cast<const hir::HIRClosureType &>(type);

    llvm::SmallVector<::mlir::Type, 4> paramTys;
    for (const auto *p : closureType.getParamTypes()) {
      paramTys.push_back(lowerHIRType(*p));
    }

    // Default to NoneType (void) if no return type is specified
    ::mlir::Type retTy = ::mlir::NoneType::get(&context);
    if (closureType.getReturnType() &&
        closureType.getReturnType()->getKind() != hir::TypeKind::Void) {
      retTy = lowerHIRType(*closureType.getReturnType());
    }

    return ::moksha::IR::ClosureType::get(&context, retTy, paramTys);
  }

  case hir::TypeKind::String:
    // Strings can remain opaque byte pointers
    return ::moksha::IR::PointerType::get(
        &context, ::mlir::IntegerType::get(&context, 8));
  case hir::TypeKind::Mut:
    return lowerHIRType(*static_cast<const hir::HIRMutType &>(type).getInner());
  case hir::TypeKind::View:
    return lowerHIRType(
        *static_cast<const hir::HIRViewType &>(type).getInner());
  case hir::TypeKind::Lock:
    return lowerHIRType(
        *static_cast<const hir::HIRLockType &>(type).getInner());
  case hir::TypeKind::Const:
    return lowerHIRType(
        *static_cast<const hir::HIRConstType &>(type).getInner());
  case hir::TypeKind::Volatile:
    return lowerHIRType(
        *static_cast<const hir::HIRVolatileType &>(type).getInner());
  case hir::TypeKind::Weak:
    return lowerHIRType(
        *static_cast<const hir::HIRWeakType &>(type).getInner());
  case hir::TypeKind::Any:
    return ::moksha::IR::AnyType::get(&context);
  case hir::TypeKind::Array: {
    auto &arrType = static_cast<const hir::ArrayType &>(type);
    ::mlir::Type elemType = lowerHIRType(*arrType.getElementType());
    return ::moksha::IR::ArrayType::get(&context, elemType, arrType.getSize());
  }
  case hir::TypeKind::Slice: {
    auto &sliceType = static_cast<const hir::SliceType &>(type);
    ::mlir::Type elemType = lowerHIRType(*sliceType.getElementType());
    return ::moksha::IR::SliceType::get(&context, elemType);
  }
  case hir::TypeKind::Struct: {
    auto &structType = static_cast<const hir::StructType &>(type);

    // 1. Handle Literal (Anonymous) Structs directly
    if (structType.getName().empty()) {
      llvm::SmallVector<::mlir::Type, 4> fieldTypes;

      // Prevent Infinite Recursion on Anonymous Types
      thread_local llvm::DenseSet<llvm::StringRef> activeFlattening;

      std::function<void(const hir::StructType &)> flattenFields =
          [&](const hir::StructType &st) {
            if (!activeFlattening.insert(st.getName()).second) {
              return; // Break the cycle safely!
            }
            for (const auto *parentTy : st.getParentTypes()) {
              const hir::HIRType *resolvedParent = parentTy;
              if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(
                      resolvedParent)) {
                resolvedParent = ptrTy->getPointee();
              } else if (auto *refTy =
                             llvm::dyn_cast_or_null<hir::ReferenceType>(
                                 resolvedParent)) {
                resolvedParent = refTy->getInner();
              }
              if (auto *parentSt =
                      llvm::dyn_cast_or_null<hir::StructType>(resolvedParent)) {
                flattenFields(*parentSt);
              }
            }
            for (auto *field : st.getFields()) {
              // --- [CRITICAL FIX] SAFE RECURSION CHECK ---
              // If the field is a pointer to a struct we are actively building,
              // immediately return an opaque pointer instead of recursing!
              bool emitOpaque = false;
              const hir::HIRType *checkTy = field;
              if (auto *ptrTy = llvm::dyn_cast<hir::PointerType>(checkTy)) {
                checkTy = ptrTy->getPointee();
              }
              if (auto *st = llvm::dyn_cast<hir::StructType>(checkTy)) {
                if (activeFlattening.count(st->getName())) {
                  emitOpaque = true;
                }
              }

              if (emitOpaque) {
                fieldTypes.push_back(::moksha::IR::PointerType::get(
                    &context, ::mlir::IntegerType::get(&context, 8)));
              } else {
                fieldTypes.push_back(lowerHIRType(*field));
              }
            }
            activeFlattening.erase(st.getName());
          };

      flattenFields(structType);
      return ::mlir::LLVM::LLVMStructType::getLiteral(&context, fieldTypes,
                                                      structType.isPacked());
    }

    // 2. Handle Identified (Named) Structs
    auto llvmStruct = ::mlir::LLVM::LLVMStructType::getIdentified(
        &context, structType.getName());

    // If it's already fully computed, return it immediately.
    if (!llvmStruct.isOpaque()) {
      return llvmStruct;
    }

    // --- Prevent Infinite Recursion on Self-Referential Types ---
    thread_local llvm::DenseSet<llvm::StringRef> activeStructs;
    if (!activeStructs.insert(structType.getName()).second) {
      // We are already actively lowering this struct higher up the call stack!
      // Return the opaque struct pointer to safely break the loop.
      return llvmStruct;
    }

    llvm::SmallVector<::mlir::Type, 4> fieldTypes;

    // Flatten Inheritance Hierarchy for MLIR/LLVM Layout
    std::function<void(const hir::StructType &)> flattenFields =
        [&](const hir::StructType &st) {
          // 1. Inject Parent Fields (Top-Down Memory Layout)
          for (const auto *parentTy : st.getParentTypes()) {
            const hir::HIRType *resolvedParent = parentTy;

            // Unwrap base class pointers if necessary (e.g. '*Entity')
            if (auto *ptrTy =
                    llvm::dyn_cast_or_null<hir::PointerType>(resolvedParent)) {
              resolvedParent = ptrTy->getPointee();
            } else if (auto *refTy = llvm::dyn_cast_or_null<hir::ReferenceType>(
                           resolvedParent)) {
              resolvedParent = refTy->getInner();
            }

            if (auto *parentSt =
                    llvm::dyn_cast_or_null<hir::StructType>(resolvedParent)) {
              flattenFields(*parentSt); // Recurse for multi-level inheritance
            }
          }

          // 2. Inject Subclass Fields
          for (auto *field : st.getFields()) {
            // --- [CRITICAL FIX] SAFE RECURSION CHECK ---
            // If the field is a pointer to a struct we are actively building,
            // immediately return an opaque pointer instead of recursing!
            bool emitOpaque = false;
            const hir::HIRType *checkTy = field;
            if (auto *ptrTy = llvm::dyn_cast<hir::PointerType>(checkTy)) {
              checkTy = ptrTy->getPointee();
            }
            if (auto *childSt = llvm::dyn_cast<hir::StructType>(checkTy)) {
              if (activeStructs.count(childSt->getName())) {
                emitOpaque = true;
              }
            }

            if (emitOpaque) {
              fieldTypes.push_back(::moksha::IR::PointerType::get(
                  &context,
                  ::mlir::IntegerType::get(&context, 8))); // Opaque Pointer
            } else {
              fieldTypes.push_back(lowerHIRType(*field));
            }
          }
        };

    flattenFields(structType);

    // Apply the computed body to the opaque struct
    if (llvmStruct.isOpaque()) {
      [[maybe_unused]] auto _ =
          llvmStruct.setBody(fieldTypes, structType.isPacked());
    }

    // Clean up the active tracker as we exit
    activeStructs.erase(structType.getName());

    return llvmStruct;
  }
  case hir::TypeKind::Union: {
    auto &unionType = static_cast<const hir::UnionType &>(type);

    // 1. Find the largest member to act as the memory backing
    ::mlir::Type largestMemberTy = ::mlir::NoneType::get(&context);
    uint64_t maxSize = 0;

    for (auto *field : unionType.getFields()) {
      ::mlir::Type mlirTy = lowerHIRType(*field);
      uint64_t memberSize = getMLIRTypeSize(mlirTy);

      if (memberSize > maxSize || maxSize == 0) {
        maxSize = memberSize;
        largestMemberTy = mlirTy;
      }
    }

    // 2. Handle Literal (Anonymous) Unions
    if (unionType.getName().empty()) {
      return ::mlir::LLVM::LLVMStructType::getLiteral(&context,
                                                      {largestMemberTy});
    }

    // 3. Handle Identified (Named) Unions
    auto llvmUnion = ::mlir::LLVM::LLVMStructType::getIdentified(
        &context, unionType.getName());

    // If it's already computed, return it
    if (!llvmUnion.isOpaque()) {
      return llvmUnion;
    }

    // Prevent Infinite Recursion on self-referential unions
    thread_local llvm::DenseSet<llvm::StringRef> activeUnions;
    if (!activeUnions.insert(unionType.getName()).second) {
      return llvmUnion;
    }

    // Apply the computed body (just the single largest field)
    if (llvmUnion.isOpaque()) {
      [[maybe_unused]] auto _ = llvmUnion.setBody({largestMemberTy}, false);
    }

    activeUnions.erase(unionType.getName());
    return llvmUnion;
  }
  case hir::TypeKind::Nullable: {
    auto &nullType = static_cast<const hir::HIRNullableType &>(type);

    // ========================================================================
    // NULL POINTER OPTIMIZATION (NPO)
    // ========================================================================
    const hir::HIRType *innerHir = nullType.getInner();
    hir::TypeKind innerKind = innerHir->getKind();

    if (innerKind == hir::TypeKind::Slice ||
        innerKind == hir::TypeKind::String ||
        innerKind == hir::TypeKind::Closure ||
        innerKind == hir::TypeKind::Any ||
        innerKind == hir::TypeKind::Pointer ||
        innerKind == hir::TypeKind::Map ||
        innerKind == hir::TypeKind::Promise ||
        (innerKind == hir::TypeKind::Struct &&
         static_cast<const hir::StructType *>(innerHir)->isRefClass())) {

      // Return the exact same physical memory layout as the inner type!
      return lowerHIRType(*innerHir);
    }

    // Fallback: If it's a primitive like 'int?', generate the standard wrapper
    ::mlir::Type inner = lowerHIRType(*innerHir);
    return ::moksha::IR::NullableType::get(&context, inner);
  }
  case hir::TypeKind::Null: {
    return ::moksha::IR::NullType::get(&context);
  }
  case hir::TypeKind::Map: {
    auto &mapType = static_cast<const hir::HIRMapType &>(type);
    ::mlir::Type keyType = lowerHIRType(*mapType.getKeyType());
    ::mlir::Type valType = lowerHIRType(*mapType.getValueType());
    return ::moksha::IR::MapType::get(&context, keyType, valType);
  }
  case hir::TypeKind::Promise: {
    auto &promType = static_cast<const hir::HIRPromiseType &>(type);
    ::mlir::Type inner = lowerHIRType(*promType.getInnerType());
    return ::moksha::IR::PromiseType::get(&context, inner);
  }
  default:
    return ::mlir::NoneType::get(&context);
  }
}

::mlir::Type TypeLowering::lowerMIRValueType(const mir::MIRValue &value) const {
  if (!value.getType())
    return ::mlir::NoneType::get(&context);
  return lowerHIRType(*value.getType());
}

bool TypeLowering::isTriviallyCopyable(const hir::HIRType &type) const {
  switch (type.getKind()) {
  case hir::TypeKind::String:
  case hir::TypeKind::Array:
  case hir::TypeKind::Map:
  case hir::TypeKind::Slice:
  case hir::TypeKind::Closure:
  case hir::TypeKind::Any:
  case hir::TypeKind::Promise:
    // Heap-allocated and managed types cannot be trivially copied.
    // They enforce Move semantics (or ARC if marked 'shared').
    return false;

  case hir::TypeKind::Struct: {
    // A 'ref class' is heap allocated and managed (Move/ARC)
    // A standard 'class' or 'struct' is just stack data, so it can be trivially
    // copied
    auto &structType = static_cast<const hir::StructType &>(type);
    return !structType.isRefClass();
  }

  default:
    // Primitives (int, float, bool), Enums, and raw Pointers (*mut, *view)
    // are just register-sized values and can be trivially copied.
    return true;
  }
}

bool TypeLowering::requiresOwnership(const hir::HIRType &type) const {
  if (type.getKind() == hir::TypeKind::String ||
      type.getKind() == hir::TypeKind::Closure ||
      type.getKind() == hir::TypeKind::Any ||
      type.getKind() == hir::TypeKind::Map ||
      type.getKind() == hir::TypeKind::Promise ||
      type.getKind() == hir::TypeKind::Slice) {
    return true;
  }
  if (type.getKind() == hir::TypeKind::Struct) {
    auto &structType = static_cast<const hir::StructType &>(type);
    return structType.isRefClass();
  }
  return false;
}

} // namespace mlir
} // namespace backend
} // namespace moksha
