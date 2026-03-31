#include "moksha/MIR/Passes/InliningPass.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRArgument.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "llvm/Support/Casting.h"
#include <unordered_map>
#include <vector>

namespace moksha {
namespace mir {

// Helper to replace all uses of the old call with the inlined return value
static void replaceAllUsesInFunction(MIRFunction *F, MIRValue *oldVal,
                                     MIRValue *newVal) {
  for (auto &blockPtr : F->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructionsMut()) {
      instPtr->replaceOperand(oldVal, newVal);
    }
  }
}

bool InliningPass::runOnModule(MIRModule &M) {
  bool changed = false;
  for (auto &func : M.getFunctions()) {
    changed |= runOnFunction(func.get(), M);
  }
  return changed;
}

bool InliningPass::shouldInline(MIRFunction *callee) {
  if (!callee || callee->isDeclaration())
    return false;

  // [FIX] Respect explicit attributes and calling conventions!
  if (callee->isNoInline() || callee->isInterrupt() || callee->isNaked()) {
    return false;
  }

  // Heuristic: Only inline simple functions
  size_t instCount = 0;
  for (auto &block : callee->getBlocks()) {
    instCount += block->getInstructions().size();

    for (auto &inst : block->getInstructions()) {
      if (inst->getOpcode() == Opcode::Throw ||
          inst->getOpcode() == Opcode::LandingPad ||
          inst->getOpcode() == Opcode::Invoke) {
        return false;
      }
    }
  }

  // Set an arbitrary limit (e.g., 20 instructions)
  return instCount < 20;
}

bool InliningPass::runOnFunction(MIRFunction *F, MIRModule &M) {
  if (F->isDeclaration())
    return false;

  bool changed = false;
  bool localChanged = true;

  while (localChanged) {
    localChanged = false;
    for (auto &block : F->getBlocks()) {
      auto &insts = block->getInstructionsMut();

      for (auto it = insts.begin(); it != insts.end(); ++it) {
        MIRInst *inst = it->get();
        MIRFunction *callee = nullptr;
        bool isInvoke = false;

        // [NEW] Detect both Standard Calls and Invokes
        if (auto *call = llvm::dyn_cast_or_null<CallInst>(inst)) {
          callee = llvm::dyn_cast_or_null<MIRFunction>(call->getCallee());
        } else if (auto *invoke = llvm::dyn_cast_or_null<InvokeInst>(inst)) {
          callee = llvm::dyn_cast_or_null<MIRFunction>(invoke->getCallee());
          isInvoke = true;
        }

        if (shouldInline(callee)) {
          if (isInvoke) {
            inlineInvoke(llvm::cast<InvokeInst>(inst), F, block.get(), it);
          } else {
            inlineCall(llvm::cast<CallInst>(inst), F, block.get(), it);
          }
          localChanged = true;
          changed = true;
          break; // Break and restart to avoid iterator invalidation
        }
      }
      if (localChanged)
        break;
    }
  }
  return changed;
}

void InliningPass::inlineCall(
    CallInst *call, MIRFunction *caller, MIRBlock *block,
    std::vector<std::unique_ptr<MIRInst>>::iterator &it) {
  MIRFunction *callee = llvm::cast<MIRFunction>(call->getCallee());
  MIRBlock *calleeBlock = callee->getEntryBlock();

  // Map formal arguments to actual passed parameters
  std::unordered_map<MIRValue *, MIRValue *> valueMap;
  auto args = callee->getRawArguments();
  auto passedArgs = call->getArgs();

  for (size_t i = 0; i < args.size(); ++i) {
    valueMap[args[i]] = passedArgs[i];
  }

  // Helper to map operands during cloning
  auto mapValue = [&](MIRValue *val) -> MIRValue * {
    if (!val)
      return nullptr;

    // Explicitly pass through all Constants and Literals
    if (llvm::isa<ConstantInt>(val) || llvm::isa<ConstantFloat>(val) ||
        llvm::isa<ConstantBool>(val) || llvm::isa<ConstantString>(val) ||
        llvm::isa<ConstantNull>(val)) {
      return val;
    }

    if (valueMap.count(val))
      return valueMap[val];

    return val; // Globals etc. remain unchanged
  };

  // Helper to prevent SSA name collisions during inlining
  auto getCloneName = [&](const std::string &oldName) -> std::string {
    if (oldName.empty())
      return "";
    return caller->getUniqueName(oldName);
  };

  std::vector<std::unique_ptr<MIRInst>> clonedInsts;
  MIRValue *mappedReturnValue = nullptr;

  // Structural Instruction Cloner
  for (auto &instPtr : calleeBlock->getInstructions()) {
    MIRInst *inst = instPtr.get();

    if (auto *ret = llvm::dyn_cast<ReturnInst>(inst)) {
      if (ret->getReturnValue()) {
        mappedReturnValue = mapValue(ret->getReturnValue());
      }
      continue;
    }

    std::unique_ptr<MIRInst> cloned;

    // Dispatch and build based on MIRInst constructors
    if (auto *load = llvm::dyn_cast<LoadInst>(inst)) {
      auto newLoad = std::make_unique<LoadInst>(
          mapValue(load->getPointer()), getCloneName(load->getName()),
          load->getLoc(), load->getAlignment());
      if (load->isVolatile())
        newLoad->setVolatile(true);
      cloned = std::move(newLoad);
    } else if (auto *store = llvm::dyn_cast<StoreInst>(inst)) {
      auto newStore = std::make_unique<StoreInst>(
          mapValue(store->getValue()), mapValue(store->getPointer()),
          store->getLoc(), store->getAlignment());
      if (store->isVolatile())
        newStore->setVolatile(true);
      cloned = std::move(newStore);
    } else if (auto *alloca = llvm::dyn_cast<AllocaInst>(inst)) {
      cloned = std::make_unique<AllocaInst>(
          alloca->getType(), alloca->getAllocatedType(),
          getCloneName(alloca->getName()), alloca->getLoc(),
          alloca->getAlignment());
    } else if (auto *cast = llvm::dyn_cast<CastInst>(inst)) {
      cloned = std::make_unique<CastInst>(
          cast->getOpcode(), mapValue(cast->getValue()), cast->getType(),
          getCloneName(cast->getName()), cast->getLoc());
    } else if (auto *gep = llvm::dyn_cast<GetElementPtrInst>(inst)) {
      std::vector<MIRValue *> newIdx;
      for (auto *idx : gep->getIndices())
        newIdx.push_back(mapValue(idx));
      cloned = std::make_unique<GetElementPtrInst>(
          mapValue(gep->getPointer()), std::move(newIdx), gep->getType(),
          gep->getType(), getCloneName(gep->getName()), gep->getLoc());
    } else if (auto *cInst = llvm::dyn_cast<CallInst>(inst)) {
      std::vector<MIRValue *> newArgs;
      for (auto *arg : cInst->getArgs())
        newArgs.push_back(mapValue(arg));
      cloned = std::make_unique<CallInst>(
          mapValue(cInst->getCallee()), std::move(newArgs), cInst->getType(),
          getCloneName(cInst->getName()), cInst->isVariadic(), cInst->getLoc());
    } else if (auto *bin = llvm::dyn_cast<BinaryInst>(inst)) {
      cloned = std::make_unique<BinaryInst>(
          bin->getOpcode(), mapValue(bin->getLHS()), mapValue(bin->getRHS()),
          getCloneName(bin->getName()), bin->getLoc());
    } else if (auto *cmp = llvm::dyn_cast<CompareInst>(inst)) {
      cloned = std::make_unique<CompareInst>(
          cmp->getPredicate(), mapValue(cmp->getLHS()), mapValue(cmp->getRHS()),
          cmp->getType(), getCloneName(cmp->getName()), cmp->getLoc());
    } else if (auto *arc = llvm::dyn_cast<ARCInst>(inst)) {
      cloned = std::make_unique<ARCInst>(
          arc->getOpcode(), mapValue(arc->getObject()), arc->getLoc());
    } else if (auto *ext = llvm::dyn_cast<ExtractValueInst>(inst)) {
      cloned = std::make_unique<ExtractValueInst>(
          mapValue(ext->getAggregate()), ext->getIndex(), ext->getType(),
          getCloneName(ext->getName()), ext->getLoc());
    } else if (auto *ins = llvm::dyn_cast<InsertValueInst>(inst)) {
      cloned = std::make_unique<InsertValueInst>(
          mapValue(ins->getAggregate()), mapValue(ins->getValue()),
          ins->getIndex(), getCloneName(ins->getName()), ins->getLoc());
    } else if (auto *mc = llvm::dyn_cast<MakeClosureInst>(inst)) {
      std::vector<MIRValue *> newCaptures;
      for (auto *cap : mc->getCaptures()) {
        newCaptures.push_back(mapValue(cap));
      }
      cloned = std::make_unique<MakeClosureInst>(
          mapValue(mc->getFunctionPointer()), std::move(newCaptures),
          mc->getType(), getCloneName(mc->getName()), mc->getLoc());
    } else if (auto *ia = llvm::dyn_cast<InlineAsmInst>(inst)) {
      std::vector<MIRValue *> newArgs;
      for (auto *arg : ia->getArgs())
        newArgs.push_back(mapValue(arg));
      cloned = std::make_unique<InlineAsmInst>(
          ia->getAsmString(), ia->getConstraints(), std::move(newArgs),
          ia->getType(), ia->getLoc());
    } else if (auto *fence = llvm::dyn_cast<FenceInst>(inst)) {
      cloned = std::make_unique<FenceInst>(fence->getOrder(), fence->getLoc());
    } else {
      continue;
    }

    if (cloned) {
      cloned->setBorrowKind(inst->getBorrowKind());
      cloned->setParent(block);
      valueMap[inst] = cloned.get();
      clonedInsts.push_back(std::move(cloned));
    }
  }

  // Insert cloned instructions right before the call
  it = block->getInstructionsMut().insert(
      it, std::make_move_iterator(clonedInsts.begin()),
      std::make_move_iterator(clonedInsts.end()));

  std::advance(it, clonedInsts.size());

  if (mappedReturnValue) {
    replaceAllUsesInFunction(caller, call, mappedReturnValue);
  }

  // Erase the original CallInst and step the iterator back
  it = block->getInstructionsMut().erase(it);
  --it;
}

// [NEW] Specialized handler for InvokeInst
void InliningPass::inlineInvoke(
    InvokeInst *invoke, MIRFunction *caller, MIRBlock *block,
    std::vector<std::unique_ptr<MIRInst>>::iterator &it) {

  MIRFunction *callee = llvm::cast<MIRFunction>(invoke->getCallee());
  MIRBlock *calleeBlock = callee->getEntryBlock();

  std::unordered_map<MIRValue *, MIRValue *> valueMap;
  auto args = callee->getRawArguments();
  auto passedArgs = invoke->getArgs();

  for (size_t i = 0; i < args.size(); ++i) {
    valueMap[args[i]] = passedArgs[i];
  }

  auto mapValue = [&](MIRValue *val) -> MIRValue * {
    if (!val)
      return nullptr;

    // Explicitly pass through all Constants and Literals
    if (llvm::isa<ConstantInt>(val) || llvm::isa<ConstantFloat>(val) ||
        llvm::isa<ConstantBool>(val) || llvm::isa<ConstantString>(val) ||
        llvm::isa<ConstantNull>(val)) {
      return val;
    }

    if (valueMap.count(val))
      return valueMap[val];

    return val;
  };

  auto getCloneName = [&](const std::string &oldName) -> std::string {
    if (oldName.empty())
      return "";
    return caller->getUniqueName(oldName);
  };

  std::vector<std::unique_ptr<MIRInst>> clonedInsts;
  MIRValue *mappedReturnValue = nullptr;

  for (auto &instPtr : calleeBlock->getInstructions()) {
    MIRInst *inst = instPtr.get();

    if (auto *ret = llvm::dyn_cast<ReturnInst>(inst)) {
      if (ret->getReturnValue()) {
        mappedReturnValue = mapValue(ret->getReturnValue());
      }
      continue;
    }

    std::unique_ptr<MIRInst> cloned;

    if (auto *load = llvm::dyn_cast<LoadInst>(inst)) {
      auto newLoad = std::make_unique<LoadInst>(
          mapValue(load->getPointer()), getCloneName(load->getName()),
          load->getLoc(), load->getAlignment());
      if (load->isVolatile())
        newLoad->setVolatile(true);
      cloned = std::move(newLoad);
    } else if (auto *store = llvm::dyn_cast<StoreInst>(inst)) {
      auto newStore = std::make_unique<StoreInst>(
          mapValue(store->getValue()), mapValue(store->getPointer()),
          store->getLoc(), store->getAlignment());
      if (store->isVolatile())
        newStore->setVolatile(true);
      cloned = std::move(newStore);
    } else if (auto *alloca = llvm::dyn_cast<AllocaInst>(inst)) {
      cloned = std::make_unique<AllocaInst>(
          alloca->getType(), alloca->getAllocatedType(),
          getCloneName(alloca->getName()), alloca->getLoc(),
          alloca->getAlignment());
    } else if (auto *cast = llvm::dyn_cast<CastInst>(inst)) {
      cloned = std::make_unique<CastInst>(
          cast->getOpcode(), mapValue(cast->getValue()), cast->getType(),
          getCloneName(cast->getName()), cast->getLoc());
    } else if (auto *gep = llvm::dyn_cast<GetElementPtrInst>(inst)) {
      std::vector<MIRValue *> newIdx;
      for (auto *idx : gep->getIndices())
        newIdx.push_back(mapValue(idx));
      cloned = std::make_unique<GetElementPtrInst>(
          mapValue(gep->getPointer()), std::move(newIdx), gep->getType(),
          gep->getType(), getCloneName(gep->getName()), gep->getLoc());
    } else if (auto *cInst = llvm::dyn_cast<CallInst>(inst)) {
      std::vector<MIRValue *> newArgs;
      for (auto *arg : cInst->getArgs())
        newArgs.push_back(mapValue(arg));
      cloned = std::make_unique<CallInst>(
          mapValue(cInst->getCallee()), std::move(newArgs), cInst->getType(),
          getCloneName(cInst->getName()), cInst->isVariadic(), cInst->getLoc());
    } else if (auto *bin = llvm::dyn_cast<BinaryInst>(inst)) {
      cloned = std::make_unique<BinaryInst>(
          bin->getOpcode(), mapValue(bin->getLHS()), mapValue(bin->getRHS()),
          getCloneName(bin->getName()), bin->getLoc());
    } else if (auto *cmp = llvm::dyn_cast<CompareInst>(inst)) {
      cloned = std::make_unique<CompareInst>(
          cmp->getPredicate(), mapValue(cmp->getLHS()), mapValue(cmp->getRHS()),
          cmp->getType(), getCloneName(cmp->getName()), cmp->getLoc());
    } else if (auto *arc = llvm::dyn_cast<ARCInst>(inst)) {
      cloned = std::make_unique<ARCInst>(
          arc->getOpcode(), mapValue(arc->getObject()), arc->getLoc());
    } else if (auto *ext = llvm::dyn_cast<ExtractValueInst>(inst)) {
      cloned = std::make_unique<ExtractValueInst>(
          mapValue(ext->getAggregate()), ext->getIndex(), ext->getType(),
          getCloneName(ext->getName()), ext->getLoc());
    } else if (auto *ins = llvm::dyn_cast<InsertValueInst>(inst)) {
      cloned = std::make_unique<InsertValueInst>(
          mapValue(ins->getAggregate()), mapValue(ins->getValue()),
          ins->getIndex(), getCloneName(ins->getName()), ins->getLoc());
    } else if (auto *mc = llvm::dyn_cast<MakeClosureInst>(inst)) {
      std::vector<MIRValue *> newCaptures;
      for (auto *cap : mc->getCaptures())
        newCaptures.push_back(mapValue(cap));
      cloned = std::make_unique<MakeClosureInst>(
          mapValue(mc->getFunctionPointer()), std::move(newCaptures),
          mc->getType(), getCloneName(mc->getName()), mc->getLoc());
    } else if (auto *ia = llvm::dyn_cast<InlineAsmInst>(inst)) {
      std::vector<MIRValue *> newArgs;
      for (auto *arg : ia->getArgs())
        newArgs.push_back(mapValue(arg));
      cloned = std::make_unique<InlineAsmInst>(
          ia->getAsmString(), ia->getConstraints(), std::move(newArgs),
          ia->getType(), ia->getLoc());
    } else if (auto *fence = llvm::dyn_cast<FenceInst>(inst)) {
      cloned = std::make_unique<FenceInst>(fence->getOrder(), fence->getLoc());
    } else {
      continue;
    }

    if (cloned) {
      cloned->setBorrowKind(inst->getBorrowKind());
      cloned->setParent(block);
      valueMap[inst] = cloned.get();
      clonedInsts.push_back(std::move(cloned));
    }
  }

  it = block->getInstructionsMut().insert(
      it, std::make_move_iterator(clonedInsts.begin()),
      std::make_move_iterator(clonedInsts.end()));

  std::advance(it, clonedInsts.size());

  if (mappedReturnValue) {
    replaceAllUsesInFunction(caller, invoke, mappedReturnValue);
  }

  // --- CFG Terminator Rewrite for Invoke ---
  MIRBlock *normalDest = invoke->getNormalDest();
  MIRBlock *unwindDest = invoke->getUnwindDest();

  // 1. Insert an unconditional branch to the Normal destination
  auto br = std::make_unique<BranchInst>(normalDest, invoke->getLoc());
  br->setParent(block);
  it = block->getInstructionsMut().insert(it, std::move(br));
  std::advance(it, 1);

  // 2. Sever the dead Unwind edge from the CFG
  block->removeSuccessor(unwindDest);
  unwindDest->removePredecessor(block);

  // 3. Erase the original InvokeInst
  it = block->getInstructionsMut().erase(it);
  --it;
}

} // namespace mir
} // namespace moksha
