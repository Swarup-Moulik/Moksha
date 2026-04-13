#include "moksha/Backend/LLVM/MLIRToLLVM.h"
#include "moksha/Dialect/MokshaDialect.h"
#include "moksha/Dialect/MokshaOps.h"

#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h"
#include "mlir/Conversion/LLVMCommon/ConversionTarget.h"
#include "mlir/Conversion/LLVMCommon/Pattern.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

namespace moksha {

namespace {

// ============================================================================
// 1. Full Type Converter
// ============================================================================
class MokshaToLLVMTypeConverter : public mlir::LLVMTypeConverter {
public:
  MokshaToLLVMTypeConverter(mlir::MLIRContext *ctx)
      : mlir::LLVMTypeConverter(ctx) {
    // Core Pointers and Arrays
    addConversion([&](IR::PointerType type) {
      return mlir::LLVM::LLVMPointerType::get(type.getContext());
    });
    addConversion([&](IR::ArrayType type) {
      return mlir::LLVM::LLVMArrayType::get(convertType(type.getElementType()),
                                            type.getSize());
    });

    // Fat Pointers (Structs)
    addConversion([&](IR::SliceType type) {
      return mlir::LLVM::LLVMStructType::getLiteral(
          type.getContext(),
          {mlir::LLVM::LLVMPointerType::get(type.getContext()), // Data ptr
           mlir::IntegerType::get(type.getContext(), 32)});
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
              mlir::IntegerType::get(type.getContext(), 64), // Mantissa
              mlir::IntegerType::get(type.getContext(), 32)  // Scale
          });
    });
    addConversion([&](IR::AnyType type) {
      return mlir::LLVM::LLVMStructType::getLiteral(
          type.getContext(),
          {
              mlir::LLVM::LLVMPointerType::get(type.getContext()), // Data
              mlir::IntegerType::get(type.getContext(), 32)        // Type ID
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

  // [FIX] Check for ANY existing symbol to prevent redefinition of func.func!
  if (!module.lookupSymbol(funcName)) {
    mlir::OpBuilder::InsertionGuard guard(rewriter);
    rewriter.setInsertionPointToStart(module.getBody());
    llvm::SmallVector<mlir::Type, 4> argTypes;
    for (auto arg : args)
      argTypes.push_back(arg.getType());
    auto funcType = mlir::LLVM::LLVMFunctionType::get(
        retTypes.empty() ? mlir::LLVM::LLVMVoidType::get(rewriter.getContext())
                         : retTypes[0],
        argTypes);
    rewriter.create<mlir::LLVM::LLVMFuncOp>(loc, funcName, funcType);
  }

  // Use a SymbolRefAttr to safely target the function regardless of its current
  // dialect
  auto symRef = mlir::SymbolRefAttr::get(rewriter.getContext(), funcName);
  return rewriter.create<mlir::LLVM::CallOp>(loc, retTypes, symRef, args);
}

// Helper to generate calls to the C++ runtime
static mlir::Value createRuntimeCall(mlir::ConversionPatternRewriter &rewriter,
                                     mlir::Location loc, mlir::Type returnType,
                                     llvm::StringRef funcName,
                                     mlir::ArrayRef<mlir::Value> args) {
  auto module =
      rewriter.getBlock()->getParentOp()->getParentOfType<mlir::ModuleOp>();

  // [FIX] Check for ANY existing symbol
  if (!module.lookupSymbol(funcName)) {
    mlir::OpBuilder::InsertionGuard guard(rewriter);
    rewriter.setInsertionPointToStart(module.getBody());
    llvm::SmallVector<mlir::Type> argTypes;
    for (auto arg : args)
      argTypes.push_back(arg.getType());
    auto funcType = mlir::LLVM::LLVMFunctionType::get(returnType, argTypes);
    rewriter.create<mlir::LLVM::LLVMFuncOp>(loc, funcName, funcType);
  }

  auto symRef = mlir::SymbolRefAttr::get(rewriter.getContext(), funcName);
  return rewriter.create<mlir::LLVM::CallOp>(loc, returnType, symRef, args)
      .getResult();
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
  if (mlir::isa<mlir::LLVM::LLVMPointerType>(type) ||
      mlir::isa<mlir::LLVM::LLVMArrayType>(type))
    return 16;
  return 17;
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
    return rewriter.create<mlir::LLVM::ConstantOp>(
        loc, llvmType, rewriter.getIntegerAttr(llvmType, intAttr.getValue()));
  }
  if (auto floatAttr = mlir::dyn_cast<mlir::FloatAttr>(attr)) {
    return rewriter.create<mlir::LLVM::ConstantOp>(
        loc, llvmType, rewriter.getFloatAttr(llvmType, floatAttr.getValue()));
  }
  return rewriter.create<mlir::LLVM::ConstantOp>(loc, llvmType, attr);
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

    // Generate the true LLVM type (e.g., `ptr` for strings)
    mlir::Type llvmType = typeConverter->convertType(elementType);

    mlir::LLVM::Linkage linkage = mlir::LLVM::Linkage::External;
    mlir::Attribute initAttr = op.getInitialValueAttr();

    if (initAttr) {
      if (auto intAttr = mlir::dyn_cast<mlir::IntegerAttr>(initAttr)) {
        initAttr = rewriter.getIntegerAttr(llvmType, intAttr.getValue());
      } else if (auto floatAttr = mlir::dyn_cast<mlir::FloatAttr>(initAttr)) {
        initAttr = rewriter.getFloatAttr(llvmType, floatAttr.getValue());
      }
    }

    if (auto linkAttr = op->getAttrOfType<mlir::StringAttr>("moksha.linkage")) {
      llvm::StringRef linkStr = linkAttr.getValue();
      if (linkStr == "internal")
        linkage = mlir::LLVM::Linkage::Internal;
      else if (linkStr == "weak")
        linkage = mlir::LLVM::Linkage::Weak; // [FIX] Weak instead of WeakAny
    }

    bool isMap = elementType && mlir::isa<IR::MapType>(elementType);
    bool generateMapInit = false;
    mlir::ArrayAttr mapEntries;

    bool generateStrInit = false;
    mlir::StringAttr strEntry;
    bool forceZeroInit = false;

    if (initAttr) {
      if (mlir::isa<mlir::ArrayAttr>(initAttr)) {
        if (!mlir::isa<mlir::LLVM::LLVMArrayType>(llvmType) &&
            !mlir::isa<mlir::LLVM::LLVMStructType>(llvmType)) {
          if (isMap) {
            generateMapInit = true;
            mapEntries = mlir::cast<mlir::ArrayAttr>(initAttr);
          }
          initAttr = nullptr;
          forceZeroInit = true;
        }
      } else if (mlir::isa<mlir::StringAttr>(initAttr)) {
        if (mlir::isa<mlir::LLVM::LLVMPointerType>(llvmType)) {
          generateStrInit = true;
          strEntry = mlir::cast<mlir::StringAttr>(initAttr);
          initAttr = nullptr;
          forceZeroInit = true;
        }
      } else if (mlir::isa<mlir::UnitAttr>(initAttr)) {
        initAttr = nullptr; // Strip UnitAttr to prevent "unsupported constant
                            // value" error
        forceZeroInit = true;
      }
    }

    if (forceZeroInit) {
      if (linkage == mlir::LLVM::Linkage::External &&
          op->hasAttr("moksha.linkage")) {
        auto strAttr = op->getAttrOfType<mlir::StringAttr>("moksha.linkage");
        if (strAttr.getValue() == "external") {
          forceZeroInit = false; // It's a true extern declaration!
        }
      } else {
        linkage = mlir::LLVM::Linkage::External;
      }
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

    // Only apply zeroinit if it's NOT a true extern
    if (forceZeroInit || op->hasAttr("moksha.zeroinit")) {
      globalOp->setAttr("moksha.zeroinit",
                        mlir::UnitAttr::get(rewriter.getContext()));
      globalOp->setAttr("moksha.linkage", rewriter.getStringAttr("internal"));
    }

    // 1. Map to native LLVM Dialect 'section_name'
    if (auto secAttr = op->getAttrOfType<mlir::StringAttr>("moksha.section")) {
      globalOp->setAttr("section_name", secAttr);
    }

    // 2. Forward all remaining Moksha attributes dynamically
    for (auto attr : op->getAttrs()) {
      if (attr.getName().strref().starts_with("moksha.") &&
          attr.getName().strref() != "moksha.linkage") {
        globalOp->setAttr(attr.getName(), attr.getValue());
      }
    }

    // Dynamic Initialization Injection (for maps and strings)
    if (generateMapInit || generateStrInit) {
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
            auto mapNewCall = createRuntimeCall(
                rewriter, loc, "__moksha_map_new", {llvmType}, {});
            mlir::Value mapPtr = mapNewCall.getResult();
            rewriter.create<mlir::LLVM::StoreOp>(loc, mapPtr, globalAddr);

            auto mapTy = mlir::cast<IR::MapType>(elementType);
            mlir::Type keyTy = typeConverter->convertType(mapTy.getKeyType());
            mlir::Type valTy = typeConverter->convertType(mapTy.getValueType());

            for (auto attr : mapEntries) {
              if (auto kvAttr = mlir::dyn_cast<mlir::ArrayAttr>(attr)) {
                if (kvAttr.size() == 2) {
                  mlir::Value kVal = materializeLLVMConstant(rewriter, globalOp,
                                                             keyTy, kvAttr[0]);
                  mlir::Value vVal = materializeLLVMConstant(rewriter, globalOp,
                                                             valTy, kvAttr[1]);
                  createRuntimeCall(rewriter, loc, "__moksha_map_insert", {},
                                    {mapPtr, kVal, vVal});
                }
              }
            }
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

        // Strip the trailing 'd' or 'D'
        while (!decStr.empty() &&
               (decStr.back() == 'd' || decStr.back() == 'D')) {
          decStr.pop_back();
        }

        // Parse coefficient and scale
        int32_t scale = 0;
        int64_t coeff = 0;
        size_t dotPos = decStr.find('.');
        if (dotPos != std::string::npos) {
          scale = decStr.length() - dotPos - 1;
          decStr.erase(dotPos, 1);
        }

        // Prevent i64 overflow during basic compiler lowering
        if (decStr.length() > 18)
          decStr = decStr.substr(0, 18);
        if (!decStr.empty())
          coeff = std::stoll(decStr);

        auto coeffAttr = rewriter.getI64IntegerAttr(coeff);
        auto scaleAttr = rewriter.getI32IntegerAttr(scale);

        // Dynamically pack the { i64, i32 } LLVM struct
        mlir::Value undef =
            rewriter.create<mlir::LLVM::UndefOp>(op.getLoc(), llvmType);
        mlir::Value coeffVal = rewriter.create<mlir::LLVM::ConstantOp>(
            op.getLoc(), rewriter.getI64Type(), coeffAttr);
        mlir::Value s1 = rewriter.create<mlir::LLVM::InsertValueOp>(
            op.getLoc(), undef, coeffVal, llvm::ArrayRef<int64_t>{0});
        mlir::Value scaleVal = rewriter.create<mlir::LLVM::ConstantOp>(
            op.getLoc(), rewriter.getI32Type(), scaleAttr);
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
    rewriter.replaceOpWithNewOp<mlir::LLVM::AllocaOp>(
        op, typeConverter->convertType(op.getType()),
        typeConverter->convertType(op.getAllocatedType()), one);
    return mlir::success();
  }
};

struct LoadOpLowering : public mlir::ConvertOpToLLVMPattern<IR::LoadOp> {
  using ConvertOpToLLVMPattern<IR::LoadOp>::ConvertOpToLLVMPattern;
  mlir::LogicalResult
  matchAndRewrite(IR::LoadOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {
    auto llvmLoad = rewriter.replaceOpWithNewOp<mlir::LLVM::LoadOp>(
        op, typeConverter->convertType(op.getType()), adaptor.getPtr());
    if (op->hasAttr("moksha.volatile")) {
      llvmLoad.setVolatile_(true);
    }
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

    // FIX: Use raw getOperand(0) instead of getBase() to bypass strict ODS type
    // assertions!
    mlir::Type origMokshaBaseTy = op->getOperand(0).getType();

    // 1. FAT POINTER EXTRACTION: If the base is a Slice or Any (which lower to
    // structs), we must extract the actual raw data pointer first!
    if (mlir::isa<IR::SliceType>(origMokshaBaseTy) ||
        mlir::isa<IR::AnyType>(origMokshaBaseTy)) {
      base = rewriter.create<mlir::LLVM::ExtractValueOp>(
          op.getLoc(), base, llvm::ArrayRef<int64_t>({0}));
      baseType = base.getType();
    }
    // 2. AGGREGATE SPILL: If it's a raw fixed array value, spill it to stack to
    // get a pointer
    else if (!mlir::isa<mlir::LLVM::LLVMPointerType>(baseType)) {
      mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));
      mlir::Value allocaPtr = rewriter.create<mlir::LLVM::AllocaOp>(
          op.getLoc(), mlir::LLVM::LLVMPointerType::get(getContext()), baseType,
          one);
      rewriter.create<mlir::LLVM::StoreOp>(op.getLoc(), base, allocaPtr);
      base = allocaPtr;
    }

    // 3. POINTEE INFERENCE: Infer the element type for LLVM 15+ opaque pointers
    mlir::Type pointeeTy = mlir::IntegerType::get(getContext(), 8); // fallback
    if (auto ptrTy = mlir::dyn_cast<IR::PointerType>(origMokshaBaseTy)) {
      pointeeTy = typeConverter->convertType(ptrTy.getPointee());
    } else if (auto arrTy = mlir::dyn_cast<IR::ArrayType>(origMokshaBaseTy)) {
      pointeeTy = typeConverter->convertType(arrTy);
    } else if (auto sliceTy = mlir::dyn_cast<IR::SliceType>(origMokshaBaseTy)) {
      pointeeTy = typeConverter->convertType(sliceTy.getElementType());
    }

    auto indices = adaptor.getOperands().drop_front(1);

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
          double fVal = floatAttr.getValue().convertToDouble();
          rewriter.replaceOpWithNewOp<mlir::LLVM::ConstantOp>(
              op, dstType, rewriter.getFloatAttr(dstType, fVal));
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
      return st && st.getBody().size() == 2 && st.getBody()[0].isInteger(64) &&
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

      mlir::Value call = createRuntimeCall(rewriter, op.getLoc(), dstType,
                                           "__moksha_dec_scale",
                                           {adaptor.getValue(), scaleVal});

      rewriter.replaceOp(op, call);
      return mlir::success();
    }

    auto isAnyStruct = [](mlir::Type t) {
      auto st = mlir::dyn_cast<mlir::LLVM::LLVMStructType>(t);
      return st && st.getBody().size() == 2 &&
             mlir::isa<mlir::LLVM::LLVMPointerType>(st.getBody()[0]) &&
             st.getBody()[1].isInteger(32);
    };

    // 1. Boxing (to Any)
    if (isAnyStruct(dstType) && !isAnyStruct(srcType)) {
      mlir::Type llvmPtrTy =
          mlir::LLVM::LLVMPointerType::get(rewriter.getContext());
      mlir::Type i32Ty = rewriter.getI32Type();

      // Step A: Calculate the size of the primitive type (sizeof trick)
      mlir::Value nullPtr =
          rewriter.create<mlir::LLVM::ZeroOp>(op.getLoc(), llvmPtrTy);
      mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), i32Ty, rewriter.getI32IntegerAttr(1));

      mlir::Value sizeGep = rewriter.create<mlir::LLVM::GEPOp>(
          op.getLoc(), llvmPtrTy, srcType, nullPtr,
          llvm::ArrayRef<mlir::LLVM::GEPArg>{one});
      mlir::Value sizeInt =
          rewriter.create<mlir::LLVM::PtrToIntOp>(op.getLoc(), i32Ty, sizeGep);

      // Step B: Allocate memory on the HEAP, not the stack!
      mlir::Value heapPtr =
          createRuntimeCall(rewriter, op.getLoc(), "__moksha_alloc",
                            {llvmPtrTy}, {sizeInt})
              .getResult();

      // Step C: Store the value into the heap pointer
      rewriter.create<mlir::LLVM::StoreOp>(op.getLoc(), adaptor.getValue(),
                                           heapPtr);

      // Step D: Construct the Fat Pointer { ptr, i32 }
      mlir::Value anyStruct =
          rewriter.create<mlir::LLVM::UndefOp>(op.getLoc(), dstType);
      anyStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
          op.getLoc(), anyStruct, heapPtr, llvm::ArrayRef<int64_t>({0}));

      uint32_t typeId = getMokshaTypeID(origSrcType);
      mlir::Value typeTag = rewriter.create<mlir::LLVM::ConstantOp>(
          op.getLoc(), i32Ty, rewriter.getI32IntegerAttr(typeId));
      anyStruct = rewriter.create<mlir::LLVM::InsertValueOp>(
          op.getLoc(), anyStruct, typeTag, llvm::ArrayRef<int64_t>({1}));

      rewriter.replaceOp(op, anyStruct);
      return mlir::success();
    }

    // 2. Unboxing (from Any)
    if (isAnyStruct(srcType) && !isAnyStruct(dstType)) {
      mlir::Value dataPtr = rewriter.create<mlir::LLVM::ExtractValueOp>(
          op.getLoc(), adaptor.getValue(), llvm::ArrayRef<int64_t>({0}));
      mlir::Value loaded =
          rewriter.create<mlir::LLVM::LoadOp>(op.getLoc(), dstType, dataPtr);
      rewriter.replaceOp(op, loaded);
      return mlir::success();
    }

    // 3. Pointer -> Array Value (Dereference / Load)
    if (mlir::isa<mlir::LLVM::LLVMPointerType>(srcType) &&
        mlir::isa<mlir::LLVM::LLVMArrayType>(dstType)) {
      rewriter.replaceOpWithNewOp<mlir::LLVM::LoadOp>(op, dstType,
                                                      adaptor.getValue());
      return mlir::success();
    }

    // 4. Array Value -> Pointer (Decay / Spill to stack)
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

    // 5. Slice Array Decay (Struct -> Ptr)
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

    // 6. Raw Pointer to Slice Struct (Ptr -> Struct)
    if (mlir::isa<mlir::LLVM::LLVMPointerType>(srcType) &&
        mlir::isa<mlir::LLVM::LLVMStructType>(dstType)) {
      auto structTy = mlir::cast<mlir::LLVM::LLVMStructType>(dstType);
      if (structTy.getBody().size() == 2 &&
          mlir::isa<mlir::LLVM::LLVMPointerType>(structTy.getBody()[0])) {
        mlir::Value slice =
            rewriter.create<mlir::LLVM::UndefOp>(op.getLoc(), dstType);
        mlir::Value ptr = adaptor.getValue();
        if (ptr.getType() != structTy.getBody()[0]) {
          ptr = rewriter.create<mlir::LLVM::BitcastOp>(
              op.getLoc(), structTy.getBody()[0], ptr);
        }
        slice = rewriter.create<mlir::LLVM::InsertValueOp>(
            op.getLoc(), slice, ptr, llvm::ArrayRef<int64_t>({0}));

        mlir::Type lenTy = structTy.getBody()[1];
        mlir::Value zeroLen = rewriter.create<mlir::LLVM::ConstantOp>(
            op.getLoc(), lenTy, rewriter.getIntegerAttr(lenTy, 0));
        slice = rewriter.create<mlir::LLVM::InsertValueOp>(
            op.getLoc(), slice, zeroLen, llvm::ArrayRef<int64_t>({1}));

        rewriter.replaceOp(op, slice);
        return mlir::success();
      }
    }

    // 7. Int <-> Int
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

    // --- [FIX] 7b. Int <-> Float Conversions ---
    if (srcType.isIntOrIndex() && mlir::isa<mlir::FloatType>(dstType)) {
      if (origSrcType.isUnsignedInteger()) { // [FIX] Use origSrcType
        rewriter.replaceOpWithNewOp<mlir::LLVM::UIToFPOp>(op, dstType,
                                                          adaptor.getValue());
      } else {
        rewriter.replaceOpWithNewOp<mlir::LLVM::SIToFPOp>(op, dstType,
                                                          adaptor.getValue());
      }
      return mlir::success();
    }
    if (mlir::isa<mlir::FloatType>(srcType) && dstType.isIntOrIndex()) {
      if (origDstType.isUnsignedInteger()) { // [FIX] Use origDstType
        rewriter.replaceOpWithNewOp<mlir::LLVM::FPToUIOp>(op, dstType,
                                                          adaptor.getValue());
      } else {
        rewriter.replaceOpWithNewOp<mlir::LLVM::FPToSIOp>(op, dstType,
                                                          adaptor.getValue());
      }
      return mlir::success();
    }

    // --- [FIX] 7c. Float <-> Float Conversions (Precision changes) ---
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

    // 8. Pointer <-> Integer
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

    // 9. Pointer <-> Pointer
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

    // 10. Standard Fallback (Aggregate Safety Hook)
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
          structTy.getBody()[0].isInteger(64) &&
          structTy.getBody()[1].isInteger(32)) {

        llvm::StringRef rtFunc = "__moksha_dec_add"; // Fallback
        if (std::is_same_v<MokshaOp, IR::SubOp>)
          rtFunc = "__moksha_dec_sub";
        else if (std::is_same_v<MokshaOp, IR::MulOp>)
          rtFunc = "__moksha_dec_mul";
        else if (std::is_same_v<MokshaOp, IR::DivOp>)
          rtFunc = "__moksha_dec_div";

        mlir::Value call =
            createRuntimeCall(rewriter, op.getLoc(), resTy, rtFunc,
                              {adaptor.getLhs(), adaptor.getRhs()});
        rewriter.replaceOp(op, call);
        return mlir::success();
      }
    }
    if (mlir::isa<mlir::FloatType>(resTy))
      rewriter.replaceOpWithNewOp<LLVMFloatOp>(op, resTy, adaptor.getLhs(),
                                               adaptor.getRhs());
    else
      rewriter.replaceOpWithNewOp<LLVMIntOp>(op, resTy, adaptor.getLhs(),
                                             adaptor.getRhs());
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

    if (mlir::isa<mlir::FloatType>(ty)) {
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
      rewriter.replaceOpWithNewOp<mlir::LLVM::FCmpOp>(
          op, llvmPred, adaptor.getLhs(), adaptor.getRhs());
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
    createRuntimeCall(rewriter, loc, "moksha_rt_release", {},
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
    llvm::SmallVector<mlir::Type, 1> resultTypes;
    if (mlir::failed(
            typeConverter->convertTypes(op.getResultTypes(), resultTypes)))
      return mlir::failure();

    auto llvmCall = rewriter.create<mlir::LLVM::CallOp>(
        op.getLoc(), resultTypes, op.getCalleeAttr(), adaptor.getOperands());

    // [FIX] LLVM requires the exact function signature for variadic calls
    if (op->hasAttr("func.varargs")) {
      auto moduleOp = op->getParentOfType<mlir::ModuleOp>();
      auto funcOp = moduleOp.lookupSymbol<mlir::func::FuncOp>(op.getCallee());

      if (funcOp) {
        auto fnTy = funcOp.getFunctionType();
        llvm::SmallVector<mlir::Type, 4> argTypes;
        for (auto ty : fnTy.getInputs())
          argTypes.push_back(typeConverter->convertType(ty));

        mlir::Type retTy = fnTy.getNumResults() == 0
                               ? mlir::LLVM::LLVMVoidType::get(getContext())
                               : typeConverter->convertType(fnTy.getResult(0));

        auto llvmFnTy = mlir::LLVM::LLVMFunctionType::get(retTy, argTypes,
                                                          /*isVarArg=*/true);
        llvmCall->setAttr("var_callee_type", mlir::TypeAttr::get(llvmFnTy));
      } else {
        llvm::SmallVector<mlir::Type, 4> argTypes;
        for (auto val : adaptor.getOperands())
          argTypes.push_back(val.getType());

        mlir::Type retTy = resultTypes.empty()
                               ? mlir::LLVM::LLVMVoidType::get(getContext())
                               : resultTypes[0];

        auto llvmFnTy = mlir::LLVM::LLVMFunctionType::get(retTy, argTypes,
                                                          /*isVarArg=*/true);
        llvmCall->setAttr("var_callee_type", mlir::TypeAttr::get(llvmFnTy));
      }
    }

    rewriter.replaceOp(op, llvmCall.getResults());
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
      mlir::Value one = rewriter.create<mlir::LLVM::ConstantOp>(
          loc, rewriter.getI32Type(), rewriter.getI32IntegerAttr(1));
      mlir::Value alloc = rewriter.create<mlir::LLVM::AllocaOp>(
          loc, opaquePtrTy, closurePtr.getType(), one);
      rewriter.create<mlir::LLVM::StoreOp>(loc, closurePtr, alloc);
      closurePtr = alloc;
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

    // 1. Declare Intrinsics via FuncOp to bypass buggy MLIR verifiers
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

    // 2. Call coro.save
    auto nullHandle = rewriter.create<mlir::LLVM::ZeroOp>(loc, llvmI8PtrTy);
    auto coroSave = rewriter.create<mlir::LLVM::CallOp>(
        loc, llvmTokenTy,
        mlir::SymbolRefAttr::get(getContext(), "llvm.coro.save"),
        mlir::ValueRange{nullHandle.getResult()});

    // 3. Call coro.suspend
    auto falseVal = rewriter.create<mlir::LLVM::ConstantOp>(
        loc, rewriter.getI1Type(), rewriter.getBoolAttr(false));
    auto coroSuspend = rewriter.create<mlir::LLVM::CallOp>(
        loc, llvmI8Ty,
        mlir::SymbolRefAttr::get(getContext(), "llvm.coro.suspend"),
        mlir::ValueRange{coroSave.getResult(), falseVal.getResult()});

    // 4. Switch blocks
    auto currentBlock = rewriter.getInsertionBlock();
    auto resumeBlock =
        rewriter.splitBlock(currentBlock, rewriter.getInsertionPoint());
    auto destroyBlock = rewriter.createBlock(resumeBlock);

    // Setup destroy block (cleanup and return)
    rewriter.setInsertionPointToEnd(destroyBlock);
    auto funcOp = op->getParentOfType<mlir::LLVM::LLVMFuncOp>();
    mlir::Type funcRetTy = funcOp.getFunctionType().getReturnType();
    if (mlir::isa<mlir::LLVM::LLVMVoidType>(funcRetTy)) {
      rewriter.create<mlir::LLVM::ReturnOp>(loc, mlir::ValueRange{});
    } else {
      auto nullRet = rewriter.create<mlir::LLVM::ZeroOp>(loc, funcRetTy);
      rewriter.create<mlir::LLVM::ReturnOp>(loc, mlir::ValueRange{nullRet});
    }

    // Branch based on suspend result
    rewriter.setInsertionPointToEnd(currentBlock);
    auto zero = rewriter.create<mlir::LLVM::ConstantOp>(
        loc, llvmI8Ty, rewriter.getI8IntegerAttr(0));
    auto isResume = rewriter.create<mlir::LLVM::ICmpOp>(
        loc, mlir::LLVM::ICmpPredicate::eq, coroSuspend.getResult(), zero);
    rewriter.create<mlir::LLVM::CondBrOp>(loc, isResume, resumeBlock,
                                          destroyBlock);

    // Resume block
    rewriter.setInsertionPointToStart(resumeBlock);
    mlir::Type expectedTy = typeConverter->convertType(op.getType());
    if (!expectedTy || mlir::isa<mlir::LLVM::LLVMVoidType>(expectedTy)) {
      rewriter.eraseOp(op);
    } else {
      auto undef = rewriter.create<mlir::LLVM::UndefOp>(loc, expectedTy);
      rewriter.replaceOp(op, undef);
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

    auto llvmInvoke = rewriter.create<mlir::LLVM::InvokeOp>(
        op.getLoc(), resultTypes, op.getCalleeAttr(), adaptor.getOperands(),
        op.getNormalDest(), mlir::ValueRange{}, unwindDest, mlir::ValueRange{});

    // Handle Variadic Signatures accurately for external functions
    auto moduleOp = op->getParentOfType<mlir::ModuleOp>();
    if (auto funcOp =
            moduleOp.lookupSymbol<mlir::func::FuncOp>(op.getCallee())) {
      if (funcOp->hasAttr("func.varargs")) {
        auto fnTy = funcOp.getFunctionType();
        llvm::SmallVector<mlir::Type, 4> argTypes;
        for (auto ty : fnTy.getInputs())
          argTypes.push_back(typeConverter->convertType(ty));

        mlir::Type retTy = fnTy.getNumResults() == 0
                               ? mlir::LLVM::LLVMVoidType::get(getContext())
                               : typeConverter->convertType(fnTy.getResult(0));

        auto llvmFnTy = mlir::LLVM::LLVMFunctionType::get(retTy, argTypes,
                                                          /*isVarArg=*/true);
        llvmInvoke->setAttr("var_callee_type", mlir::TypeAttr::get(llvmFnTy));
      }
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
// Safe AddOp Lowering (Handles String Concatenation)
// ============================================================================
struct AddOpLowering : public mlir::ConvertOpToLLVMPattern<IR::AddOp> {
  using ConvertOpToLLVMPattern<IR::AddOp>::ConvertOpToLLVMPattern;

  mlir::LogicalResult
  matchAndRewrite(IR::AddOp op, OpAdaptor adaptor,
                  mlir::ConversionPatternRewriter &rewriter) const override {

    mlir::Type resTy = typeConverter->convertType(op.getType());

    // 1. Intercept String Concatenation (Pointer Addition)
    if (mlir::isa<mlir::LLVM::LLVMPointerType>(resTy)) {
      mlir::Value call = createRuntimeCall(
          rewriter, op.getLoc(), resTy, "__moksha_string_concat",
          {adaptor.getLhs(), adaptor.getRhs()});
      rewriter.replaceOp(op, call);
      return mlir::success();
    }

    // 2. Intercept Decimal Math
    if (auto structTy = mlir::dyn_cast<mlir::LLVM::LLVMStructType>(resTy)) {
      if (structTy.getBody().size() == 2 &&
          structTy.getBody()[0].isInteger(64) &&
          structTy.getBody()[1].isInteger(32)) {
        mlir::Value call =
            createRuntimeCall(rewriter, op.getLoc(), resTy, "__moksha_dec_add",
                              {adaptor.getLhs(), adaptor.getRhs()});
        rewriter.replaceOp(op, call);
        return mlir::success();
      }
    }

    // 3. Standard Numeric Addition
    if (mlir::isa<mlir::FloatType>(resTy)) {
      rewriter.replaceOpWithNewOp<mlir::LLVM::FAddOp>(
          op, resTy, adaptor.getLhs(), adaptor.getRhs());
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
    patterns.add<GlobalOpLowering, ConstantOpLowering, AllocaOpLowering,
                 LoadOpLowering, StoreOpLowering, InlineAsmOpLowering,
                 GetElementPtrOpLowering, ExtractValueOpLowering,
                 InsertValueOpLowering, CastOpLowering, AddressOfOpLowering,
                 CmpOpLowering, UnreachableOpLowering, RetainOpLowering,
                 ReleaseOpLowering, StoreWeakOpLowering, LoadWeakOpLowering,
                 SpawnOpLowering, AwaitOpLowering, MakeClosureOpLowering,
                 CustomCallOpLowering, InvokeIndirectOpLowering,
                 InvokeOpLowering, PowOpLowering, SpawnOpLowering,
                 AwaitOpLowering, AtomicStoreOpLowering, AtomicLoadOpLowering,
                 AtomicRMWOpLowering, AtomicCmpXchgOpLowering, FenceOpLowering,
                 AddOpLowering, ThrowOpLowering, LandingPadOpLowering>(
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

    llvm::errs() << "[Debug] Starting MLIR Dialect Conversion...\n";
    if (mlir::failed(mlir::applyFullConversion(getOperation(), target,
                                               std::move(patterns)))) {
      llvm::errs() << "[FATAL] Dialect Conversion Failed! Aborting safely.\n";
      signalPassFailure();
      return;
    }
    llvm::errs() << "[Debug] Dialect Conversion Succeeded!\n";

    // --- [CRITICAL FIX] Inject Personality Function for MLIR Verifier ---
    mlir::ModuleOp module = getOperation();
    bool needsPersonality = false;
    module.walk([&](mlir::LLVM::LandingpadOp) { needsPersonality = true; });

    if (needsPersonality) {
      // 1. Declare the personality function in the MLIR module if missing
      if (!module.lookupSymbol("__gcc_personality_v0")) {
        mlir::OpBuilder builder(module.getBodyRegion());
        auto i32Ty = mlir::IntegerType::get(&getContext(), 32);
        auto fnTy =
            mlir::LLVM::LLVMFunctionType::get(i32Ty, {}, /*isVarArg=*/true);
        builder.create<mlir::LLVM::LLVMFuncOp>(module.getLoc(),
                                               "__gcc_personality_v0", fnTy);
      }
    }

    auto persAttr =
        mlir::FlatSymbolRefAttr::get(&getContext(), "__gcc_personality_v0");

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
      if (hasAttrAsync.count(name)) {
        llvmFunc->setAttr("moksha.async", unit);

        if (!llvmFunc.getBody().empty()) {
          mlir::Block &entryBlock = llvmFunc.getBody().front();
          mlir::OpBuilder builder(&getContext());
          builder.setInsertionPointToStart(&entryBlock);

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
                    llvmVoidTy,
                    {llvmI8PtrTy, builder.getI1Type(), llvmTokenTy}));
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
          // ---------------------------------------------------------------------

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

          // --- 3. Inject Cleanup into Destroy Blocks ---
          llvmFunc.walk([&](mlir::LLVM::CallOp callOp) {
            auto callee = callOp.getCallee();
            if (callee && *callee == "llvm.coro.suspend") {
              // Find the ICmpOp that checks the suspension result
              if (auto cmpOp = mlir::dyn_cast_or_null<mlir::LLVM::ICmpOp>(
                      callOp->getNextNode())) {
                // Find the CondBrOp that branches based on the result
                if (auto brOp = mlir::dyn_cast_or_null<mlir::LLVM::CondBrOp>(
                        cmpOp->getNextNode())) {

                  // The "false" destination is the destroy block
                  mlir::Block *destroyBlock = brOp.getFalseDest();
                  mlir::OpBuilder cleanupBuilder(&getContext());
                  cleanupBuilder.setInsertionPointToStart(destroyBlock);

                  // %mem = call ptr @llvm.coro.free(token %id, ptr %hdl)
                  auto coroFree = cleanupBuilder.create<mlir::LLVM::CallOp>(
                      loc, llvmI8PtrTy,
                      mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.free"),
                      mlir::ValueRange{coroId.getResult(),
                                       coroBegin.getResult()});

                  // call void @__moksha_free(ptr %mem)
                  mlir::Value memPtr = coroFree.getResult();
                  cleanupBuilder.create<mlir::LLVM::CallOp>(
                      loc, mlir::TypeRange{},
                      mlir::SymbolRefAttr::get(&getContext(), "__moksha_free"),
                      mlir::ValueRange{memPtr});
                }
              }
            }
          });

          // --- 4. Inject coro.end Before Every Return ---
          llvmFunc.walk([&](mlir::LLVM::ReturnOp retOp) {
            mlir::OpBuilder retBuilder(retOp);
            auto falseVal = retBuilder.create<mlir::LLVM::ConstantOp>(
                loc, llvmI1Ty, retBuilder.getBoolAttr(false));

            // Create a null token required by the new signature
            auto nullToken =
                retBuilder.create<mlir::LLVM::ZeroOp>(loc, llvmTokenTy);

            retBuilder.create<mlir::LLVM::CallOp>(
                loc, mlir::TypeRange{},
                mlir::SymbolRefAttr::get(&getContext(), "llvm.coro.end"),
                mlir::ValueRange{coroBegin.getResult(), falseVal.getResult(),
                                 nullToken.getResult()});
          });
        }
      }
    });
  }
};

} // end anonymous namespace

std::unique_ptr<mlir::Pass> createConvertMokshaToLLVMPass() {
  return std::make_unique<ConvertMokshaToLLVMPass>();
}

} // namespace moksha
