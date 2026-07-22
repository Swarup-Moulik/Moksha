#pragma once

#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRGlobal.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"

#include <cassert>

namespace moksha {
namespace mir {

/** @brief Base class for all MIR visitors. */
class MIRVisitor {
public:
  virtual ~MIRVisitor() = default;

  virtual void visitModule(MIRModule *module) {
    if (!module)
      return;

    preVisitModule(module);
    traverseGlobals(module);
    traverseFunctions(module);
    postVisitModule(module);
  }

  virtual void visitGlobal(MIRGlobal *global) {}

  virtual void visitFunction(MIRFunction *func) {
    if (!func)
      return;

    preVisitFunction(func);
    for (auto &block : func->getBlocks()) {
      visitBlock(block.get());
    }
    postVisitFunction(func);
  }

  virtual void visitBlock(MIRBlock *block) {
    if (!block)
      return;

    preVisitBlock(block);
    for (auto &inst : block->getInstructions()) {
      visit(inst.get());
    }
    postVisitBlock(block);
  }

  // Hooks
  virtual void preVisitModule(MIRModule *) {}
  virtual void postVisitModule(MIRModule *) {}
  virtual void preVisitFunction(MIRFunction *) {}
  virtual void postVisitFunction(MIRFunction *) {}
  virtual void preVisitBlock(MIRBlock *) {}
  virtual void postVisitBlock(MIRBlock *) {}

  // Instruction Dispatcher
  void visit(MIRInst *inst) {
    if (!inst)
      return;

    switch (inst->getOpcode()) {
    // Terminators
    case Opcode::Br:
      visitBranchInst(static_cast<BranchInst *>(inst));
      break;
    case Opcode::CondBr:
      visitCondBranchInst(static_cast<CondBranchInst *>(inst));
      break;
    case Opcode::Return:
      visitReturnInst(static_cast<ReturnInst *>(inst));
      break;
    case Opcode::Switch:
      visitSwitchInst(static_cast<SwitchInst *>(inst));
      break;
    case Opcode::Unreachable:
      visitUnreachableInst(static_cast<UnreachableInst *>(inst));
      break;

    // Memory
    case Opcode::Alloca:
      visitAllocaInst(static_cast<AllocaInst *>(inst));
      break;
    case Opcode::Load:
      visitLoadInst(static_cast<LoadInst *>(inst));
      break;
    case Opcode::Store:
      visitStoreInst(static_cast<StoreInst *>(inst));
      break;
    case Opcode::GetElementPtr:
      visitGetElementPtrInst(static_cast<GetElementPtrInst *>(inst));
      break;

    // Arithmetic
    case Opcode::Add:
    case Opcode::Sub:
    case Opcode::Mul:
    case Opcode::Div:
    case Opcode::Mod:
    case Opcode::Pow:
    case Opcode::FAdd:
    case Opcode::FSub:
    case Opcode::FMul:
    case Opcode::FDiv:
    case Opcode::And:
    case Opcode::Or:
    case Opcode::Xor:
    case Opcode::Shl:
    case Opcode::Shr:
      visitBinaryInst(static_cast<BinaryInst *>(inst));
      break;

    // Comparison
    case Opcode::ICmp:
    case Opcode::FCmp:
      visitCompareInst(static_cast<CompareInst *>(inst));
      break;

    // Casts
    case Opcode::BitCast:
    case Opcode::IntToFloat:
    case Opcode::FloatToInt:
    case Opcode::ZExt:
    case Opcode::SExt:
    case Opcode::Trunc:
    case Opcode::PtrToInt:
    case Opcode::IntToPtr:
    case Opcode::AnyCast:
    case Opcode::ArrayToSlice:
    case Opcode::SliceToArray:
    case Opcode::Upcast:
      visitCastInst(static_cast<CastInst *>(inst));
      break;

    // Others
    case Opcode::Call:
      visitCallInst(static_cast<CallInst *>(inst));
      break;
    case Opcode::Phi:
      visitPhiInst(static_cast<PhiInst *>(inst));
      break;
    case Opcode::Retain:
    case Opcode::Release:
      visitARCInst(static_cast<ARCInst *>(inst));
      break;
    case Opcode::StoreWeak:
      visitStoreWeakInst(static_cast<StoreWeakInst *>(inst));
      break;
    case Opcode::LoadWeak:
      visitLoadWeakInst(static_cast<LoadWeakInst *>(inst));
      break;
    case Opcode::InsertValue:
      visitInsertValueInst(static_cast<InsertValueInst *>(inst));
      break;
    case Opcode::ExtractValue:
      visitExtractValueInst(static_cast<ExtractValueInst *>(inst));
      break;

      // Exceptions & Stack Unwinding
    case Opcode::Invoke:
      visitInvokeInst(static_cast<InvokeInst *>(inst));
      break;
    case Opcode::LandingPad:
      visitLandingPadInst(static_cast<LandingPadInst *>(inst));
      break;
    case Opcode::Resume:
      visitResumeInst(static_cast<ResumeInst *>(inst));
      break;
    case Opcode::Throw:
      visitThrowInst(static_cast<ThrowInst *>(inst));
      break;

    // Inline Assembly
    case Opcode::InlineAsm:
      visitInlineAsmInst(static_cast<InlineAsmInst *>(inst));
      break;

      // Concurrency & Async
    case Opcode::MakeClosure:
      visitMakeClosureInst(static_cast<MakeClosureInst *>(inst));
      break;
    case Opcode::Spawn:
      visitSpawnInst(static_cast<SpawnInst *>(inst));
      break;
    case Opcode::Await:
      visitAwaitInst(static_cast<AwaitInst *>(inst));
      break;

    // Atomics
    case Opcode::AtomicLoad:
      visitAtomicLoadInst(static_cast<AtomicLoadInst *>(inst));
      break;
    case Opcode::AtomicStore:
      visitAtomicStoreInst(static_cast<AtomicStoreInst *>(inst));
      break;
    case Opcode::AtomicRMW:
      visitAtomicRMWInst(static_cast<AtomicRMWInst *>(inst));
      break;
    case Opcode::AtomicCmpXchg:
      visitAtomicCmpXchgInst(static_cast<AtomicCmpXchgInst *>(inst));
      break;
    case Opcode::Fence:
      visitFenceInst(static_cast<FenceInst *>(inst));
      break;

    default:
      visitInstruction(inst);
      break;
    }
  }

  virtual void visitInstruction(MIRInst *) {}
  virtual void visitBranchInst(BranchInst *inst) { visitInstruction(inst); }
  virtual void visitCondBranchInst(CondBranchInst *inst) {
    visitInstruction(inst);
  }
  virtual void visitReturnInst(ReturnInst *inst) { visitInstruction(inst); }
  virtual void visitSwitchInst(SwitchInst *inst) { visitInstruction(inst); }
  virtual void visitUnreachableInst(UnreachableInst *inst) {
    visitInstruction(inst);
  }
  virtual void visitAllocaInst(AllocaInst *inst) { visitInstruction(inst); }
  virtual void visitLoadInst(LoadInst *inst) { visitInstruction(inst); }
  virtual void visitStoreInst(StoreInst *inst) { visitInstruction(inst); }
  virtual void visitGetElementPtrInst(GetElementPtrInst *inst) {
    visitInstruction(inst);
  }
  virtual void visitBinaryInst(BinaryInst *inst) { visitInstruction(inst); }
  virtual void visitCompareInst(CompareInst *inst) { visitInstruction(inst); }
  virtual void visitCastInst(CastInst *inst) { visitInstruction(inst); }
  virtual void visitCallInst(CallInst *inst) { visitInstruction(inst); }
  virtual void visitPhiInst(PhiInst *inst) { visitInstruction(inst); }
  virtual void visitARCInst(ARCInst *inst) { visitInstruction(inst); }
  virtual void visitStoreWeakInst(StoreWeakInst *inst) {
    visitInstruction(inst);
  }
  virtual void visitLoadWeakInst(LoadWeakInst *inst) { visitInstruction(inst); }
  virtual void visitInsertValueInst(InsertValueInst *inst) {
    visitInstruction(inst);
  }
  virtual void visitExtractValueInst(ExtractValueInst *inst) {
    visitInstruction(inst);
  }
  virtual void visitInvokeInst(InvokeInst *inst) { visitInstruction(inst); }
  virtual void visitLandingPadInst(LandingPadInst *inst) {
    visitInstruction(inst);
  }
  virtual void visitResumeInst(ResumeInst *inst) { visitInstruction(inst); }
  virtual void visitThrowInst(ThrowInst *inst) { visitInstruction(inst); }
  virtual void visitInlineAsmInst(InlineAsmInst *inst) {
    visitInstruction(inst);
  }
  virtual void visitMakeClosureInst(MakeClosureInst *inst) {
    visitInstruction(inst);
  }
  virtual void visitMakeSharedInst(MakeSharedInst *inst) {
    visitInstruction(inst);
  }
  virtual void visitSpawnInst(SpawnInst *inst) { visitInstruction(inst); }
  virtual void visitAwaitInst(AwaitInst *inst) { visitInstruction(inst); }

  virtual void visitAtomicLoadInst(AtomicLoadInst *inst) {
    visitInstruction(inst);
  }
  virtual void visitAtomicStoreInst(AtomicStoreInst *inst) {
    visitInstruction(inst);
  }
  virtual void visitAtomicRMWInst(AtomicRMWInst *inst) {
    visitInstruction(inst);
  }
  virtual void visitAtomicCmpXchgInst(AtomicCmpXchgInst *inst) {
    visitInstruction(inst);
  }
  virtual void visitFenceInst(FenceInst *inst) { visitInstruction(inst); }

protected:
  virtual void traverseGlobals(MIRModule *module) {
    for (auto &global : module->getGlobals()) {
      visitGlobal(global.get());
    }
  }

  virtual void traverseFunctions(MIRModule *module) {
    for (auto &func : module->getFunctions()) {
      visitFunction(func.get());
    }
  }
};

} // namespace mir
} // namespace moksha
