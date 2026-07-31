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

static uint64_t getMLIRTypeSize(::mlir::Type ty, unsigned pointerSize) {
  if (ty.isIntOrFloat()) {
    return (ty.getIntOrFloatBitWidth() + 7) / 8;
  }
  // Fat pointers (Closure, Any) are 2x pointer size
  if (::llvm::isa<::moksha::IR::AnyType>(ty) ||
      ::llvm::isa<::moksha::IR::ClosureType>(ty)) {
    return 2 * pointerSize;
  }
  // Standard opaque pointers
  if (::llvm::isa<::moksha::IR::PointerType>(ty) ||
      ::llvm::isa<::moksha::IR::SliceType>(ty) ||
      ::llvm::isa<::moksha::IR::MapType>(ty) ||
      ::llvm::isa<::moksha::IR::PromiseType>(ty) ||
      ::llvm::isa<::mlir::LLVM::LLVMPointerType>(ty)) {
    return pointerSize;
  }
  if (auto arrTy = ::llvm::dyn_cast_or_null<::moksha::IR::ArrayType>(ty)) {
    return getMLIRTypeSize(arrTy.getElementType(), pointerSize) *
           arrTy.getSize();
  }
  if (auto stTy = ::llvm::dyn_cast_or_null<::mlir::LLVM::LLVMStructType>(ty)) {
    uint64_t size = 0;
    for (auto elem : stTy.getBody()) {
      size += getMLIRTypeSize(elem, pointerSize);
    }
    return size;
  }
  return pointerSize;
}

TypeLowering::TypeLowering(::mlir::MLIRContext &ctx, unsigned ptrSize)
    : context(ctx), pointerSize(ptrSize) {}

::mlir::Type TypeLowering::lowerHIRType(const hir::HIRType &type) const {
  switch (type.getKind()) {
  case hir::TypeKind::Void:
    return ::mlir::NoneType::get(&context);

  case hir::TypeKind::Bool:
    return ::mlir::IntegerType::get(&context, 1);

  case hir::TypeKind::Int: {
    auto &intType = static_cast<const hir::HIRIntType &>(type);
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

    ::mlir::Type retTy = ::mlir::NoneType::get(&context);
    if (closureType.getReturnType() &&
        closureType.getReturnType()->getKind() != hir::TypeKind::Void) {
      retTy = lowerHIRType(*closureType.getReturnType());
    }

    return ::moksha::IR::ClosureType::get(&context, retTy, paramTys);
  }

  case hir::TypeKind::String:
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
    if (structType.getName().empty()) {
      llvm::SmallVector<::mlir::Type, 4> fieldTypes;
      thread_local llvm::DenseSet<llvm::StringRef> activeFlattening;
      std::function<void(const hir::StructType &)> flattenFields =
          [&](const hir::StructType &st) {
            if (!activeFlattening.insert(st.getName()).second) {
              return;
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
              bool emitOpaque = false;
              const hir::HIRType *checkTy = field;
              if (auto *ptrTy =
                      llvm::dyn_cast_or_null<hir::PointerType>(checkTy)) {
                checkTy = ptrTy->getPointee();
              }
              if (auto *st = llvm::dyn_cast_or_null<hir::StructType>(checkTy)) {
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

    auto llvmStruct = ::mlir::LLVM::LLVMStructType::getIdentified(
        &context, structType.getName());

    if (!llvmStruct.isOpaque()) {
      return llvmStruct;
    }
    thread_local llvm::DenseSet<llvm::StringRef> activeStructs;
    if (!activeStructs.insert(structType.getName()).second) {
      return llvmStruct;
    }

    llvm::SmallVector<::mlir::Type, 4> fieldTypes;
    std::function<void(const hir::StructType &)> flattenFields =
        [&](const hir::StructType &st) {
          for (const auto *parentTy : st.getParentTypes()) {
            const hir::HIRType *resolvedParent = parentTy;
            if (auto *ptrTy =
                    llvm::dyn_cast_or_null<hir::PointerType>(resolvedParent)) {
              resolvedParent = ptrTy->getPointee();
            } else if (auto *refTy = llvm::dyn_cast_or_null<hir::ReferenceType>(
                           resolvedParent)) {
              resolvedParent = refTy->getInner();
            }
            if (auto *parentSt =
                    llvm::dyn_cast_or_null<hir::StructType>(resolvedParent)) {
              flattenFields(*parentSt);
            }
          }

          for (auto *field : st.getFields()) {
            bool emitOpaque = false;
            const hir::HIRType *checkTy = field;
            if (auto *ptrTy =
                    llvm::dyn_cast_or_null<hir::PointerType>(checkTy)) {
              checkTy = ptrTy->getPointee();
            }
            if (auto *childSt =
                    llvm::dyn_cast_or_null<hir::StructType>(checkTy)) {
              if (activeStructs.count(childSt->getName())) {
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
        };

    flattenFields(structType);
    if (llvmStruct.isOpaque()) {
      [[maybe_unused]] auto _ =
          llvmStruct.setBody(fieldTypes, structType.isPacked());
    }

    activeStructs.erase(structType.getName());
    return llvmStruct;
  }
  case hir::TypeKind::Union: {
    auto &unionType = static_cast<const hir::UnionType &>(type);
    ::mlir::Type largestMemberTy = ::mlir::NoneType::get(&context);
    uint64_t maxSize = 0;

    for (auto *field : unionType.getFields()) {
      ::mlir::Type mlirTy = lowerHIRType(*field);
      uint64_t memberSize = getMLIRTypeSize(mlirTy, pointerSize);

      if (memberSize > maxSize || maxSize == 0) {
        maxSize = memberSize;
        largestMemberTy = mlirTy;
      }
    }

    if (unionType.getName().empty()) {
      return ::mlir::LLVM::LLVMStructType::getLiteral(&context,
                                                      {largestMemberTy});
    }

    auto llvmUnion = ::mlir::LLVM::LLVMStructType::getIdentified(
        &context, unionType.getName());

    if (!llvmUnion.isOpaque()) {
      return llvmUnion;
    }

    thread_local llvm::DenseSet<llvm::StringRef> activeUnions;
    if (!activeUnions.insert(unionType.getName()).second) {
      return llvmUnion;
    }

    if (llvmUnion.isOpaque()) {
      [[maybe_unused]] auto _ = llvmUnion.setBody({largestMemberTy}, false);
    }
    activeUnions.erase(unionType.getName());
    return llvmUnion;
  }
  case hir::TypeKind::Nullable: {
    auto &nullType = static_cast<const hir::HIRNullableType &>(type);

    // NULL POINTER OPTIMIZATION (NPO)
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
      return lowerHIRType(*innerHir);
    }

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
    return false;

  case hir::TypeKind::Struct: {
    auto &structType = static_cast<const hir::StructType &>(type);
    return !structType.isRefClass();
  }

  default:
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
