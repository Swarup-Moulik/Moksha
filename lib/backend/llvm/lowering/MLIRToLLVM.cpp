#include "moksha/Backend/LLVM/MLIRToLLVM.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h"
#include "mlir/Conversion/LLVMCommon/ConversionTarget.h"
#include "mlir/Conversion/LLVMCommon/Pattern.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"
#include "moksha/Dialect/MokshaDialect.h"
#include "moksha/Dialect/MokshaOps.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"
#include <unordered_set>

namespace moksha {

namespace {

class MokshaToLLVMTypeConverter : public mlir::LLVMTypeConverter {
public:
  MokshaToLLVMTypeConverter(mlir::MLIRContext *ctx)
      : mlir::LLVMTypeConverter(ctx) {
    // Hardware Promote f8 to f16 for native LLVM support
    addConversion([&](mlir::Float8E5M2Type type) {
      return mlir::Float16Type::get(type.getContext());
    });
    addConversion([&](mlir::Float8E4M3FNType type) {
      return mlir::Float16Type::get(type.getContext());
    });
    // Core Pointers and Arrays
    addConversion([&](IR::PointerType type) {
      return mlir::LLVM::LLVMPointerType::get(type.getContext());
    });
    addConversion([&](IR::ArrayType type) {
      return mlir::LLVM::LLVMArrayType::get(convertType(type.getElementType()),
                                            type.getSize());
    });
    addConversion([&](IR::PromiseType type) -> mlir::Type {
      return mlir::LLVM::LLVMPointerType::get(ctx);
    });
    addConversion([&](IR::SliceType type) {
      return mlir::LLVM::LLVMPointerType::get(type.getContext());
    });
    addConversion([&](IR::AnyType type) {
      return mlir::LLVM::LLVMStructType::getLiteral(
          type.getContext(),
          {mlir::LLVM::LLVMPointerType::get(type.getContext()),   // data
           mlir::LLVM::LLVMPointerType::get(type.getContext())}); // vtable
    });
    addConversion([&](IR::ClosureType type) {
      return mlir::LLVM::LLVMStructType::getLiteral(
          type.getContext(),
          {mlir::LLVM::LLVMPointerType::get(type.getContext()),
           mlir::LLVM::LLVMPointerType::get(type.getContext())});
    });
    addConversion([&](IR::DecimalType type) {
      return mlir::LLVM::LLVMStructType::getLiteral(
          type.getContext(),
          {
              mlir::IntegerType::get(type.getContext(), 128), // Mantissa
              mlir::IntegerType::get(type.getContext(), 32)   // Scale
          });
    });

    // Opaque Handles (Mapped to ptr)
    addConversion([&](IR::PromiseType type) {
      return mlir::LLVM::LLVMPointerType::get(type.getContext());
    });
    addConversion([&](IR::ThreadType type) {
      return mlir::LLVM::LLVMPointerType::get(type.getContext());
    });
    addConversion([&](IR::ClassType type) {
      return mlir::LLVM::LLVMPointerType::get(type.getContext());
    });
    addConversion([&](IR::RefClassType type) {
      return mlir::LLVM::LLVMPointerType::get(type.getContext());
    });
    addConversion([&](IR::NullableType type) {
      return mlir::LLVM::LLVMPointerType::get(type.getContext());
    });
    addConversion([&](IR::NullType type) {
      return mlir::LLVM::LLVMPointerType::get(type.getContext());
    });
    addConversion([&](IR::MapType type) {
      return mlir::LLVM::LLVMPointerType::get(type.getContext());
    });
    addConversion([this](mlir::LLVM::LLVMStructType type) -> mlir::Type {
      if (type.isOpaque())
        return type;
      llvm::SmallVector<mlir::Type, 4> newBody;
      bool changed = false;

      for (auto elemTy : type.getBody()) {
        mlir::Type converted = this->convertType(elemTy);
        if (converted != elemTy)
          changed = true;
        newBody.push_back(converted);
      }

      if (!changed)
        return type;

      return mlir::LLVM::LLVMStructType::getLiteral(type.getContext(), newBody,
                                                    type.isPacked());
    });

    addConversion([this](mlir::LLVM::LLVMArrayType type) -> mlir::Type {
      mlir::Type elemTy = type.getElementType();
      mlir::Type converted = this->convertType(elemTy);
      if (converted == elemTy)
        return type;
      return mlir::LLVM::LLVMArrayType::get(converted, type.getNumElements());
    });
  }
};

// Helpers for External Runtime Calls & Constants
static mlir::LLVM::CallOp
createRuntimeCall(mlir::ConversionPatternRewriter &rewriter, mlir::Location loc,
                  llvm::StringRef funcName, mlir::TypeRange retTypes,
                  mlir::ValueRange args) {
  auto module =
      rewriter.getBlock()->getParentOp()->getParentOfType<mlir::ModuleOp>();

  llvm::SmallVector<mlir::Type, 4> argTypes;
  for (auto arg : args)
    argTypes.push_back(arg.getType());

  mlir::Type retTy = retTypes.empty()
                         ? mlir::LLVM::LLVMVoidType::get(rewriter.getContext())
                         : retTypes[0];
  auto funcType = mlir::LLVM::LLVMFunctionType::get(retTy, argTypes, false);

  if (!module.lookupSymbol(funcName)) {
    mlir::OpBuilder::InsertionGuard guard(rewriter);
    rewriter.setInsertionPointToStart(module.getBody());
    rewriter.create<mlir::LLVM::LLVMFuncOp>(loc, funcName, funcType);
  }

  auto symRef = mlir::SymbolRefAttr::get(rewriter.getContext(), funcName);
  auto callOp =
      rewriter.create<mlir::LLVM::CallOp>(loc, retTypes, symRef, args);
  callOp->setAttr("callee_type", mlir::TypeAttr::get(funcType));
  return callOp;
}

static mlir::Value createRuntimeCall(mlir::ConversionPatternRewriter &rewriter,
                                     mlir::Location loc, mlir::Type returnType,
                                     llvm::StringRef funcName,
                                     mlir::ArrayRef<mlir::Value> args) {
  auto module =
      rewriter.getBlock()->getParentOp()->getParentOfType<mlir::ModuleOp>();

  llvm::SmallVector<mlir::Type, 4> argTypes;
  for (auto arg : args)
    argTypes.push_back(arg.getType());

  auto funcType =
      mlir::LLVM::LLVMFunctionType::get(returnType, argTypes, false);
  if (!module.lookupSymbol(funcName)) {
    mlir::OpBuilder::InsertionGuard guard(rewriter);
    rewriter.setInsertionPointToStart(module.getBody());
    rewriter.create<mlir::LLVM::LLVMFuncOp>(loc, funcName, funcType);
  }

  auto symRef = mlir::SymbolRefAttr::get(rewriter.getContext(), funcName);
  auto callOp =
      rewriter.create<mlir::LLVM::CallOp>(loc, returnType, symRef, args);

  callOp->setAttr("callee_type", mlir::TypeAttr::get(funcType));
  return callOp.getResult();
}

static uint32_t getMokshaTypeID(mlir::Type type) {
  if (type.isInteger(1))
    return 0;
  if (auto intTy = mlir::dyn_cast<mlir::IntegerType>(type)) {
    bool isUnsigned = intTy.isUnsigned();
    switch (intTy.getWidth()) {
    case 8:
      return isUnsigned ? 2 : 1;
    case 16:
      return isUnsigned ? 4 : 3;
    case 32:
      return isUnsigned ? 6 : 5;
    case 64:
      return isUnsigned ? 8 : 7;
    }
  }
  if (mlir::isa<mlir::IndexType>(type))
    return 10;
  if (mlir::isa<mlir::Float8E5M2Type>(type) ||
      mlir::isa<mlir::Float8E4M3FNType>(type))
    return 11;
  if (type.isF16())
    return 12;
  if (type.isF32())
    return 13;
  if (type.isF64())
    return 14;
  if (mlir::isa<IR::DecimalType>(type))
    return 15;
  if (mlir::isa<IR::PromiseType>(type))
    return 20;
  if (mlir::isa<IR::ClosureType>(type))
    return 21;
  if (auto ptrTy = mlir::dyn_cast<IR::PointerType>(type)) {
    if (ptrTy.getPointee().isSignlessInteger(8))
      return 16;
  }

  std::string typeStr;
  llvm::raw_string_ostream os(typeStr);
  type.print(os);

  if (typeStr.find("slice") != std::string::npos ||
      typeStr.find("array") != std::string::npos) {
    return 18;
  }
  if (typeStr.find("table") != std::string::npos) {
    return 17;
  }

  return 19;
}

static mlir::LLVM::AtomicOrdering mapAtomicOrdering(int32_t ord) {
  switch (ord) {
  case 0:
    return mlir::LLVM::AtomicOrdering::not_atomic;
  case 1:
    return mlir::LLVM::AtomicOrdering::monotonic;
  case 2:
    return mlir::LLVM::AtomicOrdering::acquire;
  case 3:
    return mlir::LLVM::AtomicOrdering::release;
  case 4:
    return mlir::LLVM::AtomicOrdering::acq_rel;
  case 5:
    return mlir::LLVM::AtomicOrdering::seq_cst;
  default:
    return mlir::LLVM::AtomicOrdering::seq_cst;
  }
}

static mlir::LLVM::AtomicBinOp mapAtomicBinOp(int32_t op) {
  switch (op) {
  case 0:
    return mlir::LLVM::AtomicBinOp::xchg;
  case 1:
    return mlir::LLVM::AtomicBinOp::add;
  case 2:
    return mlir::LLVM::AtomicBinOp::sub;
  case 3:
    return mlir::LLVM::AtomicBinOp::_and;
  case 4:
    return mlir::LLVM::AtomicBinOp::_or;
  case 5:
    return mlir::LLVM::AtomicBinOp::_xor;
  default:
    return mlir::LLVM::AtomicBinOp::add;
  }
}

/** @brief Reconstructs the Frontend's sanitized stringifier names for types. */
static std::string getMangledHIRTypeName(mlir::Type type) {
  if (type.isInteger(1))
    return "bool";
  if (auto intTy = mlir::dyn_cast<mlir::IntegerType>(type)) {
    std::string prefix = intTy.isUnsigned() ? "u" : "i";
    return prefix + std::to_string(intTy.getWidth());
  }
  if (mlir::isa<mlir::IndexType>(type))
    return "usize";
  if (type.isF16())
    return "f16";
  if (type.isF32())
    return "f32";
  if (type.isF64())
    return "f64";
  if (mlir::isa<mlir::Float8E5M2Type>(type) ||
      mlir::isa<mlir::Float8E4M3FNType>(type))
    return "f8";
  if (mlir::isa<IR::DecimalType>(type))
    return "decimal";
  if (mlir::isa<IR::AnyType>(type))
    return "any";
  if (auto ptrTy = mlir::dyn_cast<IR::PointerType>(type)) {
    if (ptrTy.getPointee().isInteger(8))
      return "string";
    return "_" + getMangledHIRTypeName(ptrTy.getPointee());
  }
  if (auto arrTy = mlir::dyn_cast<IR::ArrayType>(type)) {
    return getMangledHIRTypeName(arrTy.getElementType()) + "_" +
           std::to_string(arrTy.getSize()) + "_";
  }
  if (auto slcTy = mlir::dyn_cast<IR::SliceType>(type)) {
    return getMangledHIRTypeName(slcTy.getElementType()) + "__";
  }
  if (auto mapTy = mlir::dyn_cast<IR::MapType>(type)) {
    return "table_" + getMangledHIRTypeName(mapTy.getKeyType()) + "__" +
           getMangledHIRTypeName(mapTy.getValueType()) + "_";
  }
  if (auto nullTy = mlir::dyn_cast<IR::NullableType>(type)) {
    return getMangledHIRTypeName(nullTy.getInnerType()) + "_";
  }
  if (auto stTy = mlir::dyn_cast<mlir::LLVM::LLVMStructType>(type)) {
    if (stTy.isIdentified()) {
      std::string name = stTy.getName().str();
      for (char &c : name)
        if (!isalnum(c))
          c = '_';
      return name;
    }
  }
  return "ptr";
}

static mlir::Value
getOrCreateAnyVTable(mlir::ConversionPatternRewriter &rewriter,
                     mlir::Location loc, mlir::Type origMokshaTy,
                     mlir::Type llvmUnderlyingTy) {
  auto module =
      rewriter.getBlock()->getParentOp()->getParentOfType<mlir::ModuleOp>();
  mlir::Type llvmPtrTy =
      mlir::LLVM::LLVMPointerType::get(rewriter.getContext());
  mlir::Type i32Ty = rewriter.getI32Type();
  mlir::Type underlyingTy = origMokshaTy;
  bool requiresDeref = false;
  if (auto ptrTy = mlir::dyn_cast<IR::PointerType>(origMokshaTy)) {
    if (!ptrTy.getPointee().isSignlessInteger(8)) {
      underlyingTy = ptrTy.getPointee();
      requiresDeref = true;
    }
  }

  uint32_t typeId = getMokshaTypeID(underlyingTy);
  std::string vtableName = "__moksha_any_vtable_type_" + std::to_string(typeId);
  if (requiresDeref)
    vtableName += "_ptr";

  auto globalOp = module.lookupSymbol<mlir::LLVM::GlobalOp>(vtableName);
  if (!globalOp) {
    mlir::OpBuilder::InsertionGuard guard(rewriter);
    rewriter.setInsertionPointToStart(module.getBody());
    auto vtableTy = mlir::LLVM::LLVMStructType::getLiteral(
        rewriter.getContext(), {i32Ty, llvmPtrTy, llvmPtrTy, llvmPtrTy});
    std::string baseToStrName;
    std::string extStr;
    llvm::raw_string_ostream extOs(extStr);
    origMokshaTy.print(extOs);

    if (extStr.find("table") != std::string::npos) {
      baseToStrName =
          "__moksha_map_to_string_" + getMangledHIRTypeName(origMokshaTy);
    } else if (extStr.find("slice") != std::string::npos ||
               extStr.find("array") != std::string::npos) {
      baseToStrName =
          "__moksha_array_to_string_" + getMangledHIRTypeName(origMokshaTy);
    } else {
      switch (typeId) {
      case 0:
        baseToStrName = "__moksha_bool_to_string";
        break;
      case 1:
        baseToStrName = "__moksha_char_to_string";
        break;
      case 2:
        baseToStrName = "__moksha_uchar_to_string";
        break;
      case 3:
        baseToStrName = "__moksha_short_to_string";
        break;
      case 4:
        baseToStrName = "__moksha_ushort_to_string";
        break;
      case 5:
        baseToStrName = "__moksha_int_to_string";
        break;
      case 6:
        baseToStrName = "__moksha_uint_to_string";
        break;
      case 7:
        baseToStrName = "__moksha_long_to_string";
        break;
      case 8:
        baseToStrName = "__moksha_ulong_to_string";
        break;
      case 9:
        baseToStrName = "__moksha_isize_to_string";
        break;
      case 10:
        baseToStrName = "__moksha_usize_to_string";
        break;
      case 11:
        baseToStrName = "__moksha_quarter_to_string_abi";
        break;
      case 12:
        baseToStrName = "__moksha_half_to_string_abi";
        break;
      case 13:
        baseToStrName = "__moksha_float_to_string";
        break;
      case 14:
        baseToStrName = "__moksha_double_to_string";
        break;
      case 15:
        baseToStrName = "moksha_rt_dec_to_string";
        break;
      case 16:
        baseToStrName = "__moksha_cstr_to_string";
        break;
      default:
        baseToStrName = "__moksha_ptr_to_string";
        break;
      }
    }

    std::string finalToStrName = baseToStrName;
    // Generate a Thunk for Value Types
    if (requiresDeref && typeId < 15) {
      finalToStrName = baseToStrName + "_thunk";
      if (!module.lookupSymbol(finalToStrName)) {
        mlir::OpBuilder::InsertionGuard g(rewriter);
        rewriter.setInsertionPointToStart(module.getBody());

        auto thunkTy =
            mlir::LLVM::LLVMFunctionType::get(llvmPtrTy, {llvmPtrTy}, false);
        auto thunkFunc = rewriter.create<mlir::LLVM::LLVMFuncOp>(
            loc, finalToStrName, thunkTy);
        mlir::Block *thunkBlock = thunkFunc.addEntryBlock(rewriter);

        rewriter.setInsertionPointToStart(thunkBlock);
        mlir::Value loadedVal = rewriter.create<mlir::LLVM::LoadOp>(
            loc, llvmUnderlyingTy, thunkBlock->getArgument(0));

        mlir::Value argForCall = loadedVal;
        if (baseToStrName == "__moksha_half_to_string_abi" ||
            baseToStrName == "__moksha_quarter_to_string_abi") {
          argForCall = rewriter.create<mlir::LLVM::FPExtOp>(
              loc, rewriter.getF32Type(), loadedVal);
        }

        mlir::Value callRes = createRuntimeCall(rewriter, loc, llvmPtrTy,
                                                baseToStrName, {argForCall});

        rewriter.create<mlir::LLVM::ReturnOp>(loc, callRes);
      }
    } else {
      if (!module.lookupSymbol(baseToStrName)) {
        mlir::OpBuilder::InsertionGuard guard(rewriter);
        rewriter.setInsertionPointToStart(module.getBody());
        auto fnTy =
            mlir::LLVM::LLVMFunctionType::get(llvmPtrTy, {llvmPtrTy}, false);
        rewriter.create<mlir::LLVM::LLVMFuncOp>(loc, baseToStrName, fnTy);
      }
    }

    auto toStrRef =
        mlir::FlatSymbolRefAttr::get(rewriter.getContext(), finalToStrName);

    globalOp = rewriter.create<mlir::LLVM::GlobalOp>(
        loc, vtableTy, true, mlir::LLVM::Linkage::Internal, vtableName,
        nullptr);

    mlir::Region &region = globalOp.getInitializerRegion();
    mlir::Block *block = rewriter.createBlock(&region);
    rewriter.setInsertionPointToStart(block);

    mlir::Value vtableStruct =
        rewriter.create<mlir::LLVM::UndefOp>(loc, vtableTy);
    mlir::Value idVal = rewriter.create<mlir::LLVM::ConstantOp>(
        loc, i32Ty, rewriter.getI32IntegerAttr(typeId));
    mlir::Value toStrFn =
        rewriter.create<mlir::LLVM::AddressOfOp>(loc, llvmPtrTy, toStrRef);
    mlir::Value retainFn = rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmPtrTy);
    mlir::Value dropFn = rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmPtrTy);

    vtableStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
        loc, vtableStruct, idVal, llvm::ArrayRef<int64_t>{0});
    vtableStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
        loc, vtableStruct, toStrFn, llvm::ArrayRef<int64_t>{1});
    vtableStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
        loc, vtableStruct, retainFn, llvm::ArrayRef<int64_t>{2});
    vtableStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
        loc, vtableStruct, dropFn, llvm::ArrayRef<int64_t>{3});

    rewriter.create<mlir::LLVM::ReturnOp>(loc, vtableStruct);
  }

  return rewriter.create<mlir::LLVM::AddressOfOp>(loc, llvmPtrTy, vtableName);
}

static mlir::Value getLpadStorage(mlir::ConversionPatternRewriter &rewriter,
                                  mlir::Location loc, mlir::Operation *op,
                                  mlir::Type structTy) {
  mlir::Block *entry = nullptr;

  if (auto llvmFunc = op->getParentOfType<mlir::LLVM::LLVMFuncOp>()) {
    if (!llvmFunc.getBody().empty())
      entry = &llvmFunc.getBody().front();
  } else if (auto func = op->getParentOfType<mlir::func::FuncOp>()) {
    if (!func.getBody().empty())
      entry = &func.getBody().front();
  }

  if (!entry)
    return nullptr;

  for (auto &inst : *entry) {
    if (auto alloca = mlir::dyn_cast<mlir::LLVM::AllocaOp>(&inst)) {
      if (alloca->hasAttr("moksha.lpad_storage")) {
        return alloca.getResult();
      }
    }
  }

  mlir::OpBuilder::InsertionGuard guard(rewriter);
  rewriter.setInsertionPointToStart(entry);
  mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
      loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));
  auto alloca = rewriter.create<mlir::LLVM::AllocaOp>(
      loc, mlir::LLVM::LLVMPointerType::get(rewriter.getContext()), structTy,
      one);
  alloca->setAttr("moksha.lpad_storage", rewriter.getUnitAttr());
  return alloca.getResult();
}

static mlir::Block *
getOrCreateTrampoline(mlir::ConversionPatternRewriter &rewriter,
                      mlir::Location loc, mlir::Block *unwindDest) {
  if (!unwindDest || unwindDest->empty())
    return unwindDest;

  mlir::OpBuilder::InsertionGuard guard(rewriter);

  mlir::Type i8PtrTy = mlir::LLVM::LLVMPointerType::get(rewriter.getContext());
  mlir::Type i32Ty = rewriter.getI32Type();
  mlir::Type structTy = mlir::LLVM::LLVMStructType::getLiteral(
      rewriter.getContext(), {i8PtrTy, i32Ty});

  bool hasCleanup = true;
  mlir::ArrayAttr catchClauses = nullptr;

  bool isDummyBlock = false;
  for (auto &op : *unwindDest) {
    if (auto lpad = mlir::dyn_cast<IR::LandingPadOp>(&op)) {
      if (!lpad.getCleanup())
        hasCleanup = false;
      catchClauses = lpad.getCatchClausesAttr();
    }
    if (mlir::isa<IR::ResumeOp>(&op) &&
        unwindDest->getOperations().size() <= 2) {
      isDummyBlock = true;
    }
  }

  mlir::Value nullPtr;
  if (catchClauses) {
    nullPtr = rewriter.create<mlir::LLVM::ZeroOp>(loc, i8PtrTy);
  }

  mlir::Block *trampoline = rewriter.createBlock(
      unwindDest->getParent(), unwindDest->getParent()->end());

  llvm::SmallVector<mlir::Value, 4> clauseVals;
  if (catchClauses) {
    for (size_t i = 0; i < catchClauses.size(); ++i) {
      clauseVals.push_back(nullPtr);
    }
  }

  auto lpad = rewriter.create<mlir::LLVM::LandingpadOp>(loc, structTy,
                                                        hasCleanup, clauseVals);

  if (isDummyBlock) {
    rewriter.create<mlir::LLVM::ResumeOp>(loc, lpad.getResult());
    return trampoline;
  }

  mlir::Value storage = getLpadStorage(rewriter, loc, lpad, structTy);
  if (storage) {
    rewriter.create<mlir::LLVM::StoreOp>(loc, lpad.getResult(), storage);
  }

  rewriter.create<mlir::LLVM::BrOp>(loc, mlir::ValueRange{}, unwindDest);

  return trampoline;
}

static mlir::Value
materializeLLVMConstant(mlir::ConversionPatternRewriter &rewriter,
                        mlir::Operation *op, mlir::Type llvmType,
                        mlir::Attribute attr) {
  auto loc = op->getLoc();
  if (mlir::isa<mlir::UnitAttr>(attr)) {
    return rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmType);
  }
  if (auto boolAttr = mlir::dyn_cast<mlir::BoolAttr>(attr)) {
    return rewriter.create<mlir::LLVM::ConstantOp>(
        loc, llvmType,
        rewriter.getIntegerAttr(llvmType, boolAttr.getValue() ? 1 : 0));
  }
  if (auto strAttr = mlir::dyn_cast<mlir::StringAttr>(attr)) {
    if (mlir::isa<mlir::LLVM::LLVMPointerType>(llvmType)) {
      mlir::ModuleOp module = op->getParentOfType<mlir::ModuleOp>();

      std::string nullTermStr = strAttr.getValue().str() + '\0';
      std::string globalName =
          ".str.lit." + std::to_string(llvm::hash_value(strAttr.getValue()));

      auto globalOp = module.lookupSymbol<mlir::LLVM::GlobalOp>(globalName);
      if (!globalOp) {
        mlir::OpBuilder::InsertionGuard guard(rewriter);
        rewriter.setInsertionPointToStart(module.getBody());

        auto i8Ty = mlir::IntegerType::get(rewriter.getContext(), 8);
        auto arrayTy = mlir::LLVM::LLVMArrayType::get(i8Ty, nullTermStr.size());

        auto nullTermAttr = mlir::StringAttr::get(
            rewriter.getContext(),
            llvm::StringRef(nullTermStr.data(), nullTermStr.size()));

        globalOp = rewriter.create<mlir::LLVM::GlobalOp>(
            loc, arrayTy, true, mlir::LLVM::Linkage::Private, globalName,
            nullTermAttr);
      }
      return rewriter.create<mlir::LLVM::AddressOfOp>(loc, llvmType,
                                                      globalOp.getSymName());
    }
  }
  if (auto intAttr = mlir::dyn_cast<mlir::IntegerAttr>(attr)) {
    if (!llvmType.isIntOrIndex()) {
      if (intAttr.getInt() == 0)
        return rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmType);
      if (mlir::isa<mlir::LLVM::LLVMPointerType>(llvmType)) {
        mlir::Type i64Ty = rewriter.getI64Type();
        mlir::Value intVal = rewriter.create<mlir::LLVM::ConstantOp>(
            loc, i64Ty, rewriter.getI64IntegerAttr(intAttr.getInt()));
        return rewriter.create<mlir::LLVM::IntToPtrOp>(loc, llvmType, intVal);
      }
    }
    return rewriter.create<mlir::LLVM::ConstantOp>(
        loc, llvmType, rewriter.getIntegerAttr(llvmType, intAttr.getInt()));
  }
  if (auto floatAttr = mlir::dyn_cast<mlir::FloatAttr>(attr)) {
    auto targetTy = mlir::cast<mlir::FloatType>(llvmType);
    llvm::APFloat apVal = floatAttr.getValue();
    bool losesInfo;
    apVal.convert(targetTy.getFloatSemantics(),
                  llvm::APFloat::rmNearestTiesToEven, &losesInfo);

    return rewriter.create<mlir::LLVM::ConstantOp>(
        loc, llvmType, rewriter.getFloatAttr(llvmType, apVal));
  }
  return rewriter.create<mlir::LLVM::ConstantOp>(loc, llvmType, attr);
}

static mlir::Value safeUpcastFPExt(mlir::ConversionPatternRewriter &rewriter,
                                   mlir::Location loc, mlir::Value val,
                                   mlir::Type destTy) {
  if (auto constOp = val.getDefiningOp<mlir::LLVM::ConstantOp>()) {
    if (auto floatAttr = mlir::dyn_cast<mlir::FloatAttr>(constOp.getValue())) {
      llvm::APFloat apVal = floatAttr.getValue();
      bool losesInfo;
      apVal.convert(mlir::cast<mlir::FloatType>(destTy).getFloatSemantics(),
                    llvm::APFloat::rmNearestTiesToEven, &losesInfo);
      return rewriter.create<mlir::LLVM::ConstantOp>(
          loc, destTy, rewriter.getFloatAttr(destTy, apVal));
    }
  }
  return rewriter.create<mlir::LLVM::FPExtOp>(loc, destTy, val);
}

// Global Operations
struct GlobalOpLowering : public mlir::ConvertOpToLLVMPattern<IR::GlobalOp> {
  using ConvertOpToLLVMPattern<IR::GlobalOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::GlobalOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Type elementType;
    if (auto typedAttr =
            mlir::dyn_cast_or_null<mlir::TypedAttr>(op.getInitialValueAttr())) {
      elementType = typedAttr.getType();
      if (mlir::isa<mlir::NoneType>(elementType))
        elementType = nullptr;
    }

    if (!elementType) {
      if (auto ptrTy = mlir::dyn_cast<IR::PointerType>(op.getType())) {
        elementType = ptrTy.getPointee();
      }
    }

    if (!elementType)
      return rewriter.notifyMatchFailure(op, "Could not deduce global type");

    mlir::Type llvmType = typeConverter->convertType(elementType);
    mlir::LLVM::Linkage linkage = mlir::LLVM::Linkage::External;
    mlir::Attribute initAttr = op.getInitialValueAttr();

    bool generateVTableRegion = false;
    mlir::ArrayAttr vtableEntries;

    bool isMap = elementType && mlir::isa<IR::MapType>(elementType);
    bool generateMapInit = false;
    mlir::ArrayAttr mapEntries;

    bool generateStrInit = false;
    mlir::StringAttr strEntry;

    bool generateDecInit = false;
    mlir::StringAttr decEntry;

    if (initAttr) {
      if (mlir::isa<mlir::ArrayAttr>(initAttr)) {
        if (!mlir::isa<mlir::LLVM::LLVMArrayType>(llvmType) &&
            !mlir::isa<mlir::LLVM::LLVMStructType>(llvmType)) {
          if (isMap) {
            generateMapInit = true;
            mapEntries = mlir::cast<mlir::ArrayAttr>(initAttr);
          }
          initAttr = nullptr;
        } else {
          generateVTableRegion = true;
          vtableEntries = mlir::cast<mlir::ArrayAttr>(initAttr);
          initAttr = nullptr;
        }
      } else if (mlir::isa<mlir::StringAttr>(initAttr)) {
        if (mlir::isa<mlir::LLVM::LLVMPointerType>(llvmType)) {
          generateStrInit = true;
          strEntry = mlir::cast<mlir::StringAttr>(initAttr);
          initAttr = nullptr;
        } else if (mlir::isa<IR::DecimalType>(elementType)) {
          generateDecInit = true;
          decEntry = mlir::cast<mlir::StringAttr>(initAttr);
          initAttr = nullptr;
        }
      } else if (mlir::isa<mlir::UnitAttr>(initAttr)) {
        initAttr = nullptr;
      } else if (auto intAttr = mlir::dyn_cast<mlir::IntegerAttr>(initAttr)) {
        if (!llvmType.isIntOrIndex()) {
          if (intAttr.getInt() == 0) {
            initAttr = nullptr;
          } else if (mlir::isa<mlir::LLVM::LLVMPointerType>(llvmType)) {
            initAttr = nullptr;
          }
        } else {
          initAttr = rewriter.getIntegerAttr(llvmType, intAttr.getInt());
        }
      } else if (auto floatAttr = mlir::dyn_cast<mlir::FloatAttr>(initAttr)) {
        auto targetTy = mlir::cast<mlir::FloatType>(llvmType);
        llvm::APFloat apVal = floatAttr.getValue();
        bool losesInfo;
        apVal.convert(targetTy.getFloatSemantics(),
                      llvm::APFloat::rmNearestTiesToEven, &losesInfo);
        initAttr = rewriter.getFloatAttr(llvmType, apVal);
      }
    }

    if (op->hasAttr("moksha.zeroinit")) {
      initAttr = nullptr;
    }

    auto name = op.getSymName();

    if (name == "__moksha_ex_payload") {
      linkage = mlir::LLVM::Linkage::External;
      initAttr = nullptr;
    }

    bool isConstant = op->hasAttr("moksha.constant");
    uint64_t alignment = 0;
    if (auto alignAttr =
            op->getAttrOfType<mlir::IntegerAttr>("moksha.alignment")) {
      alignment = alignAttr.getInt();
    }
    bool threadLocal = op->hasAttr("moksha.thread_local");

    if (auto linkAttr = op->getAttrOfType<mlir::StringAttr>("moksha.linkage")) {
      llvm::StringRef linkStr = linkAttr.getValue();
      if (linkStr == "internal")
        linkage = mlir::LLVM::Linkage::Internal;
      else if (linkStr == "weak")
        linkage = mlir::LLVM::Linkage::Weak;
    }

    auto globalOp = rewriter.replaceOpWithNewOp<mlir::LLVM::GlobalOp>(
        op, llvmType, isConstant, linkage, name, initAttr, alignment, 0, false,
        threadLocal);

    if (!initAttr && !generateVTableRegion) {
      globalOp->setAttr("moksha.zeroinit", rewriter.getUnitAttr());
    }

    if (auto secAttr = op->getAttrOfType<mlir::StringAttr>("moksha.section")) {
      globalOp->setAttr("section_name", secAttr);
    }

    for (auto attr : op->getAttrs()) {
      if (attr.getName().strref().starts_with("moksha.") &&
          attr.getName().strref() != "moksha.linkage" &&
          attr.getName().strref() != "moksha.zeroinit") {
        globalOp->setAttr(attr.getName(), attr.getValue());
      }
    }

    if (generateVTableRegion) {
      mlir::Region &region = globalOp.getInitializerRegion();
      mlir::Block *block = rewriter.createBlock(&region);
      mlir::OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(block);
      auto loc = op.getLoc();

      mlir::Value structVal =
          rewriter.create<mlir::LLVM::UndefOp>(loc, llvmType);
      auto stTy = mlir::dyn_cast<mlir::LLVM::LLVMStructType>(llvmType);

      for (size_t i = 0; i < vtableEntries.size(); ++i) {
        mlir::Attribute elemAttr = vtableEntries[i];
        mlir::Type fieldTy =
            stTy ? stTy.getBody()[i]
                 : mlir::LLVM::LLVMPointerType::get(getContext());
        mlir::Value elemVal;

        if (mlir::isa<mlir::UnitAttr>(elemAttr)) {
          elemVal = rewriter.create<mlir::LLVM::ZeroOp>(loc, fieldTy);
        } else if (auto innerArr = mlir::dyn_cast<mlir::ArrayAttr>(elemAttr)) {
          elemVal = rewriter.create<mlir::LLVM::UndefOp>(loc, fieldTy);
          for (size_t j = 0; j < innerArr.size(); ++j) {
            if (auto symRef =
                    mlir::dyn_cast<mlir::FlatSymbolRefAttr>(innerArr[j])) {
              std::string rawName = symRef.getValue().str();
              for (char &c : rawName) {
                if (c == '.' || c == '<' || c == '>')
                  c = '_';
              }
              auto cleanSymRef =
                  mlir::FlatSymbolRefAttr::get(getContext(), rawName);

              mlir::Value fnPtr = rewriter.create<mlir::LLVM::AddressOfOp>(
                  loc, mlir::LLVM::LLVMPointerType::get(getContext()),
                  cleanSymRef);
              elemVal = rewriter.create<mlir::LLVM::InsertValueOp>(
                  loc, elemVal, fnPtr,
                  llvm::ArrayRef<int64_t>{static_cast<int64_t>(j)});
            }
          }
        }

        if (elemVal) {
          structVal = rewriter.create<mlir::LLVM::InsertValueOp>(
              loc, structVal, elemVal,
              llvm::ArrayRef<int64_t>{static_cast<int64_t>(i)});
        }
      }
      rewriter.create<mlir::LLVM::ReturnOp>(loc, structVal);
    }

    if (generateMapInit || generateStrInit || generateDecInit) {
      auto module = globalOp->getParentOfType<mlir::ModuleOp>();
      mlir::Operation *initFunc = module.lookupSymbol("__moksha_module_init");
      if (initFunc) {
        mlir::Block *entryBlock = nullptr;
        if (auto fFunc = mlir::dyn_cast<mlir::func::FuncOp>(initFunc)) {
          if (!fFunc.getBody().empty())
            entryBlock = &fFunc.getBody().front();
        } else if (auto lFunc =
                       mlir::dyn_cast<mlir::LLVM::LLVMFuncOp>(initFunc)) {
          if (!lFunc.getBody().empty())
            entryBlock = &lFunc.getBody().front();
        }

        if (entryBlock) {
          mlir::OpBuilder::InsertionGuard guard(rewriter);
          rewriter.setInsertionPointToStart(entryBlock);
          auto loc = op.getLoc();

          mlir::Value globalAddr = rewriter.create<mlir::LLVM::AddressOfOp>(
              loc, mlir::LLVM::LLVMPointerType::get(rewriter.getContext()),
              globalOp.getSymName());

          if (generateStrInit) {
            mlir::Value strPtr =
                materializeLLVMConstant(rewriter, globalOp, llvmType, strEntry);
            rewriter.create<mlir::LLVM::StoreOp>(loc, strPtr, globalAddr);
          }

          if (generateMapInit) {
            auto module = globalOp->getParentOfType<mlir::ModuleOp>();
            mlir::Operation *initFunc =
                module.lookupSymbol("__moksha_module_init");
            if (initFunc) {
              mlir::Block *entryBlock = nullptr;
              if (auto fFunc = mlir::dyn_cast<mlir::func::FuncOp>(initFunc)) {
                if (!fFunc.getBody().empty())
                  entryBlock = &fFunc.getBody().front();
              } else if (auto lFunc =
                             mlir::dyn_cast<mlir::LLVM::LLVMFuncOp>(initFunc)) {
                if (!lFunc.getBody().empty())
                  entryBlock = &lFunc.getBody().front();
              }

              if (entryBlock) {
                mlir::OpBuilder::InsertionGuard guard(rewriter);

                auto terminator = entryBlock->getTerminator();
                if (terminator) {
                  rewriter.setInsertionPoint(terminator);
                } else {
                  rewriter.setInsertionPointToEnd(entryBlock);
                }

                auto loc = op.getLoc();
                mlir::Value globalAddr =
                    rewriter.create<mlir::LLVM::AddressOfOp>(
                        loc,
                        mlir::LLVM::LLVMPointerType::get(rewriter.getContext()),
                        globalOp.getSymName());

                auto mapNewCall = createRuntimeCall(
                    rewriter, loc, "moksha_rt_map_new", {llvmType}, {});
                mlir::Value mapPtr = mapNewCall.getResult();
                rewriter.create<mlir::LLVM::StoreOp>(loc, mapPtr, globalAddr);

                auto mapTy = llvm::dyn_cast_or_null<IR::MapType>(elementType);
                mlir::Type keyTy =
                    typeConverter->convertType(mapTy.getKeyType());
                mlir::Type valTy =
                    typeConverter->convertType(mapTy.getValueType());

                auto boxValue = [&](mlir::Value val,
                                    mlir::Type origTy) -> mlir::Value {
                  mlir::Type llvmPtrTy =
                      mlir::LLVM::LLVMPointerType::get(rewriter.getContext());
                  mlir::Type i64Ty = rewriter.getI64Type();
                  mlir::Type i32Ty = rewriter.getI32Type();

                  mlir::Value dataPtr = val;
                  if (!mlir::isa<mlir::LLVM::LLVMPointerType>(val.getType())) {
                    mlir::Value nullPtr =
                        rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmPtrTy);
                    mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
                        loc, i32Ty, rewriter.getI32IntegerAttr(1));
                    mlir::Value sizeGep = rewriter.create<mlir::LLVM::GEPOp>(
                        loc, llvmPtrTy, val.getType(), nullPtr,
                        llvm::ArrayRef<mlir::LLVM::GEPArg>{one});
                    mlir::Value sizeInt =
                        rewriter.create<mlir::LLVM::PtrToIntOp>(loc, i64Ty,
                                                                sizeGep);

                    uint32_t typeId = getMokshaTypeID(origTy);
                    mlir::Value typeTag =
                        rewriter.create<mlir::LLVM::ConstantOp>(
                            loc, i32Ty, rewriter.getI32IntegerAttr(typeId));

                    dataPtr = createRuntimeCall(rewriter, loc, llvmPtrTy,
                                                "moksha_rt_alloc",
                                                {sizeInt, typeTag});

                    auto storeOp =
                        rewriter.create<mlir::LLVM::StoreOp>(loc, val, dataPtr);
                    unsigned alignment = 8;
                    if (val.getType().isIntOrFloat()) {
                      alignment = std::max(
                          1u, val.getType().getIntOrFloatBitWidth() / 8);
                    }
                    storeOp.setAlignment(alignment);
                  } else if (dataPtr.getType() != llvmPtrTy) {
                    dataPtr = rewriter.create<mlir::LLVM::BitcastOp>(
                        loc, llvmPtrTy, dataPtr);
                  }

                  mlir::Type anyStructTy =
                      mlir::LLVM::LLVMStructType::getLiteral(
                          rewriter.getContext(), {llvmPtrTy, llvmPtrTy});

                  mlir::Value anyVal =
                      rewriter.create<mlir::LLVM::UndefOp>(loc, anyStructTy);
                  anyVal = rewriter.create<mlir::LLVM::InsertValueOp>(
                      loc, anyVal, dataPtr, llvm::ArrayRef<int64_t>{0});

                  mlir::Type mlirUnderlyingTy =
                      typeConverter->convertType(origTy);
                  mlir::Value vtablePtr = getOrCreateAnyVTable(
                      rewriter, loc, origTy, mlirUnderlyingTy);

                  anyVal = rewriter.create<mlir::LLVM::InsertValueOp>(
                      loc, anyVal, vtablePtr, llvm::ArrayRef<int64_t>{1});

                  return anyVal;
                };

                std::vector<mlir::Operation *> orphanStrings;
                for (auto &inst : *entryBlock) {
                  if (auto callOp = mlir::dyn_cast<mlir::func::CallOp>(inst)) {
                    if (callOp.getCallee() == "__moksha_cstr_to_string" &&
                        callOp.use_empty()) {
                      orphanStrings.push_back(&inst);
                    }
                  } else if (auto llvmCall =
                                 mlir::dyn_cast<mlir::LLVM::CallOp>(inst)) {
                    if (llvmCall.getCallee() == "__moksha_cstr_to_string" &&
                        llvmCall.use_empty()) {
                      orphanStrings.push_back(&inst);
                    }
                  }
                }

                size_t strIdx = 0;

                for (auto attr : mapEntries) {
                  if (auto kvAttr = llvm::dyn_cast_or_null<mlir::ArrayAttr>(attr)) {
                    if (kvAttr.size() == 2) {
                      mlir::Value kVal;
                      if (mlir::isa<mlir::UnitAttr>(kvAttr[0]) &&
                          strIdx < orphanStrings.size()) {
                        kVal = orphanStrings[strIdx++]->getResult(0);
                      } else {
                        kVal = materializeLLVMConstant(rewriter, globalOp,
                                                       keyTy, kvAttr[0]);
                      }
                      kVal = boxValue(kVal, mapTy.getKeyType());

                      mlir::Value vVal = materializeLLVMConstant(
                          rewriter, globalOp, valTy, kvAttr[1]);
                      vVal = boxValue(vVal, mapTy.getValueType());
                      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(
                          rewriter.getContext());
                      mlir::Type i32Ty = rewriter.getI32Type();
                      mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
                          loc, i32Ty, rewriter.getI32IntegerAttr(1));

                      mlir::Value kSpill =
                          rewriter.create<mlir::LLVM::AllocaOp>(
                              loc, llvmPtrTy, kVal.getType(), one);
                      rewriter.create<mlir::LLVM::StoreOp>(loc, kVal, kSpill);

                      mlir::Value vSpill =
                          rewriter.create<mlir::LLVM::AllocaOp>(
                              loc, llvmPtrTy, vVal.getType(), one);
                      rewriter.create<mlir::LLVM::StoreOp>(loc, vVal, vSpill);

                      createRuntimeCall(rewriter, loc,
                                        llvm::StringRef("moksha_rt_map_insert"),
                                        mlir::TypeRange{},
                                        {mapPtr, kSpill, vSpill});
                    }
                  }
                }
              }
            }
          }

          if (generateDecInit) {
            std::string decStr = decEntry.getValue().str();
            while (!decStr.empty() &&
                   (decStr.back() == 'd' || decStr.back() == 'D')) {
              decStr.pop_back();
            }
            int32_t scale = 0;
            size_t dotPos = decStr.find('.');
            if (dotPos != std::string::npos) {
              scale = decStr.length() - dotPos - 1;
              decStr.erase(dotPos, 1);
            }
            llvm::APInt coeffAP(128, decStr, 10);
            auto i128Ty = rewriter.getIntegerType(128);

            mlir::Value coeffVal = rewriter.create<mlir::LLVM::ConstantOp>(
                loc, i128Ty, rewriter.getIntegerAttr(i128Ty, coeffAP));
            mlir::Value scaleVal = rewriter.create<mlir::LLVM::ConstantOp>(
                loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(scale));

            mlir::Value undef =
                rewriter.create<mlir::LLVM::UndefOp>(loc, llvmType);
            mlir::Value s1 = rewriter.create<mlir::LLVM::InsertValueOp>(
                loc, undef, coeffVal, llvm::ArrayRef<int64_t>{0});
            mlir::Value decVal = rewriter.create<mlir::LLVM::InsertValueOp>(
                loc, s1, scaleVal, llvm::ArrayRef<int64_t>{1});

            rewriter.create<mlir::LLVM::StoreOp>(loc, decVal, globalAddr);
          }
        }
      }
    }
    return mlir::success();
  }
};

struct ConstantOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::ConstantOp> {
  using ConvertOpToLLVMPattern<IR::ConstantOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::ConstantOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Type llvmType = typeConverter->convertType(op.getType());
    mlir::Attribute attr = adaptor.getValue();

    if (mlir::isa<IR::DecimalType>(op.getType())) {
      if (auto strAttr = mlir::dyn_cast_or_null<mlir::StringAttr>(attr)) {
        std::string decStr = strAttr.getValue().str();
        while (!decStr.empty() &&
               (decStr.back() == 'd' || decStr.back() == 'D')) {
          decStr.pop_back();
        }
        int32_t scale = 0;
        size_t dotPos = decStr.find('.');
        if (dotPos != std::string::npos) {
          scale = decStr.length() - dotPos - 1;
          decStr.erase(dotPos, 1);
        }
        llvm::APInt coeffAP(128, decStr, 10);
        auto i128Ty = rewriter.getIntegerType(128);

        mlir::Value undef =
            rewriter.create<mlir::LLVM::UndefOp>(op.getLoc(), llvmType);

        mlir::Value coeffVal = rewriter.create<mlir::LLVM::ConstantOp>(
            op.getLoc(), i128Ty, rewriter.getIntegerAttr(i128Ty, coeffAP));

        mlir::Value s1 = rewriter.create<mlir::LLVM::InsertValueOp>(
            op.getLoc(), undef, coeffVal, llvm::ArrayRef<int64_t>{0});

        mlir::Value scaleVal = rewriter.create<mlir::LLVM::ConstantOp>(
            op.getLoc(), rewriter.getI32Type(),
            rewriter.getI32IntegerAttr(scale));

        mlir::Value s2 = rewriter.create<mlir::LLVM::InsertValueOp>(
            op.getLoc(), s1, scaleVal, llvm::ArrayRef<int64_t>{1});

        rewriter.replaceOp(op, s2);
        return mlir::success();
      }
    }

    if (mlir::isa<mlir::ArrayAttr>(attr)) {
      if (!mlir::isa<mlir::LLVM::LLVMArrayType>(llvmType) &&
          !mlir::isa<mlir::LLVM::LLVMStructType>(llvmType)) {
        rewriter.replaceOpWithNewOp<mlir::LLVM::ZeroOp>(op, llvmType);
        return mlir::success();
      }
    }

    if (auto floatAttr = mlir::dyn_cast<mlir::FloatAttr>(attr)) {
      auto targetTy = mlir::cast<mlir::FloatType>(llvmType);
      llvm::APFloat apVal = floatAttr.getValue();
      bool losesInfo;
      apVal.convert(targetTy.getFloatSemantics(),
                    llvm::APFloat::rmNearestTiesToEven, &losesInfo);
      bool isTargetQuarter = mlir::isa<mlir::Float8E5M2Type>(op.getType()) ||
                             mlir::isa<mlir::Float8E4M3FNType>(op.getType());
      if (isTargetQuarter && llvmType.isF16()) {
        llvm::APInt api = apVal.bitcastToAPInt();
        uint16_t maskVal =
            mlir::isa<mlir::Float8E5M2Type>(op.getType()) ? 0xFF00 : 0xFF80;
        api &= maskVal;
        apVal = llvm::APFloat(targetTy.getFloatSemantics(), api);
      }
      auto constOp = rewriter.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), llvmType, rewriter.getFloatAttr(llvmType, apVal));
      rewriter.replaceOp(op, constOp.getResult());
      return mlir::success();
    }

    mlir::Value val = materializeLLVMConstant(rewriter, op, llvmType, attr);
    rewriter.replaceOp(op, val);
    return mlir::success();
  }
};

struct AllocaOpLowering : public mlir::ConvertOpToLLVMPattern<IR::AllocaOp> {
  using ConvertOpToLLVMPattern<IR::AllocaOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::AllocaOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
        op.getLoc(), rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));

    mlir::Type allocatedTy = typeConverter->convertType(op.getAllocatedType());
    auto llvmAlloca = rewriter.create<mlir::LLVM::AllocaOp>(
        op.getLoc(), typeConverter->convertType(op.getType()), allocatedTy,
        one);

    mlir::Value zeroVal =
        rewriter.create<mlir::LLVM::ZeroOp>(op.getLoc(), allocatedTy);
    rewriter.create<mlir::LLVM::StoreOp>(op.getLoc(), zeroVal, llvmAlloca);

    rewriter.replaceOp(op, llvmAlloca.getResult());
    return mlir::success();
  }
};

struct LoadOpLowering : public mlir::ConvertOpToLLVMPattern<IR::LoadOp> {
  using ConvertOpToLLVMPattern<IR::LoadOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::LoadOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Value ptr = op.getOperand();
    if (mlir::isa<IR::PointerType>(op.getType())) {
      if (auto gepOp = ptr.getDefiningOp<IR::GetElementPtrOp>()) {
        if (auto castOp = gepOp.getOperand(0).getDefiningOp<IR::CastOp>()) {
          if (auto ptrTy = llvm::dyn_cast_or_null<IR::PointerType>(
                  castOp.getOperand().getType())) {
            if (llvm::isa<IR::ArrayType>(ptrTy.getPointee())) {
              rewriter.replaceOp(op, adaptor.getOperands()[0]);
              return mlir::success();
            }
          }
        }
      }
    }

    mlir::Type resTy = typeConverter->convertType(op.getType());
    rewriter.replaceOpWithNewOp<mlir::LLVM::LoadOp>(op, resTy,
                                                    adaptor.getOperands()[0]);
    return mlir::success();
  }
};

struct StoreOpLowering : public mlir::ConvertOpToLLVMPattern<IR::StoreOp> {
  using ConvertOpToLLVMPattern<IR::StoreOp>::ConvertOpToLLVMPattern;
  mlir::LogicalResult
  matchAndRewrite(IR::StoreOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    auto llvmStore = rewriter.replaceOpWithNewOp<mlir::LLVM::StoreOp>(
        op, adaptor.getValue(), adaptor.getPtr());
    unsigned alignment = 4;
    mlir::Type valTy = adaptor.getValue().getType();
    if (valTy.isIntOrFloat()) {
      alignment = std::max(1u, valTy.getIntOrFloatBitWidth() / 8);
    } else if (mlir::isa<mlir::LLVM::LLVMPointerType>(valTy)) {
      alignment = 8;
    }
    llvmStore.setAlignment(alignment);
    if (op->hasAttr("moksha.volatile")) {
      llvmStore.setVolatile_(true);
    }
    return mlir::success();
  }
};

// Inline Assembly Lowering
struct InlineAsmOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::InlineAsmOp> {
  using ConvertOpToLLVMPattern<IR::InlineAsmOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::InlineAsmOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    // Determine the exact LLVM return type
    mlir::Type resultType;
    if (op.getNumResults() == 0) {
      resultType = mlir::LLVM::LLVMVoidType::get(getContext());
    } else if (op.getNumResults() == 1) {
      resultType = typeConverter->convertType(op.getResult(0).getType());
    } else {
      llvm::SmallVector<mlir::Type, 4> resultTypes;
      for (auto res : op.getResults()) {
        resultTypes.push_back(typeConverter->convertType(res.getType()));
      }
      resultType =
          mlir::LLVM::LLVMStructType::getLiteral(getContext(), resultTypes);
    }

    // Generate the native LLVM inline assembly operation
    rewriter.replaceOpWithNewOp<mlir::LLVM::InlineAsmOp>(
        op, resultType, adaptor.getOperands(), op.getAsmString(),
        op.getConstraints(), op.getIsVolatileAttr() != nullptr, false,
        mlir::LLVM::tailcallkind::TailCallKind::None,
        mlir::LLVM::AsmDialectAttr::get(getContext(),
                                        mlir::LLVM::AsmDialect::AD_ATT),
        mlir::ArrayAttr());

    return mlir::success();
  }
};

// Aggregates
struct GetElementPtrOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::GetElementPtrOp> {
  using ConvertOpToLLVMPattern<IR::GetElementPtrOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::GetElementPtrOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Value base = adaptor.getOperands()[0];
    mlir::Type baseType = base.getType();
    mlir::Type origMokshaBaseTy = op->getOperand(0).getType();

    if (llvm::isa<IR::SliceType>(origMokshaBaseTy)) {
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      if (base.getType() != llvmPtrTy) {
        base = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(), llvmPtrTy,
                                                      base);
      }
      base = createRuntimeCall(rewriter, op.getLoc(), llvmPtrTy,
                               "moksha_rt_array_data", {base});
      baseType = base.getType();
    } else if (!llvm::isa<mlir::LLVM::LLVMPointerType>(baseType)) {
      mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), rewriter.getI64Type(), rewriter.getI64IntegerAttr(1));
      mlir::Value allocaPtr = rewriter.create<mlir::LLVM::AllocaOp>(
          op.getLoc(), mlir::LLVM::LLVMPointerType::get(getContext()), baseType,
          one);
      rewriter.create<mlir::LLVM::StoreOp>(op.getLoc(), base, allocaPtr);
      base = allocaPtr;
    }

    auto indices = adaptor.getOperands().drop_front(1);
    mlir::Type pointeeTy;

    if (auto basePtrTy = llvm::dyn_cast_or_null<IR::PointerType>(origMokshaBaseTy)) {
      auto pointee = basePtrTy.getPointee();
      if (mlir::isa<mlir::NoneType>(pointee)) {
        pointeeTy = rewriter.getI8Type();
      } else {
        pointeeTy = typeConverter->convertType(pointee);

        if (mlir::isa<mlir::LLVM::LLVMPointerType>(pointeeTy)) {
          std::string typeStr;
          llvm::raw_string_ostream os(typeStr);
          pointee.print(os);
          if (typeStr.find("slice") != std::string::npos ||
              typeStr.find("array") != std::string::npos) {
            pointeeTy = mlir::LLVM::LLVMStructType::getLiteral(
                getContext(), {rewriter.getI64Type(), rewriter.getI64Type()});
          }
        }
      }

      if (!pointeeTy) {
        return rewriter.notifyMatchFailure(
            op, "Could not convert GEP pointee type");
      }
    } else {
      pointeeTy = rewriter.getI8Type();
    }

    llvm::SmallVector<mlir::LLVM::GEPArg, 4> gepArgs;
    for (mlir::Value idxVal : indices) {
      if (auto constOp = idxVal.getDefiningOp<mlir::LLVM::ConstantOp>()) {
        if (auto intAttr =
                mlir::dyn_cast<mlir::IntegerAttr>(constOp.getValue())) {
          gepArgs.push_back(static_cast<int32_t>(intAttr.getInt()));
          continue;
        }
      }
      gepArgs.push_back(idxVal);
    }

    rewriter.replaceOpWithNewOp<mlir::LLVM::GEPOp>(
        op, typeConverter->convertType(op.getType()), pointeeTy, base, gepArgs);

    return mlir::success();
  }
};

struct ExtractValueOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::ExtractValueOp> {
  using ConvertOpToLLVMPattern<IR::ExtractValueOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::ExtractValueOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Value aggVal = adaptor.getAggregate();
    if (mlir::isa<mlir::LLVM::LLVMPointerType>(aggVal.getType())) {
      mlir::Value ptr = aggVal;
      mlir::Type expectedType = typeConverter->convertType(op.getType());
      if (expectedType && ptr.getType() != expectedType) {
        ptr = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(), expectedType,
                                                     ptr);
      }
      rewriter.replaceOp(op, ptr);
      return mlir::success();
    }

    mlir::Value ext = rewriter.create<mlir::LLVM::ExtractValueOp>(
        op.getLoc(), aggVal, op.getIndex());
    mlir::Type expectedType = typeConverter->convertType(op.getType());
    mlir::Type extractedType = ext.getType();

    if (expectedType && extractedType && extractedType != expectedType) {
      if (mlir::isa<mlir::IntegerType>(extractedType) &&
          mlir::isa<mlir::IntegerType>(expectedType)) {
        unsigned srcW = extractedType.getIntOrFloatBitWidth();
        unsigned dstW = expectedType.getIntOrFloatBitWidth();
        if (srcW > dstW) {
          ext = rewriter.create<mlir::LLVM::TruncOp>(op.getLoc(), expectedType,
                                                     ext);
        } else if (srcW < dstW) {
          ext = rewriter.create<mlir::LLVM::SExtOp>(op.getLoc(), expectedType,
                                                    ext);
        }
      } else if (mlir::isa<mlir::IndexType>(extractedType) &&
                 mlir::isa<mlir::IntegerType>(expectedType)) {
        ext = rewriter.create<mlir::LLVM::TruncOp>(op.getLoc(), expectedType,
                                                   ext);
      } else if (mlir::isa<mlir::IntegerType>(extractedType) &&
                 mlir::isa<mlir::IndexType>(expectedType)) {
        ext =
            rewriter.create<mlir::LLVM::SExtOp>(op.getLoc(), expectedType, ext);
      } else if (mlir::isa<mlir::LLVM::LLVMPointerType>(extractedType) &&
                 mlir::isa<mlir::LLVM::LLVMPointerType>(expectedType)) {
        ext = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(), expectedType,
                                                     ext);
      }
    }

    rewriter.replaceOp(op, ext);
    return mlir::success();
  }
};

struct InsertValueOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::InsertValueOp> {
  using ConvertOpToLLVMPattern<IR::InsertValueOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::InsertValueOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Type dstType = typeConverter->convertType(op.getType());
    auto operands = adaptor.getOperands();
    if (operands.size() < 2)
      return mlir::failure();

    mlir::Value container = operands[0];
    mlir::Value val = operands[1];
    int64_t idx = op.getIndex();

    if (auto structTy = mlir::dyn_cast<mlir::LLVM::LLVMStructType>(dstType)) {
      mlir::Type elemTy = structTy.getBody()[idx];
      if (val.getType() != elemTy) {
        if (val.getType().isIntOrIndex() && elemTy.isIntOrIndex()) {
          unsigned srcW = val.getType().getIntOrFloatBitWidth();
          unsigned dstW = elemTy.getIntOrFloatBitWidth();
          if (srcW < dstW) {
            val = rewriter.create<mlir::LLVM::SExtOp>(op.getLoc(), elemTy, val);
          } else if (srcW > dstW) {
            val =
                rewriter.create<mlir::LLVM::TruncOp>(op.getLoc(), elemTy, val);
          }
        } else if (mlir::isa<mlir::LLVM::LLVMPointerType>(val.getType()) &&
                   mlir::isa<mlir::LLVM::LLVMPointerType>(elemTy)) {
          val =
              rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(), elemTy, val);
        }
      }
    }

    rewriter.replaceOpWithNewOp<mlir::LLVM::InsertValueOp>(
        op, dstType, container, val, llvm::ArrayRef<int64_t>({idx}));
    return mlir::success();
  }
};

// Casts & Address
struct BitcastOpLowering : public mlir::ConvertOpToLLVMPattern<IR::BitcastOp> {
  using ConvertOpToLLVMPattern<IR::BitcastOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::BitcastOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Type dstType = typeConverter->convertType(op.getType());
    mlir::Value val = adaptor.getOperand();
    mlir::Type srcType = val.getType();

    mlir::Type origDstType = op.getType();

    if (srcType == dstType) {
      bool isF8Sim =
          dstType.isF16() && (mlir::isa<mlir::Float8E5M2Type>(origDstType) ||
                              mlir::isa<mlir::Float8E4M3FNType>(origDstType));
      if (!isF8Sim) {
        rewriter.replaceOp(op, val);
        return mlir::success();
      }
    }

    if (auto constOp = val.getDefiningOp<mlir::LLVM::ConstantOp>()) {

      // Int -> Float (Memory Reinterpretation)
      if (srcType.isIntOrIndex() && mlir::isa<mlir::FloatType>(dstType)) {
        if (auto intAttr =
                mlir::dyn_cast<mlir::IntegerAttr>(constOp.getValue())) {
          llvm::APInt api = intAttr.getValue();
          llvm::APFloat apf(
              mlir::cast<mlir::FloatType>(dstType).getFloatSemantics(), api);
          rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
              op, dstType, rewriter.getFloatAttr(dstType, apf));
          return mlir::success();
        }
      }
      // Float -> Int (Memory Reinterpretation)
      if (mlir::isa<mlir::FloatType>(srcType) && dstType.isIntOrIndex()) {
        if (auto floatAttr =
                mlir::dyn_cast<mlir::FloatAttr>(constOp.getValue())) {
          llvm::APInt api = floatAttr.getValue().bitcastToAPInt();
          rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
              op, dstType, rewriter.getIntegerAttr(dstType, api));
          return mlir::success();
        }
      }
      // Float -> Float (Precision Truncation/Extension fallback for AST
      // mismatches)
      if (mlir::isa<mlir::FloatType>(srcType) &&
          mlir::isa<mlir::FloatType>(dstType)) {
        if (auto floatAttr =
                mlir::dyn_cast<mlir::FloatAttr>(constOp.getValue())) {
          auto targetTy = mlir::cast<mlir::FloatType>(dstType);
          llvm::APFloat apVal = floatAttr.getValue();
          bool losesInfo;
          apVal.convert(targetTy.getFloatSemantics(),
                        llvm::APFloat::rmNearestTiesToEven, &losesInfo);
          bool isTargetQuarter = mlir::isa<mlir::Float8E5M2Type>(origDstType) ||
                                 mlir::isa<mlir::Float8E4M3FNType>(origDstType);
          if (isTargetQuarter && dstType.isF16()) {
            llvm::APInt api = apVal.bitcastToAPInt();
            uint16_t maskVal =
                mlir::isa<mlir::Float8E5M2Type>(origDstType) ? 0xFF00 : 0xFF80;
            api &= maskVal;
            apVal = llvm::APFloat(targetTy.getFloatSemantics(), api);
          }
          rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
              op, dstType, rewriter.getFloatAttr(dstType, apVal));
          return mlir::success();
        }
      }
      // Int -> Int (Zero Extension / Truncation fallback for AST mismatches)
      if (srcType.isIntOrIndex() && dstType.isIntOrIndex()) {
        if (auto intAttr =
                mlir::dyn_cast<mlir::IntegerAttr>(constOp.getValue())) {
          llvm::APInt api = intAttr.getValue();
          unsigned dstW = dstType.getIntOrFloatBitWidth();
          if (api.getBitWidth() < dstW)
            api = api.zext(dstW);
          else if (api.getBitWidth() > dstW)
            api = api.trunc(dstW);
          rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
              op, dstType, rewriter.getIntegerAttr(dstType, api));
          return mlir::success();
        }
      }
    }

    // Aggregate (Struct/Array) -> Pointer Spill
    if ((mlir::isa<mlir::LLVM::LLVMStructType>(srcType) ||
         mlir::isa<mlir::LLVM::LLVMArrayType>(srcType)) &&
        mlir::isa<mlir::LLVM::LLVMPointerType>(dstType)) {

      mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));

      mlir::Value allocaPtr = rewriter.create<mlir::LLVM::AllocaOp>(
          op.getLoc(), dstType, srcType, one);

      rewriter.create<mlir::LLVM::StoreOp>(op.getLoc(), val, allocaPtr);

      rewriter.replaceOp(op, allocaPtr);
      return mlir::success();
    }

    // Helper to safely extract bit-widths for primitives
    auto getBitWidth = [](mlir::Type t) -> unsigned {
      if (t.isIntOrFloat())
        return t.getIntOrFloatBitWidth();
      if (mlir::isa<mlir::LLVM::LLVMPointerType>(t))
        return 64;
      return 0;
    };

    unsigned srcW = getBitWidth(srcType);
    unsigned dstW = getBitWidth(dstType);

    if (mlir::isa<mlir::LLVM::LLVMPointerType>(srcType) &&
        dstType.isIntOrIndex()) {
      rewriter.replaceOpWithNewOp<mlir::LLVM::PtrToIntOp>(op, dstType, val);
      return mlir::success();
    }

    if (srcType.isIntOrIndex() &&
        mlir::isa<mlir::LLVM::LLVMPointerType>(dstType)) {
      rewriter.replaceOpWithNewOp<mlir::LLVM::IntToPtrOp>(op, dstType, val);
      return mlir::success();
    }

    // 1. Strict Memory Reinterpretation (Exact Size Match)
    if (srcW != 0 && srcW == dstW) {
      mlir::Value currentVal = val;
      if (srcType != dstType) {
        currentVal =
            rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(), dstType, val);
      }

      // True F8 Simulation: Masking out lower mantissa bits for f16 -> f8
      mlir::Type origDstType = op.getResult().getType();
      if (dstW == 16 && (mlir::isa<mlir::Float8E5M2Type>(origDstType) ||
                         mlir::isa<mlir::Float8E4M3FNType>(origDstType))) {
        auto i16Ty = rewriter.getI16Type();
        mlir::Value asInt = rewriter.create<mlir::LLVM::BitcastOp>(
            op.getLoc(), i16Ty, currentVal);
        uint16_t maskVal =
            mlir::isa<mlir::Float8E5M2Type>(origDstType) ? 0xFF00 : 0xFF80;
        mlir::Value mask = rewriter.create<mlir::LLVM::ConstantOp>(
            op.getLoc(), i16Ty, rewriter.getI16IntegerAttr(maskVal));
        mlir::Value masked =
            rewriter.create<mlir::LLVM::AndOp>(op.getLoc(), i16Ty, asInt, mask);
        currentVal = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(),
                                                            dstType, masked);
      }

      rewriter.replaceOp(op, currentVal);
      return mlir::success();
    }

    // 2. Graceful Fallbacks for AST Size Mismatches
    if (mlir::isa<mlir::FloatType>(srcType) &&
        mlir::isa<mlir::FloatType>(dstType)) {
      mlir::Value currentVal = val;

      if (srcW < dstW) {
        currentVal = rewriter.create<mlir::LLVM::FPExtOp>(op.getLoc(), dstType,
                                                          currentVal);
      } else {
        if (srcW == 64 && dstW <= 16) {
          auto f32Ty = rewriter.getF32Type();
          currentVal = rewriter.create<mlir::LLVM::FPTruncOp>(
              op.getLoc(), f32Ty, currentVal);
        }
        currentVal = rewriter.create<mlir::LLVM::FPTruncOp>(
            op.getLoc(), dstType, currentVal);
      }

      // True F8 Simulation: Apply the mask after truncation
      mlir::Type origDstType = op.getResult().getType();
      if (dstW == 16 && (mlir::isa<mlir::Float8E5M2Type>(origDstType) ||
                         mlir::isa<mlir::Float8E4M3FNType>(origDstType))) {
        auto i16Ty = rewriter.getI16Type();
        mlir::Value asInt = rewriter.create<mlir::LLVM::BitcastOp>(
            op.getLoc(), i16Ty, currentVal);
        uint16_t maskVal =
            mlir::isa<mlir::Float8E5M2Type>(origDstType) ? 0xFF00 : 0xFF80;
        mlir::Value mask = rewriter.create<mlir::LLVM::ConstantOp>(
            op.getLoc(), i16Ty, rewriter.getI16IntegerAttr(maskVal));
        mlir::Value masked =
            rewriter.create<mlir::LLVM::AndOp>(op.getLoc(), i16Ty, asInt, mask);
        currentVal = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(),
                                                            dstType, masked);
      }

      rewriter.replaceOp(op, currentVal);
      return mlir::success();
    }

    if (srcType.isIntOrIndex() && dstType.isIntOrIndex()) {
      if (srcW < dstW) {
        rewriter.replaceOpWithNewOp<mlir::LLVM::ZExtOp>(op, dstType, val);
      } else {
        rewriter.replaceOpWithNewOp<mlir::LLVM::TruncOp>(op, dstType, val);
      }
      return mlir::success();
    }

    if (mlir::isa<mlir::FloatType>(srcType) && dstType.isIntOrIndex()) {
      rewriter.replaceOpWithNewOp<mlir::LLVM::FPToSIOp>(op, dstType, val);
      return mlir::success();
    }
    if (srcType.isIntOrIndex() && mlir::isa<mlir::FloatType>(dstType)) {
      rewriter.replaceOpWithNewOp<mlir::LLVM::SIToFPOp>(op, dstType, val);
      return mlir::success();
    }

    if (srcType.isIntOrFloat() &&
        mlir::isa<moksha::IR::DecimalType>(origDstType)) {
      auto decTy = mlir::cast<moksha::IR::DecimalType>(origDstType);
      mlir::Value currentVal = val;

      // 1. Convert the primitive to f64 for the runtime call
      if (srcType.isIntOrIndex()) {
        currentVal = rewriter.create<mlir::LLVM::SIToFPOp>(
            op.getLoc(), rewriter.getF64Type(), currentVal);
      } else if (srcType.getIntOrFloatBitWidth() < 64) {
        currentVal = safeUpcastFPExt(rewriter, op.getLoc(), currentVal,
                                     rewriter.getF64Type());
      }

      // 2. Allocate the decimal struct memory
      mlir::Type llvmDecTy = typeConverter->convertType(decTy);
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));
      mlir::Value allocaPtr = rewriter.create<mlir::LLVM::AllocaOp>(
          op.getLoc(), llvmPtrTy, llvmDecTy, one);

      // 3. Call the runtime converter
      mlir::Value scaleVal = rewriter.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), rewriter.getI32Type(),
          rewriter.getI32IntegerAttr(decTy.getScale()));
      createRuntimeCall(rewriter, op.getLoc(), "__moksha_f64_to_decimal",
                        mlir::TypeRange{}, {allocaPtr, currentVal, scaleVal});

      // 4. Load and return the populated struct
      mlir::Value loadedDec = rewriter.create<mlir::LLVM::LoadOp>(
          op.getLoc(), llvmDecTy, allocaPtr);

      rewriter.replaceOp(op, loadedDec);
      return mlir::success();
    }

    // Pointer -> Aggregate (Dereference load instead of illegal bitcast)
    if (mlir::isa<mlir::LLVM::LLVMPointerType>(srcType) &&
        (mlir::isa<mlir::LLVM::LLVMStructType>(dstType) ||
         mlir::isa<mlir::LLVM::LLVMArrayType>(dstType))) {
      rewriter.replaceOpWithNewOp<mlir::LLVM::LoadOp>(op, dstType, val);
      return mlir::success();
    }

    auto isAggregate = [](mlir::Type t) {
      return mlir::isa<mlir::LLVM::LLVMStructType>(t) ||
             mlir::isa<mlir::LLVM::LLVMArrayType>(t);
    };

    // Universal Aggregate Coercion
    if (isAggregate(srcType) || isAggregate(dstType)) {
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));

      // 1. Allocate stack memory sized for the source
      mlir::Value alloc = rewriter.create<mlir::LLVM::AllocaOp>(
          op.getLoc(), llvmPtrTy, srcType, one);

      // 2. Store the original value
      rewriter.create<mlir::LLVM::StoreOp>(op.getLoc(), val, alloc);

      // 3. Load it back using the destination type from the same opaque pointer
      mlir::Value loaded =
          rewriter.create<mlir::LLVM::LoadOp>(op.getLoc(), dstType, alloc);

      rewriter.replaceOp(op, loaded);
      return mlir::success();
    }

    // Default Fallback (Strictly Primitives and Pointers)
    rewriter.replaceOpWithNewOp<mlir::LLVM::BitcastOp>(op, dstType, val);
    return mlir::success();
  }
};

struct CastOpLowering : public mlir::ConvertOpToLLVMPattern<IR::CastOp> {
  using ConvertOpToLLVMPattern<IR::CastOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::CastOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Type dstType = typeConverter->convertType(op.getType());
    mlir::Type srcType = adaptor.getValue().getType();
    mlir::Type origSrcType = op.getValue().getType();
    mlir::Type origDstType = op.getType();

    // WINDOWS MINGW FPU TRAP BYPASS
    if (auto constOp =
            adaptor.getValue().getDefiningOp<mlir::LLVM::ConstantOp>()) {

      // Int -> Float Pre-fold
      if (srcType.isIntOrIndex() && mlir::isa<mlir::FloatType>(dstType)) {
        if (auto intAttr =
                mlir::dyn_cast<mlir::IntegerAttr>(constOp.getValue())) {
          double dVal =
              origSrcType.isUnsignedInteger()
                  ? static_cast<double>(static_cast<uint64_t>(intAttr.getInt()))
                  : static_cast<double>(intAttr.getInt());
          rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
              op, dstType, rewriter.getFloatAttr(dstType, dVal));
          return mlir::success();
        }
      }

      // Float -> Int Pre-fold
      if (mlir::isa<mlir::FloatType>(srcType) && dstType.isIntOrIndex()) {
        if (auto floatAttr =
                mlir::dyn_cast<mlir::FloatAttr>(constOp.getValue())) {
          double fVal = floatAttr.getValue().convertToDouble();
          int64_t intVal = static_cast<int64_t>(fVal);
          rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
              op, dstType, rewriter.getIntegerAttr(dstType, intVal));
          return mlir::success();
        }
      }

      // Float -> Float Pre-fold (Precision trunc/ext)
      if (mlir::isa<mlir::FloatType>(srcType) &&
          mlir::isa<mlir::FloatType>(dstType)) {
        if (auto floatAttr =
                mlir::dyn_cast<mlir::FloatAttr>(constOp.getValue())) {
          auto targetTy = mlir::cast<mlir::FloatType>(dstType);
          llvm::APFloat apVal = floatAttr.getValue();
          bool losesInfo;
          apVal.convert(targetTy.getFloatSemantics(),
                        llvm::APFloat::rmNearestTiesToEven, &losesInfo);
          bool isTargetQuarter = mlir::isa<mlir::Float8E5M2Type>(origDstType) ||
                                 mlir::isa<mlir::Float8E4M3FNType>(origDstType);
          if (isTargetQuarter && dstType.isF16()) {
            llvm::APInt api = apVal.bitcastToAPInt();
            uint16_t maskVal =
                mlir::isa<mlir::Float8E5M2Type>(origDstType) ? 0xFF00 : 0xFF80;
            api &= maskVal;
            apVal = llvm::APFloat(targetTy.getFloatSemantics(), api);
          }
          rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
              op, dstType, rewriter.getFloatAttr(dstType, apVal));
          return mlir::success();
        }
      }
    }

    // AnyType Boxing (Value -> Any)
    if (mlir::isa<IR::AnyType>(origDstType)) {
      auto loc = op.getLoc();
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      mlir::Value dataPtr = adaptor.getValue();

      if (dataPtr.getType() != llvmPtrTy) {
        dataPtr =
            rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy, dataPtr);
      }

      mlir::Type llvmUnderlyingTy = typeConverter->convertType(origSrcType);
      if (auto ptrTy = mlir::dyn_cast<IR::PointerType>(origSrcType)) {
        llvmUnderlyingTy = typeConverter->convertType(ptrTy.getPointee());
      }

      mlir::Value vtablePtr =
          getOrCreateAnyVTable(rewriter, loc, origSrcType, llvmUnderlyingTy);

      // Assemble the {ptr, ptr} fat pointer
      mlir::Value anyStruct =
          rewriter.create<mlir::LLVM::UndefOp>(loc, dstType);
      anyStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
          loc, anyStruct, dataPtr, llvm::ArrayRef<int64_t>{0});
      anyStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
          loc, anyStruct, vtablePtr, llvm::ArrayRef<int64_t>{1});

      rewriter.replaceOp(op, anyStruct);
      return mlir::success();
    }

    // AnyType Unboxing (Any -> Value)
    if (mlir::isa<IR::AnyType>(origSrcType)) {
      auto loc = op.getLoc();

      mlir::Value dataPtr = rewriter.create<mlir::LLVM::ExtractValueOp>(
          loc, adaptor.getValue(), llvm::ArrayRef<int64_t>{0});
      if (dataPtr.getType() != dstType) {
        dataPtr = rewriter.create<mlir::LLVM::BitcastOp>(loc, dstType, dataPtr);
      }

      rewriter.replaceOp(op, dataPtr);
      return mlir::success();
    }

    // Array to Slice Cast (Stack to Heap Migration)
    if (auto arrayTy = mlir::dyn_cast<IR::ArrayType>(origSrcType)) {
      if (mlir::isa<IR::SliceType>(origDstType)) {
        auto loc = op.getLoc();
        mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
        mlir::Value arrayPtr = adaptor.getValue();

        int64_t actualSize = arrayTy.getSize();
        mlir::Type elemType =
            typeConverter->convertType(arrayTy.getElementType());
        mlir::Type i64Ty = rewriter.getI64Type();
        mlir::Type i32Ty = rewriter.getI32Type();

        // 1. If passed by value, spill array to stack to get a source pointer
        if (arrayPtr.getType() != llvmPtrTy) {
          mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
              loc, i32Ty, rewriter.getI32IntegerAttr(1));
          mlir::Value stackAlloc = rewriter.create<mlir::LLVM::AllocaOp>(
              loc, llvmPtrTy, srcType, one);
          rewriter.create<mlir::LLVM::StoreOp>(loc, arrayPtr, stackAlloc);
          arrayPtr = stackAlloc;
        }

        // 2. Calculate dynamic element size
        mlir::Value nullPtr =
            rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmPtrTy);
        mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
            loc, i32Ty, rewriter.getI32IntegerAttr(1));
        mlir::Value sizeGep = rewriter.create<mlir::LLVM::GEPOp>(
            loc, llvmPtrTy, elemType, nullPtr,
            llvm::ArrayRef<mlir::LLVM::GEPArg>{one});
        mlir::Value elemSizeInt =
            rewriter.create<mlir::LLVM::PtrToIntOp>(loc, i32Ty, sizeGep);

        // 3. Allocate new Slice on the heap
        mlir::Value countVal = rewriter.create<mlir::LLVM::ConstantOp>(
            loc, i64Ty, rewriter.getI64IntegerAttr(actualSize));
        mlir::Value heapSlicePtr =
            createRuntimeCall(rewriter, loc, llvmPtrTy, "moksha_rt_array_alloc",
                              {elemSizeInt, countVal});

        // 4. Memcpy the stack data to the new heap buffer
        mlir::Value destDataPtr = createRuntimeCall(
            rewriter, loc, llvmPtrTy, "moksha_rt_array_data", {heapSlicePtr});

        mlir::Value elemSize64 =
            rewriter.create<mlir::LLVM::ZExtOp>(loc, i64Ty, elemSizeInt);
        mlir::Value totalBytes = rewriter.create<mlir::LLVM::MulOp>(
            loc, i64Ty, elemSize64, countVal);

        createRuntimeCall(rewriter, loc, llvmPtrTy, "memcpy",
                          {destDataPtr, arrayPtr, totalBytes});

        rewriter.replaceOp(op, heapSlicePtr);
        return mlir::success();
      }
    }

    if (origSrcType.isIntOrIndex() && mlir::isa<IR::PointerType>(origDstType)) {
      auto ptrTy = mlir::cast<IR::PointerType>(origDstType);

      if (ptrTy.getPointee().isInteger(8)) {
        auto loc = op.getLoc();

        std::string funcName = origSrcType.getIntOrFloatBitWidth() == 64
                                   ? "__moksha_long_to_string"
                                   : "__moksha_int_to_string";

        mlir::Value callRes = createRuntimeCall(rewriter, loc, dstType,
                                                funcName, {adaptor.getValue()});

        rewriter.replaceOp(op, callRes);
        return mlir::success();
      }
    }

    // Intercept Decimal Scaling Casts
    auto isDecStruct = [](mlir::Type t) {
      auto st = mlir::dyn_cast<mlir::LLVM::LLVMStructType>(t);
      return st && st.getBody().size() == 2 && st.getBody()[0].isInteger(128) &&
             st.getBody()[1].isInteger(32);
    };

    if (isDecStruct(srcType) && isDecStruct(dstType)) {
      uint32_t targetScale = 0;
      if (auto decType = mlir::dyn_cast<IR::DecimalType>(origDstType)) {
        targetScale = decType.getScale();
      }
      mlir::Value scaleVal = rewriter.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), rewriter.getI32Type(),
          rewriter.getI32IntegerAttr(targetScale));

      auto loc = op.getLoc();
      mlir::Type ptrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));

      mlir::Value inPtr =
          rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, srcType, one);
      rewriter.create<mlir::LLVM::StoreOp>(loc, adaptor.getValue(), inPtr);
      mlir::Value outPtr =
          rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, dstType, one);

      createRuntimeCall(rewriter, loc, "__moksha_dec_scale", mlir::TypeRange{},
                        {outPtr, inPtr, scaleVal});

      mlir::Value loaded =
          rewriter.create<mlir::LLVM::LoadOp>(loc, dstType, outPtr);
      rewriter.replaceOp(op, loaded);
      return mlir::success();
    }

    // Intercept Float -> Decimal Casts (Safe for f8, f16, f32, f64)
    if (mlir::isa<mlir::FloatType>(srcType) && isDecStruct(dstType)) {
      uint32_t targetScale = 0;
      if (auto decType = mlir::dyn_cast<IR::DecimalType>(origDstType)) {
        targetScale = decType.getScale();
      }

      auto loc = op.getLoc();
      mlir::Type f64Ty = rewriter.getF64Type();
      mlir::Type i128Ty = rewriter.getIntegerType(128);
      mlir::Type i32Ty = rewriter.getI32Type();

      // Float -> Decimal Constant Pre-folding
      if (auto constOp =
              adaptor.getValue().getDefiningOp<mlir::LLVM::ConstantOp>()) {
        if (auto floatAttr =
                mlir::dyn_cast<mlir::FloatAttr>(constOp.getValue())) {
          double fVal = floatAttr.getValue().convertToDouble();
          double multiplier = 1.0;
          for (uint32_t i = 0; i < targetScale; ++i) {
            multiplier *= 10.0;
          }

          double scaled = fVal * multiplier;
          int64_t mantissaVal = static_cast<int64_t>(scaled);

          mlir::Value decStruct =
              rewriter.create<mlir::LLVM::UndefOp>(loc, dstType);

          // Use Sign-Extended 128-bit APInt
          llvm::APInt mantissaAPInt(128, mantissaVal, true);
          mlir::Value mantissa = rewriter.create<mlir::LLVM::ConstantOp>(
              loc, i128Ty, rewriter.getIntegerAttr(i128Ty, mantissaAPInt));

          mlir::Value scaleVal = rewriter.create<mlir::LLVM::ConstantOp>(
              loc, i32Ty, rewriter.getI32IntegerAttr(targetScale));

          decStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
              loc, decStruct, mantissa, llvm::ArrayRef<int64_t>{0});
          decStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
              loc, decStruct, scaleVal, llvm::ArrayRef<int64_t>{1});

          rewriter.replaceOp(op, decStruct);
          return mlir::success();
        }
      }

      mlir::Value valF64 = adaptor.getValue();
      if (srcType != f64Ty) {
        valF64 = safeUpcastFPExt(rewriter, loc, valF64, f64Ty);
      }

      double multiplier = 1.0;
      for (uint32_t i = 0; i < targetScale; ++i) {
        multiplier *= 10.0;
      }

      llvm::APFloat apMult(multiplier);
      mlir::Value multVal = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, f64Ty, rewriter.getFloatAttr(f64Ty, apMult));

      mlir::Value scaledFloat =
          rewriter.create<mlir::LLVM::FMulOp>(loc, f64Ty, valF64, multVal);
      mlir::Value mantissa =
          rewriter.create<mlir::LLVM::FPToSIOp>(loc, i128Ty, scaledFloat);
      mlir::Value scaleVal = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, i32Ty, rewriter.getI32IntegerAttr(targetScale));

      mlir::Value decStruct =
          rewriter.create<mlir::LLVM::UndefOp>(loc, dstType);
      decStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
          loc, decStruct, mantissa, llvm::ArrayRef<int64_t>{0});
      decStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
          loc, decStruct, scaleVal, llvm::ArrayRef<int64_t>{1});

      rewriter.replaceOp(op, decStruct);
      return mlir::success();
    }

    if (srcType.isIntOrIndex() && isDecStruct(dstType)) {
      uint32_t targetScale = 0;
      if (auto decType = mlir::dyn_cast<IR::DecimalType>(origDstType)) {
        targetScale = decType.getScale();
      }

      auto loc = op.getLoc();
      mlir::Type i128Ty = rewriter.getIntegerType(128);
      mlir::Type i32Ty = rewriter.getI32Type();

      mlir::Value mantissa;
      if (origSrcType.isUnsignedInteger()) {
        mantissa = rewriter.create<mlir::LLVM::ZExtOp>(loc, i128Ty,
                                                       adaptor.getValue());
      } else {
        mantissa = rewriter.create<mlir::LLVM::SExtOp>(loc, i128Ty,
                                                       adaptor.getValue());
      }

      int64_t multiplier = 1;
      for (uint32_t i = 0; i < targetScale; ++i) {
        multiplier *= 10;
      }

      mlir::Value multVal = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, i128Ty, rewriter.getIntegerAttr(i128Ty, multiplier));

      mantissa =
          rewriter.create<mlir::LLVM::MulOp>(loc, i128Ty, mantissa, multVal);
      mlir::Value scaleVal = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, i32Ty, rewriter.getI32IntegerAttr(targetScale));

      mlir::Value decStruct =
          rewriter.create<mlir::LLVM::UndefOp>(loc, dstType);
      decStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
          loc, decStruct, mantissa, llvm::ArrayRef<int64_t>{0});
      decStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
          loc, decStruct, scaleVal, llvm::ArrayRef<int64_t>{1});

      rewriter.replaceOp(op, decStruct);
      return mlir::success();
    }

    if (isDecStruct(srcType) && mlir::isa<mlir::FloatType>(dstType)) {
      uint32_t sourceScale = 0;
      if (auto decType = mlir::dyn_cast<IR::DecimalType>(origSrcType)) {
        sourceScale = decType.getScale();
      }

      auto loc = op.getLoc();
      mlir::Type f64Ty = rewriter.getF64Type();

      mlir::Value mantissa = rewriter.create<mlir::LLVM::ExtractValueOp>(
          loc, adaptor.getValue(), llvm::ArrayRef<int64_t>{0});

      mlir::Value mantissaF64 =
          rewriter.create<mlir::LLVM::SIToFPOp>(loc, f64Ty, mantissa);

      double divisor = 1.0;
      for (uint32_t i = 0; i < sourceScale; ++i) {
        divisor *= 10.0;
      }

      llvm::APFloat apDiv(divisor);
      mlir::Value divVal = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, f64Ty, rewriter.getFloatAttr(f64Ty, apDiv));

      mlir::Value resultF64 =
          rewriter.create<mlir::LLVM::FDivOp>(loc, f64Ty, mantissaF64, divVal);

      if (dstType != f64Ty) {
        rewriter.replaceOpWithNewOp<mlir::LLVM::FPTruncOp>(op, dstType,
                                                           resultF64);
      } else {
        rewriter.replaceOp(op, resultF64);
      }
      return mlir::success();
    }

    if (isDecStruct(srcType) && dstType.isIntOrIndex()) {
      uint32_t sourceScale = 0;
      if (auto decType = mlir::dyn_cast<IR::DecimalType>(origSrcType)) {
        sourceScale = decType.getScale();
      }

      auto loc = op.getLoc();
      mlir::Type i128Ty = rewriter.getIntegerType(128);

      mlir::Value mantissa = rewriter.create<mlir::LLVM::ExtractValueOp>(
          loc, adaptor.getValue(), llvm::ArrayRef<int64_t>{0});

      int64_t divisor = 1;
      for (uint32_t i = 0; i < sourceScale; ++i) {
        divisor *= 10;
      }

      mlir::Value divVal = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, i128Ty, rewriter.getIntegerAttr(i128Ty, divisor));

      mlir::Value resultI128 =
          rewriter.create<mlir::LLVM::SDivOp>(loc, i128Ty, mantissa, divVal);

      rewriter.replaceOpWithNewOp<mlir::LLVM::TruncOp>(op, dstType, resultI128);
      return mlir::success();
    }

    // Pointer -> Array Value
    if (mlir::isa<mlir::LLVM::LLVMPointerType>(srcType) &&
        mlir::isa<mlir::LLVM::LLVMArrayType>(dstType)) {
      rewriter.replaceOpWithNewOp<mlir::LLVM::LoadOp>(op, dstType,
                                                      adaptor.getValue());
      return mlir::success();
    }

    // Array Value -> Pointer
    if (mlir::isa<mlir::LLVM::LLVMArrayType>(srcType) &&
        mlir::isa<mlir::LLVM::LLVMPointerType>(dstType)) {

      if (mlir::isa<IR::PointerType>(origSrcType)) {
        rewriter.replaceOpWithNewOp<mlir::LLVM::BitcastOp>(op, dstType,
                                                           adaptor.getValue());
        return mlir::success();
      }

      auto parentFunc = op->getParentOfType<mlir::LLVM::LLVMFuncOp>();
      mlir::OpBuilder entryBuilder(parentFunc.getContext());
      entryBuilder.setInsertionPointToStart(&parentFunc.getBody().front());

      mlir::Value one = entryBuilder.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), entryBuilder.getI32Type(),
          entryBuilder.getI32IntegerAttr(1));

      mlir::Value allocaPtr = entryBuilder.create<mlir::LLVM::AllocaOp>(
          op.getLoc(), dstType, srcType, one);

      rewriter.create<mlir::LLVM::StoreOp>(op.getLoc(), adaptor.getValue(),
                                           allocaPtr);
      rewriter.replaceOp(op, allocaPtr);
      return mlir::success();
    }

    // 7. Raw Pointer to Slice Struct (Ptr -> Struct)
    if (mlir::isa<mlir::LLVM::LLVMPointerType>(srcType) &&
        mlir::isa<mlir::LLVM::LLVMStructType>(dstType)) {
      auto structTy = mlir::cast<mlir::LLVM::LLVMStructType>(dstType);

      if (structTy.getBody().size() == 2 &&
          mlir::isa<mlir::LLVM::LLVMPointerType>(structTy.getBody()[0]) &&
          structTy.getBody()[1].isInteger(64)) {

        mlir::Value slice =
            rewriter.create<mlir::LLVM::UndefOp>(op.getLoc(), dstType);
        mlir::Value ptr = adaptor.getValue();

        int64_t actualSize = 0;
        mlir::Type elemType = nullptr;
        mlir::Type checkTy = origSrcType;
        if (auto decayCast = op.getValue().getDefiningOp<IR::CastOp>()) {
          if (auto ptrTy = mlir::dyn_cast<IR::PointerType>(
                  decayCast.getValue().getType())) {
            if (mlir::isa<IR::ArrayType>(ptrTy.getPointee())) {
              checkTy = decayCast.getValue().getType();
            }
          }
        }
        if (auto ptrTy = mlir::dyn_cast<IR::PointerType>(checkTy)) {
          if (auto arrTy = mlir::dyn_cast<IR::ArrayType>(ptrTy.getPointee())) {
            actualSize = arrTy.getSize();
            elemType = typeConverter->convertType(arrTy.getElementType());
          }
        } else if (auto arrTy = mlir::dyn_cast<IR::ArrayType>(checkTy)) {
          actualSize = arrTy.getSize();
          elemType = typeConverter->convertType(arrTy.getElementType());
        }

        mlir::Value dataPtr = ptr;

        // Heap-allocate the array before assigning it to the slice
        if (elemType) {
          mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
          mlir::Type i64Ty = rewriter.getI64Type();
          mlir::Type i32Ty = rewriter.getI32Type();

          mlir::Value nullPtr =
              rewriter.create<mlir::LLVM::ZeroOp>(op.getLoc(), llvmPtrTy);
          mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
              op.getLoc(), i32Ty, rewriter.getI32IntegerAttr(1));
          mlir::Value sizeGep = rewriter.create<mlir::LLVM::GEPOp>(
              op.getLoc(), llvmPtrTy, elemType, nullPtr,
              llvm::ArrayRef<mlir::LLVM::GEPArg>{one});
          mlir::Value elemSizeInt = rewriter.create<mlir::LLVM::PtrToIntOp>(
              op.getLoc(), i64Ty, sizeGep);

          mlir::Value countVal = rewriter.create<mlir::LLVM::ConstantOp>(
              op.getLoc(), i64Ty, rewriter.getI64IntegerAttr(actualSize));
          mlir::Value totalBytes = rewriter.create<mlir::LLVM::MulOp>(
              op.getLoc(), elemSizeInt, countVal);

          mlir::Value typeTag = rewriter.create<mlir::LLVM::ConstantOp>(
              op.getLoc(), i32Ty, rewriter.getI32IntegerAttr(18));
          mlir::Value heapPtr =
              createRuntimeCall(rewriter, op.getLoc(), llvmPtrTy,
                                "moksha_rt_alloc", {totalBytes, typeTag});

          mlir::Value srcVoid = ptr;
          if (srcVoid.getType() != llvmPtrTy) {
            srcVoid = rewriter.create<mlir::LLVM::BitcastOp>(
                op.getLoc(), llvmPtrTy, srcVoid);
          }
          createRuntimeCall(rewriter, op.getLoc(), llvmPtrTy, "memcpy",
                            {heapPtr, srcVoid, totalBytes});

          dataPtr = heapPtr;
        }

        if (dataPtr.getType() != structTy.getBody()[0]) {
          dataPtr = rewriter.create<mlir::LLVM::BitcastOp>(
              op.getLoc(), structTy.getBody()[0], dataPtr);
        }

        slice = rewriter.create<mlir::LLVM::InsertValueOp>(
            op.getLoc(), slice, dataPtr, llvm::ArrayRef<int64_t>({0}));

        mlir::Type lenTy = structTy.getBody()[1];
        mlir::Value trueLen = rewriter.create<mlir::LLVM::ConstantOp>(
            op.getLoc(), lenTy, rewriter.getIntegerAttr(lenTy, actualSize));

        slice = rewriter.create<mlir::LLVM::InsertValueOp>(
            op.getLoc(), slice, trueLen, llvm::ArrayRef<int64_t>({1}));

        rewriter.replaceOp(op, slice);
        return mlir::success();
      }
    }

    // 8. Int <-> Int
    if (srcType.isIntOrIndex() && dstType.isIntOrIndex()) {
      unsigned srcW = srcType.getIntOrFloatBitWidth();
      unsigned dstW = dstType.getIntOrFloatBitWidth();
      if (srcW > dstW) {
        rewriter.replaceOpWithNewOp<mlir::LLVM::TruncOp>(op, dstType,
                                                         adaptor.getValue());
      } else if (srcW < dstW) {
        if (origSrcType.isUnsignedInteger()) {
          rewriter.replaceOpWithNewOp<mlir::LLVM::ZExtOp>(op, dstType,
                                                          adaptor.getValue());
        } else {
          rewriter.replaceOpWithNewOp<mlir::LLVM::SExtOp>(op, dstType,
                                                          adaptor.getValue());
        }
      } else {
        rewriter.replaceOp(op, adaptor.getValue());
      }
      return mlir::success();
    }

    // 8b. Int <-> Float Conversions
    if (srcType.isIntOrIndex() && mlir::isa<mlir::FloatType>(dstType)) {
      if (origSrcType.isUnsignedInteger()) {
        rewriter.replaceOpWithNewOp<mlir::LLVM::UIToFPOp>(op, dstType,
                                                          adaptor.getValue());
      } else {
        rewriter.replaceOpWithNewOp<mlir::LLVM::SIToFPOp>(op, dstType,
                                                          adaptor.getValue());
      }
      return mlir::success();
    }
    if (mlir::isa<mlir::FloatType>(srcType) && dstType.isIntOrIndex()) {
      if (origDstType.isUnsignedInteger()) {
        rewriter.replaceOpWithNewOp<mlir::LLVM::FPToUIOp>(op, dstType,
                                                          adaptor.getValue());
      } else {
        rewriter.replaceOpWithNewOp<mlir::LLVM::FPToSIOp>(op, dstType,
                                                          adaptor.getValue());
      }
      return mlir::success();
    }

    // 8c. Float <-> Float Conversions (Precision changes)
    if (mlir::isa<mlir::FloatType>(srcType) &&
        mlir::isa<mlir::FloatType>(dstType)) {
      unsigned srcW = srcType.getIntOrFloatBitWidth();
      unsigned dstW = dstType.getIntOrFloatBitWidth();
      mlir::Value currentVal = adaptor.getValue();

      if (srcW < dstW) {
        currentVal = rewriter.create<mlir::LLVM::FPExtOp>(op.getLoc(), dstType,
                                                          currentVal);
      } else if (srcW > dstW) {
        if (srcW == 64 && dstW <= 16) {
          auto f32Ty = rewriter.getF32Type();
          currentVal = rewriter.create<mlir::LLVM::FPTruncOp>(
              op.getLoc(), f32Ty, currentVal);
        }
        currentVal = rewriter.create<mlir::LLVM::FPTruncOp>(
            op.getLoc(), dstType, currentVal);
      }

      bool isTargetQuarter = mlir::isa<mlir::Float8E5M2Type>(origDstType) ||
                             mlir::isa<mlir::Float8E4M3FNType>(origDstType);

      if (isTargetQuarter && dstW == 16) {
        auto i16Ty = rewriter.getI16Type();
        mlir::Value asInt = rewriter.create<mlir::LLVM::BitcastOp>(
            op.getLoc(), i16Ty, currentVal);
        uint16_t maskVal =
            mlir::isa<mlir::Float8E5M2Type>(origDstType) ? 0xFF00 : 0xFF80;
        mlir::Value mask = rewriter.create<mlir::LLVM::ConstantOp>(
            op.getLoc(), i16Ty, rewriter.getI16IntegerAttr(maskVal));
        mlir::Value masked =
            rewriter.create<mlir::LLVM::AndOp>(op.getLoc(), i16Ty, asInt, mask);
        currentVal = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(),
                                                            dstType, masked);
      }

      rewriter.replaceOp(op, currentVal);
      return mlir::success();
    }

    // 9. Pointer <-> Integer
    if (mlir::isa<mlir::LLVM::LLVMPointerType>(srcType) &&
        dstType.isIntOrIndex()) {
      rewriter.replaceOpWithNewOp<mlir::LLVM::PtrToIntOp>(op, dstType,
                                                          adaptor.getValue());
      return mlir::success();
    }

    if (srcType.isIntOrIndex() &&
        mlir::isa<mlir::LLVM::LLVMPointerType>(dstType)) {
      rewriter.replaceOpWithNewOp<mlir::LLVM::IntToPtrOp>(op, dstType,
                                                          adaptor.getValue());
      return mlir::success();
    }

    // 11. Standard Fallback (Aggregate Safety Hook)
    if (srcType != dstType) {
      auto isAggregate = [](mlir::Type t) {
        return mlir::isa<mlir::LLVM::LLVMStructType>(t) ||
               mlir::isa<mlir::LLVM::LLVMArrayType>(t);
      };

      if (isAggregate(srcType) && isAggregate(dstType)) {
        mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
            op.getLoc(), rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));
        mlir::Value allocaSrc = rewriter.create<mlir::LLVM::AllocaOp>(
            op.getLoc(),
            mlir::LLVM::LLVMPointerType::get(rewriter.getContext()), srcType,
            one);
        rewriter.create<mlir::LLVM::StoreOp>(op.getLoc(), adaptor.getValue(),
                                             allocaSrc);

        mlir::Value loadedDst = rewriter.create<mlir::LLVM::LoadOp>(
            op.getLoc(), dstType, allocaSrc);
        rewriter.replaceOp(op, loadedDst);
      } else if (isAggregate(srcType) &&
                 mlir::isa<mlir::LLVM::LLVMPointerType>(dstType)) {
        mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
            op.getLoc(), rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));
        mlir::Value allocaPtr = rewriter.create<mlir::LLVM::AllocaOp>(
            op.getLoc(),
            mlir::LLVM::LLVMPointerType::get(rewriter.getContext()), srcType,
            one);

        rewriter.create<mlir::LLVM::StoreOp>(op.getLoc(), adaptor.getValue(),
                                             allocaPtr);

        if (allocaPtr.getType() != dstType) {
          rewriter.replaceOpWithNewOp<mlir::LLVM::BitcastOp>(op, dstType,
                                                             allocaPtr);
        } else {
          rewriter.replaceOp(op, allocaPtr);
        }
      } else if (isAggregate(srcType) || isAggregate(dstType)) {
        rewriter.replaceOpWithNewOp<mlir::LLVM::ZeroOp>(op, dstType);
      } else {
        if (mlir::isa<mlir::LLVM::LLVMPointerType>(srcType) &&
            mlir::isa<mlir::LLVM::LLVMFunctionType>(dstType)) {
          rewriter.replaceOp(op, adaptor.getValue());
        } else {
          if (mlir::isa<mlir::LLVM::LLVMStructType>(srcType) ||
              mlir::isa<mlir::LLVM::LLVMArrayType>(srcType) ||
              mlir::isa<mlir::LLVM::LLVMStructType>(dstType) ||
              mlir::isa<mlir::LLVM::LLVMArrayType>(dstType)) {

            mlir::Type llvmPtrTy =
                mlir::LLVM::LLVMPointerType::get(getContext());
            mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
                op.getLoc(), rewriter.getI32Type(),
                rewriter.getI32IntegerAttr(1));
            mlir::Value allocaSrc = rewriter.create<mlir::LLVM::AllocaOp>(
                op.getLoc(), llvmPtrTy, srcType, one);
            rewriter.create<mlir::LLVM::StoreOp>(op.getLoc(),
                                                 adaptor.getValue(), allocaSrc);

            mlir::Value castPtr = rewriter.create<mlir::LLVM::BitcastOp>(
                op.getLoc(), llvmPtrTy, allocaSrc);
            mlir::Value loadedDst = rewriter.create<mlir::LLVM::LoadOp>(
                op.getLoc(), dstType, castPtr);
            rewriter.replaceOp(op, loadedDst);
          } else {
            rewriter.replaceOpWithNewOp<mlir::LLVM::BitcastOp>(
                op, dstType, adaptor.getValue());
          }
        }
      }
    } else {
      rewriter.replaceOp(op, adaptor.getValue());
    }
    return mlir::success();
  }
};

struct UpcastOpLowering : public mlir::ConvertOpToLLVMPattern<IR::UpcastOp> {
  using ConvertOpToLLVMPattern<IR::UpcastOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::UpcastOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    auto loc = op.getLoc();
    mlir::Type dstType = typeConverter->convertType(op.getType());
    mlir::Value val = adaptor.getOperand();
    mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());

    int64_t byteOffset = 0;
    if (auto offsetAttr = op->getAttrOfType<mlir::IntegerAttr>("offset")) {
      byteOffset = offsetAttr.getInt();
    }

    // Fast Path: Single Inheritance / Primary Parent
    // No byte shift is required, just reinterpret the pointer type.
    if (byteOffset == 0) {
      if (val.getType() != dstType) {
        rewriter.replaceOpWithNewOp<mlir::LLVM::BitcastOp>(op, dstType, val);
      } else {
        rewriter.replaceOp(op, val);
      }
      return mlir::success();
    }

    // Multiple Inheritance Path: Apply Byte Offset
    // 1. Ensure the value is a standard opaque pointer
    if (val.getType() != llvmPtrTy) {
      val = rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy, val);
    }

    // 2. Create the offset value (i32 is standard for GEP indices)
    mlir::Type i32Ty = rewriter.getI32Type();
    mlir::Value offsetVal = rewriter.create<mlir::LLVM::ConstantOp>(
        loc, i32Ty, rewriter.getI32IntegerAttr(byteOffset));

    // 3. Emit the GEP (GetElementPtr) using i8 to do raw byte arithmetic
    mlir::Type i8Ty = rewriter.getI8Type();
    mlir::Value gepOp = rewriter.create<mlir::LLVM::GEPOp>(
        loc, llvmPtrTy, i8Ty, val,
        llvm::ArrayRef<mlir::LLVM::GEPArg>{offsetVal});

    // 4. Bitcast the shifted pointer to the target parent class type
    if (gepOp.getType() != dstType) {
      rewriter.replaceOpWithNewOp<mlir::LLVM::BitcastOp>(op, dstType, gepOp);
    } else {
      rewriter.replaceOp(op, gepOp);
    }

    return mlir::success();
  }
};

// AnyCastOp Lowering (Boxing & Unboxing the Fat Pointer)
struct AnyCastOpLowering : public mlir::ConvertOpToLLVMPattern<IR::AnyCastOp> {
  using ConvertOpToLLVMPattern<IR::AnyCastOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::AnyCastOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    auto loc = op.getLoc();
    mlir::Type dstType = typeConverter->convertType(op.getType());
    mlir::Type origSrcType = op.getOperand().getType();
    mlir::Type origDstType = op.getType();
    mlir::Value val = adaptor.getOperand();

    mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());

    // BOXING (Value -> Any)
    if (mlir::isa<IR::AnyType>(origDstType)) {
      mlir::Value dataPtr = val;

      // Ensure the payload is an opaque pointer
      if (dataPtr.getType() != llvmPtrTy) {
        dataPtr =
            rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy, dataPtr);
      }

      // Extract the underlying concrete MLIR type for the thunk
      mlir::Type llvmUnderlyingTy = typeConverter->convertType(origSrcType);
      if (auto ptrTy = mlir::dyn_cast<IR::PointerType>(origSrcType)) {
        llvmUnderlyingTy = typeConverter->convertType(ptrTy.getPointee());
      }

      // Map the original source type to a valid VTable
      mlir::Value vtablePtr =
          getOrCreateAnyVTable(rewriter, loc, origSrcType, llvmUnderlyingTy);

      // Assemble the {ptr, ptr} fat pointer struct
      mlir::Value anyStruct =
          rewriter.create<mlir::LLVM::UndefOp>(loc, dstType);
      anyStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
          loc, anyStruct, dataPtr, llvm::ArrayRef<int64_t>{0});
      anyStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
          loc, anyStruct, vtablePtr, llvm::ArrayRef<int64_t>{1});

      rewriter.replaceOp(op, anyStruct);
      return mlir::success();
    }

    // UNBOXING (Any -> Value)
    if (mlir::isa<IR::AnyType>(origSrcType)) {
      // Extract the data pointer (Index 0) from the fat pointer struct
      mlir::Value dataPtr = rewriter.create<mlir::LLVM::ExtractValueOp>(
          loc, val, llvm::ArrayRef<int64_t>{0});

      // Cast it back to the expected concrete type
      if (dataPtr.getType() != dstType) {
        dataPtr = rewriter.create<mlir::LLVM::BitcastOp>(loc, dstType, dataPtr);
      }

      rewriter.replaceOp(op, dataPtr);
      return mlir::success();
    }

    return mlir::failure();
  }
};

struct AddressOfOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::AddressOfOp> {
  using ConvertOpToLLVMPattern<IR::AddressOfOp>::ConvertOpToLLVMPattern;
  mlir::LogicalResult
  matchAndRewrite(IR::AddressOfOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto symRef =
        mlir::FlatSymbolRefAttr::get(getContext(), op.getGlobalName());
    rewriter.replaceOpWithNewOp<mlir::LLVM::AddressOfOp>(
        op, typeConverter->convertType(op.getType()), symRef);
    return mlir::success();
  }
};

// Generic Binary Op Macro
template <typename MokshaOp, typename LLVMIntOp, typename LLVMFloatOp>
struct BinaryOpLowering : public mlir::ConvertOpToLLVMPattern<MokshaOp> {
  using mlir::ConvertOpToLLVMPattern<MokshaOp>::ConvertOpToLLVMPattern;
  mlir::LogicalResult
  matchAndRewrite(MokshaOp op, typename MokshaOp::Adaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    mlir::Type resTy = this->typeConverter->convertType(op.getType());

    if (auto structTy = mlir::dyn_cast<mlir::LLVM::LLVMStructType>(resTy)) {
      if (structTy.getBody().size() == 2 &&
          structTy.getBody()[0].isInteger(128) &&
          structTy.getBody()[1].isInteger(32)) {

        llvm::StringRef rtFunc = "__moksha_dec_add";
        if (std::is_same_v<MokshaOp, IR::SubOp>)
          rtFunc = "__moksha_dec_sub";
        else if (std::is_same_v<MokshaOp, IR::MulOp>)
          rtFunc = "__moksha_dec_mul";
        else if (std::is_same_v<MokshaOp, IR::DivOp>)
          rtFunc = "__moksha_dec_div";
        else if (std::is_same_v<MokshaOp, IR::ModOp>)
          rtFunc = "__moksha_dec_mod";

        auto loc = op.getLoc();
        mlir::Type ptrTy = mlir::LLVM::LLVMPointerType::get(this->getContext());
        mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
            loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));

        mlir::Value aPtr =
            rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, resTy, one);
        rewriter.create<mlir::LLVM::StoreOp>(loc, adaptor.getLhs(), aPtr);
        mlir::Value bPtr =
            rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, resTy, one);
        rewriter.create<mlir::LLVM::StoreOp>(loc, adaptor.getRhs(), bPtr);
        mlir::Value resPtr =
            rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, resTy, one);

        createRuntimeCall(rewriter, loc, rtFunc, mlir::TypeRange{},
                          {resPtr, aPtr, bPtr});

        mlir::Value loaded =
            rewriter.create<mlir::LLVM::LoadOp>(loc, resTy, resPtr);
        rewriter.replaceOp(op, loaded);
        return mlir::success();
      }
    }

    if (auto lhsConst =
            adaptor.getLhs().template getDefiningOp<mlir::LLVM::ConstantOp>()) {
      if (auto rhsConst =
              adaptor.getRhs()
                  .template getDefiningOp<mlir::LLVM::ConstantOp>()) {
        if (mlir::isa<mlir::FloatType>(resTy)) {
          if (auto lAttr =
                  mlir::dyn_cast<mlir::FloatAttr>(lhsConst.getValue())) {
            if (auto rAttr =
                    mlir::dyn_cast<mlir::FloatAttr>(rhsConst.getValue())) {
              llvm::APFloat lVal = lAttr.getValue();
              llvm::APFloat rVal = rAttr.getValue();

              if constexpr (std::is_same_v<MokshaOp, IR::SubOp>)
                lVal.subtract(rVal, llvm::APFloat::rmNearestTiesToEven);
              else if constexpr (std::is_same_v<MokshaOp, IR::MulOp>)
                lVal.multiply(rVal, llvm::APFloat::rmNearestTiesToEven);
              else if constexpr (std::is_same_v<MokshaOp, IR::DivOp>) {
                if (!rVal.isZero())
                  lVal.divide(rVal, llvm::APFloat::rmNearestTiesToEven);
                else
                  return mlir::failure();
              } else if constexpr (std::is_same_v<MokshaOp, IR::ModOp>) {
                if (!rVal.isZero())
                  lVal.remainder(rVal);
                else
                  return mlir::failure();
              }

              rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
                  op, resTy, rewriter.getFloatAttr(resTy, lVal));
              return mlir::success();
            }
          }
        } else if (resTy.isIntOrIndex()) {
          if (auto lAttr =
                  mlir::dyn_cast<mlir::IntegerAttr>(lhsConst.getValue())) {
            if (auto rAttr =
                    mlir::dyn_cast<mlir::IntegerAttr>(rhsConst.getValue())) {
              llvm::APInt lVal = lAttr.getValue();
              llvm::APInt rVal = rAttr.getValue();
              llvm::APInt resVal;

              bool isUnsigned = false;
              if (auto intTy = mlir::dyn_cast<mlir::IntegerType>(resTy))
                isUnsigned = intTy.isUnsigned();

              if constexpr (std::is_same_v<MokshaOp, IR::SubOp>)
                resVal = lVal - rVal;
              else if constexpr (std::is_same_v<MokshaOp, IR::MulOp>)
                resVal = lVal * rVal;
              else if constexpr (std::is_same_v<MokshaOp, IR::DivOp>) {
                if (rVal.isZero())
                  return mlir::failure();
                resVal = isUnsigned ? lVal.udiv(rVal) : lVal.sdiv(rVal);
              } else if constexpr (std::is_same_v<MokshaOp, IR::ModOp>) {
                if (rVal.isZero())
                  return mlir::failure();
                resVal = isUnsigned ? lVal.urem(rVal) : lVal.srem(rVal);
              }

              rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
                  op, resTy, rewriter.getIntegerAttr(resTy, resVal));
              return mlir::success();
            }
          }
        }
      }
    }

    if (mlir::isa<mlir::FloatType>(resTy)) {
      unsigned width = resTy.getIntOrFloatBitWidth();
      if (width < 32) {
        mlir::Type f32Ty = rewriter.getF32Type();
        mlir::Value lhs32 =
            safeUpcastFPExt(rewriter, op.getLoc(), adaptor.getLhs(), f32Ty);
        mlir::Value rhs32 =
            safeUpcastFPExt(rewriter, op.getLoc(), adaptor.getRhs(), f32Ty);

        mlir::Value res32 =
            rewriter.create<LLVMFloatOp>(op.getLoc(), f32Ty, lhs32, rhs32);

        rewriter.replaceOpWithNewOp<mlir::LLVM::FPTruncOp>(op, resTy, res32);
      } else {
        rewriter.replaceOpWithNewOp<LLVMFloatOp>(op, resTy, adaptor.getLhs(),
                                                 adaptor.getRhs());
      }
    } else {
      rewriter.replaceOpWithNewOp<LLVMIntOp>(op, resTy, adaptor.getLhs(),
                                             adaptor.getRhs());
    }
    return mlir::success();
  }
};

template <typename MokshaOp, typename LLVMOp>
struct BitwiseOpLowering : public mlir::ConvertOpToLLVMPattern<MokshaOp> {
  using mlir::ConvertOpToLLVMPattern<MokshaOp>::ConvertOpToLLVMPattern;
  mlir::LogicalResult
  matchAndRewrite(MokshaOp op, typename MokshaOp::Adaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    rewriter.replaceOpWithNewOp<LLVMOp>(
        op, this->typeConverter->convertType(op.getType()), adaptor.getLhs(),
        adaptor.getRhs());
    return mlir::success();
  }
};

struct CmpOpLowering : public mlir::ConvertOpToLLVMPattern<IR::CmpOp> {
  using ConvertOpToLLVMPattern<IR::CmpOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::CmpOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Type ty = adaptor.getLhs().getType();
    uint32_t pred = op.getPredicate(); // 0=EQ, 1=NE, 2=LT, 3=LE, 4=GT, 5=GE

    // Intercept Decimal Comparisons
    if (auto structTy = mlir::dyn_cast<mlir::LLVM::LLVMStructType>(ty)) {
      if (structTy.getBody().size() == 2 &&
          structTy.getBody()[0].isInteger(128)) {
        auto loc = op.getLoc();
        mlir::Type ptrTy = mlir::LLVM::LLVMPointerType::get(getContext());
        mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
            loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));

        mlir::Value aPtr =
            rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, ty, one);
        rewriter.create<mlir::LLVM::StoreOp>(loc, adaptor.getLhs(), aPtr);
        mlir::Value bPtr =
            rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, ty, one);
        rewriter.create<mlir::LLVM::StoreOp>(loc, adaptor.getRhs(), bPtr);

        mlir::Type i32Ty = rewriter.getI32Type();
        mlir::Value cmpCall = createRuntimeCall(
            rewriter, loc, i32Ty, "__moksha_dec_cmp", {aPtr, bPtr});
        mlir::Value zero = rewriter.create<mlir::LLVM::ConstantOp>(
            loc, i32Ty, rewriter.getI32IntegerAttr(0));

        mlir::LLVM::ICmpPredicate llvmPred;
        switch (pred) {
        case 0:
          llvmPred = mlir::LLVM::ICmpPredicate::eq;
          break;
        case 1:
          llvmPred = mlir::LLVM::ICmpPredicate::ne;
          break;
        case 2:
          llvmPred = mlir::LLVM::ICmpPredicate::slt;
          break;
        case 3:
          llvmPred = mlir::LLVM::ICmpPredicate::sle;
          break;
        case 4:
          llvmPred = mlir::LLVM::ICmpPredicate::sgt;
          break;
        case 5:
          llvmPred = mlir::LLVM::ICmpPredicate::sge;
          break;
        default:
          llvmPred = mlir::LLVM::ICmpPredicate::eq;
          break;
        }

        rewriter.replaceOpWithNewOp<mlir::LLVM::ICmpOp>(op, llvmPred, cmpCall,
                                                        zero);
        return mlir::success();
      }
    }

    if (mlir::isa<mlir::FloatType>(ty)) {
      if (auto lhsConst =
              adaptor.getLhs().getDefiningOp<mlir::LLVM::ConstantOp>()) {
        if (auto rhsConst =
                adaptor.getRhs().getDefiningOp<mlir::LLVM::ConstantOp>()) {
          if (auto lAttr =
                  mlir::dyn_cast<mlir::FloatAttr>(lhsConst.getValue())) {
            if (auto rAttr =
                    mlir::dyn_cast<mlir::FloatAttr>(rhsConst.getValue())) {
              llvm::APFloat lVal = lAttr.getValue();
              llvm::APFloat rVal = rAttr.getValue();

              llvm::APFloat::cmpResult cmpRes = lVal.compare(rVal);
              bool result = false;
              switch (pred) {
              case 0:
                result = (cmpRes == llvm::APFloat::cmpEqual);
                break; // EQ
              case 1:
                result = (cmpRes != llvm::APFloat::cmpEqual);
                break; // NE
              case 2:
                result = (cmpRes == llvm::APFloat::cmpLessThan);
                break; // LT
              case 3:
                result = (cmpRes == llvm::APFloat::cmpLessThan ||
                          cmpRes == llvm::APFloat::cmpEqual);
                break; // LE
              case 4:
                result = (cmpRes == llvm::APFloat::cmpGreaterThan);
                break; // GT
              case 5:
                result = (cmpRes == llvm::APFloat::cmpGreaterThan ||
                          cmpRes == llvm::APFloat::cmpEqual);
                break; // GE
              }

              mlir::Type i1Ty = rewriter.getI1Type();
              rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
                  op, i1Ty, rewriter.getIntegerAttr(i1Ty, result ? 1 : 0));
              return mlir::success();
            }
          }
        }
      }

      mlir::Value lhs = adaptor.getLhs();
      mlir::Value rhs = adaptor.getRhs();
      unsigned width = ty.getIntOrFloatBitWidth();

      // Upcast f8/f16 to f32 before comparison
      if (width < 32) {
        mlir::Type f32Ty = rewriter.getF32Type();
        lhs = safeUpcastFPExt(rewriter, op.getLoc(), lhs, f32Ty);
        rhs = safeUpcastFPExt(rewriter, op.getLoc(), rhs, f32Ty);
      }

      mlir::LLVM::FCmpPredicate llvmPred;

      switch (pred) {
      case 0:
        llvmPred = mlir::LLVM::FCmpPredicate::oeq;
        break;
      case 1:
        llvmPred = mlir::LLVM::FCmpPredicate::one;
        break;
      case 2:
        llvmPred = mlir::LLVM::FCmpPredicate::olt;
        break;
      case 3:
        llvmPred = mlir::LLVM::FCmpPredicate::ole;
        break;
      case 4:
        llvmPred = mlir::LLVM::FCmpPredicate::ogt;
        break;
      case 5:
        llvmPred = mlir::LLVM::FCmpPredicate::oge;
        break;
      default:
        llvmPred = mlir::LLVM::FCmpPredicate::oeq;
        break;
      }
      rewriter.replaceOpWithNewOp<mlir::LLVM::FCmpOp>(op, llvmPred, lhs, rhs);
    } else if (mlir::isa<mlir::LLVM::LLVMPointerType>(ty)) {
      mlir::LLVM::ICmpPredicate llvmPred;
      switch (pred) {
      case 0:
        llvmPred = mlir::LLVM::ICmpPredicate::eq;
        break;
      case 1:
        llvmPred = mlir::LLVM::ICmpPredicate::ne;
        break;
      case 2:
        llvmPred = mlir::LLVM::ICmpPredicate::ult;
        break;
      case 3:
        llvmPred = mlir::LLVM::ICmpPredicate::ule;
        break;
      case 4:
        llvmPred = mlir::LLVM::ICmpPredicate::ugt;
        break;
      case 5:
        llvmPred = mlir::LLVM::ICmpPredicate::uge;
        break;
      default:
        llvmPred = mlir::LLVM::ICmpPredicate::eq;
        break;
      }
      rewriter.replaceOpWithNewOp<mlir::LLVM::ICmpOp>(
          op, llvmPred, adaptor.getLhs(), adaptor.getRhs());

    } else if (mlir::isa<mlir::LLVM::LLVMStructType>(ty) ||
               mlir::isa<mlir::LLVM::LLVMArrayType>(ty)) {
      if (pred != 0 && pred != 1) {
        op.emitError("Relational comparisons (<, >) on raw structs/arrays are "
                     "not supported natively.");
        return mlir::failure();
      }

      auto loc = op.getLoc();
      mlir::Type ptrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      mlir::Type i32Ty = rewriter.getI32Type();
      mlir::Type i64Ty = rewriter.getI64Type();

      mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, i32Ty, rewriter.getI32IntegerAttr(1));
      mlir::Value aPtr =
          rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, ty, one);
      mlir::Value bPtr =
          rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, ty, one);

      rewriter.create<mlir::LLVM::StoreOp>(loc, adaptor.getLhs(), aPtr);
      rewriter.create<mlir::LLVM::StoreOp>(loc, adaptor.getRhs(), bPtr);

      mlir::Value nullPtr = rewriter.create<mlir::LLVM::ZeroOp>(loc, ptrTy);
      mlir::Value gep = rewriter.create<mlir::LLVM::GEPOp>(
          loc, ptrTy, ty, nullPtr, llvm::ArrayRef<mlir::LLVM::GEPArg>{1});
      mlir::Value structSize =
          rewriter.create<mlir::LLVM::PtrToIntOp>(loc, i64Ty, gep);

      mlir::Value callRes = createRuntimeCall(rewriter, loc, i32Ty, "memcmp",
                                              {aPtr, bPtr, structSize});

      mlir::Value zero = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, i32Ty, rewriter.getI32IntegerAttr(0));

      mlir::LLVM::ICmpPredicate llvmPred = (pred == 0)
                                               ? mlir::LLVM::ICmpPredicate::eq
                                               : mlir::LLVM::ICmpPredicate::ne;

      rewriter.replaceOpWithNewOp<mlir::LLVM::ICmpOp>(op, llvmPred, callRes,
                                                      zero);

    } else {
      bool isUnsigned = ty.isUnsignedInteger() || ty.isInteger(1);
      mlir::LLVM::ICmpPredicate llvmPred;
      switch (pred) {
      case 0:
        llvmPred = mlir::LLVM::ICmpPredicate::eq;
        break;
      case 1:
        llvmPred = mlir::LLVM::ICmpPredicate::ne;
        break;
      case 2:
        llvmPred = isUnsigned ? mlir::LLVM::ICmpPredicate::ult
                              : mlir::LLVM::ICmpPredicate::slt;
        break;
      case 3:
        llvmPred = isUnsigned ? mlir::LLVM::ICmpPredicate::ule
                              : mlir::LLVM::ICmpPredicate::sle;
        break;
      case 4:
        llvmPred = isUnsigned ? mlir::LLVM::ICmpPredicate::ugt
                              : mlir::LLVM::ICmpPredicate::sgt;
        break;
      case 5:
        llvmPred = isUnsigned ? mlir::LLVM::ICmpPredicate::uge
                              : mlir::LLVM::ICmpPredicate::sge;
        break;
      default:
        llvmPred = mlir::LLVM::ICmpPredicate::eq;
        break;
      }
      rewriter.replaceOpWithNewOp<mlir::LLVM::ICmpOp>(
          op, llvmPred, adaptor.getLhs(), adaptor.getRhs());
    }
    return mlir::success();
  }
};

struct UnreachableOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::UnreachableOp> {
  using ConvertOpToLLVMPattern<IR::UnreachableOp>::ConvertOpToLLVMPattern;
  mlir::LogicalResult
  matchAndRewrite(IR::UnreachableOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    rewriter.replaceOpWithNewOp<mlir::LLVM::UnreachableOp>(op);
    return mlir::success();
  }
};

struct RetainOpLowering : public mlir::ConvertOpToLLVMPattern<IR::RetainOp> {
  using ConvertOpToLLVMPattern<IR::RetainOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::RetainOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Value val = adaptor.getValue();
    mlir::Type origTy = op->getOperand(0).getType();

    if (!mlir::isa<IR::AnyType>(origTy) &&
        !mlir::isa<IR::ClosureType>(origTy) &&
        !mlir::isa<mlir::LLVM::LLVMPointerType>(val.getType())) {
      rewriter.eraseOp(op);
      return mlir::success();
    }

    // Standardize argument to a raw pointer
    if (mlir::isa<mlir::LLVM::LLVMStructType>(val.getType())) {
      int64_t ptrIdx = 0;
      if (mlir::isa<IR::ClosureType>(origTy)) {
        ptrIdx = 1;
      }
      val = rewriter.create<mlir::LLVM::ExtractValueOp>(
          op.getLoc(), val, llvm::ArrayRef<int64_t>{ptrIdx});
    }

    mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
    if (val.getType() != llvmPtrTy) {
      val = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(), llvmPtrTy, val);
    }

    createRuntimeCall(rewriter, op.getLoc(), "moksha_rt_retain", {}, {val});
    rewriter.eraseOp(op);
    return mlir::success();
  }
};

struct ReleaseOpLowering : public mlir::ConvertOpToLLVMPattern<IR::ReleaseOp> {
  using ConvertOpToLLVMPattern<IR::ReleaseOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::ReleaseOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
    mlir::Value val = adaptor.getValue();
    mlir::Type origTy = op->getOperand(0).getType();

    if (!mlir::isa<IR::AnyType>(origTy) &&
        !mlir::isa<IR::ClosureType>(origTy) &&
        !mlir::isa<mlir::LLVM::LLVMPointerType>(val.getType())) {
      rewriter.eraseOp(op);
      return mlir::success();
    }

    mlir::Value dropFuncPtr;

    if (mlir::isa<mlir::LLVM::LLVMStructType>(val.getType())) {
      if (mlir::isa<IR::ClosureType>(origTy)) {
        mlir::Value envPtr = rewriter.create<mlir::LLVM::ExtractValueOp>(
            loc, val, llvm::ArrayRef<int64_t>{1});

        // Load dynamic destructor
        mlir::Block *currentBlock = rewriter.getInsertionBlock();
        mlir::Block *mergeBlock =
            rewriter.splitBlock(currentBlock, rewriter.getInsertionPoint());
        mlir::Block *loadBlock = rewriter.createBlock(mergeBlock);

        rewriter.setInsertionPointToEnd(currentBlock);
        mlir::Value nullPtr =
            rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmPtrTy);
        mlir::Value isNotNull = rewriter.create<mlir::LLVM::ICmpOp>(
            loc, mlir::LLVM::ICmpPredicate::ne, envPtr, nullPtr);
        rewriter.create<mlir::LLVM::CondBrOp>(loc, isNotNull, loadBlock,
                                              mlir::ValueRange{}, mergeBlock,
                                              mlir::ValueRange{nullPtr});

        rewriter.setInsertionPointToEnd(loadBlock);
        mlir::Value dtorVal =
            rewriter.create<mlir::LLVM::LoadOp>(loc, llvmPtrTy, envPtr);
        rewriter.create<mlir::LLVM::BrOp>(loc, mlir::ValueRange{dtorVal},
                                          mergeBlock);

        rewriter.setInsertionPointToStart(mergeBlock);
        dropFuncPtr = mergeBlock->addArgument(llvmPtrTy, loc);
        val = envPtr;
      } else {
        val = rewriter.create<mlir::LLVM::ExtractValueOp>(
            loc, val, llvm::ArrayRef<int64_t>{0});
      }
    }

    if (!dropFuncPtr) {
      if (auto dropSym = op.getDropFuncAttr()) {
        dropFuncPtr =
            rewriter.create<mlir::LLVM::AddressOfOp>(loc, llvmPtrTy, dropSym);
        auto opaqueFnPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
        dropFuncPtr = rewriter.create<mlir::LLVM::BitcastOp>(loc, opaqueFnPtrTy,
                                                             dropFuncPtr);
      } else {
        dropFuncPtr = rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmPtrTy);
      }
    }

    if (val.getType() != llvmPtrTy) {
      val = rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy, val);
    }

    createRuntimeCall(rewriter, loc, "moksha_rt_release_with_dtor", {},
                      {val, dropFuncPtr});
    rewriter.eraseOp(op);
    return mlir::success();
  }
};

// Weak ARC Lowering
struct StoreWeakOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::StoreWeakOp> {
  using ConvertOpToLLVMPattern<IR::StoreWeakOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::StoreWeakOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
    mlir::Value ptr = adaptor.getPtr();
    mlir::Value val = adaptor.getValue();

    if (ptr.getType() != llvmPtrTy) {
      ptr = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(), llvmPtrTy, ptr);
    }
    if (val.getType() != llvmPtrTy) {
      val = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(), llvmPtrTy, val);
    }

    createRuntimeCall(rewriter, op.getLoc(), "moksha_rt_store_weak", {},
                      {ptr, val});
    rewriter.eraseOp(op);
    return mlir::success();
  }
};

struct LoadWeakOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::LoadWeakOp> {
  using ConvertOpToLLVMPattern<IR::LoadWeakOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::LoadWeakOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
    mlir::Value ptr = adaptor.getPtr();

    if (ptr.getType() != llvmPtrTy) {
      ptr = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(), llvmPtrTy, ptr);
    }

    mlir::Value loaded = createRuntimeCall(rewriter, op.getLoc(), llvmPtrTy,
                                           "moksha_rt_load_weak", {ptr});

    mlir::Type expectedTy = typeConverter->convertType(op.getType());
    if (loaded.getType() != expectedTy) {
      loaded = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(), expectedTy,
                                                      loaded);
    }

    rewriter.replaceOp(op, loaded);
    return mlir::success();
  }
};

// Throw Op Lowering (Exception Control Flow)
struct ThrowOpLowering : public mlir::ConvertOpToLLVMPattern<IR::ThrowOp> {
  using ConvertOpToLLVMPattern<IR::ThrowOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::ThrowOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    mlir::Location loc = op.getLoc();
    mlir::Type llvmVoidTy = mlir::LLVM::LLVMVoidType::get(getContext());
    mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
    auto module = op->getParentOfType<mlir::ModuleOp>();

    if (!module.lookupSymbol("moksha_rt_throw")) {
      mlir::OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());

      auto fnType =
          mlir::LLVM::LLVMFunctionType::get(llvmVoidTy, {llvmPtrTy}, false);
      rewriter.create<mlir::LLVM::LLVMFuncOp>(loc, "moksha_rt_throw", fnType);
    }

    mlir::Value payload = adaptor.getOperands()[0];
    if (op->getNumSuccessors() > 0) {
      mlir::Block *unwindDest = op.getSuccessor(0);
      mlir::Block *actualUnwindDest =
          getOrCreateTrampoline(rewriter, loc, unwindDest);

      mlir::Block *normalDest = rewriter.createBlock(
          op->getBlock()->getParent(), op->getBlock()->getParent()->end());
      rewriter.setInsertionPointToStart(normalDest);
      rewriter.create<mlir::LLVM::UnreachableOp>(loc);

      rewriter.setInsertionPoint(op);
      auto invokeOp = rewriter.replaceOpWithNewOp<mlir::LLVM::InvokeOp>(
          op, mlir::TypeRange{},
          mlir::SymbolRefAttr::get(getContext(), "moksha_rt_throw"),
          mlir::ValueRange{payload}, normalDest, mlir::ValueRange{},
          actualUnwindDest, mlir::ValueRange{});

      auto fnType =
          mlir::LLVM::LLVMFunctionType::get(llvmVoidTy, {llvmPtrTy}, false);
      invokeOp->setAttr("callee_type", mlir::TypeAttr::get(fnType));
    } else {
      auto symRef = mlir::SymbolRefAttr::get(getContext(), "moksha_rt_throw");
      auto callOp = rewriter.create<mlir::LLVM::CallOp>(
          loc, mlir::TypeRange{}, symRef, mlir::ValueRange{payload});

      auto fnType =
          mlir::LLVM::LLVMFunctionType::get(llvmVoidTy, {llvmPtrTy}, false);
      callOp->setAttr("callee_type", mlir::TypeAttr::get(fnType));

      rewriter.replaceOpWithNewOp<mlir::LLVM::UnreachableOp>(op);
    }
    return mlir::success();
  }
};

struct ResumeOpLowering : public mlir::ConvertOpToLLVMPattern<IR::ResumeOp> {
  using ConvertOpToLLVMPattern<IR::ResumeOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::ResumeOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
    mlir::Type i32Ty = rewriter.getI32Type();
    mlir::Type structTy = mlir::LLVM::LLVMStructType::getLiteral(
        getContext(), {llvmPtrTy, i32Ty});

    mlir::Value storage = getLpadStorage(rewriter, loc, op, structTy);
    mlir::Value authenticLpad =
        rewriter.create<mlir::LLVM::LoadOp>(loc, structTy, storage);
    rewriter.replaceOpWithNewOp<mlir::LLVM::ResumeOp>(op, authenticLpad);

    return mlir::success();
  }
};

struct LandingPadOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::LandingPadOp> {
  using ConvertOpToLLVMPattern<IR::LandingPadOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::LandingPadOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Type expectedTy = typeConverter->convertType(op.getType());
    mlir::Type i8PtrTy = mlir::LLVM::LLVMPointerType::get(getContext());

    auto module = op->getParentOfType<mlir::ModuleOp>();
    if (!module.lookupSymbol("__moksha_ex_payload")) {
      mlir::OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());
      rewriter.create<mlir::LLVM::GlobalOp>(
          op.getLoc(), i8PtrTy, false, mlir::LLVM::Linkage::Internal,
          "__moksha_ex_payload", nullptr, 0, 0, false, true);
    }

    auto globalAddr = rewriter.create<mlir::LLVM::AddressOfOp>(
        op.getLoc(), i8PtrTy, "__moksha_ex_payload");
    mlir::Value payload =
        rewriter.create<mlir::LLVM::LoadOp>(op.getLoc(), i8PtrTy, globalAddr);

    if (mlir::isa<mlir::LLVM::LLVMPointerType>(expectedTy)) {
      if (payload.getType() != expectedTy) {
        payload = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(),
                                                         expectedTy, payload);
      }
      rewriter.replaceOp(op, payload);
    } else {
      mlir::Type i32Ty = rewriter.getI32Type();
      mlir::Type structTy = mlir::LLVM::LLVMStructType::getLiteral(
          getContext(), {i8PtrTy, i32Ty});
      mlir::Value zero = rewriter.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), i32Ty, rewriter.getI32IntegerAttr(0));
      mlir::Value undef =
          rewriter.create<mlir::LLVM::UndefOp>(op.getLoc(), structTy);
      mlir::Value s1 = rewriter.create<mlir::LLVM::InsertValueOp>(
          op.getLoc(), undef, payload, llvm::ArrayRef<int64_t>{0});
      mlir::Value s2 = rewriter.create<mlir::LLVM::InsertValueOp>(
          op.getLoc(), s1, zero, llvm::ArrayRef<int64_t>{1});

      rewriter.replaceOp(op, s2);
    }
    return mlir::success();
  }
};

// Closure Lowering
struct MakeClosureOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::MakeClosureOp> {
  using ConvertOpToLLVMPattern<IR::MakeClosureOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::MakeClosureOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
    mlir::Type expectedRetTy = typeConverter->convertType(op.getType());

    // 1. Allocate a temporary stack slot for the struct
    mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
        loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));
    mlir::Value closureAlloc = rewriter.create<mlir::LLVM::AllocaOp>(
        loc, llvmPtrTy, expectedRetTy, one);

    // 2. Extract Function Pointer & Store at Index 0
    mlir::Value fnPtr = rewriter.create<mlir::LLVM::AddressOfOp>(
        loc, llvmPtrTy, op.getCalleeAttr());
    mlir::Value gep0 = rewriter.create<mlir::LLVM::GEPOp>(
        loc, llvmPtrTy, expectedRetTy, closureAlloc,
        llvm::ArrayRef<mlir::LLVM::GEPArg>{0, 0});
    rewriter.create<mlir::LLVM::StoreOp>(loc, fnPtr, gep0);

    // 3. Extract Environment Pointer & Store at Index 1
    mlir::Value envPtr;
    auto captures = adaptor.getCaptures();
    if (captures.empty()) {
      envPtr = rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmPtrTy);
    } else {
      envPtr = captures[0];
      if (envPtr.getType() != llvmPtrTy) {
        envPtr = rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy, envPtr);
      }
    }

    mlir::Value gep1 = rewriter.create<mlir::LLVM::GEPOp>(
        loc, llvmPtrTy, expectedRetTy, closureAlloc,
        llvm::ArrayRef<mlir::LLVM::GEPArg>{0, 1});
    rewriter.create<mlir::LLVM::StoreOp>(loc, envPtr, gep1);

    // 4. Load the assembled struct and return it
    mlir::Value loadedClosure =
        rewriter.create<mlir::LLVM::LoadOp>(loc, expectedRetTy, closureAlloc);

    rewriter.replaceOp(op, loadedClosure);
    return mlir::success();
  }
};

// Variadic Call Lowering
struct CustomCallOpLowering
    : public mlir::ConvertOpToLLVMPattern<mlir::func::CallOp> {
  using ConvertOpToLLVMPattern<mlir::func::CallOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(mlir::func::CallOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    llvm::StringRef callee = op.getCallee();

    // AVX SIMD VECTORIZATION INTERCEPTS
    if (callee == "llvm.x86.avx.loadu.ps.256" ||
        callee == "llvm.x86.avx.add.ps.256" ||
        callee == "llvm.x86.avx.mul.ps.256" ||
        callee == "llvm.x86.fma.vfmadd.ps.256" ||
        callee == "llvm.x86.avx.storeu.ps.256") {

      auto loc = op.getLoc();
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      mlir::Type f32Ty = rewriter.getF32Type();
      mlir::Type vecTy = mlir::VectorType::get({8}, f32Ty);
      mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));

      // 1. Intercept LOADS
      if (callee == "llvm.x86.avx.loadu.ps.256") {
        mlir::Value ptr = adaptor.getOperands()[0];
        if (ptr.getType() != llvmPtrTy)
          ptr = rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy, ptr);

        // Load memory directly into a native AVX 256-bit Vector
        mlir::Value loadVec =
            rewriter.create<mlir::LLVM::LoadOp>(loc, vecTy, ptr);

        // Cast Vector to Array (Moksha's expected return type) via Stack Spill
        mlir::Type resTy = typeConverter->convertType(op.getResultTypes()[0]);
        mlir::Value alloc =
            rewriter.create<mlir::LLVM::AllocaOp>(loc, llvmPtrTy, vecTy, one);
        rewriter.create<mlir::LLVM::StoreOp>(loc, loadVec, alloc);
        mlir::Value resArray =
            rewriter.create<mlir::LLVM::LoadOp>(loc, resTy, alloc);

        rewriter.replaceOp(op, resArray);
        return mlir::success();
      }

      // 2. Intercept STORES
      if (callee == "llvm.x86.avx.storeu.ps.256") {
        mlir::Value ptr = adaptor.getOperands()[0];
        if (ptr.getType() != llvmPtrTy)
          ptr = rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy, ptr);
        mlir::Value arrayVal = adaptor.getOperands()[1];

        // Cast Array to Vector via Stack Spill
        mlir::Value alloc = rewriter.create<mlir::LLVM::AllocaOp>(
            loc, llvmPtrTy, arrayVal.getType(), one);
        rewriter.create<mlir::LLVM::StoreOp>(loc, arrayVal, alloc);
        mlir::Value vecVal =
            rewriter.create<mlir::LLVM::LoadOp>(loc, vecTy, alloc);

        // Store Vector to memory
        rewriter.create<mlir::LLVM::StoreOp>(loc, vecVal, ptr);
        rewriter.eraseOp(op);
        return mlir::success();
      }

      // 3. Intercept MATH
      if (callee == "llvm.x86.avx.add.ps.256" ||
          callee == "llvm.x86.avx.mul.ps.256" ||
          callee == "llvm.x86.fma.vfmadd.ps.256") {
        // Helper to spill an Array and load it back as a Vector
        auto toVector = [&](mlir::Value arrayVal) -> mlir::Value {
          mlir::Value alloc = rewriter.create<mlir::LLVM::AllocaOp>(
              loc, llvmPtrTy, arrayVal.getType(), one);
          rewriter.create<mlir::LLVM::StoreOp>(loc, arrayVal, alloc);
          return rewriter.create<mlir::LLVM::LoadOp>(loc, vecTy, alloc);
        };

        mlir::Value vecA = toVector(adaptor.getOperands()[0]);
        mlir::Value vecB = toVector(adaptor.getOperands()[1]);

        mlir::Value vecRes;

        if (callee == "llvm.x86.fma.vfmadd.ps.256") {
          // Extract the third parameter specifically for FMA
          mlir::Value vecC = toVector(adaptor.getOperands()[2]);
          vecRes =
              rewriter.create<mlir::LLVM::FMAOp>(loc, vecTy, vecA, vecB, vecC);
        } else if (callee == "llvm.x86.avx.add.ps.256") {
          vecRes = rewriter.create<mlir::LLVM::FAddOp>(loc, vecTy, vecA, vecB);
        } else {
          vecRes = rewriter.create<mlir::LLVM::FMulOp>(loc, vecTy, vecA, vecB);
        }

        mlir::Type resTy = typeConverter->convertType(op.getResultTypes()[0]);
        mlir::Value allocRes =
            rewriter.create<mlir::LLVM::AllocaOp>(loc, llvmPtrTy, vecTy, one);
        rewriter.create<mlir::LLVM::StoreOp>(loc, vecRes, allocRes);
        mlir::Value resArray =
            rewriter.create<mlir::LLVM::LoadOp>(loc, resTy, allocRes);

        rewriter.replaceOp(op, resArray);
        return mlir::success();
      }
    }

    // PREVENT STACK-FREE CRASHES
    if (callee == "__moksha_free") {
      auto loc = op.getLoc();
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      mlir::Value ptrToFree = adaptor.getOperands()[0];

      if (ptrToFree.getType() != llvmPtrTy) {
        ptrToFree =
            rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy, ptrToFree);
      }

      mlir::Value dropFuncPtr =
          rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmPtrTy);
      createRuntimeCall(rewriter, loc, "moksha_rt_release_with_dtor",
                        mlir::TypeRange{}, {ptrToFree, dropFuncPtr});

      rewriter.eraseOp(op);
      return mlir::success();
    }

    // DYNAMIC ARRAY SPREAD / ALLOC INTERCEPTION
    if (callee == "__moksha_alloc") {
      auto loc = op.getLoc();
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      mlir::Value sizeVal = adaptor.getOperands()[0];
      mlir::Value sizeInt64 = sizeVal;
      if (sizeVal.getType().getIntOrFloatBitWidth() < 64) {
        sizeInt64 = rewriter.create<mlir::LLVM::ZExtOp>(
            loc, rewriter.getI64Type(), sizeVal);
      }

      mlir::Value typeTag = adaptor.getOperands()[1];
      mlir::Value ptr = createRuntimeCall(
          rewriter, loc, llvmPtrTy, "moksha_rt_alloc", {sizeInt64, typeTag});
      rewriter.replaceOp(op, ptr);
      return mlir::success();
    }

    // ARRAY COPY INTERCEPTION (PREVENT HEAP CORRUPTION)
    if (callee == "__moksha_array_copy") {
      auto loc = op.getLoc();
      mlir::Value dest = adaptor.getOperands()[0];
      mlir::Value src = adaptor.getOperands()[1];
      mlir::Value sizeVal = adaptor.getOperands()[2];

      mlir::Value sizeInt64 = sizeVal;
      if (sizeVal.getType().getIntOrFloatBitWidth() < 64) {
        sizeInt64 = rewriter.create<mlir::LLVM::ZExtOp>(
            loc, rewriter.getI64Type(), sizeVal);
      }

      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());

      // Route directly to standard memcpy
      createRuntimeCall(rewriter, loc, llvmPtrTy, "memcpy",
                        {dest, src, sizeInt64});

      rewriter.eraseOp(op);
      return mlir::success();
    }

    // DYNAMIC MAP INTERCEPTION
    if (callee == "moksha_rt_map_insert" || callee == "moksha_rt_map_get" ||
        callee == "__moksha_map_insert" || callee == "__moksha_map_get") {
      auto loc = op.getLoc();
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      mlir::Value mapPtr = adaptor.getOperands()[0];

      if (mapPtr.getType() != llvmPtrTy) {
        mapPtr = rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy, mapPtr);
      }

      if (callee == "moksha_rt_map_insert" || callee == "__moksha_map_insert") {
        mlir::Value keyAny = adaptor.getOperands()[1];
        mlir::Value valAny = adaptor.getOperands()[2];

        createRuntimeCall(rewriter, loc, "moksha_rt_map_insert",
                          mlir::TypeRange{}, {mapPtr, keyAny, valAny});

        rewriter.eraseOp(op);
        return mlir::success();
      } else {
        mlir::Value rawKey = adaptor.getOperands()[1];
        mlir::Type origKeyTy = op.getOperand(1).getType();
        bool isAlreadyBoxed = false;
        if (mlir::isa<IR::AnyType>(origKeyTy)) {
          isAlreadyBoxed = true;
        } else if (auto ptrTy = mlir::dyn_cast<IR::PointerType>(origKeyTy)) {
          if (mlir::isa<IR::AnyType>(ptrTy.getPointee())) {
            isAlreadyBoxed = true;
          }
        }

        mlir::Value anyPtrAlloc;
        if (isAlreadyBoxed) {
          anyPtrAlloc = rawKey;
          if (!mlir::isa<mlir::LLVM::LLVMPointerType>(anyPtrAlloc.getType())) {
            mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
                loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));
            mlir::Value stackAlloc = rewriter.create<mlir::LLVM::AllocaOp>(
                loc, llvmPtrTy, anyPtrAlloc.getType(), one);
            rewriter.create<mlir::LLVM::StoreOp>(loc, anyPtrAlloc, stackAlloc);
            anyPtrAlloc = stackAlloc;
          }
          if (anyPtrAlloc.getType() != llvmPtrTy) {
            anyPtrAlloc = rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy,
                                                                 anyPtrAlloc);
          }
        } else {
          mlir::Type mlirUnderlyingTy = typeConverter->convertType(origKeyTy);
          if (auto ptrTy = mlir::dyn_cast<IR::PointerType>(origKeyTy)) {
            mlirUnderlyingTy = typeConverter->convertType(ptrTy.getPointee());
          }
          mlir::Value vtablePtr =
              getOrCreateAnyVTable(rewriter, loc, origKeyTy, mlirUnderlyingTy);

          mlir::Value keyDataPtr = rawKey;
          if (!mlir::isa<mlir::LLVM::LLVMPointerType>(rawKey.getType())) {
            mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
                loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));
            mlir::Value stackAlloc = rewriter.create<mlir::LLVM::AllocaOp>(
                loc, llvmPtrTy, rawKey.getType(), one);
            rewriter.create<mlir::LLVM::StoreOp>(loc, rawKey, stackAlloc);
            keyDataPtr = stackAlloc;
          } else {
            keyDataPtr = rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy,
                                                                keyDataPtr);
          }

          mlir::Type anyStructTy = mlir::LLVM::LLVMStructType::getLiteral(
              getContext(), {llvmPtrTy, llvmPtrTy});
          mlir::Value anyStruct =
              rewriter.create<mlir::LLVM::UndefOp>(loc, anyStructTy);
          anyStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
              loc, anyStruct, keyDataPtr, llvm::ArrayRef<int64_t>{0});
          anyStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
              loc, anyStruct, vtablePtr, llvm::ArrayRef<int64_t>{1});

          mlir::Value oneForAny = rewriter.create<mlir::LLVM::ConstantOp>(
              loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));
          anyPtrAlloc = rewriter.create<mlir::LLVM::AllocaOp>(
              loc, llvmPtrTy, anyStructTy, oneForAny);
          rewriter.create<mlir::LLVM::StoreOp>(loc, anyStruct, anyPtrAlloc);
        }

        mlir::Value anyPtr =
            createRuntimeCall(rewriter, loc, llvmPtrTy, "moksha_rt_map_get",
                              {mapPtr, anyPtrAlloc});
        mlir::Value nullPtr =
            rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmPtrTy);
        mlir::Value isNull = rewriter.create<mlir::LLVM::ICmpOp>(
            loc, mlir::LLVM::ICmpPredicate::eq, anyPtr, nullPtr);

        mlir::Block *currentBlock = rewriter.getInsertionBlock();
        mlir::Block *contBlock =
            rewriter.splitBlock(currentBlock, rewriter.getInsertionPoint());

        mlir::Block *nullBlock = rewriter.createBlock(contBlock);
        mlir::Block *safeBlock = rewriter.createBlock(contBlock);

        rewriter.setInsertionPointToEnd(currentBlock);
        rewriter.create<mlir::LLVM::CondBrOp>(loc, isNull, nullBlock,
                                              safeBlock);

        mlir::Type origRetTy = op.getResultTypes()[0];
        mlir::Type expectedRetTy = typeConverter->convertType(origRetTy);

        rewriter.setInsertionPointToEnd(nullBlock);
        mlir::Value nullResult =
            rewriter.create<mlir::LLVM::ZeroOp>(loc, expectedRetTy);
        rewriter.create<mlir::LLVM::BrOp>(loc, mlir::ValueRange{nullResult},
                                          contBlock);
        rewriter.setInsertionPointToEnd(safeBlock);
        bool expectsBoxedAny = false;
        if (mlir::isa<IR::AnyType>(origRetTy)) {
          expectsBoxedAny = true;
        } else if (auto ptrTy = mlir::dyn_cast<IR::PointerType>(origRetTy)) {
          if (mlir::isa<IR::AnyType>(ptrTy.getPointee())) {
            expectsBoxedAny = true;
          }
        }

        mlir::Value finalVal;
        if (expectsBoxedAny) {
          finalVal = anyPtr;
          if (finalVal.getType() != expectedRetTy) {
            finalVal = rewriter.create<mlir::LLVM::BitcastOp>(
                loc, expectedRetTy, finalVal);
          }
        } else {
          mlir::Type anyStructTy = mlir::LLVM::LLVMStructType::getLiteral(
              getContext(), {llvmPtrTy, llvmPtrTy});
          mlir::Value anyVal =
              rewriter.create<mlir::LLVM::LoadOp>(loc, anyStructTy, anyPtr);
          mlir::Value dataPtr = rewriter.create<mlir::LLVM::ExtractValueOp>(
              loc, anyVal, llvm::ArrayRef<int64_t>{0});

          if (dataPtr.getType() != expectedRetTy) {
            if (mlir::isa<mlir::LLVM::LLVMPointerType>(expectedRetTy)) {
              finalVal = rewriter.create<mlir::LLVM::BitcastOp>(
                  loc, expectedRetTy, dataPtr);
            } else {
              mlir::Value castPtr = rewriter.create<mlir::LLVM::BitcastOp>(
                  loc, llvmPtrTy, dataPtr);
              finalVal = rewriter.create<mlir::LLVM::LoadOp>(loc, expectedRetTy,
                                                             castPtr);
            }
          } else {
            finalVal = dataPtr;
          }
        }

        rewriter.create<mlir::LLVM::BrOp>(loc, mlir::ValueRange{finalVal},
                                          contBlock);
        rewriter.setInsertionPointToStart(contBlock);
        contBlock->addArgument(expectedRetTy, loc);
        rewriter.replaceOp(op, contBlock->getArgument(0));

        return mlir::success();
      }
    }

    // LENGTH & IS_EMPTY INTERCEPTION
    if (callee == "length" || callee.starts_with("length_") ||
        callee == "is_empty" || callee.starts_with("is_empty_")) {

      auto loc = op.getLoc();
      mlir::Value arg = adaptor.getOperands()[0];
      mlir::Type origArgTy = op.getOperand(0).getType();
      bool isCheckEmpty = callee.contains("is_empty");

      auto emitResult = [&](mlir::Value len32) {
        if (isCheckEmpty) {
          mlir::Value zero = rewriter.create<mlir::LLVM::ConstantOp>(
              loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(0));
          mlir::Value isZero = rewriter.create<mlir::LLVM::ICmpOp>(
              loc, mlir::LLVM::ICmpPredicate::eq, len32, zero);
          rewriter.replaceOp(op, isZero);
        } else {
          rewriter.replaceOp(op, len32);
        }
      };

      // 1. DYNAMIC SLICE LENGTH (Calls C Runtime)
      if (mlir::isa<IR::SliceType>(origArgTy)) {
        mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
        if (arg.getType() != llvmPtrTy) {
          arg = rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy, arg);
        }
        mlir::Value len32 =
            createRuntimeCall(rewriter, loc, rewriter.getI32Type(),
                              "moksha_rt_array_length", {arg});
        emitResult(len32);
        return mlir::success();
      }

      // 2. NATIVE STRING LENGTH FALLBACK
      mlir::Value len64 = createRuntimeCall(
          rewriter, loc, rewriter.getI64Type(), "strlen", {arg});
      mlir::Value len32 = rewriter.create<mlir::LLVM::TruncOp>(
          loc, rewriter.getI32Type(), len64);
      emitResult(len32);
      return mlir::success();
    }

    // AT INTERCEPTION
    if (callee == "at" || callee.starts_with("at_")) {
      auto loc = op.getLoc();
      mlir::Value arg = adaptor.getOperands()[0];
      mlir::Value idx = adaptor.getOperands()[1];
      mlir::Type origArgTy = op.getOperand(0).getType();

      mlir::Value idx64 = idx;
      if (idx.getType().getIntOrFloatBitWidth() < 64) {
        idx64 = rewriter.create<mlir::LLVM::ZExtOp>(loc, rewriter.getI64Type(),
                                                    idx);
      }

      // 1. DYNAMIC SLICE FAST-PATH
      if (mlir::isa<IR::SliceType>(origArgTy)) {
        mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
        if (arg.getType() != llvmPtrTy) {
          arg = rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy, arg);
        }

        mlir::Value dataPtr = createRuntimeCall(rewriter, loc, llvmPtrTy,
                                                "moksha_rt_array_data", {arg});

        mlir::Type retTy =
            this->typeConverter->convertType(op.getResultTypes()[0]);
        mlir::Value gep = rewriter.create<mlir::LLVM::GEPOp>(
            loc, llvmPtrTy, retTy, dataPtr,
            llvm::ArrayRef<mlir::LLVM::GEPArg>{idx64});

        mlir::Value loaded =
            rewriter.create<mlir::LLVM::LoadOp>(loc, retTy, gep);
        rewriter.replaceOp(op, loaded);
        return mlir::success();
      }

      // 2. Normal string `at` fallback
      mlir::Type i8Ty = rewriter.getI8Type();
      mlir::Value gep = rewriter.create<mlir::LLVM::GEPOp>(
          loc, mlir::LLVM::LLVMPointerType::get(getContext()), i8Ty, arg,
          llvm::ArrayRef<mlir::LLVM::GEPArg>{idx64});
      mlir::Value loaded = rewriter.create<mlir::LLVM::LoadOp>(loc, i8Ty, gep);

      mlir::Type retTy =
          this->typeConverter->convertType(op.getResultTypes()[0]);
      if (loaded.getType() != retTy) {
        if (retTy.isIntOrIndex()) {
          loaded = rewriter.create<mlir::LLVM::SExtOp>(loc, retTy, loaded);
        } else {
          loaded = rewriter.create<mlir::LLVM::BitcastOp>(loc, retTy, loaded);
        }
      }
      rewriter.replaceOp(op, loaded);
      return mlir::success();
    }

    // HALF & QUARTER TO STRING ABI FIX
    if (callee == "__moksha_half_to_string" ||
        callee == "__moksha_quarter_to_string") {
      auto loc = op.getLoc();
      mlir::Type f32Ty = rewriter.getF32Type();
      mlir::Value arg = adaptor.getOperands()[0];
      mlir::Value f32Val = safeUpcastFPExt(rewriter, loc, arg, f32Ty);
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      auto module = op->getParentOfType<mlir::ModuleOp>();
      std::string safeSymbolName = (callee + "_abi").str();

      if (!module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(safeSymbolName)) {
        mlir::OpBuilder::InsertionGuard guard(rewriter);
        rewriter.setInsertionPointToStart(module.getBody());
        auto funcType =
            mlir::LLVM::LLVMFunctionType::get(llvmPtrTy, {f32Ty}, false);
        rewriter.create<mlir::LLVM::LLVMFuncOp>(loc, safeSymbolName, funcType);
      }

      auto symRef = mlir::SymbolRefAttr::get(getContext(), safeSymbolName);
      auto callOp = rewriter.create<mlir::LLVM::CallOp>(
          loc, mlir::TypeRange{llvmPtrTy}, symRef, mlir::ValueRange{f32Val});

      rewriter.replaceOp(op, callOp.getResult());
      return mlir::success();
    }

    // DECIMAL TO STRING ABI
    if (callee == "__moksha_decimal_to_string") {
      auto loc = op.getLoc();
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      mlir::Value decStruct = adaptor.getOperands()[0]; // Already an llvm.ptr
      mlir::Value strPtr = createRuntimeCall(
          rewriter, loc, llvmPtrTy, "moksha_rt_dec_to_string", {decStruct});
      rewriter.replaceOp(op, strPtr);
      return mlir::success();
    }

    llvm::SmallVector<mlir::Type, 1> resultTypes;
    if (mlir::failed(
            typeConverter->convertTypes(op.getResultTypes(), resultTypes)))
      return mlir::failure();
    llvm::SmallVector<mlir::Value, 4> newOperands;
    llvm::SmallVector<mlir::Attribute, 4> argAttrs;
    bool hasByVal = false;

    auto getByteSize = [&](mlir::Type t) -> size_t {
      auto impl = [&](auto &self, mlir::Type t_) -> size_t {
        if (t_.isIntOrFloat())
          return std::max<size_t>(1, t_.getIntOrFloatBitWidth() / 8);
        if (auto arr = mlir::dyn_cast<mlir::LLVM::LLVMArrayType>(t_))
          return self(self, arr.getElementType()) * arr.getNumElements();
        if (auto st = mlir::dyn_cast<mlir::LLVM::LLVMStructType>(t_)) {
          size_t size = 0;
          for (auto f : st.getBody())
            size += self(self, f);
          return size;
        }
        return 8;
      };
      return impl(impl, t);
    };

    for (size_t i = 0; i < adaptor.getOperands().size(); ++i) {
      mlir::Value argVal = adaptor.getOperands()[i];
      mlir::Type argTy = argVal.getType();
      bool isAggregate = mlir::isa<mlir::LLVM::LLVMStructType>(argTy) ||
                         mlir::isa<mlir::LLVM::LLVMArrayType>(argTy);

      mlir::DictionaryAttr dictAttr = rewriter.getDictionaryAttr({});
      if (isAggregate && getByteSize(argTy) > 8) {
        mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
        mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
            op.getLoc(), rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));
        mlir::Value stackAlloc = rewriter.create<mlir::LLVM::AllocaOp>(
            op.getLoc(), llvmPtrTy, argTy, one);
        rewriter.create<mlir::LLVM::StoreOp>(op.getLoc(), argVal, stackAlloc);
        argVal = stackAlloc;
        auto byvalAttr =
            rewriter.getNamedAttr("llvm.byval", mlir::TypeAttr::get(argTy));
        dictAttr = rewriter.getDictionaryAttr({byvalAttr});
        hasByVal = true;
      }

      newOperands.push_back(argVal);
      argAttrs.push_back(dictAttr);
    }

    auto llvmCall = rewriter.create<mlir::LLVM::CallOp>(
        op.getLoc(), resultTypes, op.getCalleeAttr(), newOperands);
    if (hasByVal) {
      llvmCall->setAttr("arg_attrs", rewriter.getArrayAttr(argAttrs));
    }

    auto moduleOp = op->getParentOfType<mlir::ModuleOp>();
    auto funcOp = moduleOp.lookupSymbol<mlir::func::FuncOp>(op.getCallee());
    auto llvmFuncOp =
        moduleOp.lookupSymbol<mlir::LLVM::LLVMFuncOp>(op.getCallee());

    bool isVarArg = op->hasAttr("func.varargs") || op->hasAttr("vararg");
    if (funcOp) {
      isVarArg |= funcOp->hasAttr("func.varargs") || funcOp->hasAttr("vararg");
    } else if (llvmFuncOp) {
      isVarArg |= llvmFuncOp.getFunctionType().isVarArg();
    }

    llvm::SmallVector<mlir::Type, 4> argTypes;
    for (auto val : newOperands) {
      argTypes.push_back(val.getType());
    }

    mlir::Type retTy = resultTypes.empty()
                           ? mlir::LLVM::LLVMVoidType::get(getContext())
                           : resultTypes[0];
    auto llvmFnTy =
        mlir::LLVM::LLVMFunctionType::get(retTy, argTypes, isVarArg);

    llvmCall->setAttr("callee_type", mlir::TypeAttr::get(llvmFnTy));
    if (isVarArg) {
      llvmCall->setAttr("var_callee_type", mlir::TypeAttr::get(llvmFnTy));
    }

    rewriter.replaceOp(op, llvmCall.getResults());
    return mlir::success();
  }
};

struct ReturnOpLowering
    : public mlir::ConvertOpToLLVMPattern<mlir::func::ReturnOp> {
  using ConvertOpToLLVMPattern<mlir::func::ReturnOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(mlir::func::ReturnOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    if (adaptor.getOperands().empty()) {
      rewriter.replaceOpWithNewOp<mlir::LLVM::ReturnOp>(op, mlir::ValueRange{});
      return mlir::success();
    }

    mlir::Value retVal = adaptor.getOperands()[0];

    if (mlir::isa<mlir::LLVM::LLVMPointerType>(retVal.getType())) {
      if (auto constOp = retVal.getDefiningOp<mlir::LLVM::ConstantOp>()) {
        if (mlir::isa<mlir::UnitAttr>(constOp.getValue())) {
          retVal = rewriter.create<mlir::LLVM::ZeroOp>(op.getLoc(),
                                                       retVal.getType());
        }
      }
    }

    rewriter.replaceOpWithNewOp<mlir::LLVM::ReturnOp>(op, retVal);
    return mlir::success();
  }
};

struct SpawnOpLowering : public mlir::ConvertOpToLLVMPattern<IR::SpawnOp> {
  using ConvertOpToLLVMPattern<IR::SpawnOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::SpawnOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    mlir::Type opaquePtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
    mlir::Value closurePtr = adaptor.getClosure();

    if (!mlir::isa<mlir::LLVM::LLVMPointerType>(closurePtr.getType())) {
      mlir::Type i32Ty = rewriter.getI32Type();
      mlir::Type i64Ty = rewriter.getI64Type();
      mlir::Value size = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, i64Ty, rewriter.getI64IntegerAttr(16));
      mlir::Value typeTag = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, i32Ty, rewriter.getI32IntegerAttr(19));

      mlir::Value heapAlloc = createRuntimeCall(
          rewriter, loc, opaquePtrTy, "moksha_rt_alloc", {size, typeTag});
      mlir::Value castedAlloc = rewriter.create<mlir::LLVM::BitcastOp>(
          loc, mlir::LLVM::LLVMPointerType::get(getContext()), heapAlloc);
      auto storeOp =
          rewriter.create<mlir::LLVM::StoreOp>(loc, closurePtr, castedAlloc);
      storeOp.setAlignment(8);
      closurePtr = castedAlloc;
    }

    bool isWeak = false;
    if (auto weakAttr = op->getAttrOfType<mlir::BoolAttr>("is_weak")) {
      isWeak = weakAttr.getValue();
    } else if (op->hasAttr("is_weak")) {
      isWeak = true;
    }

    llvm::StringRef rtFunc =
        isWeak ? "moksha_rt_spawn_weak_thread" : "moksha_rt_spawn_thread";
    mlir::Value defaultPriority = rewriter.create<mlir::LLVM::ConstantOp>(
        loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));
    mlir::Value threadHandle = createRuntimeCall(
        rewriter, loc, opaquePtrTy, rtFunc, {closurePtr, defaultPriority});

    rewriter.replaceOp(op, threadHandle);
    return mlir::success();
  }
};

struct AwaitOpLowering : public mlir::ConvertOpToLLVMPattern<IR::AwaitOp> {
  using ConvertOpToLLVMPattern<IR::AwaitOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::AwaitOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    auto llvmI8PtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
    auto llvmI8Ty = mlir::IntegerType::get(getContext(), 8);
    auto llvmTokenTy = mlir::LLVM::LLVMTokenType::get(getContext());
    auto module = op->getParentOfType<mlir::ModuleOp>();
    auto funcOp = op->getParentOfType<mlir::LLVM::LLVMFuncOp>();

    if (funcOp.getName() == "main" || funcOp.getName() == "__moksha_main") {
      mlir::Value blockRes =
          createRuntimeCall(rewriter, loc, llvmI8PtrTy, "moksha_rt_block_on",
                            {adaptor.getPromise()});

      mlir::Type expectedTy = typeConverter->convertType(op.getType());
      if (!expectedTy || mlir::isa<mlir::LLVM::LLVMVoidType>(expectedTy)) {
        rewriter.eraseOp(op);
      } else {
        if (expectedTy.isIntOrIndex()) {
          auto intVal = rewriter.create<mlir::LLVM::PtrToIntOp>(
              loc, rewriter.getI64Type(), blockRes);
          auto truncVal =
              rewriter.create<mlir::LLVM::TruncOp>(loc, expectedTy, intVal);
          rewriter.replaceOp(op, truncVal);
        } else if (mlir::isa<mlir::LLVM::LLVMPointerType>(expectedTy)) {
          auto casted =
              rewriter.create<mlir::LLVM::BitcastOp>(loc, expectedTy, blockRes);
          rewriter.replaceOp(op, casted);
        } else {
          auto loaded =
              rewriter.create<mlir::LLVM::LoadOp>(loc, expectedTy, blockRes);
          rewriter.replaceOp(op, loaded);
        }
      }
      return mlir::success();
    }

    if (!module.lookupSymbol("llvm.coro.save")) {
      mlir::OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());
      rewriter.create<mlir::LLVM::LLVMFuncOp>(
          loc, "llvm.coro.save",
          mlir::LLVM::LLVMFunctionType::get(llvmTokenTy, {llvmI8PtrTy}));
    }
    if (!module.lookupSymbol("llvm.coro.suspend")) {
      mlir::OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());
      rewriter.create<mlir::LLVM::LLVMFuncOp>(
          loc, "llvm.coro.suspend",
          mlir::LLVM::LLVMFunctionType::get(
              llvmI8Ty, {llvmTokenTy, rewriter.getI1Type()}));
    }

    auto nullHandle = rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmI8PtrTy);

    mlir::Value saveArgs[] = {nullHandle.getResult()};
    auto saveFnTy =
        mlir::LLVM::LLVMFunctionType::get(llvmTokenTy, {llvmI8PtrTy});
    auto coroSave = rewriter.create<mlir::LLVM::CallOp>(
        loc, llvmTokenTy,
        mlir::SymbolRefAttr::get(getContext(), "llvm.coro.save"),
        mlir::ValueRange(llvm::ArrayRef<mlir::Value>(saveArgs)));
    coroSave->setAttr("callee_type", mlir::TypeAttr::get(saveFnTy));
    createRuntimeCall(rewriter, loc, "moksha_rt_register_await",
                      mlir::TypeRange{},
                      {adaptor.getPromise(), nullHandle.getResult()});

    auto falseVal = rewriter.create<mlir::LLVM::ConstantOp>(
        loc, rewriter.getI1Type(),
        rewriter.getIntegerAttr(rewriter.getI1Type(), 0));

    mlir::Value suspendArgs[] = {coroSave.getResult(), falseVal.getResult()};
    auto suspFnTy = mlir::LLVM::LLVMFunctionType::get(
        llvmI8Ty, {llvmTokenTy, rewriter.getI1Type()});
    auto coroSuspend = rewriter.create<mlir::LLVM::CallOp>(
        loc, llvmI8Ty,
        mlir::SymbolRefAttr::get(getContext(), "llvm.coro.suspend"),
        mlir::ValueRange(llvm::ArrayRef<mlir::Value>(suspendArgs)));
    coroSuspend->setAttr("callee_type", mlir::TypeAttr::get(suspFnTy));

    auto currentBlock = rewriter.getInsertionBlock();
    mlir::Block *unwindDest = nullptr;
    for (auto *pred : currentBlock->getPredecessors()) {
      if (auto invokeOp =
              mlir::dyn_cast_or_null<IR::InvokeOp>(pred->getTerminator())) {
        unwindDest = invokeOp.getUnwindDest();
        break;
      } else if (auto llvmInvoke = mlir::dyn_cast_or_null<mlir::LLVM::InvokeOp>(
                     pred->getTerminator())) {
        unwindDest = llvmInvoke.getUnwindDest();
        break;
      }
    }
    if (!unwindDest) {
      for (mlir::Block &b : funcOp.getBody()) {
        if (!b.empty() && (mlir::isa<IR::LandingPadOp>(b.front()) ||
                           mlir::isa<mlir::LLVM::LandingpadOp>(b.front()))) {
          unwindDest = &b;
          break;
        }
      }
    }

    auto resumeBlock =
        rewriter.splitBlock(currentBlock, rewriter.getInsertionPoint());
    auto destroyBlock = rewriter.createBlock(resumeBlock);
    auto suspendBlock = rewriter.createBlock(resumeBlock);

    rewriter.setInsertionPointToEnd(currentBlock);

    auto caseValuesType = mlir::RankedTensorType::get({2}, llvmI8Ty);
    llvm::SmallVector<int8_t, 2> caseVals = {0, 1};
    auto caseValuesAttr =
        mlir::DenseIntElementsAttr::get(caseValuesType, caseVals);

    rewriter.create<mlir::LLVM::SwitchOp>(
        loc, coroSuspend.getResult(), suspendBlock, mlir::ValueRange{},
        caseValuesAttr, mlir::BlockRange{resumeBlock, destroyBlock},
        llvm::ArrayRef<mlir::ValueRange>{mlir::ValueRange{},
                                         mlir::ValueRange{}});

    rewriter.setInsertionPointToEnd(suspendBlock);
    mlir::Type funcRetTy = funcOp.getFunctionType().getReturnType();

    if (mlir::isa<mlir::LLVM::LLVMVoidType>(funcRetTy)) {
      auto retOp =
          rewriter.create<mlir::LLVM::ReturnOp>(loc, mlir::ValueRange{});
      retOp->setAttr("moksha.yield", rewriter.getUnitAttr());
    } else {
      auto nullRet = rewriter.create<mlir::LLVM::ZeroOp>(loc, funcRetTy);
      mlir::Value retArgs[] = {nullRet};
      auto retOp = rewriter.create<mlir::LLVM::ReturnOp>(
          loc, mlir::ValueRange(llvm::ArrayRef<mlir::Value>(retArgs)));
      retOp->setAttr("moksha.yield", rewriter.getUnitAttr());
    }

    rewriter.setInsertionPointToEnd(destroyBlock);

    if (mlir::isa<mlir::LLVM::LLVMVoidType>(funcRetTy)) {
      auto destroyRetOp =
          rewriter.create<mlir::LLVM::ReturnOp>(loc, mlir::ValueRange{});
      destroyRetOp->setAttr("moksha.yield", rewriter.getUnitAttr());
    } else {
      auto nullRet = rewriter.create<mlir::LLVM::ZeroOp>(loc, funcRetTy);
      mlir::Value retArgs[] = {nullRet};
      auto destroyRetOp = rewriter.create<mlir::LLVM::ReturnOp>(
          loc, mlir::ValueRange(llvm::ArrayRef<mlir::Value>(retArgs)));
      destroyRetOp->setAttr("moksha.yield", rewriter.getUnitAttr());
    }

    rewriter.setInsertionPointToStart(resumeBlock);
    mlir::Type expectedTy = typeConverter->convertType(op.getType());

    mlir::Value payloadArgs[] = {adaptor.getPromise()};
    mlir::Value payloadPtrResult;

    if (unwindDest) {
      mlir::Block *actualUnwindDest =
          getOrCreateTrampoline(rewriter, loc, unwindDest);
      mlir::Block *normalDest =
          rewriter.splitBlock(resumeBlock, rewriter.getInsertionPoint());
      rewriter.setInsertionPointToEnd(resumeBlock);

      if (!module.lookupSymbol("moksha_rt_await_payload")) {
        mlir::OpBuilder::InsertionGuard guard(rewriter);
        rewriter.setInsertionPointToStart(module.getBody());
        auto payloadFnTy = mlir::LLVM::LLVMFunctionType::get(
            llvmI8PtrTy, {llvmI8PtrTy}, /*isVarArg=*/false);
        rewriter.create<mlir::LLVM::LLVMFuncOp>(loc, "moksha_rt_await_payload",
                                                payloadFnTy);
      }

      auto invokeOp = rewriter.create<mlir::LLVM::InvokeOp>(
          loc, llvmI8PtrTy,
          mlir::SymbolRefAttr::get(getContext(), "moksha_rt_await_payload"),
          mlir::ValueRange(llvm::ArrayRef<mlir::Value>(payloadArgs)),
          normalDest, mlir::ValueRange{}, actualUnwindDest, mlir::ValueRange{});

      auto fnType =
          mlir::LLVM::LLVMFunctionType::get(llvmI8PtrTy, {llvmI8PtrTy}, false);
      invokeOp->setAttr("callee_type", mlir::TypeAttr::get(fnType));

      payloadPtrResult = invokeOp.getResult();
      rewriter.setInsertionPointToStart(normalDest);
    } else {
      payloadPtrResult =
          createRuntimeCall(rewriter, loc, llvmI8PtrTy,
                            "moksha_rt_await_payload", {adaptor.getPromise()});
    }

    if (!expectedTy || mlir::isa<mlir::LLVM::LLVMVoidType>(expectedTy)) {
      rewriter.eraseOp(op);
    } else {
      if (expectedTy.isIntOrIndex()) {
        auto intVal = rewriter.create<mlir::LLVM::PtrToIntOp>(
            loc, rewriter.getI64Type(), payloadPtrResult);
        auto truncVal =
            rewriter.create<mlir::LLVM::TruncOp>(loc, expectedTy, intVal);
        rewriter.replaceOp(op, truncVal);
      } else if (mlir::isa<mlir::LLVM::LLVMPointerType>(expectedTy)) {
        auto casted = rewriter.create<mlir::LLVM::BitcastOp>(loc, expectedTy,
                                                             payloadPtrResult);
        rewriter.replaceOp(op, casted);
      } else {
        auto loaded = rewriter.create<mlir::LLVM::LoadOp>(loc, expectedTy,
                                                          payloadPtrResult);
        rewriter.replaceOp(op, loaded);
      }
    }

    return mlir::success();
  }
};

struct InvokeOpLowering : public mlir::ConvertOpToLLVMPattern<IR::InvokeOp> {
  using ConvertOpToLLVMPattern<IR::InvokeOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::InvokeOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    llvm::SmallVector<mlir::Type, 1> resultTypes;
    if (mlir::failed(
            typeConverter->convertTypes(op.getResultTypes(), resultTypes)))
      return mlir::failure();

    mlir::Block *actualUnwindDest =
        getOrCreateTrampoline(rewriter, op.getLoc(), op.getUnwindDest());

    llvm::SmallVector<mlir::Value, 4> newOperands;
    llvm::SmallVector<mlir::Attribute, 4> argAttrs;
    bool hasByVal = false;

    auto getInvokeByteSize = [&](mlir::Type t) -> size_t {
      auto impl = [&](auto &self, mlir::Type t_) -> size_t {
        if (t_.isIntOrFloat())
          return std::max<size_t>(1, t_.getIntOrFloatBitWidth() / 8);
        if (auto arr = mlir::dyn_cast<mlir::LLVM::LLVMArrayType>(t_))
          return self(self, arr.getElementType()) * arr.getNumElements();
        if (auto st = mlir::dyn_cast<mlir::LLVM::LLVMStructType>(t_)) {
          size_t size = 0;
          for (auto f : st.getBody())
            size += self(self, f);
          return size;
        }
        return 8;
      };
      return impl(impl, t);
    };

    for (size_t i = 0; i < adaptor.getOperands().size(); ++i) {
      mlir::Value argVal = adaptor.getOperands()[i];
      mlir::Type argTy = argVal.getType();
      bool isAggregate = mlir::isa<mlir::LLVM::LLVMStructType>(argTy) ||
                         mlir::isa<mlir::LLVM::LLVMArrayType>(argTy);

      mlir::DictionaryAttr dictAttr = rewriter.getDictionaryAttr({});

      if (isAggregate && getInvokeByteSize(argTy) > 8) {
        mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
        mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
            op.getLoc(), rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));
        mlir::Value stackAlloc = rewriter.create<mlir::LLVM::AllocaOp>(
            op.getLoc(), llvmPtrTy, argTy, one);
        rewriter.create<mlir::LLVM::StoreOp>(op.getLoc(), argVal, stackAlloc);
        argVal = stackAlloc;

        auto byvalAttr =
            rewriter.getNamedAttr("llvm.byval", mlir::TypeAttr::get(argTy));
        dictAttr = rewriter.getDictionaryAttr({byvalAttr});
        hasByVal = true;
      }

      newOperands.push_back(argVal);
      argAttrs.push_back(dictAttr);
    }

    auto llvmInvoke = rewriter.create<mlir::LLVM::InvokeOp>(
        op.getLoc(), resultTypes, op.getCalleeAttr(), newOperands,
        op.getNormalDest(), mlir::ValueRange{}, actualUnwindDest,
        mlir::ValueRange{});

    if (hasByVal) {
      llvmInvoke->setAttr("arg_attrs", rewriter.getArrayAttr(argAttrs));
    }

    auto moduleOp = op->getParentOfType<mlir::ModuleOp>();
    auto funcOp = moduleOp.lookupSymbol<mlir::func::FuncOp>(op.getCallee());
    auto llvmFuncOp =
        moduleOp.lookupSymbol<mlir::LLVM::LLVMFuncOp>(op.getCallee());
    bool isVarArg = op->hasAttr("func.varargs") || op->hasAttr("vararg");
    if (funcOp) {
      isVarArg |= funcOp->hasAttr("func.varargs") || funcOp->hasAttr("vararg");
    } else if (llvmFuncOp) {
      isVarArg |= llvmFuncOp.getFunctionType().isVarArg();
    }

    llvm::SmallVector<mlir::Type, 4> argTypes;
    for (auto val : newOperands) {
      argTypes.push_back(val.getType());
    }

    mlir::Type retTy = resultTypes.empty()
                           ? mlir::LLVM::LLVMVoidType::get(getContext())
                           : resultTypes[0];
    auto llvmFnTy =
        mlir::LLVM::LLVMFunctionType::get(retTy, argTypes, isVarArg);

    llvmInvoke->setAttr("callee_type", mlir::TypeAttr::get(llvmFnTy));
    if (isVarArg) {
      llvmInvoke->setAttr("var_callee_type", mlir::TypeAttr::get(llvmFnTy));
    }

    rewriter.replaceOp(op, llvmInvoke.getResults());
    return mlir::success();
  }
};

struct InvokeIndirectOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::InvokeIndirectOp> {
  using ConvertOpToLLVMPattern<IR::InvokeIndirectOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::InvokeIndirectOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    auto loc = op.getLoc();
    mlir::Value callee = adaptor.getCallee();
    mlir::Type calleeTy = callee.getType();

    if (mlir::isa<mlir::LLVM::LLVMStructType>(calleeTy)) {
      callee = rewriter.create<mlir::LLVM::ExtractValueOp>(
          loc, callee, mlir::ArrayRef<int64_t>{0});
    }

    mlir::Block *actualUnwindDest =
        getOrCreateTrampoline(rewriter, loc, op.getUnwindDest());

    llvm::SmallVector<mlir::Type, 1> resultTypes;
    if (mlir::failed(
            typeConverter->convertTypes(op.getResultTypes(), resultTypes)))
      return mlir::failure();

    mlir::Type retTy = resultTypes.empty()
                           ? mlir::LLVM::LLVMVoidType::get(getContext())
                           : resultTypes[0];

    llvm::SmallVector<mlir::Value, 4> invokeOperands;
    invokeOperands.push_back(callee);
    invokeOperands.append(adaptor.getCallArgs().begin(),
                          adaptor.getCallArgs().end());

    auto llvmInvoke = rewriter.create<mlir::LLVM::InvokeOp>(
        loc, resultTypes, nullptr, invokeOperands, op.getNormalDest(),
        mlir::ValueRange{}, actualUnwindDest, mlir::ValueRange{});

    llvm::SmallVector<mlir::Type, 4> argTypes;
    for (auto arg : adaptor.getCallArgs())
      argTypes.push_back(arg.getType());

    auto llvmFnTy = mlir::LLVM::LLVMFunctionType::get(retTy, argTypes, false);
    llvmInvoke->setAttr("callee_type", mlir::TypeAttr::get(llvmFnTy));

    rewriter.replaceOp(op, llvmInvoke.getResults());
    return mlir::success();
  }
};

// Exponentiation Lowering
struct PowOpLowering : public mlir::ConvertOpToLLVMPattern<IR::PowOp> {
  using ConvertOpToLLVMPattern<IR::PowOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::PowOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Type resTy = typeConverter->convertType(op.getType());
    unsigned width = resTy.getIntOrFloatBitWidth();
    std::string funcName =
        mlir::isa<mlir::FloatType>(resTy) ? "__moksha_powf" : "__moksha_powi";
    funcName += std::to_string(width);

    auto call = createRuntimeCall(rewriter, op.getLoc(), funcName, {resTy},
                                  {adaptor.getLhs(), adaptor.getRhs()});

    rewriter.replaceOp(op, call.getResult());
    return mlir::success();
  }
};

// Atomics Lowering
struct AtomicStoreOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::AtomicStoreOp> {
  using ConvertOpToLLVMPattern<IR::AtomicStoreOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::AtomicStoreOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto llvmStore = rewriter.create<mlir::LLVM::StoreOp>(
        op.getLoc(), adaptor.getValue(), adaptor.getPtr());
    llvmStore.setOrdering(mapAtomicOrdering(op.getOrdering()));
    unsigned alignment = 4;
    mlir::Type valTy = adaptor.getValue().getType();
    if (valTy.isIntOrFloat()) {
      alignment = std::max(1u, valTy.getIntOrFloatBitWidth() / 8);
    } else if (mlir::isa<mlir::LLVM::LLVMPointerType>(valTy)) {
      alignment = 8;
    }
    llvmStore.setAlignment(alignment);

    rewriter.eraseOp(op);
    return mlir::success();
  }
};

struct AtomicLoadOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::AtomicLoadOp> {
  using ConvertOpToLLVMPattern<IR::AtomicLoadOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::AtomicLoadOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto llvmType = typeConverter->convertType(op.getType());
    auto llvmLoad = rewriter.create<mlir::LLVM::LoadOp>(op.getLoc(), llvmType,
                                                        adaptor.getPtr());
    llvmLoad.setOrdering(mapAtomicOrdering(op.getOrdering()));

    unsigned alignment = 4;
    if (llvmType.isIntOrFloat()) {
      alignment = std::max(1u, llvmType.getIntOrFloatBitWidth() / 8);
    } else if (mlir::isa<mlir::LLVM::LLVMPointerType>(llvmType)) {
      alignment = 8;
    }
    llvmLoad.setAlignment(alignment);

    rewriter.replaceOp(op, llvmLoad.getResult());
    return mlir::success();
  }
};

struct AtomicRMWOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::AtomicRMWOp> {
  using ConvertOpToLLVMPattern<IR::AtomicRMWOp>::ConvertOpToLLVMPattern;
  mlir::LogicalResult
  matchAndRewrite(IR::AtomicRMWOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto rmwOp = rewriter.create<mlir::LLVM::AtomicRMWOp>(
        op.getLoc(), mapAtomicBinOp(op.getBinOp()), adaptor.getPtr(),
        adaptor.getValue(), mapAtomicOrdering(op.getOrdering()));
    rewriter.replaceOp(op, rmwOp.getResult());
    return mlir::success();
  }
};

struct AtomicCmpXchgOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::AtomicCmpXchgOp> {
  using ConvertOpToLLVMPattern<IR::AtomicCmpXchgOp>::ConvertOpToLLVMPattern;
  mlir::LogicalResult
  matchAndRewrite(IR::AtomicCmpXchgOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto cmpxchgOp = rewriter.create<mlir::LLVM::AtomicCmpXchgOp>(
        op.getLoc(), adaptor.getPtr(), adaptor.getExpected(),
        adaptor.getDesired(), mapAtomicOrdering(op.getSuccessOrder()),
        mapAtomicOrdering(op.getFailureOrder()));
    auto extractOp = rewriter.create<mlir::LLVM::ExtractValueOp>(
        op.getLoc(), cmpxchgOp.getResult(), llvm::ArrayRef<int64_t>{0});

    rewriter.replaceOp(op, extractOp.getResult());
    return mlir::success();
  }
};

struct FenceOpLowering : public mlir::ConvertOpToLLVMPattern<IR::FenceOp> {
  using ConvertOpToLLVMPattern<IR::FenceOp>::ConvertOpToLLVMPattern;
  mlir::LogicalResult
  matchAndRewrite(IR::FenceOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    rewriter.create<mlir::LLVM::FenceOp>(
        op.getLoc(), mapAtomicOrdering(op.getOrdering()), "");
    rewriter.eraseOp(op);
    return mlir::success();
  }
};

// Safe AddOp Lowering (Handles String Concatenation & Precision)
struct AddOpLowering : public mlir::ConvertOpToLLVMPattern<IR::AddOp> {
  using ConvertOpToLLVMPattern<IR::AddOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::AddOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Type resTy = typeConverter->convertType(op.getType());

    if (mlir::isa<mlir::LLVM::LLVMPointerType>(resTy)) {
      mlir::Value call = createRuntimeCall(
          rewriter, op.getLoc(), resTy, "__moksha_string_concat",
          {adaptor.getLhs(), adaptor.getRhs()});
      rewriter.replaceOp(op, call);
      return mlir::success();
    }

    if (auto structTy = mlir::dyn_cast<mlir::LLVM::LLVMStructType>(resTy)) {
      if (structTy.getBody().size() == 2 &&
          structTy.getBody()[0].isInteger(128) &&
          structTy.getBody()[1].isInteger(32)) {

        auto loc = op.getLoc();
        mlir::Type ptrTy = mlir::LLVM::LLVMPointerType::get(getContext());
        mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
            loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));

        mlir::Value aPtr =
            rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, resTy, one);
        rewriter.create<mlir::LLVM::StoreOp>(loc, adaptor.getLhs(), aPtr);
        mlir::Value bPtr =
            rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, resTy, one);
        rewriter.create<mlir::LLVM::StoreOp>(loc, adaptor.getRhs(), bPtr);
        mlir::Value resPtr =
            rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, resTy, one);

        createRuntimeCall(rewriter, loc, "__moksha_dec_add", mlir::TypeRange{},
                          {resPtr, aPtr, bPtr});

        mlir::Value loaded =
            rewriter.create<mlir::LLVM::LoadOp>(loc, resTy, resPtr);
        rewriter.replaceOp(op, loaded);
        return mlir::success();
      }
    }

    if (auto lhsConst =
            adaptor.getLhs().getDefiningOp<mlir::LLVM::ConstantOp>()) {
      if (auto rhsConst =
              adaptor.getRhs().getDefiningOp<mlir::LLVM::ConstantOp>()) {
        if (mlir::isa<mlir::FloatType>(resTy)) {
          if (auto lAttr =
                  mlir::dyn_cast<mlir::FloatAttr>(lhsConst.getValue())) {
            if (auto rAttr =
                    mlir::dyn_cast<mlir::FloatAttr>(rhsConst.getValue())) {
              llvm::APFloat lVal = lAttr.getValue();
              llvm::APFloat rVal = rAttr.getValue();
              lVal.add(rVal, llvm::APFloat::rmNearestTiesToEven);
              rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
                  op, resTy, rewriter.getFloatAttr(resTy, lVal));
              return mlir::success();
            }
          }
        } else if (resTy.isIntOrIndex()) {
          if (auto lAttr =
                  mlir::dyn_cast<mlir::IntegerAttr>(lhsConst.getValue())) {
            if (auto rAttr =
                    mlir::dyn_cast<mlir::IntegerAttr>(rhsConst.getValue())) {
              llvm::APInt lVal = lAttr.getValue();
              llvm::APInt rVal = rAttr.getValue();
              rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
                  op, resTy, rewriter.getIntegerAttr(resTy, lVal + rVal));
              return mlir::success();
            }
          }
        }
      }
    }

    if (mlir::isa<mlir::FloatType>(resTy)) {
      unsigned width = resTy.getIntOrFloatBitWidth();
      if (width < 32) {
        mlir::Type f32Ty = rewriter.getF32Type();
        mlir::Value lhs32 =
            safeUpcastFPExt(rewriter, op.getLoc(), adaptor.getLhs(), f32Ty);
        mlir::Value rhs32 =
            safeUpcastFPExt(rewriter, op.getLoc(), adaptor.getRhs(), f32Ty);

        mlir::Value res32 = rewriter.create<mlir::LLVM::FAddOp>(
            op.getLoc(), f32Ty, lhs32, rhs32);

        rewriter.replaceOpWithNewOp<mlir::LLVM::FPTruncOp>(op, resTy, res32);
      } else {
        rewriter.replaceOpWithNewOp<mlir::LLVM::FAddOp>(
            op, resTy, adaptor.getLhs(), adaptor.getRhs());
      }
    } else {
      rewriter.replaceOpWithNewOp<mlir::LLVM::AddOp>(
          op, resTy, adaptor.getLhs(), adaptor.getRhs());
    }
    return mlir::success();
  }
};

// Safe Division & Modulo (Branchless GPU-Safe Evaluation)
template <typename OpType, typename IntLLVMOp, typename FloatLLVMOp>
struct SafeDivModOpLowering : public mlir::ConvertOpToLLVMPattern<OpType> {
  using mlir::ConvertOpToLLVMPattern<OpType>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(OpType op, typename OpType::Adaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    mlir::Value lhs = adaptor.getLhs();
    mlir::Value rhs = adaptor.getRhs();
    mlir::Type type = lhs.getType();

    if (auto lhsConst =
            adaptor.getLhs().template getDefiningOp<mlir::LLVM::ConstantOp>()) {
      if (auto rhsConst =
              adaptor.getRhs()
                  .template getDefiningOp<mlir::LLVM::ConstantOp>()) {
        if (mlir::isa<mlir::FloatType>(type)) {
          if (auto lAttr =
                  mlir::dyn_cast<mlir::FloatAttr>(lhsConst.getValue())) {
            if (auto rAttr =
                    mlir::dyn_cast<mlir::FloatAttr>(rhsConst.getValue())) {
              llvm::APFloat lVal = lAttr.getValue();
              llvm::APFloat rVal = rAttr.getValue();

              bool canFold = true;
              if constexpr (std::is_same_v<OpType, IR::DivOp>) {
                if (!rVal.isZero())
                  lVal.divide(rVal, llvm::APFloat::rmNearestTiesToEven);
                else
                  canFold = false;
              } else if constexpr (std::is_same_v<OpType, IR::ModOp>) {
                if (!rVal.isZero())
                  lVal.remainder(rVal);
                else
                  canFold = false;
              }
              if (canFold) {
                rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
                    op, type, rewriter.getFloatAttr(type, lVal));
                return mlir::success();
              }
            }
          }
        } else if (type.isIntOrIndex()) {
          if (auto lAttr =
                  mlir::dyn_cast<mlir::IntegerAttr>(lhsConst.getValue())) {
            if (auto rAttr =
                    mlir::dyn_cast<mlir::IntegerAttr>(rhsConst.getValue())) {
              llvm::APInt lVal = lAttr.getValue();
              llvm::APInt rVal = rAttr.getValue();
              llvm::APInt resVal;

              bool isUnsigned = false;
              if (auto intTy = mlir::dyn_cast<mlir::IntegerType>(type))
                isUnsigned = intTy.isUnsigned();

              bool canFold = true;
              if constexpr (std::is_same_v<OpType, IR::DivOp>) {
                if (rVal.isZero())
                  canFold = false;
                else
                  resVal = isUnsigned ? lVal.udiv(rVal) : lVal.sdiv(rVal);
              } else if constexpr (std::is_same_v<OpType, IR::ModOp>) {
                if (rVal.isZero())
                  canFold = false;
                else
                  resVal = isUnsigned ? lVal.urem(rVal) : lVal.srem(rVal);
              }
              if (canFold) {
                rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
                    op, type, rewriter.getIntegerAttr(type, resVal));
                return mlir::success();
              }
            }
          }
        }
      }
    }

    // Decimal Div/Mod Runtime Interception
    if (auto structTy = mlir::dyn_cast<mlir::LLVM::LLVMStructType>(type)) {
      if (structTy.getBody().size() == 2 &&
          structTy.getBody()[0].isInteger(128) &&
          structTy.getBody()[1].isInteger(32)) {

        llvm::StringRef rtFunc = "__moksha_dec_div";
        if constexpr (std::is_same_v<OpType, IR::ModOp>)
          rtFunc = "__moksha_dec_mod";

        mlir::Type ptrTy = mlir::LLVM::LLVMPointerType::get(this->getContext());
        mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
            loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));

        mlir::Value aPtr =
            rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, type, one);
        rewriter.create<mlir::LLVM::StoreOp>(loc, lhs, aPtr);

        mlir::Value bPtr =
            rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, type, one);
        rewriter.create<mlir::LLVM::StoreOp>(loc, rhs, bPtr);

        mlir::Value resPtr =
            rewriter.create<mlir::LLVM::AllocaOp>(loc, ptrTy, type, one);

        createRuntimeCall(rewriter, loc, rtFunc, mlir::TypeRange{},
                          {resPtr, aPtr, bPtr});

        mlir::Value loaded =
            rewriter.create<mlir::LLVM::LoadOp>(loc, type, resPtr);
        rewriter.replaceOp(op, loaded);
        return mlir::success();
      }
    }

    // Standard Float Division
    if (llvm::isa<mlir::FloatType>(type)) {
      unsigned width = type.getIntOrFloatBitWidth();
      if (width < 32) {
        mlir::Type f32Ty = rewriter.getF32Type();
        mlir::Value lhs32 = safeUpcastFPExt(rewriter, loc, lhs, f32Ty);
        mlir::Value rhs32 = safeUpcastFPExt(rewriter, loc, rhs, f32Ty);
        mlir::Value res32 =
            rewriter.create<FloatLLVMOp>(loc, f32Ty, lhs32, rhs32);
        rewriter.replaceOpWithNewOp<mlir::LLVM::FPTruncOp>(op, type, res32);
      } else {
        rewriter.replaceOpWithNewOp<FloatLLVMOp>(op, type, lhs, rhs);
      }
      return mlir::success();
    }

    // Safe Integer Division (Zero-checked)
    if (llvm::isa<mlir::IntegerType>(type)) {
      mlir::Value zero = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, type, rewriter.getIntegerAttr(type, 0));
      mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, type, rewriter.getIntegerAttr(type, 1));

      mlir::Value isZero = rewriter.create<mlir::LLVM::ICmpOp>(
          loc, mlir::LLVM::ICmpPredicate::eq, rhs, zero);
      mlir::Value safeRhs =
          rewriter.create<mlir::LLVM::SelectOp>(loc, isZero, one, rhs);
      mlir::Value safeDiv = rewriter.create<IntLLVMOp>(loc, type, lhs, safeRhs);
      mlir::Value finalRes =
          rewriter.create<mlir::LLVM::SelectOp>(loc, isZero, zero, safeDiv);

      rewriter.replaceOp(op, finalRes);
      return mlir::success();
    }
    return mlir::failure();
  }
};

// Pass Definition
struct ConvertMokshaToLLVMPass
    : public mlir::PassWrapper<ConvertMokshaToLLVMPass,
                               mlir::OperationPass<mlir::ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(ConvertMokshaToLLVMPass)

  void getDependentDialects(mlir::DialectRegistry &registry) const override {
    registry.insert<mlir::LLVM::LLVMDialect>();
  }

  llvm::StringRef getArgument() const final { return "convert-moksha-to-llvm"; }
  llvm::StringRef getDescription() const final {
    return "Lower Moksha to LLVM dialect";
  }

  void runOnOperation() override {
    llvm::errs() << "[DEBUG] Starting ConvertMokshaToLLVMPass...\n";
    auto sanitizeName = [](llvm::StringRef name) -> std::string {
      std::string s = name.str();
      if (name.starts_with("llvm.")) {
        return s;
      }
      for (char &c : s) {
        if (c == '.' || c == '<' || c == '>') {
          c = '_';
        }
      }
      return s;
    };

    getOperation().walk([&](mlir::Operation *op) {
      if (auto funcOp = mlir::dyn_cast<mlir::func::FuncOp>(op)) {
        funcOp.setName(sanitizeName(funcOp.getName()));
      } else if (auto globalOp = mlir::dyn_cast<IR::GlobalOp>(op)) {
        globalOp.setSymNameAttr(mlir::StringAttr::get(
            &getContext(), sanitizeName(globalOp.getSymName())));
      } else if (auto callOp = mlir::dyn_cast<mlir::func::CallOp>(op)) {
        callOp.setCallee(sanitizeName(callOp.getCallee()));
      } else if (auto invokeOp = mlir::dyn_cast<IR::InvokeOp>(op)) {
        invokeOp.setCalleeAttr(mlir::FlatSymbolRefAttr::get(
            &getContext(), sanitizeName(invokeOp.getCallee())));
      } else if (auto makeClosure = mlir::dyn_cast<IR::MakeClosureOp>(op)) {
        makeClosure.setCalleeAttr(mlir::FlatSymbolRefAttr::get(
            &getContext(), sanitizeName(makeClosure.getCallee())));
      } else if (auto addrOf = mlir::dyn_cast<IR::AddressOfOp>(op)) {
        addrOf->setAttr(
            "global_name",
            mlir::FlatSymbolRefAttr::get(&getContext(),
                                         sanitizeName(addrOf.getGlobalName())));
      } else if (auto releaseOp = mlir::dyn_cast<IR::ReleaseOp>(op)) {
        if (auto attr = releaseOp.getDropFuncAttr()) {
          releaseOp.setDropFuncAttr(mlir::FlatSymbolRefAttr::get(
              &getContext(), sanitizeName(attr.getValue())));
        }
      }
    });

    // 1. Cache Custom Attributes before Dialect Conversion
    llvm::StringMap<bool> hasAttrNaked, hasAttrNoRet, hasAttrInline,
        hasAttrNoInline;
    llvm::StringMap<bool> hasAttrPure, hasAttrCold, hasAttrUsed,
        hasAttrInterrupt;
    llvm::StringMap<bool> hasAttrAsync;

    getOperation().walk([&](mlir::func::FuncOp funcOp) {
      auto name = funcOp.getName();
      if (funcOp->hasAttr("vararg")) {
        funcOp->setAttr("func.varargs",
                        mlir::BoolAttr::get(&getContext(), true));
      }
      if (name == "print" || name == "println" ||
          name == "__moksha_template_join_strs") {
        funcOp->setAttr("func.varargs",
                        mlir::BoolAttr::get(&getContext(), true));
      }
      if (funcOp->hasAttr("moksha.naked"))
        hasAttrNaked[name] = true;
      if (funcOp->hasAttr("moksha.noreturn"))
        hasAttrNoRet[name] = true;
      if (funcOp->hasAttr("moksha.inline"))
        hasAttrInline[name] = true;
      if (funcOp->hasAttr("moksha.noinline"))
        hasAttrNoInline[name] = true;
      if (funcOp->hasAttr("moksha.pure"))
        hasAttrPure[name] = true;
      if (funcOp->hasAttr("moksha.cold"))
        hasAttrCold[name] = true;
      if (funcOp->hasAttr("moksha.used"))
        hasAttrUsed[name] = true;
      if (funcOp->hasAttr("moksha.interrupt"))
        hasAttrInterrupt[name] = true;
      if (funcOp->hasAttr("moksha.async") || funcOp->hasAttr("async"))
        hasAttrAsync[name] = true;
      if (funcOp.getFunctionType().getNumResults() == 1) {
        if (mlir::isa<IR::PromiseType>(funcOp.getFunctionType().getResult(0))) {
          hasAttrAsync[name] = true;
        }
      }
      funcOp.walk([&](IR::AwaitOp) { hasAttrAsync[name] = true; });
    });

    mlir::LLVMConversionTarget target(getContext());
    target.addLegalOp<mlir::ModuleOp>();

    MokshaToLLVMTypeConverter typeConverter(&getContext());
    mlir::RewritePatternSet patterns(&getContext());

    // 1. Lower Core MLIR (Func, ControlFlow)
    mlir::populateFuncToLLVMConversionPatterns(typeConverter, patterns);
    mlir::cf::populateControlFlowToLLVMConversionPatterns(typeConverter,
                                                          patterns);

    // 2. Lower Moksha Custom Ops
    patterns.add<
        ReturnOpLowering, GlobalOpLowering, ConstantOpLowering,
        AllocaOpLowering, LoadOpLowering, StoreOpLowering, InlineAsmOpLowering,
        GetElementPtrOpLowering, ExtractValueOpLowering, InsertValueOpLowering,
        BitcastOpLowering, CastOpLowering, UpcastOpLowering, AnyCastOpLowering,
        AddressOfOpLowering, CmpOpLowering, UnreachableOpLowering,
        RetainOpLowering, ReleaseOpLowering, StoreWeakOpLowering,
        LoadWeakOpLowering, SpawnOpLowering, AwaitOpLowering,
        MakeClosureOpLowering, CustomCallOpLowering, InvokeIndirectOpLowering,
        InvokeOpLowering, PowOpLowering, SpawnOpLowering, AwaitOpLowering,
        AtomicStoreOpLowering, ResumeOpLowering, AtomicLoadOpLowering,
        AtomicRMWOpLowering, AtomicCmpXchgOpLowering, FenceOpLowering,
        AddOpLowering, ThrowOpLowering, LandingPadOpLowering>(typeConverter);

    // Math & Bitwise
    patterns.add<
        BinaryOpLowering<IR::SubOp, mlir::LLVM::SubOp, mlir::LLVM::FSubOp>,
        BinaryOpLowering<IR::MulOp, mlir::LLVM::MulOp, mlir::LLVM::FMulOp>,
        BitwiseOpLowering<IR::AndOp, mlir::LLVM::AndOp>,
        BitwiseOpLowering<IR::OrOp, mlir::LLVM::OrOp>,
        BitwiseOpLowering<IR::XorOp, mlir::LLVM::XOrOp>,
        BitwiseOpLowering<IR::ShlOp, mlir::LLVM::ShlOp>,
        BitwiseOpLowering<IR::ShrOp, mlir::LLVM::AShrOp>,
        SafeDivModOpLowering<IR::DivOp, mlir::LLVM::SDivOp, mlir::LLVM::FDivOp>,
        SafeDivModOpLowering<IR::ModOp, mlir::LLVM::SRemOp,
                             mlir::LLVM::FRemOp>>(typeConverter);

    // We make LLVM completely legal
    target.addLegalDialect<mlir::LLVM::LLVMDialect>();
    target.addIllegalDialect<IR::MokshaDialect>();
    target.addIllegalDialect<mlir::func::FuncDialect>();
    target.addIllegalDialect<mlir::cf::ControlFlowDialect>();

    llvm::errs() << "[DEBUG] Executing applyFullConversion...\n";
    if (mlir::failed(mlir::applyFullConversion(getOperation(), target,
                                               std::move(patterns)))) {
      llvm::errs() << "[FATAL] applyFullConversion failed!\n";
      signalPassFailure();
      return;
    }
    llvm::errs() << "[DEBUG] applyFullConversion succeeded!\n";

    mlir::ModuleOp module = getOperation();
    module.walk([&](mlir::LLVM::LLVMFuncOp llvmFunc) {
      auto getByteSize = [&](mlir::Type t) -> size_t {
        auto impl = [&](auto &self, mlir::Type t_) -> size_t {
          if (t_.isIntOrFloat())
            return std::max<size_t>(1, t_.getIntOrFloatBitWidth() / 8);
          if (auto arr = mlir::dyn_cast<mlir::LLVM::LLVMArrayType>(t_))
            return self(self, arr.getElementType()) * arr.getNumElements();
          if (auto st = mlir::dyn_cast<mlir::LLVM::LLVMStructType>(t_)) {
            size_t size = 0;
            for (auto f : st.getBody())
              size += self(self, f);
            return size;
          }
          return 8;
        };
        return impl(impl, t);
      };

      bool needsChange = false;
      auto fnTy = llvmFunc.getFunctionType();
      llvm::SmallVector<mlir::Type, 4> newArgTypes;
      llvm::SmallVector<unsigned, 4> changedArgs;
      llvm::SmallVector<mlir::Type, 4> oldAggregateTypes;

      for (unsigned i = 0; i < fnTy.getNumParams(); ++i) {
        mlir::Type argTy = fnTy.getParamType(i);
        bool isAggregate = mlir::isa<mlir::LLVM::LLVMStructType>(argTy) ||
                           mlir::isa<mlir::LLVM::LLVMArrayType>(argTy);

        if (isAggregate && getByteSize(argTy) > 8) {
          needsChange = true;
          newArgTypes.push_back(
              mlir::LLVM::LLVMPointerType::get(&getContext()));
          changedArgs.push_back(i);
          oldAggregateTypes.push_back(argTy);
        } else {
          newArgTypes.push_back(argTy);
        }
      }

      if (!needsChange)
        return;

      auto newFnTy = mlir::LLVM::LLVMFunctionType::get(
          fnTy.getReturnType(), newArgTypes, fnTy.isVarArg());
      llvmFunc.setFunctionType(newFnTy);
      mlir::OpBuilder builder(&getContext());
      llvm::SmallVector<mlir::Attribute, 4> newArgAttrs;
      auto existingArgAttrs =
          llvmFunc->getAttrOfType<mlir::ArrayAttr>("arg_attrs");

      size_t changedIdx = 0;
      for (unsigned i = 0; i < newArgTypes.size(); ++i) {
        mlir::DictionaryAttr dict = builder.getDictionaryAttr({});
        if (existingArgAttrs && i < existingArgAttrs.size()) {
          if (auto d =
                  mlir::dyn_cast<mlir::DictionaryAttr>(existingArgAttrs[i])) {
            dict = d;
          }
        }

        if (changedIdx < changedArgs.size() && changedArgs[changedIdx] == i) {
          mlir::NamedAttribute byvalAttr = builder.getNamedAttr(
              "llvm.byval", mlir::TypeAttr::get(oldAggregateTypes[changedIdx]));
          llvm::SmallVector<mlir::NamedAttribute, 2> attrs(
              dict.getValue().begin(), dict.getValue().end());
          attrs.push_back(byvalAttr);
          dict = builder.getDictionaryAttr(attrs);
          changedIdx++;
        }
        newArgAttrs.push_back(dict);
      }
      llvmFunc->setAttr("arg_attrs", builder.getArrayAttr(newArgAttrs));
      if (!llvmFunc.getBody().empty()) {
        mlir::Block &entry = llvmFunc.getBody().front();
        mlir::OpBuilder entryBuilder(&entry, entry.begin());

        changedIdx = 0;
        for (unsigned idx : changedArgs) {
          mlir::BlockArgument blkArg = entry.getArgument(idx);
          mlir::Type oldTy = oldAggregateTypes[changedIdx++];
          blkArg.setType(newArgTypes[idx]);
          auto loadOp = entryBuilder.create<mlir::LLVM::LoadOp>(
              llvmFunc.getLoc(), oldTy, blkArg);
          blkArg.replaceAllUsesExcept(loadOp.getResult(), loadOp);
        }
      }
    });

    bool needsPersonality = false;
    module.walk([&](mlir::LLVM::LandingpadOp) { needsPersonality = true; });

    // INJECT C-ABI COROUTINE RESUME WRAPPER
    if (!module.lookupSymbol("moksha_rt_resume_coro")) {
      mlir::OpBuilder builder(module.getBodyRegion());
      auto loc = module.getLoc();
      auto llvmVoidTy = mlir::LLVM::LLVMVoidType::get(&getContext());
      auto llvmI8PtrTy = mlir::LLVM::LLVMPointerType::get(&getContext());
      bool hasMain =
          module.lookupSymbol("main") || module.lookupSymbol("__moksha_main");
      auto resumeLinkage = hasMain ? mlir::LLVM::Linkage::External
                                   : mlir::LLVM::Linkage::Internal;

      auto resumeFnTy =
          mlir::LLVM::LLVMFunctionType::get(llvmVoidTy, {llvmI8PtrTy});
      auto resumeFunc = builder.create<mlir::LLVM::LLVMFuncOp>(
          loc, "moksha_rt_resume_coro", resumeFnTy, resumeLinkage);

      mlir::Block *block = resumeFunc.addEntryBlock(builder);
      mlir::OpBuilder funcBuilder(block, block->begin());

      if (!module.lookupSymbol("llvm.coro.resume")) {
        builder.create<mlir::LLVM::LLVMFuncOp>(loc, "llvm.coro.resume",
                                               resumeFnTy);
      }

      auto callOp = funcBuilder.create<mlir::LLVM::CallOp>(
          loc, mlir::TypeRange{},
          mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.resume"),
          mlir::ValueRange{block->getArgument(0)});

      callOp->setAttr("callee_type", mlir::TypeAttr::get(resumeFnTy));
      funcBuilder.create<mlir::LLVM::ReturnOp>(loc, mlir::ValueRange{});
    }

    llvm::errs() << "[DEBUG] Injecting Personality & Attributes...\n";

    llvm::Triple triple(llvm::sys::getDefaultTargetTriple());
    llvm::StringRef persFnName = "__gxx_personality_v0";
    if (needsPersonality) {
      // Target-aware personality routing
      if (triple.isOSWindows()) {
        if (triple.isGNUEnvironment()) {
          persFnName = "__gxx_personality_seh0"; // MinGW SEH
        } else {
          persFnName = "__CxxFrameHandler3"; // MSVC
        }
      }

      // 1. Declare the personality function in the MLIR module if missing
      if (!module.lookupSymbol(persFnName)) {
        mlir::OpBuilder builder(module.getBodyRegion());
        auto i32Ty = mlir::IntegerType::get(&getContext(), 32);
        auto fnTy = mlir::LLVM::LLVMFunctionType::get(i32Ty, {}, true);
        builder.create<mlir::LLVM::LLVMFuncOp>(module.getLoc(), persFnName,
                                               fnTy);
      }
    }

    auto persAttr = mlir::FlatSymbolRefAttr::get(&getContext(), persFnName);

    // 2. Restore Custom Attributes onto the LLVMFuncOp
    getOperation().walk([&](mlir::LLVM::LLVMFuncOp llvmFunc) {
      if (auto linkAttr =
              llvmFunc->getAttrOfType<mlir::StringAttr>("moksha.linkage")) {
        llvm::StringRef linkStr = linkAttr.getValue();
        if (llvmFunc.getBody().empty()) {
          llvmFunc.setLinkage(mlir::LLVM::Linkage::External);
        } else if (linkStr == "internal") {
          llvmFunc.setLinkage(mlir::LLVM::Linkage::Internal);
        } else if (linkStr == "weak") {
          llvmFunc.setLinkage(mlir::LLVM::Linkage::Weak);
        } else if (linkStr == "external") {
          llvmFunc.setLinkage(mlir::LLVM::Linkage::External);
        }
      }

      // 2a. Satisfy the MLIR LLVM Dialect Verifier!
      bool hasPad = false;
      llvmFunc.walk([&](mlir::LLVM::LandingpadOp) { hasPad = true; });
      if (hasPad) {
        llvmFunc.setPersonalityAttr(persAttr);
      }

      // 2b. Restore Moksha attributes
      auto name = llvmFunc.getName();
      auto unit = mlir::UnitAttr::get(&getContext());
      if (hasAttrNaked.count(name))
        llvmFunc->setAttr("moksha.naked", unit);
      if (hasAttrNoRet.count(name))
        llvmFunc->setAttr("moksha.noreturn", unit);
      if (hasAttrInline.count(name))
        llvmFunc->setAttr("moksha.inline", unit);
      if (hasAttrNoInline.count(name))
        llvmFunc->setAttr("moksha.noinline", unit);
      if (hasAttrPure.count(name))
        llvmFunc->setAttr("moksha.pure", unit);
      if (hasAttrCold.count(name))
        llvmFunc->setAttr("moksha.cold", unit);
      if (hasAttrUsed.count(name))
        llvmFunc->setAttr("moksha.used", unit);
      if (hasAttrInterrupt.count(name))
        llvmFunc->setAttr("moksha.interrupt", unit);

      bool hasAwait = false;
      llvmFunc.walk([&](mlir::LLVM::CallOp callOp) {
        if (callOp.getCallee() && *callOp.getCallee() == "llvm.coro.suspend") {
          hasAwait = true;
        }
      });

      if (hasAwait) {
        llvmFunc->setAttr("moksha.async", unit);

        if (!llvmFunc.getBody().empty()) {
          mlir::Block &entryBlock = llvmFunc.getBody().front();
          mlir::OpBuilder builder(&getContext());
          builder.setInsertionPointToStart(&entryBlock);

          auto loc = llvmFunc.getLoc();
          auto llvmI32Ty = mlir::IntegerType::get(&getContext(), 32);
          auto llvmI64Ty = mlir::IntegerType::get(&getContext(), 64);
          auto llvmI8PtrTy = mlir::LLVM::LLVMPointerType::get(&getContext());
          auto llvmTokenTy = mlir::LLVM::LLVMTokenType::get(&getContext());
          auto llvmI1Ty = builder.getI1Type();
          auto llvmVoidTy = mlir::LLVM::LLVMVoidType::get(&getContext());
          auto module = llvmFunc->getParentOfType<mlir::ModuleOp>();

          // 1. Declare Intrinsics
          if (!module.lookupSymbol("llvm.coro.id")) {
            mlir::OpBuilder::InsertionGuard guard(builder);
            builder.setInsertionPointToStart(module.getBody());
            builder.create<mlir::LLVM::LLVMFuncOp>(
                loc, "llvm.coro.id",
                mlir::LLVM::LLVMFunctionType::get(
                    llvmTokenTy,
                    {llvmI32Ty, llvmI8PtrTy, llvmI8PtrTy, llvmI8PtrTy}));
          }
          if (!module.lookupSymbol("llvm.coro.size.i64")) {
            mlir::OpBuilder::InsertionGuard guard(builder);
            builder.setInsertionPointToStart(module.getBody());
            builder.create<mlir::LLVM::LLVMFuncOp>(
                loc, "llvm.coro.size.i64",
                mlir::LLVM::LLVMFunctionType::get(llvmI64Ty, {}));
          }
          if (!module.lookupSymbol("llvm.coro.begin")) {
            mlir::OpBuilder::InsertionGuard guard(builder);
            builder.setInsertionPointToStart(module.getBody());
            builder.create<mlir::LLVM::LLVMFuncOp>(
                loc, "llvm.coro.begin",
                mlir::LLVM::LLVMFunctionType::get(llvmI8PtrTy,
                                                  {llvmTokenTy, llvmI8PtrTy}));
          }
          if (!module.lookupSymbol("llvm.coro.free")) {
            mlir::OpBuilder::InsertionGuard guard(builder);
            builder.setInsertionPointToStart(module.getBody());
            builder.create<mlir::LLVM::LLVMFuncOp>(
                loc, "llvm.coro.free",
                mlir::LLVM::LLVMFunctionType::get(llvmI8PtrTy,
                                                  {llvmTokenTy, llvmI8PtrTy}));
          }
          if (!module.lookupSymbol("llvm.coro.end")) {
            mlir::OpBuilder::InsertionGuard guard(builder);
            builder.setInsertionPointToStart(module.getBody());
            builder.create<mlir::LLVM::LLVMFuncOp>(
                loc, "llvm.coro.end",
                mlir::LLVM::LLVMFunctionType::get(
                    llvmVoidTy, {llvmI8PtrTy, llvmI1Ty, llvmTokenTy}));
          }
          if (!module.lookupSymbol("__moksha_alloc")) {
            mlir::OpBuilder::InsertionGuard guard(builder);
            builder.setInsertionPointToStart(module.getBody());
            builder.create<mlir::LLVM::LLVMFuncOp>(
                loc, "__moksha_alloc",
                mlir::LLVM::LLVMFunctionType::get(llvmI8PtrTy,
                                                  {llvmI64Ty, llvmI32Ty}));
          }
          if (!module.lookupSymbol("__moksha_free")) {
            mlir::OpBuilder::InsertionGuard guard(builder);
            builder.setInsertionPointToStart(module.getBody());
            builder.create<mlir::LLVM::LLVMFuncOp>(
                loc, "__moksha_free",
                mlir::LLVM::LLVMFunctionType::get(llvmVoidTy, {llvmI8PtrTy}));
          }

          // 2. Setup Frame
          auto nullPtr = builder.create<mlir::LLVM::ZeroOp>(loc, llvmI8PtrTy);
          auto zero32 = builder.create<mlir::LLVM::ConstantOp>(
              loc, llvmI32Ty, builder.getI32IntegerAttr(0));

          auto coroId = builder.create<mlir::LLVM::CallOp>(
              loc, llvmTokenTy,
              mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.id"),
              mlir::ValueRange{zero32, nullPtr, nullPtr, nullPtr});
          coroId->setAttr("callee_type",
                          mlir::TypeAttr::get(mlir::LLVM::LLVMFunctionType::get(
                              llvmTokenTy, {llvmI32Ty, llvmI8PtrTy, llvmI8PtrTy,
                                            llvmI8PtrTy})));

          auto coroSize = builder.create<mlir::LLVM::CallOp>(
              loc, llvmI64Ty,
              mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.size.i64"),
              mlir::ValueRange{});
          coroSize->setAttr(
              "callee_type",
              mlir::TypeAttr::get(
                  mlir::LLVM::LLVMFunctionType::get(llvmI64Ty, {})));

          auto typeTag = builder.create<mlir::LLVM::ConstantOp>(
              loc, llvmI32Ty, builder.getI32IntegerAttr(19));

          auto allocCall = builder.create<mlir::LLVM::CallOp>(
              loc, llvmI8PtrTy,
              mlir::SymbolRefAttr::get(&getContext(), "__moksha_alloc"),
              mlir::ValueRange{coroSize.getResult(), typeTag.getResult()});

          allocCall->setAttr(
              "callee_type",
              mlir::TypeAttr::get(mlir::LLVM::LLVMFunctionType::get(
                  llvmI8PtrTy, {llvmI64Ty, llvmI32Ty})));

          auto coroBegin = builder.create<mlir::LLVM::CallOp>(
              loc, llvmI8PtrTy,
              mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.begin"),
              mlir::ValueRange{coroId.getResult(), allocCall.getResult()});
          coroBegin->setAttr(
              "callee_type",
              mlir::TypeAttr::get(mlir::LLVM::LLVMFunctionType::get(
                  llvmI8PtrTy, {llvmTokenTy, llvmI8PtrTy})));

          if (!module.lookupSymbol("moksha_rt_coro_setup")) {
            mlir::OpBuilder::InsertionGuard guard(builder);
            builder.setInsertionPointToStart(module.getBody());
            builder.create<mlir::LLVM::LLVMFuncOp>(
                loc, "moksha_rt_coro_setup",
                mlir::LLVM::LLVMFunctionType::get(llvmI8PtrTy, {llvmI8PtrTy}));
          }

          auto setupCall = builder.create<mlir::LLVM::CallOp>(
              loc, llvmI8PtrTy,
              mlir::SymbolRefAttr::get(&getContext(), "moksha_rt_coro_setup"),
              mlir::ValueRange{coroBegin.getResult()});
          setupCall->setAttr(
              "callee_type",
              mlir::TypeAttr::get(mlir::LLVM::LLVMFunctionType::get(
                  llvmI8PtrTy, {llvmI8PtrTy})));

          mlir::Value promiseHandle = setupCall.getResult();

          // 2.5 Declare the Scheduler Hook
          if (!module.lookupSymbol("moksha_scheduler_schedule")) {
            mlir::OpBuilder::InsertionGuard guard(builder);
            builder.setInsertionPointToStart(module.getBody());
            builder.create<mlir::LLVM::LLVMFuncOp>(
                loc, "moksha_scheduler_schedule",
                mlir::LLVM::LLVMFunctionType::get(llvmVoidTy, {llvmI8PtrTy}));
          }

          // 3. Link Await Calls, Unify Suspend Blocks, and Inject Cleanup
          llvm::SmallVector<mlir::LLVM::CallOp, 4> callOpsToProcess;
          llvmFunc.walk([&](mlir::LLVM::CallOp callOp) {
            callOpsToProcess.push_back(callOp);
          });

          for (auto callOp : callOpsToProcess) {
            auto callee = callOp.getCallee();

            if (callee && *callee == "moksha_rt_register_await") {
              callOp.setOperand(1, coroBegin.getResult());
            }

            if (callee && *callee == "llvm.coro.save") {
              callOp.setOperand(0, coroBegin.getResult());
            }

            if (callee && *callee == "llvm.coro.suspend") {
              if (auto switchOp = mlir::dyn_cast_or_null<mlir::LLVM::SwitchOp>(
                      callOp->getNextNode())) {

                mlir::Block *destroyBlock = switchOp.getCaseDestinations()[1];

                if (destroyBlock->front().getName().getStringRef() !=
                    "llvm.coro.free") {
                  mlir::OpBuilder cleanupBuilder(&getContext());
                  cleanupBuilder.setInsertionPointToStart(destroyBlock);

                  mlir::Value freeArgs[] = {coroId.getResult(),
                                            coroBegin.getResult()};
                  auto coroFree = cleanupBuilder.create<mlir::LLVM::CallOp>(
                      loc, llvmI8PtrTy,
                      mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.free"),
                      mlir::ValueRange(llvm::ArrayRef<mlir::Value>(freeArgs)));
                  coroFree->setAttr(
                      "callee_type",
                      mlir::TypeAttr::get(mlir::LLVM::LLVMFunctionType::get(
                          llvmI8PtrTy, {llvmTokenTy, llvmI8PtrTy})));

                  mlir::Value mokshaFreeArgs[] = {coroFree.getResult()};
                  auto mFree = cleanupBuilder.create<mlir::LLVM::CallOp>(
                      loc, mlir::TypeRange{},
                      mlir::SymbolRefAttr::get(&getContext(), "__moksha_free"),
                      mlir::ValueRange(
                          llvm::ArrayRef<mlir::Value>(mokshaFreeArgs)));
                  mFree->setAttr(
                      "callee_type",
                      mlir::TypeAttr::get(mlir::LLVM::LLVMFunctionType::get(
                          llvmVoidTy, {llvmI8PtrTy})));

                  auto trueVal = cleanupBuilder.create<mlir::LLVM::ConstantOp>(
                      loc, llvmI1Ty, 1);
                  auto nullToken =
                      cleanupBuilder.create<mlir::LLVM::NoneTokenOp>(
                          loc, llvmTokenTy);

                  mlir::Value endArgs[] = {coroBegin.getResult(),
                                           trueVal.getResult(),
                                           nullToken.getResult()};
                  auto coroEnd = cleanupBuilder.create<mlir::LLVM::CallOp>(
                      loc, mlir::TypeRange{},
                      mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.end"),
                      mlir::ValueRange(llvm::ArrayRef<mlir::Value>(endArgs)));
                  coroEnd->setAttr(
                      "callee_type",
                      mlir::TypeAttr::get(mlir::LLVM::LLVMFunctionType::get(
                          llvmVoidTy, {llvmI8PtrTy, llvmI1Ty, llvmTokenTy})));
                }
              }
            }
          }

          // 4. Unify Returns and Inject coro.end
          llvm::SmallVector<mlir::LLVM::ReturnOp, 4> finalReturns;
          llvm::SmallVector<mlir::LLVM::ReturnOp, 4> yieldReturns;

          llvmFunc.walk([&](mlir::LLVM::ReturnOp retOp) {
            if (retOp->hasAttr("moksha.yield")) {
              yieldReturns.push_back(retOp);
            } else {
              finalReturns.push_back(retOp);
            }
          });

          for (auto yRet : yieldReturns) {
            if (yRet.getNumOperands() > 0 &&
                yRet.getOperand(0).getType() == llvmI8PtrTy) {
              yRet.setOperand(0, promiseHandle);
            }
          }

          if (finalReturns.size() > 1) {
            mlir::OpBuilder builder(&getContext());
            mlir::Block *unifiedRetBlock = llvmFunc.addBlock();
            builder.setInsertionPointToEnd(unifiedRetBlock);

            mlir::Type retTy = llvmFunc.getFunctionType().getReturnType();
            mlir::Value retVal = nullptr;

            if (!mlir::isa<mlir::LLVM::LLVMVoidType>(retTy)) {
              unifiedRetBlock->addArgument(retTy, loc);
              retVal = unifiedRetBlock->getArgument(0);
            }

            auto unifiedRet = builder.create<mlir::LLVM::ReturnOp>(
                loc, retVal ? mlir::ValueRange{retVal} : mlir::ValueRange{});

            for (auto retOp : finalReturns) {
              builder.setInsertionPoint(retOp);
              if (retVal) {
                builder.create<mlir::LLVM::BrOp>(
                    loc, mlir::ValueRange{retOp.getOperand(0)},
                    unifiedRetBlock);
              } else {
                builder.create<mlir::LLVM::BrOp>(loc, mlir::ValueRange{},
                                                 unifiedRetBlock);
              }
              retOp.erase();
            }

            finalReturns.clear();
            finalReturns.push_back(unifiedRet);
          }

          for (auto retOp : finalReturns) {
            mlir::OpBuilder retBuilder(retOp);

            if (retOp.getNumOperands() > 0 &&
                retOp.getOperand(0).getType() == llvmI8PtrTy) {
              mlir::Value originalRetVal = retOp.getOperand(0);

              if (!module.lookupSymbol("moksha_rt_coro_finish")) {
                mlir::OpBuilder::InsertionGuard guard(retBuilder);
                retBuilder.setInsertionPointToStart(module.getBody());
                retBuilder.create<mlir::LLVM::LLVMFuncOp>(
                    loc, "moksha_rt_coro_finish",
                    mlir::LLVM::LLVMFunctionType::get(
                        llvmVoidTy, {llvmI8PtrTy, llvmI8PtrTy}));
              }

              mlir::Value finishArgs[] = {promiseHandle, originalRetVal};
              auto rtFinish = retBuilder.create<mlir::LLVM::CallOp>(
                  loc, mlir::TypeRange{},
                  mlir::SymbolRefAttr::get(&getContext(),
                                           "moksha_rt_coro_finish"),
                  mlir::ValueRange(llvm::ArrayRef<mlir::Value>(finishArgs)));
              rtFinish->setAttr(
                  "callee_type",
                  mlir::TypeAttr::get(mlir::LLVM::LLVMFunctionType::get(
                      llvmVoidTy, {llvmI8PtrTy, llvmI8PtrTy})));
            }

            auto falseVal = retBuilder.create<mlir::LLVM::ConstantOp>(
                loc, llvmI1Ty, retBuilder.getIntegerAttr(llvmI1Ty, 0));

            auto nullToken =
                retBuilder.create<mlir::LLVM::NoneTokenOp>(loc, llvmTokenTy);

            mlir::Value retEndArgs[] = {coroBegin.getResult(),
                                        falseVal.getResult(),
                                        nullToken.getResult()};
            auto rtEnd = retBuilder.create<mlir::LLVM::CallOp>(
                loc, mlir::TypeRange{},
                mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.end"),
                mlir::ValueRange(llvm::ArrayRef<mlir::Value>(retEndArgs)));
            rtEnd->setAttr(
                "callee_type",
                mlir::TypeAttr::get(mlir::LLVM::LLVMFunctionType::get(
                    llvmVoidTy, {llvmI8PtrTy, llvmI1Ty, llvmTokenTy})));

            if (retOp.getNumOperands() > 0 &&
                retOp.getOperand(0).getType() == llvmI8PtrTy) {
              retOp.setOperand(0, promiseHandle);
            }
          }
        }
      }
    });

    // 4. Hoist Allocas to the Entry Block (CRITICAL FOR COROUTINES & ABI)
    module.walk([&](mlir::LLVM::LLVMFuncOp llvmFunc) {
      if (llvmFunc.empty())
        return;
      mlir::Block &entryBlock = llvmFunc.front();

      auto insertPtIter = entryBlock.begin();
      while (insertPtIter != entryBlock.end()) {
        if (auto callOp = mlir::dyn_cast<mlir::LLVM::CallOp>(*insertPtIter)) {
          auto callee = callOp.getCallee();
          if (callee &&
              (*callee == "llvm.coro.id" || *callee == "llvm.coro.size.i64" ||
               *callee == "__moksha_alloc" || *callee == "llvm.coro.begin" ||
               *callee == "moksha_rt_coro_setup")) {
            ++insertPtIter;
            continue;
          }
        }
        if (mlir::isa<mlir::LLVM::AllocaOp>(*insertPtIter) ||
            mlir::isa<mlir::LLVM::ConstantOp>(*insertPtIter)) {
          ++insertPtIter;
          continue;
        }
        break;
      }

      // If the block is entirely allocas/constants, do nothing.
      if (insertPtIter == entryBlock.end())
        return;

      mlir::Operation *safeInsertBoundary = &*insertPtIter;

      llvm::SmallVector<mlir::LLVM::AllocaOp, 4> allocasToMove;
      llvmFunc.walk([&](mlir::LLVM::AllocaOp allocaOp) {
        if (allocaOp->getBlock() != &entryBlock ||
            !allocaOp->isBeforeInBlock(safeInsertBoundary)) {
          allocasToMove.push_back(allocaOp);
        }
      });

      std::unordered_set<mlir::Operation *> movedSizes;
      mlir::Operation *currentInsertPoint = safeInsertBoundary;

      for (auto allocaOp : allocasToMove) {
        mlir::Operation *sizeOp = allocaOp.getArraySize().getDefiningOp();

        if (sizeOp) {
          if (movedSizes.insert(sizeOp).second) {
            if (sizeOp->getBlock() != &entryBlock ||
                !sizeOp->isBeforeInBlock(safeInsertBoundary)) {
              sizeOp->moveBefore(currentInsertPoint);
            }
          }
        }
        allocaOp->moveBefore(safeInsertBoundary);
      }
    });

    // 3. Rename "main" to "__moksha_main" for the C-Runtime entry point
    if (auto mainFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("main")) {
      mainFunc.setName("__moksha_main");
    }
    llvm::errs() << "[DEBUG] ConvertMokshaToLLVMPass Complete!\n";
  }
};
} // end anonymous namespace

std::unique_ptr<mlir::Pass> createConvertMokshaToLLVMPass() {
  return std::make_unique<ConvertMokshaToLLVMPass>();
}

} // namespace moksha
