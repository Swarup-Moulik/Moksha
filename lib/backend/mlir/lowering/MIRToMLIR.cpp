#include "moksha/Backend/MLIR/MIRToMLIR.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "moksha/Backend/MLIR/TypeLowering.h"
#include "moksha/Dialect/MokshaDialect.h"
#include "moksha/Dialect/MokshaOps.h"
#include "moksha/MIR/MIRArgument.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRGlobal.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "moksha/Support/Diagnostics.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace moksha {
namespace backend {
namespace mlir {

namespace {

class MIRToMLIRConverter {
public:
  MIRToMLIRConverter(::mlir::MLIRContext &context, DiagnosticEngine &diags)
      : context(context), builder(&context), diags(diags),
        typeLowering(context) {
    context.getOrLoadDialect<::moksha::IR::MokshaDialect>();
    context.getOrLoadDialect<::mlir::func::FuncDialect>();
    context.getOrLoadDialect<::mlir::cf::ControlFlowDialect>();
  }

  ::mlir::OwningOpRef<::mlir::ModuleOp> convert(mir::MIRModule &mirModule) {
    auto moduleLoc = builder.getUnknownLoc();
    ::mlir::ModuleOp mlirModule = ::mlir::ModuleOp::create(moduleLoc);
    builder.setInsertionPointToStart(mlirModule.getBody());

    for (auto &func : mirModule.getFunctions()) {
      if (func->getName() == "main" || func->getName() == "__moksha_main") {
        this->hasMain = true;
      }
    }

    for (auto &global : mirModule.getGlobals()) {
      lowerGlobal(global);
    }

    for (auto &func : mirModule.getFunctions()) {
      createFunctionDecl(func);
    }

    for (auto &func : mirModule.getFunctions()) {
      if (!func->isDeclaration()) {
        if (failed(lowerFunctionBody(func)))
          return nullptr;
      }
    }
    return mlirModule;
  }

private:
  bool hasMain = false;
  ::mlir::MLIRContext &context;
  ::mlir::OpBuilder builder;
  DiagnosticEngine &diags;
  TypeLowering typeLowering;

  std::unordered_map<mir::MIRFunction *, ::mlir::func::FuncOp> funcMap;
  std::unordered_map<mir::MIRValue *, ::mlir::Value> valueMap;
  std::unordered_map<mir::MIRBlock *, ::mlir::Block *> blockMap;

  ::mlir::Location getLoc(SourceLocation loc) {
    return builder.getUnknownLoc();
  }

  ::mlir::Type getMLIRType(const hir::HIRType *type) {
    if (!type)
      return builder.getNoneType();
    return typeLowering.lowerHIRType(*type);
  }

  // Helper to correctly map Functions, Globals, and Constants to MLIR
  // Attributes
  ::mlir::Attribute getAttributeForValue(mir::MIRValue *val) {
    if (auto *func = ::llvm::dyn_cast<mir::MIRFunction>(val)) {
      return ::mlir::FlatSymbolRefAttr::get(builder.getContext(),
                                            func->getName());
    }
    if (auto *global = ::llvm::dyn_cast<mir::MIRGlobal>(val)) {
      return ::mlir::FlatSymbolRefAttr::get(builder.getContext(),
                                            global->getName());
    }
    if (auto *constant = ::llvm::dyn_cast<mir::MIRConstant>(val)) {
      return getConstantAttribute(constant);
    }
    return builder.getUnitAttr();
  }

  // Helper to recursively map MIR constants to MLIR Attributes
  ::mlir::Attribute getConstantAttribute(mir::MIRConstant *constant) {
    if (auto *intConst = ::llvm::dyn_cast<mir::ConstantInt>(constant)) {
      ::mlir::Type exactType = getMLIRType(intConst->getType());

      // [FIX] Safely route ConstantInts based on the target MLIR type
      if (exactType.isIntOrIndex()) {
        return builder.getIntegerAttr(exactType, intConst->getValue());
      } else if (::llvm::isa<::mlir::FloatType>(exactType)) {
        return builder.getFloatAttr(exactType,
                                    static_cast<double>(intConst->getValue()));
      } else if (::llvm::isa<::moksha::IR::DecimalType>(exactType)) {
        return builder.getStringAttr(std::to_string(intConst->getValue()));
      } else {
        // Fallback for Pointers, Structs, and Arrays zero-initialized via
        // ConstantInt(0)
        return builder.getUnitAttr();
      }

    } else if (auto *floatConst =
                   ::llvm::dyn_cast<mir::ConstantFloat>(constant)) {
      ::mlir::Type exactType = getMLIRType(floatConst->getType());

      // [FIX] Safely route ConstantFloats
      if (::llvm::isa<::mlir::FloatType>(exactType)) {
        return builder.getFloatAttr(exactType, floatConst->getValue());
      } else if (::llvm::isa<::moksha::IR::DecimalType>(exactType)) {
        return builder.getStringAttr(std::to_string(floatConst->getValue()));
      } else {
        return builder.getUnitAttr();
      }

    } else if (auto *strConst =
                   ::llvm::dyn_cast<mir::ConstantString>(constant)) {
      return builder.getStringAttr(strConst->getValue());
    } else if (auto *boolConst =
                   ::llvm::dyn_cast<mir::ConstantBool>(constant)) {
      return builder.getBoolAttr(boolConst->getValue());
    } else if (auto *decConst =
                   ::llvm::dyn_cast<mir::ConstantDecimal>(constant)) {
      return builder.getStringAttr(decConst->getValue());
    } else if (::llvm::isa<mir::ConstantNull>(constant)) {
      return builder.getUnitAttr();
    } else if (auto *arrConst =
                   ::llvm::dyn_cast<mir::ConstantArray>(constant)) {
      llvm::SmallVector<::mlir::Attribute, 4> elements;
      for (mir::MIRValue *elem : arrConst->getElements()) {
        elements.push_back(getAttributeForValue(elem));
      }
      return builder.getArrayAttr(elements);
    } else if (auto *sliceConst =
                   ::llvm::dyn_cast<mir::ConstantSlice>(constant)) {
      llvm::SmallVector<::mlir::Attribute, 4> elements;
      for (mir::MIRValue *elem : sliceConst->getElements()) {
        elements.push_back(getAttributeForValue(elem));
      }
      return builder.getArrayAttr(elements);
    } else if (auto *mapConst = ::llvm::dyn_cast<mir::ConstantMap>(constant)) {
      llvm::SmallVector<::mlir::Attribute, 4> entries;
      for (const auto &pair : mapConst->getEntries()) {
        llvm::SmallVector<::mlir::Attribute, 2> kv;

        if (auto *kConst = ::llvm::dyn_cast<mir::MIRConstant>(pair.first))
          kv.push_back(getConstantAttribute(kConst));
        else
          kv.push_back(builder.getUnitAttr());

        if (auto *vConst = ::llvm::dyn_cast<mir::MIRConstant>(pair.second))
          kv.push_back(getConstantAttribute(vConst));
        else
          kv.push_back(builder.getUnitAttr());

        entries.push_back(builder.getArrayAttr(kv));
      }
      return builder.getArrayAttr(entries);
    } else if (auto *unionConst =
                   ::llvm::dyn_cast<mir::ConstantUnion>(constant)) {
      if (mir::MIRConstant *activeField = unionConst->getActiveField()) {
        return builder.getArrayAttr({getConstantAttribute(activeField)});
      } else {
        return builder.getUnitAttr();
      }
    } else if (auto *structConst =
                   ::llvm::dyn_cast<mir::ConstantStruct>(constant)) {
      llvm::SmallVector<::mlir::Attribute, 4> elements;
      for (mir::MIRValue *elem : structConst->getFields()) {
        // [FIX] Prevent Infinite Recursion on cyclic struct pointers
        if (elem && elem->getType() &&
            elem->getType()->getKind() == hir::TypeKind::Pointer) {
          if (auto *global = ::llvm::dyn_cast<mir::MIRGlobal>(elem)) {
            elements.push_back(::mlir::FlatSymbolRefAttr::get(
                builder.getContext(), global->getName()));
          } else if (auto *func = ::llvm::dyn_cast<mir::MIRFunction>(elem)) {
            elements.push_back(::mlir::FlatSymbolRefAttr::get(
                builder.getContext(), func->getName()));
          } else {
            // Otherwise, it's an opaque memory pointer. Just emit a null
            // placeholder.
            elements.push_back(builder.getUnitAttr());
          }
        } else {
          elements.push_back(getAttributeForValue(elem));
        }
      }
      return builder.getArrayAttr(elements);
    } else if (auto *bitcast =
                   ::llvm::dyn_cast<mir::ConstantBitCast>(constant)) {
      return getAttributeForValue(bitcast->getValue());
    } else if (auto *anycast =
                   ::llvm::dyn_cast<mir::ConstantAnyCast>(constant)) {
      return getAttributeForValue(anycast->getValue());
    } else if (auto *arrToSlice =
                   ::llvm::dyn_cast<mir::ConstantArrayToSlice>(constant)) {
      return getAttributeForValue(arrToSlice->getValue());
    } else if (auto *sliceToArr =
                   ::llvm::dyn_cast<mir::ConstantSliceToArray>(constant)) {
      return getAttributeForValue(sliceToArr->getValue());
    }

    return builder.getUnitAttr();
  }

  ::mlir::Value getValue(mir::MIRValue *val) {
    if (!val)
      return nullptr;

    bool isInstOrArg = (val->getKind() == mir::ValueKind::Instruction) ||
                       (val->getKind() == mir::ValueKind::Argument);

    if (isInstOrArg && valueMap.find(val) != valueMap.end()) {
      return valueMap[val];
    }

    auto loc = builder.getUnknownLoc();

    if (auto *func = ::llvm::dyn_cast<mir::MIRFunction>(val)) {
      llvm::SmallVector<::mlir::Type, 4> argTypes;
      for (const auto &arg : func->getArguments()) {
        argTypes.push_back(getMLIRType(arg->getType()));
      }

      llvm::SmallVector<::mlir::Type, 1> retTypes;
      if (func->getType() &&
          func->getType()->getKind() != hir::TypeKind::Void) {
        retTypes.push_back(getMLIRType(func->getType()));
      }

      ::mlir::Type funcTy = builder.getFunctionType(argTypes, retTypes);
      ::mlir::Type ptrTy = ::moksha::IR::PointerType::get(&context, funcTy);

      auto addrOp = builder.create<::moksha::IR::AddressOfOp>(
          loc, ptrTy,
          ::mlir::FlatSymbolRefAttr::get(builder.getContext(),
                                         func->getName()));
      return addrOp.getResult();
    }

    if (auto *global = ::llvm::dyn_cast<mir::MIRGlobal>(val)) {
      // --- THE FIX: Only wrap in a PointerType if it isn't already one ---
      ::mlir::Type globalTy = getMLIRType(global->getType());
      ::mlir::Type ptrTy = globalTy;
      if (!::llvm::isa<::moksha::IR::PointerType>(globalTy)) {
        ptrTy = ::moksha::IR::PointerType::get(&context, globalTy);
      }

      auto addrOp = builder.create<::moksha::IR::AddressOfOp>(
          loc, ptrTy,
          ::mlir::FlatSymbolRefAttr::get(builder.getContext(),
                                         global->getName()));
      return addrOp.getResult();
    }

    // 2. Materialize Constants On-Demand
    if (auto *constant = ::llvm::dyn_cast<mir::MIRConstant>(val)) {

      // Handle ConstantBitCast transparently so we don't lose the pointer!
      if (auto *bitcast = ::llvm::dyn_cast<mir::ConstantBitCast>(constant)) {
        ::mlir::Value underlying = getValue(bitcast->getValue());
        if (!underlying)
          return nullptr;
        auto castOp = builder.create<::moksha::IR::BitcastOp>(
            loc, getMLIRType(bitcast->getType()), underlying);
        return castOp.getResult();
      } else if (auto *anycast =
                     ::llvm::dyn_cast<mir::ConstantAnyCast>(constant)) {
        ::mlir::Value underlying = getValue(anycast->getValue());
        if (!underlying)
          return nullptr;
        auto castOp = builder.create<::moksha::IR::CastOp>(
            loc, getMLIRType(anycast->getType()), underlying);
        return castOp.getResult();
      } else if (auto *a2s =
                     ::llvm::dyn_cast<mir::ConstantArrayToSlice>(constant)) {
        ::mlir::Value underlying = getValue(a2s->getValue());
        if (!underlying)
          return nullptr;
        auto castOp = builder.create<::moksha::IR::CastOp>(
            loc, getMLIRType(a2s->getType()), underlying);
        return castOp.getResult();
      } else if (auto *s2a =
                     ::llvm::dyn_cast<mir::ConstantSliceToArray>(constant)) {
        ::mlir::Value underlying = getValue(s2a->getValue());
        if (!underlying)
          return nullptr;
        auto castOp = builder.create<::moksha::IR::CastOp>(
            loc, getMLIRType(s2a->getType()), underlying);
        return castOp.getResult();
      }

      ::mlir::Attribute attr = getConstantAttribute(constant);
      auto constOp = builder.create<::moksha::IR::ConstantOp>(
          loc, getMLIRType(constant->getType()), attr);
      // We don't cache constants anymore!
      return constOp.getResult();
    }
    return nullptr;
  }

  void createFunctionDecl(mir::MIRFunction *func) {
    llvm::SmallVector<::mlir::Type, 4> argTypes;
    bool isSRet = false;
    ::mlir::Type sretType;
    bool isHFA = false; // For Vector4 Float Coercion

    // --- ABI COERCION: Struct Return (sret) ---
    if (func->isDeclaration() && func->getType() &&
        func->getType()->getKind() == hir::TypeKind::Struct) {

      auto &structTy = static_cast<const hir::StructType &>(*func->getType());

      auto getByteSize = [&](const hir::HIRType *t) -> size_t {
        auto impl = [&](auto &self, const hir::HIRType *t_) -> size_t {
          if (!t_)
            return 8;
          if (t_->getKind() == hir::TypeKind::Int)
            return std::max<size_t>(
                1, static_cast<const hir::HIRIntType *>(t_)->getWidth() / 8);
          if (t_->getKind() == hir::TypeKind::Float)
            return std::max<size_t>(
                1, static_cast<const hir::HIRFloatType *>(t_)->getWidth() / 8);
          if (t_->getKind() == hir::TypeKind::Array) {
            auto *arr = static_cast<const hir::ArrayType *>(t_);
            return self(self, arr->getElementType()) * arr->getSize();
          }
          if (t_->getKind() == hir::TypeKind::Struct) {
            size_t size = 0;
            for (auto *f :
                 static_cast<const hir::StructType *>(t_)->getFields())
              size += self(self, f);
            return size;
          }
          if (t_->getKind() == hir::TypeKind::Any ||
              t_->getKind() == hir::TypeKind::Slice ||
              t_->getKind() == hir::TypeKind::Closure ||
              t_->getKind() == hir::TypeKind::Map ||
              t_->getKind() == hir::TypeKind::Decimal)
            return 16;
          return 8;
        };
        return impl(impl, t);
      };

      size_t structSize = getByteSize(&structTy);

      // Windows x64 ABI: Structs larger than 8 bytes are returned via hidden
      // pointer (sret)
      if (structSize > 8) {
        isSRet = true;
        sretType = getMLIRType(func->getType());
        argTypes.push_back(::moksha::IR::PointerType::get(&context, sretType));
      }
    }

    for (const auto &arg : func->getArguments()) {
      ::mlir::Type mlirArgTy = getMLIRType(arg->getType());
      argTypes.push_back(mlirArgTy);
    }

    llvm::SmallVector<::mlir::Type, 1> retTypes;
    if (!isSRet && func->getType() &&
        func->getType()->getKind() != hir::TypeKind::Void) {
      retTypes.push_back(getMLIRType(func->getType()));
    }

    auto funcType = builder.getFunctionType(argTypes, retTypes);
    auto funcOp = builder.create<::mlir::func::FuncOp>(
        builder.getUnknownLoc(), func->getName(), funcType);

    if (!func->isDeclaration() &&
        func->getLinkage() == mir::Linkage::Internal) {
      funcOp.setPrivate();
    }
    if (func->isVariadic())
      funcOp->setAttr("vararg", builder.getUnitAttr());

    // Tag the function so we can debug the IR easily
    if (isSRet) {
      funcOp->setAttr("moksha.sret", ::mlir::TypeAttr::get(sretType));
    }
    if (isHFA) {
      funcOp->setAttr("moksha.coerced_hfa", builder.getUnitAttr());
    }
    if (func->isInterrupt())
      funcOp->setAttr("moksha.interrupt", builder.getUnitAttr());
    if (func->isNaked())
      funcOp->setAttr("moksha.naked", builder.getUnitAttr());
    if (func->isNoReturn())
      funcOp->setAttr("moksha.noreturn", builder.getUnitAttr());
    if (func->isInline())
      funcOp->setAttr("moksha.inline", builder.getUnitAttr());
    if (func->isNoInline())
      funcOp->setAttr("moksha.noinline", builder.getUnitAttr());
    if (func->isPure())
      funcOp->setAttr("moksha.pure", builder.getUnitAttr());
    if (func->isCold())
      funcOp->setAttr("moksha.cold", builder.getUnitAttr());
    if (func->isUsed())
      funcOp->setAttr("moksha.used", builder.getUnitAttr());
    if (!func->getSection().empty()) {
      funcOp->setAttr("moksha.section",
                      builder.getStringAttr(func->getSection()));
    }

    // Map Function Linkage
    std::string fnLinkStr;
    switch (func->getLinkage()) {
    case mir::Linkage::Internal:
      fnLinkStr = "internal";
      break;
    case mir::Linkage::Weak:
      fnLinkStr = "weak";
      break;
    case mir::Linkage::LinkOnce:
      fnLinkStr = "linkonce";
      break;
    default:
      fnLinkStr = "external";
      break;
    }

    llvm::StringRef funcName = func->getName();

    // 1. Struct constructors and destructors are local to the object file
    if (funcName.contains(".constructor") || funcName.contains(".destructor")) {
      fnLinkStr = func->isDeclaration() ? "external" : "internal";
    }
    // 2. Export init/destroy ONLY if this is the main module
    else if (funcName == "__moksha_module_init" ||
             funcName == "__moksha_module_destroy") {
      fnLinkStr = this->hasMain ? "external" : "internal";
    }

    funcOp->setAttr("moksha.linkage", builder.getStringAttr(fnLinkStr));

    // Map Calling Conventions
    if (func->getCallingConv() != mir::CallingConv::C) {
      std::string ccStr;
      switch (func->getCallingConv()) {
      case mir::CallingConv::StdCall:
        ccStr = "stdcall";
        break;
      case mir::CallingConv::FastCall:
        ccStr = "fastcall";
        break;
      case mir::CallingConv::Interrupt:
        ccStr = "interrupt";
        break;
      default:
        ccStr = "cdecl";
        break;
      }
      funcOp->setAttr("moksha.calling_conv", builder.getStringAttr(ccStr));
    }
    funcMap[func] = funcOp;
  }

  llvm::SmallVector<::mlir::Value, 4> getPhiOperands(mir::MIRBlock *source,
                                                     mir::MIRBlock *target) {
    llvm::SmallVector<::mlir::Value, 4> ops;
    for (auto &tInst : target->getInstructions()) {
      if (auto *phi = ::llvm::dyn_cast<mir::PhiInst>(tInst.get())) {
        for (auto &pair : phi->getIncoming()) {
          if (pair.second == source) {
            ::mlir::Value val = getValue(pair.first);
            assert(val && "Phi incoming value not found in valueMap!");
            ops.push_back(val);
            break;
          }
        }
      } else {
        break; // Phis are always strictly at the top of the block
      }
    }
    return ops;
  }

  ::mlir::LogicalResult lowerFunctionBody(mir::MIRFunction *func) {
    auto funcOp = funcMap[func];
    ::mlir::Block *mlirEntry = funcOp.addEntryBlock();

    blockMap.clear();
    valueMap.clear();

    blockMap[func->getEntryBlock()] = mlirEntry;

    // Stabilized Block Layout
    std::vector<mir::MIRBlock *> layout;
    std::unordered_set<mir::MIRBlock *> visited;
    std::queue<mir::MIRBlock *> queue;

    if (func->getEntryBlock()) {
      queue.push(func->getEntryBlock());
      visited.insert(func->getEntryBlock());

      while (!queue.empty()) {
        mir::MIRBlock *b = queue.front();
        queue.pop();
        layout.push_back(b);

        for (auto *succ : b->getSuccessors()) {
          if (visited.find(succ) == visited.end()) {
            visited.insert(succ);
            queue.push(succ);
          }
        }
      }
    }

    // Append any dead/unreachable blocks just in case so they don't get lost
    for (auto &block : func->getBlocks()) {
      if (visited.find(block.get()) == visited.end()) {
        layout.push_back(block.get());
      }
    }

    // 1. Create MLIR Blocks in Topological Order
    for (mir::MIRBlock *block : layout) {
      if (block == func->getEntryBlock())
        continue;

      ::mlir::Block *newBlock = new ::mlir::Block();
      funcOp.getBody().push_back(newBlock);
      blockMap[block] = newBlock;
    }

    // 2. Map Function Arguments
    for (size_t i = 0; i < func->getArguments().size(); ++i) {
      mir::MIRValue *arg = func->getArguments()[i].get();
      valueMap[arg] = mlirEntry->getArgument(i);
    }

    // 3. Create Phi Block Arguments
    for (mir::MIRBlock *block : layout) {
      ::mlir::Block *mlirBlock = blockMap[block];
      if (block == func->getEntryBlock())
        continue;

      for (auto &inst : block->getInstructions()) {
        if (auto *phi = ::llvm::dyn_cast<mir::PhiInst>(inst.get())) {
          ::mlir::Type phiType = getMLIRType(phi->getType());
          ::mlir::Value arg =
              mlirBlock->addArgument(phiType, builder.getUnknownLoc());
          valueMap[phi] = arg;
        } else {
          break; // Phis are always strictly at the start of the block
        }
      }
    }

    // 4. Lower Instructions in Topological Order
    for (mir::MIRBlock *block : layout) {
      builder.setInsertionPointToStart(blockMap[block]);
      for (auto &instPtr : block->getInstructions()) {
        if (failed(lowerInstruction(instPtr.get()))) {
          // ---> ADD THIS DEBUG DUMP <---
          llvm::errs() << "\n[CRITICAL] MLIR Lowering Failed on Instruction:\n";
          instPtr->dump(llvm::errs());
          llvm::errs() << "\nIn Function: " << func->getName() << "\n";
          return ::mlir::failure();
        }
      }
    }

    return ::mlir::success();
  }

  void lowerGlobal(mir::MIRGlobal *global) {
    auto loc = builder.getUnknownLoc();
    auto type = getMLIRType(global->getType());

    ::mlir::Attribute initAttr = nullptr;

    if (mir::MIRConstant *initVal = global->getInitializer()) {
      initAttr = getConstantAttribute(initVal);
    } else {
      initAttr = builder.getUnitAttr();
    }

    auto globalOp = builder.create<::moksha::IR::GlobalOp>(
        loc, builder.getStringAttr(global->getName()),
        ::mlir::TypeAttr::get(type), initAttr);

    // 1. Map Constant Flag
    if (global->isConstant()) {
      globalOp->setAttr("moksha.constant", builder.getUnitAttr());
    }

    // 2. Map Global Linkage
    std::string linkStr;
    switch (global->getLinkage()) {
    case mir::Linkage::Internal:
      linkStr = "internal";
      break;
    case mir::Linkage::Weak:
      linkStr = "weak";
      break;
    case mir::Linkage::LinkOnce:
      linkStr = "linkonce";
      break;
    default:
      linkStr = "external";
      break;
    }

    globalOp->setAttr("moksha.linkage", builder.getStringAttr(linkStr));

    // --- Attach all Global Attributes ---
    if (global->isVolatile())
      globalOp->setAttr("moksha.volatile", builder.getUnitAttr());
    if (global->isThreadLocal())
      globalOp->setAttr("moksha.thread_local", builder.getUnitAttr());
    if (global->isUsed())
      globalOp->setAttr("moksha.used", builder.getUnitAttr());

    if (!global->getSection().empty()) {
      globalOp->setAttr("moksha.section",
                        builder.getStringAttr(global->getSection()));
    }

    if (global->getAlignment() > 0) {
      globalOp->setAttr("moksha.alignment",
                        builder.getI32IntegerAttr(global->getAlignment()));
    }
  }

  ::mlir::LogicalResult lowerInstruction(mir::MIRInst *inst) {
    if (!inst)
      return ::mlir::failure();

    // --- ADDED DEBUG TRACE ---
    // llvm::errs() << "[LOWERING] ";
    // inst->dump(llvm::errs());
    // llvm::errs() << "\n";

    auto loc = builder.getUnknownLoc();
    auto op = inst->getOpcode();

    switch (op) {
      // --- Terminators ---
    case mir::Opcode::Br: {
      auto *br = static_cast<mir::BranchInst *>(inst);
      mir::MIRBlock *target = br->getTarget();
      builder.create<::mlir::cf::BranchOp>(
          loc, blockMap[target], getPhiOperands(inst->getParent(), target));
      break;
    }
    case mir::Opcode::CondBr: {
      auto *cbr = static_cast<mir::CondBranchInst *>(inst);
      ::mlir::Value cond = getValue(cbr->getCondition());
      if (!cond)
        return ::mlir::failure();

      builder.create<::mlir::cf::CondBranchOp>(
          loc, cond, blockMap[cbr->getTrueBlock()],
          getPhiOperands(inst->getParent(), cbr->getTrueBlock()),
          blockMap[cbr->getFalseBlock()],
          getPhiOperands(inst->getParent(), cbr->getFalseBlock()));
      break;
    }
    case mir::Opcode::Switch: {
      auto *swInst = static_cast<mir::SwitchInst *>(inst);
      ::mlir::Value cond = getValue(swInst->getCondition());
      if (!cond)
        return ::mlir::failure();

      ::mlir::Type signlessI32 = builder.getI32Type();
      if (cond.getType() != signlessI32) {
        cond = builder.create<::moksha::IR::CastOp>(loc, signlessI32, cond)
                   .getResult();
      }

      ::mlir::Block *defaultBlock = blockMap[swInst->getDefaultBlock()];
      llvm::SmallVector<int32_t, 4> caseValues;
      llvm::SmallVector<::mlir::Block *, 4> caseDests;

      // Store the operand vectors so their memory stays alive for ValueRange
      llvm::SmallVector<llvm::SmallVector<::mlir::Value, 4>, 4> caseOpsStorage;

      std::unordered_set<int32_t> seenCases;
      for (const auto &c : swInst->getCases()) {
        if (auto *cInt = llvm::dyn_cast<mir::ConstantInt>(c.first)) {
          int32_t val = cInt->getValue();
          if (seenCases.insert(val).second) {
            caseValues.push_back(val);
            caseDests.push_back(blockMap[c.second]);
            caseOpsStorage.push_back(
                getPhiOperands(inst->getParent(), c.second));
          }
        }
      }

      llvm::SmallVector<::mlir::ValueRange, 4> caseOperands;
      for (const auto &ops : caseOpsStorage) {
        caseOperands.push_back(ops);
      }

      builder.create<::mlir::cf::SwitchOp>(
          loc, cond, defaultBlock,
          getPhiOperands(inst->getParent(), swInst->getDefaultBlock()),
          builder.getDenseI32ArrayAttr(caseValues), caseDests, caseOperands);
      break;
    }
    case mir::Opcode::Unreachable: {
      builder.create<::moksha::IR::UnreachableOp>(loc);
      break;
    }
    case mir::Opcode::Return: {
      auto *ret = static_cast<mir::ReturnInst *>(inst);
      if (ret->getReturnValue()) {
        ::mlir::Value val = getValue(ret->getReturnValue());
        if (!val)
          return ::mlir::failure();
        builder.create<::mlir::func::ReturnOp>(loc, val);
      } else {
        builder.create<::mlir::func::ReturnOp>(loc);
      }
      break;
    }
    case mir::Opcode::Resume: {
      auto *resumeInst = static_cast<mir::ResumeInst *>(inst);
      ::mlir::Value val = getValue(resumeInst->getException());
      if (!val)
        return ::mlir::failure();

      builder.create<::moksha::IR::ResumeOp>(loc, val);
      break;
    }

    // --- Memory Ops ---
    case mir::Opcode::Alloca: {
      auto *alloca = static_cast<mir::AllocaInst *>(inst);
      ::mlir::Type elemType = getMLIRType(alloca->getAllocatedType());
      auto mlirAlloca = builder.create<::moksha::IR::AllocaOp>(
          loc, ::moksha::IR::PointerType::get(&context, elemType),
          ::mlir::TypeAttr::get(elemType));
      valueMap[inst] = mlirAlloca.getResult();
      break;
    }
    case mir::Opcode::Load: {
      auto *loadInst = static_cast<mir::LoadInst *>(inst);
      ::mlir::Value ptr = getValue(loadInst->getPointer());
      if (!ptr)
        return ::mlir::failure();
      auto mlirLoad = builder.create<::moksha::IR::LoadOp>(
          loc, getMLIRType(loadInst->getType()), ptr);
      if (loadInst->isVolatile()) {
        mlirLoad->setAttr("moksha.volatile", builder.getUnitAttr());
      }
      valueMap[inst] = mlirLoad.getResult();
      break;
    }
    case mir::Opcode::Store: {
      auto *storeInst = static_cast<mir::StoreInst *>(inst);
      ::mlir::Value val = getValue(storeInst->getValue());
      ::mlir::Value ptr = getValue(storeInst->getPointer());
      if (!val || !ptr)
        return ::mlir::failure();
      auto mlirStore = builder.create<::moksha::IR::StoreOp>(loc, val, ptr);
      if (storeInst->isVolatile()) {
        mlirStore->setAttr("moksha.volatile", builder.getUnitAttr());
      }
      break;
    }
    case mir::Opcode::ExtractValue: {
      auto *extInst = static_cast<mir::ExtractValueInst *>(inst);

      ::mlir::Value aggVal = getValue(extInst->getAggregate());
      if (!aggVal)
        return ::mlir::failure();

      ::mlir::Type resType = typeLowering.lowerHIRType(*extInst->getType());

      // MLIR builder automatically handles converting uint32_t to I32Attr
      auto mlirExt = builder.create<::moksha::IR::ExtractValueOp>(
          loc, resType, aggVal, extInst->getIndex());
      valueMap[inst] = mlirExt.getResult();
      break;
    }

    case mir::Opcode::InsertValue: {
      auto *insInst = static_cast<mir::InsertValueInst *>(inst);

      ::mlir::Value aggVal = getValue(insInst->getAggregate());
      ::mlir::Value insertVal = getValue(insInst->getValue());
      if (!aggVal || !insertVal)
        return ::mlir::failure();

      ::mlir::Type resType = typeLowering.lowerHIRType(*insInst->getType());

      auto mlirIns = builder.create<::moksha::IR::InsertValueOp>(
          loc, resType, aggVal, insertVal, insInst->getIndex());
      valueMap[inst] = mlirIns.getResult();
      break;
    }
    case mir::Opcode::LoadWeak: {
      auto *ldInst = static_cast<mir::LoadWeakInst *>(inst);
      ::mlir::Value ptr = getValue(ldInst->getPointer());
      if (!ptr)
        return ::mlir::failure();

      auto resType = getMLIRType(ldInst->getType());
      auto loadOp = builder.create<::moksha::IR::LoadWeakOp>(loc, resType, ptr);
      valueMap[inst] = loadOp.getResult();
      break;
    }
    case mir::Opcode::StoreWeak: {
      auto *stInst = static_cast<mir::StoreWeakInst *>(inst);
      ::mlir::Value val = getValue(stInst->getValue());
      ::mlir::Value ptr = getValue(stInst->getPointer());
      if (!val || !ptr)
        return ::mlir::failure();

      builder.create<::moksha::IR::StoreWeakOp>(loc, val, ptr);
      break;
    }
    case mir::Opcode::GetElementPtr: {
      auto *gep = static_cast<mir::GetElementPtrInst *>(inst);
      ::mlir::Value basePtr = getValue(gep->getPointer());

      // Safely determine if the base pointer points to a Union
      bool isUnion = false;
      const hir::HIRType *baseTy = gep->getPointer()->getType();
      if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(baseTy)) {
        if (ptrTy->getPointee() &&
            ptrTy->getPointee()->getKind() == hir::TypeKind::Union) {
          isUnion = true;
        }
      }

      ::mlir::Type expectedPtrTy = getMLIRType(gep->getType());

      if (isUnion) {
        // UNION FIX: Do NOT offset the pointer.
        auto castOp =
            builder.create<::moksha::IR::CastOp>(loc, expectedPtrTy, basePtr);
        valueMap[inst] = castOp.getResult();
      } else {
        // STANDARD STRUCT/ARRAY LOGIC
        llvm::SmallVector<::mlir::Value, 2> indices;
        for (auto *idxVal : gep->getIndices()) {
          indices.push_back(getValue(idxVal));
        }
        auto gepOp = builder.create<::moksha::IR::GetElementPtrOp>(
            loc, expectedPtrTy, basePtr, indices);
        valueMap[inst] = gepOp.getResult();
      }
      break;
    }

    // --- Arithmetic ---
    case mir::Opcode::Add:
    case mir::Opcode::Sub:
    case mir::Opcode::Mul:
    case mir::Opcode::Div:
    case mir::Opcode::Mod:
    case mir::Opcode::Pow:
    case mir::Opcode::FAdd:
    case mir::Opcode::FSub:
    case mir::Opcode::FMul:
    case mir::Opcode::FDiv:
    case mir::Opcode::And:
    case mir::Opcode::Or:
    case mir::Opcode::Xor:
    case mir::Opcode::Shl:
    case mir::Opcode::Shr: {
      auto *binInst = static_cast<mir::BinaryInst *>(inst);
      ::mlir::Value lhs = getValue(binInst->getLHS());
      ::mlir::Value rhs = getValue(binInst->getRHS());
      if (!lhs || !rhs)
        return ::mlir::failure();

      ::mlir::Type resTy = getMLIRType(binInst->getType());
      ::mlir::Operation *mlirOp = nullptr;

      if (op == mir::Opcode::Pow) {
        mlirOp = builder.create<::moksha::IR::PowOp>(loc, resTy, lhs, rhs);
      } else if (op == mir::Opcode::Add || op == mir::Opcode::FAdd) {
        mlirOp = builder.create<::moksha::IR::AddOp>(loc, resTy, lhs, rhs);
      } else if (op == mir::Opcode::Sub || op == mir::Opcode::FSub) {
        mlirOp = builder.create<::moksha::IR::SubOp>(loc, resTy, lhs, rhs);
      } else if (op == mir::Opcode::Mul || op == mir::Opcode::FMul) {
        mlirOp = builder.create<::moksha::IR::MulOp>(loc, resTy, lhs, rhs);
      } else if (op == mir::Opcode::Div || op == mir::Opcode::FDiv) {
        mlirOp = builder.create<::moksha::IR::DivOp>(loc, resTy, lhs, rhs);
      } else if (op == mir::Opcode::Mod) {
        mlirOp = builder.create<::moksha::IR::ModOp>(loc, resTy, lhs, rhs);
      } else if (op == mir::Opcode::And) {
        mlirOp = builder.create<::moksha::IR::AndOp>(loc, resTy, lhs, rhs);
      } else if (op == mir::Opcode::Or) {
        mlirOp = builder.create<::moksha::IR::OrOp>(loc, resTy, lhs, rhs);
      } else if (op == mir::Opcode::Xor) {
        mlirOp = builder.create<::moksha::IR::XorOp>(loc, resTy, lhs, rhs);
      } else if (op == mir::Opcode::Shl) {
        mlirOp = builder.create<::moksha::IR::ShlOp>(loc, resTy, lhs, rhs);
      } else if (op == mir::Opcode::Shr) {
        mlirOp = builder.create<::moksha::IR::ShrOp>(loc, resTy, lhs, rhs);
      }

      if (mlirOp)
        valueMap[inst] = mlirOp->getResult(0);
      break;
    }

    // --- Comparisons ---
    case mir::Opcode::ICmp:
    case mir::Opcode::FCmp: {
      auto *cmpInst = static_cast<mir::CompareInst *>(inst);
      ::mlir::Value lhs = getValue(cmpInst->getLHS());
      ::mlir::Value rhs = getValue(cmpInst->getRHS());
      if (!lhs || !rhs)
        return ::mlir::failure();

      ::mlir::Type resTy = getMLIRType(cmpInst->getType());
      auto mlirCmp = builder.create<::moksha::IR::CmpOp>(
          loc, resTy,
          builder.getI32IntegerAttr(
              static_cast<int32_t>(cmpInst->getPredicate())),
          lhs, rhs);
      valueMap[inst] = mlirCmp.getResult();
      break;
    }

    // --- Casts ---
    case mir::Opcode::IntToFloat:
    case mir::Opcode::FloatToInt:
    case mir::Opcode::Trunc:
    case mir::Opcode::SExt:
    case mir::Opcode::ZExt:
    case mir::Opcode::PtrToInt:
    case mir::Opcode::IntToPtr:
    case mir::Opcode::ArrayToSlice:
    case mir::Opcode::SliceToArray: {
      auto *castInst = static_cast<mir::CastInst *>(inst);
      ::mlir::Value val = getValue(castInst->getValue());
      if (!val)
        return ::mlir::failure();

      ::mlir::Type resType = getMLIRType(castInst->getType());
      auto mlirCast = builder.create<::moksha::IR::CastOp>(loc, resType, val);
      valueMap[inst] = mlirCast.getResult();
      break;
    }
    case mir::Opcode::AnyCast: {
      auto *castInst = static_cast<mir::CastInst *>(inst);
      ::mlir::Value val = getValue(castInst->getValue());
      if (!val)
        return ::mlir::failure();

      ::mlir::Type resType = getMLIRType(castInst->getType());
      auto mlirCast =
          builder.create<::moksha::IR::AnyCastOp>(loc, resType, val);
      valueMap[inst] = mlirCast.getResult();
      break;
    }
    case mir::Opcode::BitCast: {
      auto *castInst = static_cast<mir::CastInst *>(inst);
      ::mlir::Value operand = getValue(castInst->getValue());
      if (!operand)
        return ::mlir::failure();

      ::mlir::Type resType = getMLIRType(castInst->getType());

      if (auto prevCast = operand.getDefiningOp<::moksha::IR::BitcastOp>()) {
        // Use ->getOperand(0) to safely fetch the inner value of the MLIR
        // operation
        if (prevCast->getOperand(0).getType() == resType) {
          // The types match perfectly, bypass both casts entirely.
          valueMap[inst] = prevCast->getOperand(0);
          break;
        }
      }

      auto mlirBitCastOp =
          builder.create<::moksha::IR::BitcastOp>(loc, resType, operand);
      valueMap[inst] = mlirBitCastOp.getResult();
      break;
    }
    case mir::Opcode::Upcast: {
      auto *castInst = static_cast<mir::CastInst *>(inst);
      ::mlir::Value operand = getValue(castInst->getValue());
      if (!operand)
        return ::mlir::failure();

      ::mlir::Type resType = getMLIRType(castInst->getType());

      // Fetch the calculated struct offset from your MIRCast instruction
      int32_t byteOffset = castInst->getByteOffset();

      auto mlirUpcast = builder.create<::moksha::IR::UpcastOp>(
          loc, resType, operand, builder.getI32IntegerAttr(byteOffset));

      valueMap[inst] = mlirUpcast.getResult();
      break;
    }

    // --- Call / Function ---
    case mir::Opcode::Call: {
      auto *callInst = static_cast<mir::CallInst *>(inst);

      llvm::SmallVector<::mlir::Value, 4> operands;
      for (auto *arg : callInst->getArgs()) {
        ::mlir::Value v = getValue(arg);
        if (!v)
          return ::mlir::failure();
        operands.push_back(v);
      }

      auto *calleeVal = callInst->getCallee();
      bool isSRet = false;
      ::mlir::Type sretTy;

      if (auto *calleeFunc =
              llvm::dyn_cast_or_null<mir::MIRFunction>(calleeVal)) {
        if (calleeFunc->isDeclaration() && calleeFunc->getType() &&
            calleeFunc->getType()->getKind() == hir::TypeKind::Struct) {

          auto &structTy =
              static_cast<const hir::StructType &>(*calleeFunc->getType());

          auto getByteSize = [&](const hir::HIRType *t) -> size_t {
            auto impl = [&](auto &self, const hir::HIRType *t_) -> size_t {
              if (!t_)
                return 8;
              if (t_->getKind() == hir::TypeKind::Int)
                return std::max<size_t>(
                    1,
                    static_cast<const hir::HIRIntType *>(t_)->getWidth() / 8);
              if (t_->getKind() == hir::TypeKind::Float)
                return std::max<size_t>(
                    1,
                    static_cast<const hir::HIRFloatType *>(t_)->getWidth() / 8);
              if (t_->getKind() == hir::TypeKind::Array) {
                auto *arr = static_cast<const hir::ArrayType *>(t_);
                return self(self, arr->getElementType()) * arr->getSize();
              }
              if (t_->getKind() == hir::TypeKind::Struct) {
                size_t size = 0;
                for (auto *f :
                     static_cast<const hir::StructType *>(t_)->getFields())
                  size += self(self, f);
                return size;
              }
              if (t_->getKind() == hir::TypeKind::Any ||
                  t_->getKind() == hir::TypeKind::Slice ||
                  t_->getKind() == hir::TypeKind::Closure ||
                  t_->getKind() == hir::TypeKind::Map ||
                  t_->getKind() == hir::TypeKind::Decimal)
                return 16;
              return 8;
            };
            return impl(impl, t);
          };

          size_t structSize = getByteSize(&structTy);

          if (structSize > 8) {
            isSRet = true;
            sretTy = getMLIRType(calleeFunc->getType());
          }
        }
      }

      ::mlir::Value sretAlloc;
      if (isSRet) {
        sretAlloc = builder.create<::moksha::IR::AllocaOp>(
            loc, ::moksha::IR::PointerType::get(&context, sretTy),
            ::mlir::TypeAttr::get(sretTy));
        operands.insert(operands.begin(), sretAlloc);
      }

      llvm::SmallVector<::mlir::Type, 1> resTypes;
      if (!isSRet && callInst->getType() &&
          callInst->getType()->getKind() != hir::TypeKind::Void) {
        resTypes.push_back(getMLIRType(callInst->getType()));
      }

      ::mlir::Operation *mlirCallOp = nullptr;

      if (auto *mirFunc = llvm::dyn_cast_or_null<mir::MIRFunction>(calleeVal)) {
        // DIRECT CALL
        mlirCallOp = builder.create<::mlir::func::CallOp>(
            loc, resTypes, builder.getStringAttr(mirFunc->getName()), operands);
        // [FIX] Use isVariadic() instead of isVarArg()
        if (callInst->isVariadic())
          mlirCallOp->setAttr("func.varargs", builder.getUnitAttr());
      } else {
        // INDIRECT CALL (Closures / Function Pointers)
        ::mlir::Value calleeMLIRVal = getValue(calleeVal);
        ::mlir::Type fnType = calleeMLIRVal.getType();

        // Strip the Moksha pointer wrapper to expose the raw FunctionType
        if (auto ptrTy = llvm::dyn_cast<::moksha::IR::PointerType>(fnType)) {
          fnType = ptrTy.getPointee();
          calleeMLIRVal =
              builder.create<::moksha::IR::CastOp>(loc, fnType, calleeMLIRVal)
                  .getResult();
        }

        // [FIX] Use OperationState to bypass missing C++ builder overloads
        ::mlir::OperationState state(
            loc, ::mlir::func::CallIndirectOp::getOperationName());
        state.addTypes(resTypes);
        state.addOperands(calleeMLIRVal); // Callee must be the first operand
        state.addOperands(operands);      // Followed by the arguments

        mlirCallOp = builder.create(state);
      }

      if (isSRet) {
        auto loadOp =
            builder.create<::moksha::IR::LoadOp>(loc, sretTy, sretAlloc);
        valueMap[inst] = loadOp.getResult();
      } else if (!resTypes.empty()) {
        valueMap[inst] = mlirCallOp->getResult(0);
      }
      break;
    }

      // --- ARC ---
    case mir::Opcode::Retain: {
      auto *arc = static_cast<mir::ARCInst *>(inst);
      ::mlir::Value obj = getValue(arc->getObject());
      if (!obj)
        return ::mlir::failure();
      builder.create<::moksha::IR::RetainOp>(loc, obj);
      break;
    }
    case mir::Opcode::Release: {
      auto *arc = static_cast<mir::ARCInst *>(inst);
      ::mlir::Value obj = getValue(arc->getObject());
      if (!obj)
        return ::mlir::failure();

      ::mlir::FlatSymbolRefAttr dropAttr = nullptr;
      if (arc->getDropFunc()) {
        dropAttr = ::mlir::FlatSymbolRefAttr::get(
            builder.getContext(), arc->getDropFunc()->getName());
      }

      builder.create<::moksha::IR::ReleaseOp>(loc, obj, dropAttr);
      break;
    }

    case mir::Opcode::Phi: {
      // Typically handled in a separate pass block population step
      break;
    }

      // --- Closures ---
    case mir::Opcode::MakeClosure: {
      auto *mc = static_cast<mir::MakeClosureInst *>(inst);

      // 1. Get the target function symbol
      auto *func = llvm::dyn_cast<mir::MIRFunction>(mc->getFunctionPointer());
      if (!func)
        return ::mlir::failure();
      auto symRef =
          ::mlir::FlatSymbolRefAttr::get(builder.getContext(), func->getName());

      // 2. Resolve all captured variables
      llvm::SmallVector<::mlir::Value, 4> captures;
      for (auto *cap : mc->getCaptures()) {
        ::mlir::Value capVal = getValue(cap);
        if (!capVal)
          return ::mlir::failure();
        captures.push_back(capVal);
      }

      // 3. Create the MLIR operation
      ::mlir::Type resType = getMLIRType(mc->getType());
      auto makeClosureOp = builder.create<::moksha::IR::MakeClosureOp>(
          loc, resType, symRef, captures);

      valueMap[inst] = makeClosureOp.getResult();
      break;
    }

      // --- Atomics & Fences ---
    case mir::Opcode::Fence: {
      auto *fence = static_cast<mir::FenceInst *>(inst);
      builder.create<::moksha::IR::FenceOp>(
          loc,
          builder.getI32IntegerAttr(static_cast<int32_t>(fence->getOrder())));
      break;
    }

    case mir::Opcode::AtomicRMW: {
      auto *rmw = static_cast<mir::AtomicRMWInst *>(inst);
      ::mlir::Value ptr = getValue(rmw->getPointer());
      ::mlir::Value val = getValue(rmw->getValue());
      if (!ptr || !val)
        return ::mlir::failure();

      auto op = builder.create<::moksha::IR::AtomicRMWOp>(
          loc, getMLIRType(rmw->getType()),
          builder.getI32IntegerAttr(static_cast<int32_t>(rmw->getAtomicOp())),
          ptr, val,
          builder.getI32IntegerAttr(static_cast<int32_t>(rmw->getOrder())));
      valueMap[inst] = op.getResult();
      break;
    }

    case mir::Opcode::AtomicCmpXchg: {
      auto *cas = static_cast<mir::AtomicCmpXchgInst *>(inst);
      ::mlir::Value ptr = getValue(cas->getPointer());
      ::mlir::Value expected = getValue(cas->getExpected());
      ::mlir::Value desired = getValue(cas->getDesired());
      if (!ptr || !expected || !desired)
        return ::mlir::failure();

      auto op = builder.create<::moksha::IR::AtomicCmpXchgOp>(
          loc,
          getMLIRType(cas->getType()), // returns the old value
          ptr, expected, desired,
          builder.getI32IntegerAttr(
              static_cast<int32_t>(cas->getSuccessOrder())),
          builder.getI32IntegerAttr(
              static_cast<int32_t>(cas->getFailureOrder())));
      valueMap[inst] = op.getResult();
      break;
    }

    case mir::Opcode::AtomicLoad: {
      auto *load = static_cast<mir::AtomicLoadInst *>(inst);
      ::mlir::Value ptr = getValue(load->getPointer());
      if (!ptr)
        return ::mlir::failure();

      auto op = builder.create<::moksha::IR::AtomicLoadOp>(
          loc, getMLIRType(load->getType()), ptr,
          builder.getI32IntegerAttr(static_cast<int32_t>(load->getOrder())));
      valueMap[inst] = op.getResult();
      break;
    }

    case mir::Opcode::AtomicStore: {
      auto *store = static_cast<mir::AtomicStoreInst *>(inst);
      ::mlir::Value val = getValue(store->getValue());
      ::mlir::Value ptr = getValue(store->getPointer());
      if (!val || !ptr)
        return ::mlir::failure();

      builder.create<::moksha::IR::AtomicStoreOp>(
          loc, val, ptr,
          builder.getI32IntegerAttr(static_cast<int32_t>(store->getOrder())));
      break;
    }

    // --- Concurrency ---
    case mir::Opcode::Spawn: {
      auto *spawnInst = static_cast<mir::SpawnInst *>(inst);
      ::mlir::Value closure = getValue(spawnInst->getClosure());
      if (!closure)
        return ::mlir::failure();

      ::mlir::Type resType = getMLIRType(spawnInst->getType());

      auto threadKindAttr = builder.getI32IntegerAttr(
          static_cast<int32_t>(spawnInst->getThreadKind()));

      auto spawnOp = builder.create<::moksha::IR::SpawnOp>(
          loc, resType, closure, threadKindAttr);

      valueMap[inst] = spawnOp.getResult();
      break;
    }

    case mir::Opcode::Await: {
      auto *awaitInst = static_cast<mir::AwaitInst *>(inst);
      ::mlir::Value promise = getValue(awaitInst->getPromise());
      if (!promise)
        return ::mlir::failure();

      ::mlir::Type resType = getMLIRType(awaitInst->getType());
      auto awaitOp =
          builder.create<::moksha::IR::AwaitOp>(loc, resType, promise);

      valueMap[inst] = awaitOp.getResult();
      break;
    }

    case mir::Opcode::Throw: {
      auto *throwInst = static_cast<mir::ThrowInst *>(inst);

      // 1. Get the exception value
      ::mlir::Value val = getValue(throwInst->getException());
      if (!val) {
        return ::mlir::failure();
      }

      // 2. Resolve the destination block for the CFG
      ::mlir::Block *unwindBlock = nullptr;
      if (mir::MIRBlock *dest = throwInst->getUnwindDest()) {
        unwindBlock = blockMap[dest];
      }

      // 3. Create the MLIR Terminator
      if (unwindBlock) {
        builder.create<::moksha::IR::ThrowOp>(loc, val, unwindBlock);
      } else {
        builder.create<::moksha::IR::ThrowOp>(loc, val, ::mlir::BlockRange{});
      }

      break;
    }

    case mir::Opcode::Invoke: {
      auto *invInst = static_cast<mir::InvokeInst *>(inst);

      llvm::SmallVector<::mlir::Value, 4> operands;
      for (auto *arg : invInst->getArgs()) {
        operands.push_back(getValue(arg));
      }

      ::mlir::Block *normalDest = blockMap[invInst->getNormalDest()];
      ::mlir::Block *unwindDest = blockMap[invInst->getUnwindDest()];

      // --- [FIX] Add sret ABI logic for Invoke ---
      bool isSRet = false;
      ::mlir::Type sretTy;
      mir::MIRValue *calleeVal = invInst->getCallee();

      if (auto *calleeFunc =
              llvm::dyn_cast_or_null<mir::MIRFunction>(calleeVal)) {
        if (calleeFunc->isDeclaration() && calleeFunc->getType() &&
            calleeFunc->getType()->getKind() == hir::TypeKind::Struct) {
          auto &structTy =
              static_cast<const hir::StructType &>(*calleeFunc->getType());

          auto getByteSize = [&](const hir::HIRType *t) -> size_t {
            auto impl = [&](auto &self, const hir::HIRType *t_) -> size_t {
              if (!t_)
                return 8;
              if (t_->getKind() == hir::TypeKind::Int)
                return std::max<size_t>(
                    1,
                    static_cast<const hir::HIRIntType *>(t_)->getWidth() / 8);
              if (t_->getKind() == hir::TypeKind::Float)
                return std::max<size_t>(
                    1,
                    static_cast<const hir::HIRFloatType *>(t_)->getWidth() / 8);
              if (t_->getKind() == hir::TypeKind::Array) {
                auto *arr = static_cast<const hir::ArrayType *>(t_);
                return self(self, arr->getElementType()) * arr->getSize();
              }
              if (t_->getKind() == hir::TypeKind::Struct) {
                size_t size = 0;
                for (auto *f :
                     static_cast<const hir::StructType *>(t_)->getFields())
                  size += self(self, f);
                return size;
              }
              if (t_->getKind() == hir::TypeKind::Any ||
                  t_->getKind() == hir::TypeKind::Slice ||
                  t_->getKind() == hir::TypeKind::Closure ||
                  t_->getKind() == hir::TypeKind::Map ||
                  t_->getKind() == hir::TypeKind::Decimal)
                return 16;
              return 8;
            };
            return impl(impl, t);
          };

          size_t structSize = getByteSize(&structTy);
          if (structSize > 8) {
            isSRet = true;
            sretTy = getMLIRType(calleeFunc->getType());
          }
        }
      }

      ::mlir::Value sretAlloc;
      if (isSRet) {
        sretAlloc = builder.create<::moksha::IR::AllocaOp>(
            loc, ::moksha::IR::PointerType::get(&context, sretTy),
            ::mlir::TypeAttr::get(sretTy));
        operands.insert(operands.begin(), sretAlloc);
      }

      llvm::SmallVector<::mlir::Type, 1> resTypes;
      if (!isSRet && invInst->getType() &&
          invInst->getType()->getKind() != hir::TypeKind::Void) {
        resTypes.push_back(getMLIRType(invInst->getType()));
      }

      ::mlir::Operation *invokeOp = nullptr;

      if (auto *mirFunc = llvm::dyn_cast_or_null<mir::MIRFunction>(calleeVal)) {
        // DIRECT INVOKE
        invokeOp = builder.create<::moksha::IR::InvokeOp>(
            loc, resTypes, builder.getStringAttr(mirFunc->getName()), operands,
            normalDest, unwindDest);
      } else {
        // INDIRECT INVOKE (Closures / Function Pointers)
        ::mlir::Value calleeMLIRVal = getValue(calleeVal);
        ::mlir::Type fnType = calleeMLIRVal.getType();

        // [FIX] Use llvm::dyn_cast instead of mlir::dyn_cast
        if (auto ptrTy = llvm::dyn_cast<::moksha::IR::PointerType>(fnType)) {
          fnType = ptrTy.getPointee();
          calleeMLIRVal =
              builder.create<::moksha::IR::CastOp>(loc, fnType, calleeMLIRVal)
                  .getResult();
        }

        // Use our brand new dialect operation!
        invokeOp = builder.create<::moksha::IR::InvokeIndirectOp>(
            loc, resTypes, calleeMLIRVal, operands, normalDest, unwindDest);
      }

      if (isSRet) {
        auto loadOp =
            builder.create<::moksha::IR::LoadOp>(loc, sretTy, sretAlloc);
        valueMap[inst] = loadOp.getResult();
      } else if (!resTypes.empty()) {
        valueMap[inst] = invokeOp->getResult(0);
      }
      break;
    }

    case mir::Opcode::LandingPad: {
      auto *lpadInst = static_cast<mir::LandingPadInst *>(inst);
      ::mlir::Type resType = getMLIRType(lpadInst->getType());

      // 1. Convert HIR catch types to MLIR TypeAttrs
      llvm::SmallVector<::mlir::Attribute, 4> catchAttrs;
      for (const auto *catchTy : lpadInst->getCatchTypes()) {
        if (catchTy) {
          ::mlir::Type mlirTy = getMLIRType(catchTy);
          catchAttrs.push_back(::mlir::TypeAttr::get(mlirTy));
        }
      }

      // 2. Determine if it's a cleanup pad (no catches = cleanup)
      ::mlir::UnitAttr cleanupAttr =
          catchAttrs.empty() ? builder.getUnitAttr() : nullptr;
      ::mlir::ArrayAttr catchArrayAttr = builder.getArrayAttr(catchAttrs);

      // 3. Create the MLIR Operation with the new attributes
      auto lpadOp = builder.create<::moksha::IR::LandingPadOp>(
          loc, resType, cleanupAttr, catchArrayAttr);

      // Store it in the map so the subsequent StoreInst can find it!
      valueMap[inst] = lpadOp.getResult();
      break;
    }

    // --- Inline Assembly ---
    case mir::Opcode::InlineAsm: {
      auto *asmInst = static_cast<mir::InlineAsmInst *>(inst);

      llvm::SmallVector<::mlir::Value, 4> operands;
      for (auto *arg : asmInst->getArgs()) {
        ::mlir::Value v = getValue(arg);
        if (!v)
          return ::mlir::failure();
        operands.push_back(v);
      }

      llvm::SmallVector<::mlir::Type, 1> resTypes;
      if (asmInst->getType() &&
          asmInst->getType()->getKind() != hir::TypeKind::Void) {
        resTypes.push_back(getMLIRType(asmInst->getType()));
      }

      auto mlirAsmOp = builder.create<::moksha::IR::InlineAsmOp>(
          loc, resTypes, builder.getStringAttr(asmInst->getAsmString()),
          builder.getStringAttr(asmInst->getConstraints()),
          asmInst->getIsVolatile() ? builder.getUnitAttr() : nullptr, operands);

      if (!resTypes.empty()) {
        valueMap[inst] = mlirAsmOp.getResult(0);
      }
      break;
    }

    case mir::Opcode::MakeShared: {
      auto *sharedInst = static_cast<mir::MakeSharedInst *>(inst);

      // Get the value we are putting on the heap
      ::mlir::Value operand = getValue(sharedInst->getOperand());
      if (!operand)
        return ::mlir::failure();

      // Lower the resulting reference type
      ::mlir::Type resType = getMLIRType(sharedInst->getType());

      // Emit the MLIR operation
      auto mlirSharedOp =
          builder.create<::moksha::IR::MakeSharedOp>(loc, resType, operand);

      // Register it in the SSA value map
      valueMap[inst] = mlirSharedOp.getResult();
      break;
    }

    default:
      break;
    }

    return ::mlir::success();
  }
};

} // namespace

::mlir::OwningOpRef<::mlir::ModuleOp>
convertMIRToMLIR(mir::MIRModule &mirModule, ::mlir::MLIRContext &context,
                 DiagnosticEngine &diags) {
  MIRToMLIRConverter converter(context, diags);
  return converter.convert(mirModule);
}

} // namespace mlir
} // namespace backend
} // namespace moksha
