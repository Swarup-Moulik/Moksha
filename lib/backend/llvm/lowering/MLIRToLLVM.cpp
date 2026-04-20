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

namespace moksha {

namespace {

// ============================================================================
// 1. Full Type Converter
// ============================================================================
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

    // Fat Pointers (Structs)
    addConversion([&](IR::SliceType type) {
      return mlir::LLVM::LLVMStructType::getLiteral(
          type.getContext(),
          {mlir::LLVM::LLVMPointerType::get(type.getContext()),
           mlir::IntegerType::get(type.getContext(), 64)});
    });
    addConversion([&](IR::ClosureType type) {
      return mlir::LLVM::LLVMStructType::getLiteral(
          type.getContext(), {
                                 mlir::LLVM::LLVMPointerType::get(
                                     type.getContext()), // Function ptr
                                 mlir::LLVM::LLVMPointerType::get(
                                     type.getContext()) // Environment ptr
                             });
    });
    addConversion([&](IR::DecimalType type) {
      return mlir::LLVM::LLVMStructType::getLiteral(
          type.getContext(),
          {
              mlir::IntegerType::get(type.getContext(), 128), // Mantissa
              mlir::IntegerType::get(type.getContext(), 32)   // Scale
          });
    });
    addConversion([&](IR::AnyType type) {
      return mlir::LLVM::LLVMPointerType::get(type.getContext());
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
  }
};

// ============================================================================
// 2. Helpers for External Runtime Calls & Constants
// ============================================================================
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
  auto funcType =
      mlir::LLVM::LLVMFunctionType::get(retTy, argTypes, /*isVarArg=*/false);

  if (!module.lookupSymbol(funcName)) {
    mlir::OpBuilder::InsertionGuard guard(rewriter);
    rewriter.setInsertionPointToStart(module.getBody());
    rewriter.create<mlir::LLVM::LLVMFuncOp>(loc, funcName, funcType);
  }

  auto symRef = mlir::SymbolRefAttr::get(rewriter.getContext(), funcName);
  auto callOp =
      rewriter.create<mlir::LLVM::CallOp>(loc, retTypes, symRef, args);

  // [MLIR 22 FIX] The verifier requires this property to resolve opaque
  // pointers!
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

  auto funcType = mlir::LLVM::LLVMFunctionType::get(returnType, argTypes,
                                                    /*isVarArg=*/false);

  if (!module.lookupSymbol(funcName)) {
    mlir::OpBuilder::InsertionGuard guard(rewriter);
    rewriter.setInsertionPointToStart(module.getBody());
    rewriter.create<mlir::LLVM::LLVMFuncOp>(loc, funcName, funcType);
  }

  auto symRef = mlir::SymbolRefAttr::get(rewriter.getContext(), funcName);
  auto callOp =
      rewriter.create<mlir::LLVM::CallOp>(loc, returnType, symRef, args);

  // [MLIR 22 FIX] Property mapping for opaque pointers
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
  if (auto ptrTy = mlir::dyn_cast<IR::PointerType>(type)) {
    if (ptrTy.getPointee().isInteger(8))
      return 16; // MOKSHA_TYPE_STRING
    return 19;   // MOKSHA_TYPE_PTR (Generic)
  }

  if (mlir::isa<IR::ArrayType>(type))
    return 18; // MOKSHA_TYPE_ARRAY
  if (mlir::isa<IR::MapType>(type))
    return 17; // MOKSHA_TYPE_TABLE

  return 19;
}

static mlir::LLVM::AtomicOrdering mapAtomicOrdering(int32_t ord) {
  switch (ord) {
  case 0:
    return mlir::LLVM::AtomicOrdering::not_atomic;
  case 1:
    return mlir::LLVM::AtomicOrdering::monotonic; // Relaxed
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

// Extracted Constant Materialization to reuse in Global Table Init
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

      // Append the null terminator
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
            loc, arrayTy, /*isConstant=*/true, mlir::LLVM::Linkage::Private,
            globalName, nullTermAttr);
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
        // [NUCLEAR FIX] Completely bypass the Pointer ConstantOp verifier!
        // We create a strict i64 constant first, then cleanly IntToPtr it.
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
    // Safely apply semantic APFloat downcasting to f32/f16
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

// Bypasses LLVM 22's broken FPExt ConstantFolder by evaluating precision
// extensions natively in MLIR.
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

// ============================================================================
// 4. Global Operations
// ============================================================================
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

    if (initAttr) {
      if (mlir::isa<mlir::UnitAttr>(initAttr)) {
        initAttr = nullptr; // [CRITICAL FIX] Prevent UnitAttr crashes
      } else if (auto intAttr = mlir::dyn_cast<mlir::IntegerAttr>(initAttr)) {
        if (!llvmType.isIntOrIndex()) {
          if (intAttr.getInt() == 0) {
            initAttr = nullptr; // Safe zero-init, let TranslateToLLVM handle it
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

    if (auto linkAttr = op->getAttrOfType<mlir::StringAttr>("moksha.linkage")) {
      llvm::StringRef linkStr = linkAttr.getValue();
      if (linkStr == "internal")
        linkage = mlir::LLVM::Linkage::Internal;
      else if (linkStr == "weak")
        linkage = mlir::LLVM::Linkage::Weak;
    }

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
          initAttr = nullptr; // [CRITICAL FIX] Prevent ArrayAttr crashes
        }
      } else if (mlir::isa<mlir::StringAttr>(initAttr)) {
        if (mlir::isa<mlir::LLVM::LLVMPointerType>(llvmType)) {
          generateStrInit = true;
          strEntry = mlir::cast<mlir::StringAttr>(initAttr);
          initAttr = nullptr; // [CRITICAL FIX] Prevent StringAttr crashes
        } else if (mlir::isa<IR::DecimalType>(elementType)) {
          generateDecInit = true;
          decEntry = mlir::cast<mlir::StringAttr>(initAttr);
          initAttr = nullptr;
        }
      }
    }

    // --- [CRITICAL FIX] Ensure NO UnitAttr makes its way to llvm.mlir.global
    // ---
    if (op->hasAttr("moksha.zeroinit")) {
      initAttr = nullptr;
    }

    auto name = op.getSymName();
    bool isConstant = op->hasAttr("moksha.constant");
    uint64_t alignment = 0;
    if (auto alignAttr =
            op->getAttrOfType<mlir::IntegerAttr>("moksha.alignment")) {
      alignment = alignAttr.getInt();
    }
    bool threadLocal = op->hasAttr("moksha.thread_local");

    auto globalOp = rewriter.replaceOpWithNewOp<mlir::LLVM::GlobalOp>(
        op, llvmType, isConstant, linkage, name, initAttr, alignment,
        /*addrSpace=*/0, /*dsoLocal=*/false, /*threadLocal=*/threadLocal);

    // ALWAYS tag missing initializers for post-processing so TranslateToLLVM
    // forces a definition
    if (!initAttr) {
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

    // --- Complex Initialization Injection (Strings, Maps, Decimals) ---
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

                auto mapTy = llvm::dyn_cast<IR::MapType>(elementType);
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
                  mlir::Type convertedTy =
                      typeConverter->convertType(val.getType());
                  if (convertedTy && convertedTy != val.getType()) {
                    val = rewriter
                              .create<mlir::UnrealizedConversionCastOp>(
                                  loc, convertedTy, val)
                              .getResult(0);
                  }
                  mlir::Value nullPtr =
                      rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmPtrTy);
                  mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
                      loc, i32Ty, rewriter.getI32IntegerAttr(1));
                  mlir::Value sizeGep = rewriter.create<mlir::LLVM::GEPOp>(
                      loc, llvmPtrTy, val.getType(), nullPtr,
                      llvm::ArrayRef<mlir::LLVM::GEPArg>{one});
                  mlir::Value sizeInt = rewriter.create<mlir::LLVM::PtrToIntOp>(
                      loc, i64Ty, sizeGep);

                  uint32_t typeId = getMokshaTypeID(origTy);
                  mlir::Value typeTag = rewriter.create<mlir::LLVM::ConstantOp>(
                      loc, i32Ty, rewriter.getI32IntegerAttr(typeId));

                  mlir::Value heapPtr =
                      createRuntimeCall(rewriter, loc, llvmPtrTy,
                                        "moksha_rt_alloc", {sizeInt, typeTag});
                  auto storeOp =
                      rewriter.create<mlir::LLVM::StoreOp>(loc, val, heapPtr);
                  storeOp.setAlignment(8);
                  return heapPtr;
                };

                std::vector<mlir::Operation *> orphanStrings;
                for (auto &inst : *entryBlock) {
                  if (auto callOp = mlir::dyn_cast<mlir::func::CallOp>(inst)) {
                    if (callOp.getCallee() == "__moksha_string_concat" &&
                        callOp.use_empty()) {
                      orphanStrings.push_back(&inst);
                    }
                  } else if (auto llvmCall =
                                 mlir::dyn_cast<mlir::LLVM::CallOp>(inst)) {
                    if (llvmCall.getCallee() == "__moksha_string_concat" &&
                        llvmCall.use_empty()) {
                      orphanStrings.push_back(&inst);
                    }
                  }
                }

                size_t strIdx = 0;

                for (auto attr : mapEntries) {
                  if (auto kvAttr = llvm::dyn_cast<mlir::ArrayAttr>(attr)) {
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

                      createRuntimeCall(rewriter, loc, "moksha_rt_map_insert",
                                        {}, {mapPtr, kVal, vVal});
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
      // Safely apply semantic APFloat downcasting for inline constants
      auto targetTy = mlir::cast<mlir::FloatType>(llvmType);
      llvm::APFloat apVal = floatAttr.getValue();
      bool losesInfo;
      apVal.convert(targetTy.getFloatSemantics(),
                    llvm::APFloat::rmNearestTiesToEven, &losesInfo);

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

    // Get the very first block of the current function
    mlir::Block *entryBlock = &op->getParentRegion()->front();

    // Use an InsertionGuard so we can safely warp the builder's position
    mlir::OpBuilder::InsertionGuard guard(rewriter);

    // Move the builder to the top of the entry block
    rewriter.setInsertionPointToStart(entryBlock);

    mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
        op.getLoc(), rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));

    auto llvmAlloca = rewriter.create<mlir::LLVM::AllocaOp>(
        op.getLoc(), typeConverter->convertType(op.getType()),
        typeConverter->convertType(op.getAllocatedType()), one);

    // Replace the original op in its original location with the hoisted
    // allocation
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

    // === BOGUS ARRAY DECAY INTERCEPTION ===
    // [CRITICAL FIX] Only intercept if the load EXPECTS to return a pointer!
    if (mlir::isa<IR::PointerType>(op.getType())) {
      if (auto gepOp = ptr.getDefiningOp<IR::GetElementPtrOp>()) {
        if (auto castOp = gepOp.getOperand(0).getDefiningOp<IR::CastOp>()) {
          if (auto ptrTy = llvm::dyn_cast<IR::PointerType>(
                  castOp.getOperand().getType())) {
            if (llvm::isa<IR::ArrayType>(ptrTy.getPointee())) {
              // Bypass the load entirely! The GEP itself holds the correct
              // pointer offset.
              rewriter.replaceOp(op, adaptor.getOperands()[0]);
              return mlir::success();
            }
          }
        }
      }
    }

    // Standard load lowering...
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

    // Create the store operation
    auto llvmStore = rewriter.replaceOpWithNewOp<mlir::LLVM::StoreOp>(
        op, adaptor.getValue(), adaptor.getPtr());

    // Dynamically calculate and enforce natural ABI alignment
    unsigned alignment = 4; // Fallback default
    mlir::Type valTy = adaptor.getValue().getType();
    if (valTy.isIntOrFloat()) {
      alignment = std::max(1u, valTy.getIntOrFloatBitWidth() / 8);
    } else if (mlir::isa<mlir::LLVM::LLVMPointerType>(valTy)) {
      alignment = 8; // 64-bit architecture pointers
    }
    llvmStore.setAlignment(alignment);
    if (op->hasAttr("moksha.volatile")) {
      llvmStore.setVolatile_(true);
    }
    return mlir::success();
  }
};

// ============================================================================
// Inline Assembly Lowering
// ============================================================================
struct InlineAsmOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::InlineAsmOp> {
  using ConvertOpToLLVMPattern<IR::InlineAsmOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::InlineAsmOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    // 1. Determine the exact LLVM return type
    mlir::Type resultType;
    if (op.getNumResults() == 0) {
      resultType = mlir::LLVM::LLVMVoidType::get(getContext());
    } else if (op.getNumResults() == 1) {
      resultType = typeConverter->convertType(op.getResult(0).getType());
    } else {
      // Pack multiple return values into an LLVM struct
      llvm::SmallVector<mlir::Type, 4> resultTypes;
      for (auto res : op.getResults()) {
        resultTypes.push_back(typeConverter->convertType(res.getType()));
      }
      resultType =
          mlir::LLVM::LLVMStructType::getLiteral(getContext(), resultTypes);
    }

    // 2. Generate the native LLVM inline assembly operation
    rewriter.replaceOpWithNewOp<mlir::LLVM::InlineAsmOp>(
        op, resultType, adaptor.getOperands(), op.getAsmString(),
        op.getConstraints(),
        /*has_side_effects=*/true,
        /*is_align_stack=*/false,
        /*tail_call_kind=*/mlir::LLVM::tailcallkind::TailCallKind::None,
        /*asm_dialect=*/
        mlir::LLVM::AsmDialectAttr::get(getContext(),
                                        mlir::LLVM::AsmDialect::AD_ATT),
        /*operand_attrs=*/mlir::ArrayAttr() // Empty array for optional operand
                                            // attributes
    );

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

    // 1. FAT POINTER EXTRACTION: If the base is a Slice we must extract the raw
    // pointer
    if (llvm::isa<IR::SliceType>(origMokshaBaseTy)) {
      base = rewriter.create<mlir::LLVM::ExtractValueOp>(
          op.getLoc(), base, llvm::ArrayRef<int64_t>({0}));
      baseType = base.getType();
    }
    // 2. AGGREGATE SPILL: If it's a raw fixed array value, spill it to stack to
    // get a pointer
    else if (!llvm::isa<mlir::LLVM::LLVMPointerType>(baseType)) {
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

    // 3. POINTEE INFERENCE (Dynamic)
    if (indices.size() > 1) {
      if (auto ptrTy = llvm::dyn_cast<IR::PointerType>(origMokshaBaseTy)) {
        pointeeTy = typeConverter->convertType(ptrTy.getPointee());
      } else {
        pointeeTy = typeConverter->convertType(origMokshaBaseTy);
      }
    } else {
      if (auto resPtrTy = llvm::dyn_cast<IR::PointerType>(op.getType())) {
        pointeeTy = typeConverter->convertType(resPtrTy.getPointee());
      } else {
        pointeeTy = mlir::IntegerType::get(getContext(), 8); // fallback
      }
    }

    if (!pointeeTy) {
      pointeeTy = mlir::IntegerType::get(getContext(), 8);
    }

    rewriter.replaceOpWithNewOp<mlir::LLVM::GEPOp>(
        op, typeConverter->convertType(op.getType()), pointeeTy, base, indices);

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

    // [FIX] Bypass extractvalue for types lowered to flat pointers
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

    // Protect against silent type mismatches triggering conversion casts
    mlir::Type expectedType = typeConverter->convertType(op.getType());
    mlir::Type extractedType = ext.getType();

    // [FIX] Abort coercion logic if either type is null!
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

    // Safely extract from the generic operand array to avoid ODS naming issues
    auto operands = adaptor.getOperands();
    if (operands.size() < 2)
      return mlir::failure();

    mlir::Value container = operands[0];
    mlir::Value val = operands[1];
    int64_t idx = op.getIndex();

    // Auto-cast values to match the strict struct field types
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

// ============================================================================
// Casts & Address (Replaces your old CastOpLowering)
// ============================================================================
struct CastOpLowering : public mlir::ConvertOpToLLVMPattern<IR::CastOp> {
  using ConvertOpToLLVMPattern<IR::CastOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::CastOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Type dstType = typeConverter->convertType(op.getType());
    mlir::Type srcType = adaptor.getValue().getType();
    mlir::Type origSrcType = op.getValue().getType();
    mlir::Type origDstType = op.getType();

    // == == == == == == == == == == == == == == == == == == == == == == == ==
    // [FIX] WINDOWS MINGW FPU TRAP BYPASS
    // Pre-fold constants safely in C++ to prevent LLVM's ConstantFolder
    // from triggering a hardware SIGFPE on inexact float casts during
    // translation.
    // ========================================================================
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
          int64_t intVal = static_cast<int64_t>(fVal); // Safe C++ cast
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

          // [CRITICAL FIX] Downcast the f64 parsed float to match dstType
          // perfectly
          auto targetTy = mlir::cast<mlir::FloatType>(dstType);
          llvm::APFloat apVal = floatAttr.getValue();
          bool losesInfo;
          apVal.convert(targetTy.getFloatSemantics(),
                        llvm::APFloat::rmNearestTiesToEven, &losesInfo);

          rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
              op, dstType, rewriter.getFloatAttr(dstType, apVal));
          return mlir::success();
        }
      }
    }

    // If we are casting an integer to !moksha.ptr<i8> (the MLIR string type)
    if (origSrcType.isIntOrIndex() && mlir::isa<IR::PointerType>(origDstType)) {
      auto ptrTy = mlir::cast<IR::PointerType>(origDstType);

      if (ptrTy.getPointee().isInteger(8)) {
        auto loc = op.getLoc();
        auto module = op->getParentOfType<mlir::ModuleOp>();

        // Route to the correct C-runtime function based on bit width
        std::string funcName = origSrcType.getIntOrFloatBitWidth() == 64
                                   ? "__moksha_i64_to_string"
                                   : "__moksha_int_to_string";

        // Ensure the runtime function declaration exists in the LLVM module
        if (!module.lookupSymbol(funcName)) {
          mlir::OpBuilder::InsertionGuard guard(rewriter);
          rewriter.setInsertionPointToStart(module.getBody());
          auto fnType = mlir::LLVM::LLVMFunctionType::get(
              dstType, {adaptor.getValue().getType()});
          rewriter.create<mlir::LLVM::LLVMFuncOp>(loc, funcName, fnType);
        }

        // Replace the dangerous IntToPtr cast with a safe runtime call!
        auto call = rewriter.create<mlir::LLVM::CallOp>(
            loc, dstType,
            mlir::SymbolRefAttr::get(rewriter.getContext(), funcName),
            mlir::ValueRange{adaptor.getValue()});

        rewriter.replaceOp(op, call.getResult());
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

    bool isAny = mlir::isa<IR::AnyType>(origDstType);
    bool wasAny = mlir::isa<IR::AnyType>(origSrcType);

    // 1. Boxing (to Any Pointer)
    if (isAny && !wasAny) {
      mlir::Type llvmPtrTy =
          mlir::LLVM::LLVMPointerType::get(rewriter.getContext());
      mlir::Type i32Ty = rewriter.getI32Type();
      mlir::Type i64Ty = rewriter.getI64Type();

      // Step A: Calculate the size of the primitive type (sizeof trick)
      mlir::Value nullPtr =
          rewriter.create<mlir::LLVM::ZeroOp>(op.getLoc(), llvmPtrTy);
      mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), i32Ty, rewriter.getI32IntegerAttr(1));
      mlir::Value sizeGep = rewriter.create<mlir::LLVM::GEPOp>(
          op.getLoc(), llvmPtrTy, srcType, nullPtr,
          llvm::ArrayRef<mlir::LLVM::GEPArg>{one});
      mlir::Value sizeInt =
          rewriter.create<mlir::LLVM::PtrToIntOp>(op.getLoc(), i64Ty, sizeGep);

      // Step B: Get Type ID
      uint32_t typeId = getMokshaTypeID(origSrcType);
      mlir::Value typeTag = rewriter.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), i32Ty, rewriter.getI32IntegerAttr(typeId));

      // Step C: Allocate on HEAP using moksha_rt_alloc(size_t, uint32_t)
      mlir::Value heapPtr =
          createRuntimeCall(rewriter, op.getLoc(), llvmPtrTy, "moksha_rt_alloc",
                            {sizeInt, typeTag});

      // Step D: Store value directly into the heap pointer!
      auto storeOp = rewriter.create<mlir::LLVM::StoreOp>(
          op.getLoc(), adaptor.getValue(), heapPtr);
      storeOp.setAlignment(8);

      rewriter.replaceOp(op, heapPtr);
      return mlir::success();
    }

    // 2. Unboxing (from Any Pointer)
    if (wasAny && !isAny) {
      mlir::Value dataPtr = adaptor.getValue();
      if (dataPtr.getType() != mlir::LLVM::LLVMPointerType::get(getContext())) {
        dataPtr = rewriter.create<mlir::LLVM::BitcastOp>(
            op.getLoc(), mlir::LLVM::LLVMPointerType::get(getContext()),
            dataPtr);
      }
      mlir::Value loaded =
          rewriter.create<mlir::LLVM::LoadOp>(op.getLoc(), dstType, dataPtr);
      rewriter.replaceOp(op, loaded);
      return mlir::success();
    }

    // 3. Boxing to Promise Pointer
    bool isPromise = mlir::isa<IR::PromiseType>(origDstType);
    bool wasPromise = mlir::isa<IR::PromiseType>(origSrcType);

    if (isPromise && !wasPromise) {
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      mlir::Value val = adaptor.getValue();

      // Ensure value is an opaque pointer for the C runtime
      if (!mlir::isa<mlir::LLVM::LLVMPointerType>(val.getType())) {
        if (val.getType().isIntOrIndex()) {
          mlir::Value ext = rewriter.create<mlir::LLVM::ZExtOp>(
              op.getLoc(), rewriter.getI64Type(), val);
          val = rewriter.create<mlir::LLVM::IntToPtrOp>(op.getLoc(), llvmPtrTy,
                                                        ext);
        } else {
          mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
              op.getLoc(), rewriter.getI32Type(),
              rewriter.getI32IntegerAttr(1));
          mlir::Value alloca = rewriter.create<mlir::LLVM::AllocaOp>(
              op.getLoc(), llvmPtrTy, val.getType(), one);
          rewriter.create<mlir::LLVM::StoreOp>(op.getLoc(), val, alloca);
          val = alloca;
        }
      } else if (val.getType() != llvmPtrTy) {
        val =
            rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(), llvmPtrTy, val);
      }

      // Call the new runtime allocator
      mlir::Value promisePtr =
          createRuntimeCall(rewriter, op.getLoc(), llvmPtrTy,
                            "moksha_rt_make_resolved_promise", {val});

      if (promisePtr.getType() != dstType) {
        promisePtr = rewriter.create<mlir::LLVM::BitcastOp>(
            op.getLoc(), dstType, promisePtr);
      }
      rewriter.replaceOp(op, promisePtr);
      return mlir::success();
    }

    // 4. Pointer -> Array Value (Dereference / Load)
    if (mlir::isa<mlir::LLVM::LLVMPointerType>(srcType) &&
        mlir::isa<mlir::LLVM::LLVMArrayType>(dstType)) {
      rewriter.replaceOpWithNewOp<mlir::LLVM::LoadOp>(op, dstType,
                                                      adaptor.getValue());
      return mlir::success();
    }

    // 5. Array Value -> Pointer (Decay / Spill to stack)
    if (mlir::isa<mlir::LLVM::LLVMArrayType>(srcType) &&
        mlir::isa<mlir::LLVM::LLVMPointerType>(dstType)) {
      mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));
      mlir::Value allocaPtr = rewriter.create<mlir::LLVM::AllocaOp>(
          op.getLoc(), dstType, srcType, one);
      rewriter.create<mlir::LLVM::StoreOp>(op.getLoc(), adaptor.getValue(),
                                           allocaPtr);
      rewriter.replaceOp(op, allocaPtr);
      return mlir::success();
    }

    // 6. Slice Array Decay (Struct -> Ptr)
    if (mlir::isa<mlir::LLVM::LLVMStructType>(srcType) &&
        mlir::isa<mlir::LLVM::LLVMPointerType>(dstType)) {

      auto structTy = mlir::cast<mlir::LLVM::LLVMStructType>(srcType);
      if (structTy.getBody().size() == 2 &&
          mlir::isa<mlir::LLVM::LLVMPointerType>(structTy.getBody()[0])) {

        mlir::Value dataPtr = rewriter.create<mlir::LLVM::ExtractValueOp>(
            op.getLoc(), adaptor.getValue(), llvm::ArrayRef<int64_t>({0}));

        if (dataPtr.getType() != dstType) {
          dataPtr = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(), dstType,
                                                           dataPtr);
        }
        rewriter.replaceOp(op, dataPtr);
        return mlir::success();
      }
    }

    // 7. Raw Pointer to Slice Struct (Ptr -> Struct)
    if (mlir::isa<mlir::LLVM::LLVMPointerType>(srcType) &&
        mlir::isa<mlir::LLVM::LLVMStructType>(dstType)) {
      auto structTy = mlir::cast<mlir::LLVM::LLVMStructType>(dstType);
      if (structTy.getBody().size() == 2 &&
          mlir::isa<mlir::LLVM::LLVMPointerType>(structTy.getBody()[0])) {
        mlir::Value slice =
            rewriter.create<mlir::LLVM::UndefOp>(op.getLoc(), dstType);
        mlir::Value ptr = adaptor.getValue();

        // === BUGSQUASH: Extract the true array size from the Moksha type! ===
        int64_t actualSize = 0;
        mlir::Type elemType = nullptr;
        if (auto ptrTy = mlir::dyn_cast<IR::PointerType>(origSrcType)) {
          if (auto arrTy = mlir::dyn_cast<IR::ArrayType>(ptrTy.getPointee())) {
            actualSize = arrTy.getSize();
            elemType = typeConverter->convertType(arrTy.getElementType());
          }
        } else if (auto arrTy = mlir::dyn_cast<IR::ArrayType>(origSrcType)) {
          actualSize = arrTy.getSize();
          elemType = typeConverter->convertType(arrTy.getElementType());
        }

        mlir::Value dataPtr = ptr;

        // Heap-allocate the array before assigning it to the slice
        if (elemType) {
          mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
          mlir::Type i64Ty = rewriter.getI64Type();
          mlir::Type i32Ty = rewriter.getI32Type();

          // 1. Calculate bytes to allocate via GEP trick
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

          // 2. Call moksha_rt_alloc (Type 20 = MOKSHA_TYPE_ARRAY)
          mlir::Value typeTag = rewriter.create<mlir::LLVM::ConstantOp>(
              op.getLoc(), i32Ty, rewriter.getI32IntegerAttr(20));
          mlir::Value heapPtr =
              createRuntimeCall(rewriter, op.getLoc(), llvmPtrTy,
                                "moksha_rt_alloc", {totalBytes, typeTag});

          // 3. memcpy from stack array (ptr) to heap array (heapPtr)
          mlir::Value srcVoid = ptr;
          if (srcVoid.getType() != llvmPtrTy) {
            srcVoid = rewriter.create<mlir::LLVM::BitcastOp>(
                op.getLoc(), llvmPtrTy, srcVoid);
          }
          createRuntimeCall(rewriter, op.getLoc(), llvmPtrTy, "memcpy",
                            {heapPtr, srcVoid, totalBytes});

          // 4. Update dataPtr so the slice owns the heap memory
          dataPtr = heapPtr;
        }

        if (dataPtr.getType() != structTy.getBody()[0]) {
          dataPtr = rewriter.create<mlir::LLVM::BitcastOp>(
              op.getLoc(), structTy.getBody()[0], dataPtr);
        }

        // Insert data pointer
        slice = rewriter.create<mlir::LLVM::InsertValueOp>(
            op.getLoc(), slice, dataPtr, llvm::ArrayRef<int64_t>({0}));

        mlir::Type lenTy = structTy.getBody()[1];
        mlir::Value trueLen = rewriter.create<mlir::LLVM::ConstantOp>(
            op.getLoc(), lenTy, rewriter.getIntegerAttr(lenTy, actualSize));

        // Insert true length
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
        rewriter.replaceOpWithNewOp<mlir::LLVM::SExtOp>(op, dstType,
                                                        adaptor.getValue());
      } else {
        rewriter.replaceOp(op, adaptor.getValue()); // No-op if same size
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
      if (srcW < dstW) {
        rewriter.replaceOpWithNewOp<mlir::LLVM::FPExtOp>(op, dstType,
                                                         adaptor.getValue());
      } else if (srcW > dstW) {
        rewriter.replaceOpWithNewOp<mlir::LLVM::FPTruncOp>(op, dstType,
                                                           adaptor.getValue());
      } else {
        rewriter.replaceOp(op, adaptor.getValue()); // No-op if same size
      }
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

    // 10. Pointer <-> Pointer
    if (mlir::isa<mlir::LLVM::LLVMPointerType>(srcType) &&
        mlir::isa<mlir::LLVM::LLVMPointerType>(dstType)) {
      if (srcType != dstType) {
        rewriter.replaceOpWithNewOp<mlir::LLVM::BitcastOp>(op, dstType,
                                                           adaptor.getValue());
      } else {
        rewriter.replaceOp(op, adaptor.getValue());
      }
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
      }
      // Standard Primitive/Pointer BitCast
      else {
        if (mlir::isa<mlir::LLVM::LLVMPointerType>(srcType) &&
            mlir::isa<mlir::LLVM::LLVMFunctionType>(dstType)) {
          rewriter.replaceOp(op, adaptor.getValue());
        } else {
          rewriter.replaceOpWithNewOp<mlir::LLVM::BitcastOp>(
              op, dstType, adaptor.getValue());
        }
      }
    } else {
      rewriter.replaceOp(op, adaptor.getValue());
    }
    return mlir::success();
  }
};

struct AddressOfOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::AddressOfOp> {
  using ConvertOpToLLVMPattern<IR::AddressOfOp>::ConvertOpToLLVMPattern;
  mlir::LogicalResult
  matchAndRewrite(IR::AddressOfOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    rewriter.replaceOpWithNewOp<mlir::LLVM::AddressOfOp>(
        op, typeConverter->convertType(op.getType()), op.getGlobalName());
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

        llvm::StringRef rtFunc = "__moksha_dec_add"; // Fallback
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

    // --- [CRITICAL FIX] LLVM 22 ConstantFold Crash Bypass ---
    // Pre-fold constants here so the LLVM translator never sees a dual-constant
    // math operation!
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
                  return mlir::failure(); // Let runtime handle Div By Zero
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
    // --------------------------------------------------------

    if (mlir::isa<mlir::FloatType>(resTy)) {
      unsigned width = resTy.getIntOrFloatBitWidth();
      if (width < 32) {
        mlir::Type f32Ty = rewriter.getF32Type();

        // [FIX] Use the safe MLIR upcaster instead of generating FPExtOps
        // directly
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

    // --- [FIX] Intercept Decimal Comparisons ---
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
      mlir::Value lhs = adaptor.getLhs();
      mlir::Value rhs = adaptor.getRhs();
      unsigned width = ty.getIntOrFloatBitWidth();

      // Upcast f8/f16 to f32 before comparison
      if (width < 32) {
        mlir::Type f32Ty = rewriter.getF32Type();

        // [FIX] Use the safe MLIR upcaster
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
    } else {
      bool isUnsigned = ty.isUnsignedInteger();
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

// Runtime Hooks (ARC, Spawn, Await)
struct RetainOpLowering : public mlir::ConvertOpToLLVMPattern<IR::RetainOp> {
  using ConvertOpToLLVMPattern<IR::RetainOp>::ConvertOpToLLVMPattern;
  mlir::LogicalResult
  matchAndRewrite(IR::RetainOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Value val = adaptor.getValue();

    // Standardize argument to a raw pointer
    if (mlir::isa<mlir::LLVM::LLVMStructType>(val.getType())) {
      val = rewriter.create<mlir::LLVM::ExtractValueOp>(
          op.getLoc(), val, llvm::ArrayRef<int64_t>{0});
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

    // Standardize argument to a raw pointer
    if (mlir::isa<mlir::LLVM::LLVMStructType>(val.getType())) {
      val = rewriter.create<mlir::LLVM::ExtractValueOp>(
          loc, val, llvm::ArrayRef<int64_t>{0});
    }

    // 1. Resolve the destructor function pointer
    mlir::Value dropFuncPtr;
    if (auto dropSym = op.getDropFuncAttr()) {
      // If a destructor exists, get its memory address
      dropFuncPtr =
          rewriter.create<mlir::LLVM::AddressOfOp>(loc, llvmPtrTy, dropSym);
    } else {
      // Otherwise, pass a null pointer
      dropFuncPtr = rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmPtrTy);
    }

    // 2. Call the runtime with TWO arguments (object, destructor)
    createRuntimeCall(rewriter, loc, "moksha_rt_release_with_dtor", {},
                      {val, dropFuncPtr});

    rewriter.eraseOp(op);
    return mlir::success();
  }
};

// ============================================================================
// Weak ARC Lowering
// ============================================================================
struct StoreWeakOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::StoreWeakOp> {
  using ConvertOpToLLVMPattern<IR::StoreWeakOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::StoreWeakOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
    mlir::Value ptr = adaptor.getPtr();
    mlir::Value val = adaptor.getValue();

    // Standardize to raw opaque pointers for the C runtime API (void**, void*)
    if (ptr.getType() != llvmPtrTy) {
      ptr = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(), llvmPtrTy, ptr);
    }
    if (val.getType() != llvmPtrTy) {
      val = rewriter.create<mlir::LLVM::BitcastOp>(op.getLoc(), llvmPtrTy, val);
    }

    // Call: void moksha_rt_store_weak(void **dest, void *obj)
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

    // Call: void* moksha_rt_load_weak(void **src)
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

// ============================================================================
// Throw Op Lowering (Exception Control Flow)
// ============================================================================
struct ThrowOpLowering : public mlir::ConvertOpToLLVMPattern<IR::ThrowOp> {
  using ConvertOpToLLVMPattern<IR::ThrowOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::ThrowOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    // If the throw has a successor, branch directly to it.
    if (op->getNumSuccessors() > 0) {
      rewriter.replaceOpWithNewOp<mlir::LLVM::BrOp>(op, mlir::ValueRange{},
                                                    op->getSuccessor(0));
    } else {
      // If there is no local catch block, it's an unhandled exception.
      rewriter.replaceOpWithNewOp<mlir::LLVM::UnreachableOp>(op);
    }
    return mlir::success();
  }
};

struct LandingPadOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::LandingPadOp> {
  using ConvertOpToLLVMPattern<IR::LandingPadOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::LandingPadOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    mlir::Type structTy = mlir::LLVM::LLVMStructType::getLiteral(
        getContext(), {mlir::LLVM::LLVMPointerType::get(getContext()),
                       rewriter.getI32Type()});

    mlir::OperationState lpState(op.getLoc(), "llvm.landingpad");
    lpState.addTypes(structTy);
    // --- [FIX] Use UnitAttr instead of BoolAttr! ---
    lpState.addAttribute("cleanup", rewriter.getUnitAttr());
    mlir::Operation *landingPad = rewriter.create(lpState);

    // Extract the pointer payload
    mlir::Type expectedTy = typeConverter->convertType(op.getType());
    if (mlir::isa<mlir::LLVM::LLVMPointerType>(expectedTy)) {
      mlir::Value ptr = rewriter.create<mlir::LLVM::ExtractValueOp>(
          op.getLoc(), landingPad->getResult(0), llvm::ArrayRef<int64_t>{0});
      rewriter.replaceOp(op, ptr);
    } else {
      rewriter.replaceOp(op, landingPad->getResult(0));
    }
    return mlir::success();
  }
};

// ============================================================================
// Closure Lowering
// ============================================================================
struct MakeClosureOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::MakeClosureOp> {
  using ConvertOpToLLVMPattern<IR::MakeClosureOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::MakeClosureOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
    mlir::Type expectedRetTy = typeConverter->convertType(op.getType());

    // 1. Create an empty struct to hold the fat pointer
    mlir::Value closureStruct =
        rewriter.create<mlir::LLVM::UndefOp>(loc, expectedRetTy);

    // 2. Extract Function Pointer
    mlir::Value fnPtr = rewriter.create<mlir::LLVM::AddressOfOp>(
        loc, llvmPtrTy, op.getCalleeAttr());

    // 3. Insert Function Pointer at index 0
    closureStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
        loc, closureStruct, fnPtr, llvm::ArrayRef<int64_t>({0}));

    // 4. Extract Environment Pointer
    mlir::Value envPtr;
    auto captures = adaptor.getCaptures();
    if (captures.empty()) {
      envPtr = rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmPtrTy);
    } else {
      envPtr = captures[0]; // The first capture is the Env pointer
      if (envPtr.getType() != llvmPtrTy) {
        envPtr = rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy, envPtr);
      }
    }

    // 5. Insert Environment Pointer at index 1
    closureStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
        loc, closureStruct, envPtr, llvm::ArrayRef<int64_t>({1}));

    // 6. Return the assembled struct by value!
    rewriter.replaceOp(op, closureStruct);
    return mlir::success();
  }
};

// ============================================================================
// Variadic Call Lowering
// ============================================================================
struct CustomCallOpLowering
    : public mlir::ConvertOpToLLVMPattern<mlir::func::CallOp> {
  using ConvertOpToLLVMPattern<mlir::func::CallOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(mlir::func::CallOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    llvm::StringRef callee = op.getCallee();

    // === PREVENT STACK-FREE CRASHES ===
    if (callee == "__moksha_free") {
      auto loc = op.getLoc();
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      mlir::Value ptrToFree = adaptor.getOperands()[0];

      // Ensure it's an opaque pointer for the runtime
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

    // === DYNAMIC ARRAY SPREAD / ALLOC INTERCEPTION ===
    if (callee == "__moksha_alloc") {
      auto loc = op.getLoc();
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      mlir::Type i32Ty = rewriter.getI32Type();

      // 1. Zero-extend the 32-bit size to 64-bit size_t
      mlir::Value sizeInt32 = adaptor.getOperands()[0];
      mlir::Value sizeInt64 = rewriter.create<mlir::LLVM::ZExtOp>(
          loc, rewriter.getI64Type(), sizeInt32);

      // 2. We MUST use moksha_rt_alloc so the array gets a valid ARC header!
      // Type 20 represents MOKSHA_TYPE_ARRAY (or generic heap object)
      mlir::Value typeTag = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, i32Ty, rewriter.getI32IntegerAttr(20));

      mlir::Value ptr = createRuntimeCall(
          rewriter, loc, llvmPtrTy, "moksha_rt_alloc", {sizeInt64, typeTag});
      rewriter.replaceOp(op, ptr);
      return mlir::success();
    }

    if (callee == "__moksha_array_copy") {
      auto loc = op.getLoc();
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());

      mlir::Value dest = adaptor.getOperands()[0];
      mlir::Value src = adaptor.getOperands()[1];
      mlir::Value sizeInt32 = adaptor.getOperands()[2];

      // Zero-extend size to size_t
      mlir::Value sizeInt64 = rewriter.create<mlir::LLVM::ZExtOp>(
          loc, rewriter.getI64Type(), sizeInt32);

      // Route directly to standard C 'memcpy'.
      // memcpy returns void*, so we expect a ptr back but ignore the result.
      createRuntimeCall(rewriter, loc, llvmPtrTy, "memcpy",
                        {dest, src, sizeInt64});

      rewriter.eraseOp(op);
      return mlir::success();
    }

    // === DYNAMIC MAP INTERCEPTION ===
    if (callee == "__moksha_map_insert" || callee == "__moksha_map_get") {
      auto loc = op.getLoc();
      mlir::Type llvmPtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
      mlir::Type i32Ty = rewriter.getI32Type();
      mlir::Type i64Ty = rewriter.getI64Type();

      // Helper to dynamically heap-allocate and tag primitive values
      auto boxArg = [&](mlir::Value val, mlir::Type origTy) -> mlir::Value {
        if (mlir::isa<IR::AnyType>(origTy))
          return val; // Already boxed!

        mlir::Value nullPtr =
            rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmPtrTy);
        mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
            loc, i32Ty, rewriter.getI32IntegerAttr(1));
        mlir::Value sizeGep = rewriter.create<mlir::LLVM::GEPOp>(
            loc, llvmPtrTy, val.getType(), nullPtr,
            llvm::ArrayRef<mlir::LLVM::GEPArg>{one});
        mlir::Value sizeInt =
            rewriter.create<mlir::LLVM::PtrToIntOp>(loc, i64Ty, sizeGep);

        uint32_t typeId = getMokshaTypeID(origTy);
        mlir::Value typeTag = rewriter.create<mlir::LLVM::ConstantOp>(
            loc, i32Ty, rewriter.getI32IntegerAttr(typeId));

        mlir::Value heapPtr = createRuntimeCall(
            rewriter, loc, llvmPtrTy, "moksha_rt_alloc", {sizeInt, typeTag});
        auto storeOp = rewriter.create<mlir::LLVM::StoreOp>(loc, val, heapPtr);
        storeOp.setAlignment(8);

        return heapPtr;
      };

      mlir::Value mapPtr = adaptor.getOperands()[0];
      if (mapPtr.getType() != llvmPtrTy) {
        mapPtr = rewriter.create<mlir::LLVM::BitcastOp>(loc, llvmPtrTy, mapPtr);
      }

      mlir::Value boxedKey =
          boxArg(adaptor.getOperands()[1], op.getOperand(1).getType());

      if (callee == "__moksha_map_insert") {
        mlir::Value boxedVal =
            boxArg(adaptor.getOperands()[2], op.getOperand(2).getType());

        // [FIX] Call the renamed C runtime function
        createRuntimeCall(rewriter, loc, "moksha_rt_map_insert",
                          mlir::TypeRange{}, {mapPtr, boxedKey, boxedVal});

        rewriter.eraseOp(op);
        return mlir::success();
      } else {
        // Map Get
        // [FIX] Call the renamed C runtime function
        mlir::Value retAny = createRuntimeCall(
            rewriter, loc, llvmPtrTy, "moksha_rt_map_get", {mapPtr, boxedKey});
        mlir::Type expectedRetTy =
            typeConverter->convertType(op.getResultTypes()[0]);

        // Unbox the payload directly out of the returned ARC Any pointer
        mlir::Value unboxed =
            rewriter.create<mlir::LLVM::LoadOp>(loc, expectedRetTy, retAny);
        rewriter.replaceOp(op, unboxed);
        return mlir::success();
      }
    }

    llvm::SmallVector<mlir::Type, 1> resultTypes;
    if (mlir::failed(
            typeConverter->convertTypes(op.getResultTypes(), resultTypes)))
      return mlir::failure();

    auto llvmCall = rewriter.create<mlir::LLVM::CallOp>(
        op.getLoc(), resultTypes, op.getCalleeAttr(), adaptor.getOperands());

    auto moduleOp = op->getParentOfType<mlir::ModuleOp>();
    auto funcOp = moduleOp.lookupSymbol<mlir::func::FuncOp>(op.getCallee());
    auto llvmFuncOp =
        moduleOp.lookupSymbol<mlir::LLVM::LLVMFuncOp>(op.getCallee());

    bool isVarArg = false;
    mlir::Type retTy;
    llvm::SmallVector<mlir::Type, 4> argTypes;

    if (funcOp) {
      auto fnTy = funcOp.getFunctionType();
      isVarArg = funcOp->hasAttr("func.varargs");
      for (auto ty : fnTy.getInputs())
        argTypes.push_back(typeConverter->convertType(ty));
      retTy = fnTy.getNumResults() == 0
                  ? mlir::LLVM::LLVMVoidType::get(getContext())
                  : typeConverter->convertType(fnTy.getResult(0));
    } else if (llvmFuncOp) {
      auto fnTy = llvmFuncOp.getFunctionType();
      isVarArg = fnTy.isVarArg();
      for (auto ty : fnTy.getParams())
        argTypes.push_back(ty);
      retTy = fnTy.getReturnType();
    } else {
      for (auto val : adaptor.getOperands())
        argTypes.push_back(val.getType());
      retTy = resultTypes.empty() ? mlir::LLVM::LLVMVoidType::get(getContext())
                                  : resultTypes[0];
    }

    auto llvmFnTy =
        mlir::LLVM::LLVMFunctionType::get(retTy, argTypes, isVarArg);

    // [CRITICAL FIX] MLIR 22 requires BOTH properties for variadic calls!
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

// Lower 'moksha.spawn' -> call @moksha_rt_spawn_thread(closure_ptr)
struct SpawnOpLowering : public mlir::ConvertOpToLLVMPattern<IR::SpawnOp> {
  using ConvertOpToLLVMPattern<IR::SpawnOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::SpawnOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    mlir::Type opaquePtrTy = mlir::LLVM::LLVMPointerType::get(getContext());
    mlir::Value closurePtr = adaptor.getClosure();

    // If the closure is passed by value (struct), push it to the stack to get a
    // pointer
    if (!mlir::isa<mlir::LLVM::LLVMPointerType>(closurePtr.getType())) {
      // [FIX] Heap-allocate closures for OS threads to prevent
      // stack-use-after-free!
      mlir::Type i32Ty = rewriter.getI32Type();
      mlir::Type i64Ty = rewriter.getI64Type();

      // 16 bytes = 2 pointers (function_ptr + environment_ptr)
      mlir::Value size = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, i64Ty, rewriter.getI64IntegerAttr(16));
      mlir::Value typeTag = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, i32Ty, rewriter.getI32IntegerAttr(20));

      mlir::Value heapAlloc = createRuntimeCall(
          rewriter, loc, opaquePtrTy, "moksha_rt_alloc", {size, typeTag});

      mlir::Value castedAlloc = rewriter.create<mlir::LLVM::BitcastOp>(
          loc, mlir::LLVM::LLVMPointerType::get(getContext()), heapAlloc);
      auto storeOp =
          rewriter.create<mlir::LLVM::StoreOp>(loc, closurePtr, castedAlloc);
      storeOp.setAlignment(8);

      closurePtr = castedAlloc;
    }

    // Determine if it's a weak (detached) thread
    bool isWeak = false;
    if (auto weakAttr = op->getAttrOfType<mlir::BoolAttr>("is_weak")) {
      isWeak = weakAttr.getValue();
    } else if (op->hasAttr("is_weak")) {
      isWeak = true;
    }

    llvm::StringRef rtFunc =
        isWeak ? "moksha_rt_spawn_weak_thread" : "moksha_rt_spawn_thread";

    // Call the C runtime thread spawner, returning a Thread Handle (Promise)
    mlir::Value threadHandle =
        createRuntimeCall(rewriter, loc, opaquePtrTy, rtFunc, {closurePtr});

    rewriter.replaceOp(op, threadHandle);
    return mlir::success();
  }
};

// Lower 'moksha.await' -> call @llvm.coro.suspend state machine
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

    if (funcOp.getName() == "main") {
      if (!module.lookupSymbol("moksha_rt_block_on")) {
        mlir::OpBuilder::InsertionGuard guard(rewriter);
        rewriter.setInsertionPointToStart(module.getBody());
        rewriter.create<mlir::LLVM::LLVMFuncOp>(
            loc, "moksha_rt_block_on",
            mlir::LLVM::LLVMFunctionType::get(llvmI8PtrTy, {llvmI8PtrTy}));
      }

      mlir::Value blockArgs[] = {adaptor.getPromise()};
      auto blockCall = rewriter.create<mlir::LLVM::CallOp>(
          loc, llvmI8PtrTy,
          mlir::SymbolRefAttr::get(getContext(), "moksha_rt_block_on"),
          mlir::ValueRange(llvm::ArrayRef<mlir::Value>(blockArgs)));

      mlir::Type expectedTy = typeConverter->convertType(op.getType());
      if (!expectedTy || mlir::isa<mlir::LLVM::LLVMVoidType>(expectedTy)) {
        rewriter.eraseOp(op);
      } else {
        if (expectedTy.isIntOrIndex()) {
          auto intVal = rewriter.create<mlir::LLVM::PtrToIntOp>(
              loc, rewriter.getI64Type(), blockCall.getResult());
          auto truncVal =
              rewriter.create<mlir::LLVM::TruncOp>(loc, expectedTy, intVal);
          rewriter.replaceOp(op, truncVal);
        } else if (mlir::isa<mlir::LLVM::LLVMPointerType>(expectedTy)) {
          auto casted = rewriter.create<mlir::LLVM::BitcastOp>(
              loc, expectedTy, blockCall.getResult());
          rewriter.replaceOp(op, casted);
        } else {
          auto loaded = rewriter.create<mlir::LLVM::LoadOp>(
              loc, expectedTy, blockCall.getResult());
          rewriter.replaceOp(op, loaded);
        }
      }
      return mlir::success();
    }

    // 1. Declare Intrinsics & Runtime Hooks (For normal Coroutines)
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
    if (!module.lookupSymbol("moksha_rt_register_await")) {
      mlir::OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());
      rewriter.create<mlir::LLVM::LLVMFuncOp>(
          loc, "moksha_rt_register_await",
          mlir::LLVM::LLVMFunctionType::get(
              mlir::LLVM::LLVMVoidType::get(getContext()),
              {llvmI8PtrTy, llvmI8PtrTy}));
    }
    if (!module.lookupSymbol("moksha_rt_await_payload")) {
      mlir::OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());
      rewriter.create<mlir::LLVM::LLVMFuncOp>(
          loc, "moksha_rt_await_payload",
          mlir::LLVM::LLVMFunctionType::get(llvmI8PtrTy, {llvmI8PtrTy}));
    }

    // 2. Register the Await
    auto nullHandle = rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmI8PtrTy);

    mlir::Value registerArgs[] = {adaptor.getPromise(), nullHandle.getResult()};
    rewriter.create<mlir::LLVM::CallOp>(
        loc, mlir::TypeRange{},
        mlir::SymbolRefAttr::get(getContext(), "moksha_rt_register_await"),
        mlir::ValueRange(llvm::ArrayRef<mlir::Value>(registerArgs)));

    // 3. Suspend State Machine
    mlir::Value saveArgs[] = {nullHandle.getResult()};
    auto saveFnTy =
        mlir::LLVM::LLVMFunctionType::get(llvmTokenTy, {llvmI8PtrTy});
    auto coroSave = rewriter.create<mlir::LLVM::CallOp>(
        loc, llvmTokenTy,
        mlir::SymbolRefAttr::get(getContext(), "llvm.coro.save"),
        mlir::ValueRange(llvm::ArrayRef<mlir::Value>(saveArgs)));
    coroSave->setAttr("callee_type", mlir::TypeAttr::get(saveFnTy));

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

    // 4. Branching
    auto currentBlock = rewriter.getInsertionBlock();
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

    // 5. Suspend Block
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

    // 6. Destroy Block
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

    // 7. Resume Block
    rewriter.setInsertionPointToStart(resumeBlock);
    mlir::Type expectedTy = typeConverter->convertType(op.getType());

    if (!expectedTy || mlir::isa<mlir::LLVM::LLVMVoidType>(expectedTy)) {
      rewriter.eraseOp(op);
    } else {
      mlir::Value payloadArgs[] = {adaptor.getPromise()};
      auto payloadPtr = rewriter.create<mlir::LLVM::CallOp>(
          loc, llvmI8PtrTy,
          mlir::SymbolRefAttr::get(getContext(), "moksha_rt_await_payload"),
          mlir::ValueRange(llvm::ArrayRef<mlir::Value>(payloadArgs)));

      if (expectedTy.isIntOrIndex()) {
        auto intVal = rewriter.create<mlir::LLVM::PtrToIntOp>(
            loc, rewriter.getI64Type(), payloadPtr.getResult());
        auto truncVal =
            rewriter.create<mlir::LLVM::TruncOp>(loc, expectedTy, intVal);
        rewriter.replaceOp(op, truncVal);
      } else if (mlir::isa<mlir::LLVM::LLVMPointerType>(expectedTy)) {
        auto casted = rewriter.create<mlir::LLVM::BitcastOp>(
            loc, expectedTy, payloadPtr.getResult());
        rewriter.replaceOp(op, casted);
      } else {
        auto loaded = rewriter.create<mlir::LLVM::LoadOp>(
            loc, expectedTy, payloadPtr.getResult());
        rewriter.replaceOp(op, loaded);
      }
    }

    return mlir::success();
  }
};

struct InvokeIndirectOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::InvokeIndirectOp> {
  using ConvertOpToLLVMPattern<IR::InvokeIndirectOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::InvokeIndirectOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    llvm::SmallVector<mlir::Type, 1> resultTypes;
    if (mlir::failed(
            typeConverter->convertTypes(op.getResultTypes(), resultTypes)))
      return mlir::failure();

    // --- [CRITICAL FIX] Inject missing LandingPad and Resume ---
    mlir::Block *unwindDest = op.getUnwindDest();
    if (unwindDest && !unwindDest->empty()) {
      mlir::Operation &firstOp = unwindDest->front();
      if (firstOp.getName().getStringRef() != "llvm.landingpad") {
        mlir::OpBuilder::InsertionGuard guard(rewriter);

        // 1. Inject LandingPad at the top
        rewriter.setInsertionPointToStart(unwindDest);
        mlir::Type structTy = mlir::LLVM::LLVMStructType::getLiteral(
            getContext(), {mlir::LLVM::LLVMPointerType::get(getContext()),
                           rewriter.getI32Type()});
        mlir::OperationState lpState(op.getLoc(), "llvm.landingpad");
        lpState.addTypes(structTy);
        lpState.addAttribute("cleanup", rewriter.getUnitAttr());
        mlir::Operation *landingPad = rewriter.create(lpState);

        // 2. Check if the block is missing a terminator, and inject Resume!
        mlir::Operation &lastOp = unwindDest->back();
        if (!lastOp.hasTrait<mlir::OpTrait::IsTerminator>()) {
          rewriter.setInsertionPointToEnd(unwindDest);
          mlir::OperationState resumeState(op.getLoc(), "llvm.resume");
          resumeState.addOperands(landingPad->getResult(0));
          rewriter.create(resumeState);
        }
      }
    }

    mlir::OperationState state(op.getLoc(),
                               mlir::LLVM::InvokeOp::getOperationName());
    state.addTypes(resultTypes);
    state.addOperands(adaptor.getCallee());
    state.addOperands(adaptor.getCallArgs());
    state.addSuccessors(op.getNormalDest());
    state.addSuccessors(unwindDest);

    mlir::Operation *llvmInvoke = rewriter.create(state);
    rewriter.replaceOp(op, llvmInvoke->getResults());

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

    mlir::Block *unwindDest = op.getUnwindDest();
    if (unwindDest && !unwindDest->empty()) {
      mlir::Operation &firstOp = unwindDest->front();
      if (firstOp.getName().getStringRef() != "llvm.landingpad") {
        mlir::OpBuilder::InsertionGuard guard(rewriter);

        rewriter.setInsertionPointToStart(unwindDest);
        mlir::Type ptrTy = mlir::LLVM::LLVMPointerType::get(getContext());
        mlir::Type structTy = mlir::LLVM::LLVMStructType::getLiteral(
            getContext(), {ptrTy, rewriter.getI32Type()});

        mlir::OperationState lpState(op.getLoc(), "llvm.landingpad");
        lpState.addTypes(structTy);
        lpState.addAttribute("cleanup", rewriter.getUnitAttr());
        mlir::Operation *landingPad = rewriter.create(lpState);

        mlir::Operation &lastOp = unwindDest->back();
        if (!lastOp.hasTrait<mlir::OpTrait::IsTerminator>()) {
          rewriter.setInsertionPointToEnd(unwindDest);
          mlir::OperationState resumeState(op.getLoc(), "llvm.resume");
          resumeState.addOperands(landingPad->getResult(0));
          rewriter.create(resumeState);
        }
      }
    }

    auto llvmInvoke = rewriter.create<mlir::LLVM::InvokeOp>(
        op.getLoc(), resultTypes, op.getCalleeAttr(), adaptor.getOperands(),
        op.getNormalDest(), mlir::ValueRange{}, unwindDest, mlir::ValueRange{});

    auto moduleOp = op->getParentOfType<mlir::ModuleOp>();
    auto funcOp = moduleOp.lookupSymbol<mlir::func::FuncOp>(op.getCallee());
    auto llvmFuncOp =
        moduleOp.lookupSymbol<mlir::LLVM::LLVMFuncOp>(op.getCallee());

    bool isVarArg = false;
    mlir::Type retTy;
    llvm::SmallVector<mlir::Type, 4> argTypes;

    if (funcOp) {
      auto fnTy = funcOp.getFunctionType();
      isVarArg = funcOp->hasAttr("func.varargs");
      for (auto ty : fnTy.getInputs())
        argTypes.push_back(typeConverter->convertType(ty));
      retTy = fnTy.getNumResults() == 0
                  ? mlir::LLVM::LLVMVoidType::get(getContext())
                  : typeConverter->convertType(fnTy.getResult(0));
    } else if (llvmFuncOp) {
      auto fnTy = llvmFuncOp.getFunctionType();
      isVarArg = fnTy.isVarArg();
      for (auto ty : fnTy.getParams())
        argTypes.push_back(ty);
      retTy = fnTy.getReturnType();
    } else {
      for (auto val : adaptor.getOperands())
        argTypes.push_back(val.getType());
      retTy = resultTypes.empty() ? mlir::LLVM::LLVMVoidType::get(getContext())
                                  : resultTypes[0];
    }

    auto llvmFnTy =
        mlir::LLVM::LLVMFunctionType::get(retTy, argTypes, isVarArg);

    // [CRITICAL FIX] MLIR 22 requires BOTH properties for variadic calls!
    llvmInvoke->setAttr("callee_type", mlir::TypeAttr::get(llvmFnTy));
    if (isVarArg) {
      llvmInvoke->setAttr("var_callee_type", mlir::TypeAttr::get(llvmFnTy));
    }

    rewriter.replaceOp(op, llvmInvoke.getResults());
    return mlir::success();
  }
};

// ============================================================================
// Exponentiation Lowering
// ============================================================================
struct PowOpLowering : public mlir::ConvertOpToLLVMPattern<IR::PowOp> {
  using ConvertOpToLLVMPattern<IR::PowOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::PowOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Type resTy = typeConverter->convertType(op.getType());
    unsigned width = resTy.getIntOrFloatBitWidth();

    // Dynamically route to the correct C-runtime math function
    // e.g., __moksha_powi32, __moksha_powi64, __moksha_powf64
    std::string funcName =
        mlir::isa<mlir::FloatType>(resTy) ? "__moksha_powf" : "__moksha_powi";
    funcName += std::to_string(width);

    auto call = createRuntimeCall(rewriter, op.getLoc(), funcName, {resTy},
                                  {adaptor.getLhs(), adaptor.getRhs()});

    rewriter.replaceOp(op, call.getResult());
    return mlir::success();
  }
};

// ============================================================================
// Atomics Lowering
// ============================================================================
struct AtomicStoreOpLowering
    : public mlir::ConvertOpToLLVMPattern<IR::AtomicStoreOp> {
  using ConvertOpToLLVMPattern<IR::AtomicStoreOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::AtomicStoreOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto llvmStore = rewriter.create<mlir::LLVM::StoreOp>(
        op.getLoc(), adaptor.getValue(), adaptor.getPtr());
    llvmStore.setOrdering(mapAtomicOrdering(op.getOrdering()));

    // LLVM requires explicit alignment for atomic stores.
    // Calculate natural alignment from the type's bit width (e.g., i32 -> 4
    // bytes).
    unsigned alignment = 4; // Fallback
    mlir::Type valTy = adaptor.getValue().getType();
    if (valTy.isIntOrFloat()) {
      alignment = std::max(1u, valTy.getIntOrFloatBitWidth() / 8);
    } else if (mlir::isa<mlir::LLVM::LLVMPointerType>(valTy)) {
      alignment = 8; // Assuming 64-bit pointers
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

    // LLVM requires explicit alignment for atomic loads.
    unsigned alignment = 4; // Fallback
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

    // Extract the old value (index 0) from the returned {T, i1} struct
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

// ============================================================================
// Safe AddOp Lowering (Handles String Concatenation & Precision)
// ============================================================================
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

    // --- [CRITICAL FIX] LLVM 22 ConstantFold Crash Bypass ---
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
    // --------------------------------------------------------

    if (mlir::isa<mlir::FloatType>(resTy)) {
      unsigned width = resTy.getIntOrFloatBitWidth();
      if (width < 32) {
        mlir::Type f32Ty = rewriter.getF32Type();

        // [FIX] Use the safe MLIR upcaster
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

// ============================================================================
// 4. Pass Definition
// ============================================================================

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

    // --- 1. Cache Custom Attributes before Dialect Conversion ---
    llvm::StringMap<bool> hasAttrNaked, hasAttrNoRet, hasAttrInline,
        hasAttrNoInline;
    llvm::StringMap<bool> hasAttrPure, hasAttrCold, hasAttrUsed,
        hasAttrInterrupt;
    llvm::StringMap<bool> hasAttrAsync;

    getOperation().walk([&](mlir::func::FuncOp funcOp) {
      auto name = funcOp.getName();
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
        CastOpLowering, AddressOfOpLowering, CmpOpLowering,
        UnreachableOpLowering, RetainOpLowering, ReleaseOpLowering,
        StoreWeakOpLowering, LoadWeakOpLowering, SpawnOpLowering,
        AwaitOpLowering, MakeClosureOpLowering, CustomCallOpLowering,
        InvokeIndirectOpLowering, InvokeOpLowering, PowOpLowering,
        SpawnOpLowering, AwaitOpLowering, AtomicStoreOpLowering,
        AtomicLoadOpLowering, AtomicRMWOpLowering, AtomicCmpXchgOpLowering,
        FenceOpLowering, AddOpLowering, ThrowOpLowering, LandingPadOpLowering>(
        typeConverter);

    // Math & Bitwise
    patterns.add<
        BinaryOpLowering<IR::SubOp, mlir::LLVM::SubOp, mlir::LLVM::FSubOp>,
        BinaryOpLowering<IR::MulOp, mlir::LLVM::MulOp, mlir::LLVM::FMulOp>,
        BinaryOpLowering<IR::DivOp, mlir::LLVM::SDivOp, mlir::LLVM::FDivOp>,
        BinaryOpLowering<IR::ModOp, mlir::LLVM::SRemOp, mlir::LLVM::FRemOp>,
        BitwiseOpLowering<IR::AndOp, mlir::LLVM::AndOp>,
        BitwiseOpLowering<IR::OrOp, mlir::LLVM::OrOp>,
        BitwiseOpLowering<IR::XorOp, mlir::LLVM::XOrOp>,
        BitwiseOpLowering<IR::ShlOp, mlir::LLVM::ShlOp>,
        BitwiseOpLowering<IR::ShrOp, mlir::LLVM::AShrOp>>(typeConverter);

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
    bool needsPersonality = false;
    module.walk([&](mlir::LLVM::LandingpadOp) { needsPersonality = true; });

    // INJECT C-ABI COROUTINE RESUME WRAPPER
    if (!module.lookupSymbol("moksha_rt_resume_coro")) {
      mlir::OpBuilder builder(module.getBodyRegion());
      auto loc = module.getLoc();
      auto llvmVoidTy = mlir::LLVM::LLVMVoidType::get(&getContext());
      auto llvmI8PtrTy = mlir::LLVM::LLVMPointerType::get(&getContext());

      // Create the C-compatible wrapper function
      auto resumeFunc = builder.create<mlir::LLVM::LLVMFuncOp>(
          loc, "moksha_rt_resume_coro",
          mlir::LLVM::LLVMFunctionType::get(llvmVoidTy, {llvmI8PtrTy}));

      mlir::Block *block = resumeFunc.addEntryBlock(builder);
      mlir::OpBuilder funcBuilder(block, block->begin());

      // Ensure llvm.coro.resume intrinsic is declared
      if (!module.lookupSymbol("llvm.coro.resume")) {
        builder.create<mlir::LLVM::LLVMFuncOp>(
            loc, "llvm.coro.resume",
            mlir::LLVM::LLVMFunctionType::get(llvmVoidTy, {llvmI8PtrTy}));
      }

      // Call the intrinsic (LLVM handles the fastcc translation automatically
      // here!)
      funcBuilder.create<mlir::LLVM::CallOp>(
          loc, mlir::TypeRange{},
          mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.resume"),
          mlir::ValueRange{block->getArgument(0)});

      funcBuilder.create<mlir::LLVM::ReturnOp>(loc, mlir::ValueRange{});
    }

    llvm::errs() << "[DEBUG] Injecting Personality & Attributes...\n";

    llvm::Triple triple(llvm::sys::getDefaultTargetTriple());
    llvm::StringRef persFnName = "__gcc_personality_v0";
    if (needsPersonality) {
      // Target-aware personality routing
      if (triple.isOSWindows()) {
        if (triple.isGNUEnvironment()) {
          persFnName = "__gcc_personality_seh0"; // MinGW SEH
        } else {
          persFnName = "__CxxFrameHandler3"; // MSVC
        }
      }

      // 1. Declare the personality function in the MLIR module if missing
      if (!module.lookupSymbol(persFnName)) {
        mlir::OpBuilder builder(module.getBodyRegion());
        auto i32Ty = mlir::IntegerType::get(&getContext(), 32);
        auto fnTy =
            mlir::LLVM::LLVMFunctionType::get(i32Ty, {}, /*isVarArg=*/true);
        builder.create<mlir::LLVM::LLVMFuncOp>(module.getLoc(), persFnName,
                                               fnTy);
      }
    }

    auto persAttr = mlir::FlatSymbolRefAttr::get(&getContext(), persFnName);

    // --- 2. Restore Custom Attributes onto the LLVMFuncOp ---
    getOperation().walk([&](mlir::LLVM::LLVMFuncOp llvmFunc) {
      // 2a. Satisfy the MLIR LLVM Dialect Verifier!
      bool hasPad = false;
      llvmFunc.walk([&](mlir::LLVM::LandingpadOp) { hasPad = true; });
      if (hasPad) {
        llvmFunc->setAttr("personality", persAttr);
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
          auto insertPt = entryBlock.begin();
          while (insertPt != entryBlock.end() &&
                 (mlir::isa<mlir::LLVM::AllocaOp>(*insertPt) ||
                  mlir::isa<mlir::LLVM::ConstantOp>(*insertPt))) {
            ++insertPt;
          }
          builder.setInsertionPoint(&entryBlock, insertPt);

          auto loc = llvmFunc.getLoc();
          auto llvmI32Ty = mlir::IntegerType::get(&getContext(), 32);
          auto llvmI8PtrTy = mlir::LLVM::LLVMPointerType::get(&getContext());
          auto llvmTokenTy = mlir::LLVM::LLVMTokenType::get(&getContext());
          auto llvmI1Ty = builder.getI1Type();
          auto llvmVoidTy = mlir::LLVM::LLVMVoidType::get(&getContext());
          auto module = llvmFunc->getParentOfType<mlir::ModuleOp>();

          // --- 1. Declare Intrinsics ---
          if (!module.lookupSymbol("llvm.coro.id")) {
            mlir::OpBuilder::InsertionGuard guard(builder);
            builder.setInsertionPointToStart(module.getBody());
            builder.create<mlir::LLVM::LLVMFuncOp>(
                loc, "llvm.coro.id",
                mlir::LLVM::LLVMFunctionType::get(
                    llvmTokenTy,
                    {llvmI32Ty, llvmI8PtrTy, llvmI8PtrTy, llvmI8PtrTy}));
          }
          if (!module.lookupSymbol("llvm.coro.size.i32")) {
            mlir::OpBuilder::InsertionGuard guard(builder);
            builder.setInsertionPointToStart(module.getBody());
            builder.create<mlir::LLVM::LLVMFuncOp>(
                loc, "llvm.coro.size.i32",
                mlir::LLVM::LLVMFunctionType::get(llvmI32Ty, {}));
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
                mlir::LLVM::LLVMFunctionType::get(llvmI8PtrTy, {llvmI32Ty}));
          }
          if (!module.lookupSymbol("__moksha_free")) {
            mlir::OpBuilder::InsertionGuard guard(builder);
            builder.setInsertionPointToStart(module.getBody());
            builder.create<mlir::LLVM::LLVMFuncOp>(
                loc, "__moksha_free",
                mlir::LLVM::LLVMFunctionType::get(llvmVoidTy, {llvmI8PtrTy}));
          }

          // --- 2. Setup Frame ---
          auto nullPtr = builder.create<mlir::LLVM::ZeroOp>(loc, llvmI8PtrTy);
          auto zero32 = builder.create<mlir::LLVM::ConstantOp>(
              loc, llvmI32Ty, builder.getI32IntegerAttr(0));

          auto coroId = builder.create<mlir::LLVM::CallOp>(
              loc, llvmTokenTy,
              mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.id"),
              mlir::ValueRange{zero32, nullPtr, nullPtr, nullPtr});

          auto coroSize = builder.create<mlir::LLVM::CallOp>(
              loc, llvmI32Ty,
              mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.size.i32"),
              mlir::ValueRange{});

          auto allocCall = builder.create<mlir::LLVM::CallOp>(
              loc, llvmI8PtrTy,
              mlir::SymbolRefAttr::get(&getContext(), "__moksha_alloc"),
              mlir::ValueRange{coroSize.getResult()});

          auto coroBegin = builder.create<mlir::LLVM::CallOp>(
              loc, llvmI8PtrTy,
              mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.begin"),
              mlir::ValueRange{coroId.getResult(), allocCall.getResult()});

          // --- 2.5 Declare the Scheduler Hook ---
          if (!module.lookupSymbol("moksha_scheduler_schedule")) {
            mlir::OpBuilder::InsertionGuard guard(builder);
            builder.setInsertionPointToStart(module.getBody());
            builder.create<mlir::LLVM::LLVMFuncOp>(
                loc, "moksha_scheduler_schedule",
                mlir::LLVM::LLVMFunctionType::get(llvmVoidTy, {llvmI8PtrTy}));
          }

          // --- 3. Link Await Calls, Unify Suspend Blocks, and Inject Cleanup
          // Phase 1: Collect the CallOps safely to prevent iterator
          // invalidation
          llvm::SmallVector<mlir::LLVM::CallOp, 4> callOpsToProcess;
          llvmFunc.walk([&](mlir::LLVM::CallOp callOp) {
            callOpsToProcess.push_back(callOp);
          });

          // Phase 2: Iterate over our safe copy and perform the mutations
          for (auto callOp : callOpsToProcess) {
            auto callee = callOp.getCallee();

            // Link the caller's Coroutine Handle to the Promise
            if (callee && *callee == "moksha_rt_register_await") {
              callOp.setOperand(1, coroBegin.getResult());
            }

            // Link the caller's Coroutine Handle to the Save Intrinsic
            if (callee && *callee == "llvm.coro.save") {
              callOp.setOperand(0, coroBegin.getResult());
            }

            if (callee && *callee == "llvm.coro.suspend") {
              if (auto switchOp = mlir::dyn_cast_or_null<mlir::LLVM::SwitchOp>(
                      callOp->getNextNode())) {

                mlir::Block *destroyBlock = switchOp.getCaseDestinations()[1];

                // Inject Coro.Free and Coro.End into the Destroy Block.
                if (destroyBlock->front().getName().getStringRef() !=
                    "llvm.coro.free") {
                  mlir::OpBuilder cleanupBuilder(&getContext());
                  cleanupBuilder.setInsertionPointToStart(destroyBlock);

                  // [FIX] Explicit ArrayRef wrapper for ValueRange
                  mlir::Value freeArgs[] = {coroId.getResult(),
                                            coroBegin.getResult()};
                  auto coroFree = cleanupBuilder.create<mlir::LLVM::CallOp>(
                      loc, llvmI8PtrTy,
                      mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.free"),
                      mlir::ValueRange(llvm::ArrayRef<mlir::Value>(freeArgs)));

                  mlir::Value mokshaFreeArgs[] = {coroFree.getResult()};
                  cleanupBuilder.create<mlir::LLVM::CallOp>(
                      loc, mlir::TypeRange{},
                      mlir::SymbolRefAttr::get(&getContext(), "__moksha_free"),
                      mlir::ValueRange(
                          llvm::ArrayRef<mlir::Value>(mokshaFreeArgs)));

                  auto falseVal = cleanupBuilder.create<mlir::LLVM::ConstantOp>(
                      loc, llvmI1Ty, 0);
                  auto nullToken =
                      cleanupBuilder.create<mlir::LLVM::NoneTokenOp>(
                          loc, llvmTokenTy);

                  mlir::Value endArgs[] = {coroBegin.getResult(),
                                           falseVal.getResult(),
                                           nullToken.getResult()};
                  cleanupBuilder.create<mlir::LLVM::CallOp>(
                      loc, mlir::TypeRange{},
                      mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.end"),
                      mlir::ValueRange(llvm::ArrayRef<mlir::Value>(endArgs)));
                }
              }
            }
          } // <-- Closes Phase 2 loop

          // --- 4. Inject coro.end ONLY Before Final Termination Returns ---
          llvmFunc.walk([&](mlir::LLVM::ReturnOp retOp) {
            if (retOp->hasAttr("moksha.yield")) {
              if (retOp.getNumOperands() > 0 &&
                  retOp.getOperand(0).getType() == llvmI8PtrTy) {
                retOp.setOperand(0, coroBegin.getResult());
              }
              return;
            }

            mlir::OpBuilder retBuilder(retOp);

            auto falseVal = retBuilder.create<mlir::LLVM::ConstantOp>(
                loc, llvmI1Ty, retBuilder.getIntegerAttr(llvmI1Ty, 0));

            auto nullToken =
                retBuilder.create<mlir::LLVM::NoneTokenOp>(loc, llvmTokenTy);

            // [FIX] Explicit ArrayRef wrapper for ValueRange
            mlir::Value retEndArgs[] = {coroBegin.getResult(),
                                        falseVal.getResult(),
                                        nullToken.getResult()};
            retBuilder.create<mlir::LLVM::CallOp>(
                loc, mlir::TypeRange{},
                mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.end"),
                mlir::ValueRange(llvm::ArrayRef<mlir::Value>(retEndArgs)));

            if (retOp.getNumOperands() > 0 &&
                retOp.getOperand(0).getType() == llvmI8PtrTy) {
              retOp.setOperand(0, coroBegin.getResult());
            }
          });
        }
      }
    });

    // --- 4. Hoist Allocas to the Entry Block (CRITICAL FOR COROUTINES) ---
    // CoroSplit requires all local variables to be in the entry block so it can
    // safely move them to the heap-allocated Coroutine Frame.
    module.walk([&](mlir::LLVM::LLVMFuncOp llvmFunc) {
      if (llvmFunc.empty())
        return;
      mlir::Block &entryBlock = llvmFunc.front();

      // Only hoist allocas if this function is a coroutine
      bool isCoro = false;
      llvmFunc.walk([&](mlir::LLVM::CallOp callOp) {
        if (callOp.getCallee() && *callOp.getCallee() == "llvm.coro.begin")
          isCoro = true;
      });

      if (!isCoro)
        return;

      // Phase 1: Collect allocas that are trapped in continuation blocks
      llvm::SmallVector<mlir::LLVM::AllocaOp, 4> allocasToMove;
      llvmFunc.walk([&](mlir::LLVM::AllocaOp allocaOp) {
        if (allocaOp->getBlock() != &entryBlock) {
          allocasToMove.push_back(allocaOp);
        }
      });

      // Phase 2: Hoist them and their size operands to the entry block
      auto insertPt = entryBlock.begin();
      for (auto allocaOp : allocasToMove) {
        // Move the constant size operand first to prevent SSA dominance errors
        if (auto sizeOp = allocaOp.getArraySize().getDefiningOp()) {
          sizeOp->moveBefore(&entryBlock, insertPt);
        }
        allocaOp->moveBefore(&entryBlock, insertPt);
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
