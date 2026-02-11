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
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "moksha/Support/Diagnostics.h"
#include <unordered_map>

namespace moksha {
namespace backend {
namespace mlir {

namespace {

class MIRToMLIRConverter {
public:
  MIRToMLIRConverter(::mlir::MLIRContext &context, DiagnosticEngine &diags)
      : context(context), builder(&context), diags(diags),
        typeLowering(context) {
    context.getOrLoadDialect<::moksha::MokshaDialect>();
    context.getOrLoadDialect<::mlir::func::FuncDialect>();
    context.getOrLoadDialect<::mlir::cf::ControlFlowDialect>();
  }

  ::mlir::OwningOpRef<::mlir::ModuleOp> convert(mir::MIRModule &mirModule) {
    auto moduleLoc = builder.getUnknownLoc();
    ::mlir::ModuleOp mlirModule = ::mlir::ModuleOp::create(moduleLoc);
    builder.setInsertionPointToStart(mlirModule.getBody());

    for (auto &func : mirModule.getFunctions()) {
      createFunctionDecl(func.get());
    }

    for (auto &func : mirModule.getFunctions()) {
      if (!func->isDeclaration()) {
        if (failed(lowerFunctionBody(func.get())))
          return nullptr;
      }
    }
    return mlirModule;
  }

private:
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

  // [FIX] Helper to lower types using TypeLowering
  ::mlir::Type getMLIRType(const hir::HIRType *type) {
    if (!type)
      return builder.getNoneType();
    return typeLowering.lowerHIRType(*type);
  }

  ::mlir::LogicalResult checkUse(mir::MIRValue *val) {
    if (valueMap.find(val) == valueMap.end())
      return ::mlir::failure();
    return ::mlir::success();
  }

  void createFunctionDecl(mir::MIRFunction *func) {
    llvm::SmallVector<::mlir::Type, 4> argTypes;
    for (const auto &arg : func->getArguments()) {
      argTypes.push_back(getMLIRType(arg->getType()));
    }

    llvm::SmallVector<::mlir::Type, 1> retTypes;
    if (func->getType()) {
      retTypes.push_back(getMLIRType(func->getType()));
    }

    auto funcType = builder.getFunctionType(argTypes, retTypes);
    auto funcOp = builder.create<::mlir::func::FuncOp>(
        builder.getUnknownLoc(), func->getName(), funcType);
    funcMap[func] = funcOp;
  }

  ::mlir::LogicalResult lowerFunctionBody(mir::MIRFunction *func) {
    auto funcOp = funcMap[func];
    ::mlir::Block *mlirEntry = funcOp.addEntryBlock();

    blockMap.clear();
    valueMap.clear();

    blockMap[func->getEntryBlock()] = mlirEntry;

    for (auto &block : func->getBlocks()) {
      if (block.get() == func->getEntryBlock())
        continue;
      blockMap[block.get()] = funcOp.addBlock();
    }

    for (size_t i = 0; i < func->getArguments().size(); ++i) {
      mir::MIRValue *arg = func->getArguments()[i].get();
      valueMap[arg] = mlirEntry->getArgument(i);
    }

    for (auto &block : func->getBlocks()) {
      builder.setInsertionPointToStart(blockMap[block.get()]);
      for (auto &instPtr : block->getInstructions()) {
        if (failed(lowerInstruction(instPtr.get())))
          return ::mlir::failure();
      }
    }
    return ::mlir::success();
  }

  ::mlir::LogicalResult lowerInstruction(mir::MIRInst *inst) {
    auto loc = getLoc(inst->getLoc());

    switch (inst->getOpcode()) {
    case mir::Opcode::Add: {
      auto *bin = static_cast<mir::BinaryInst *>(inst);
      if (failed(checkUse(bin->getLHS())) || failed(checkUse(bin->getRHS())))
        return ::mlir::failure();

      // [FIX] Explicitly pass result type
      valueMap[inst] = builder.create<::moksha::AddOp>(
          loc, valueMap[bin->getLHS()].getType(), valueMap[bin->getLHS()],
          valueMap[bin->getRHS()]);
      break;
    }
    case mir::Opcode::Alloca: {
      auto *alloca = static_cast<mir::AllocaInst *>(inst);
      // [FIX] Use getMLIRType on HIRType
      auto ptrType = getMLIRType(alloca->getType());
      valueMap[inst] = builder.create<::moksha::AllocaOp>(
          loc, ptrType,
          ::mlir::TypeAttr::get(getMLIRType(alloca->getAllocatedType())));
      break;
    }
    case mir::Opcode::Load: {
      auto *load = static_cast<mir::LoadInst *>(inst);
      if (failed(checkUse(load->getPointer())))
        return ::mlir::failure();

      // [FIX] Explicitly pass result type
      valueMap[inst] = builder.create<::moksha::LoadOp>(
          loc, getMLIRType(load->getType()), valueMap[load->getPointer()]);
      break;
    }
    case mir::Opcode::Store: {
      auto *store = static_cast<mir::StoreInst *>(inst);
      if (failed(checkUse(store->getValue())))
        return ::mlir::failure();
      builder.create<::moksha::StoreOp>(loc, valueMap[store->getValue()],
                                        valueMap[store->getPointer()]);
      break;
    }
    case mir::Opcode::Return: {
      auto *ret = static_cast<mir::ReturnInst *>(inst);

      // [FIX 1] Use getReturnValue() to check for void vs non-void
      if (auto *val = ret->getReturnValue()) {
        builder.create<::mlir::func::ReturnOp>(loc, valueMap[val]);
      } else {
        builder.create<::mlir::func::ReturnOp>(loc);
      }
      break;
    }

    // [FIX 2] Ensure Opcode is 'Br' (not Branch)
    case mir::Opcode::Br: {
      auto *br = static_cast<mir::BranchInst *>(inst);
      builder.create<::mlir::cf::BranchOp>(loc, blockMap[br->getTarget()]);
      break;
    }

    // [FIX 3] Ensure Opcode is 'CondBr' (not CondBranch)
    case mir::Opcode::CondBr: {
      auto *br = static_cast<mir::CondBranchInst *>(inst);
      builder.create<::mlir::cf::CondBranchOp>(
          loc, valueMap[br->getCondition()], blockMap[br->getTrueBlock()],
          blockMap[br->getFalseBlock()]);
      break;
    }

    case mir::Opcode::Retain:
    case mir::Opcode::Release: {
      auto *arc = static_cast<mir::ARCInst *>(inst);

      // [FIX 4] Use getObject() instead of getOperand(0)
      if (inst->getOpcode() == mir::Opcode::Retain)
        builder.create<::moksha::RetainOp>(loc, valueMap[arc->getObject()]);
      else
        builder.create<::moksha::ReleaseOp>(loc, valueMap[arc->getObject()]);
      break;
    }
    default:
      return ::mlir::failure();
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
