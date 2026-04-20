#include "moksha/MIR/Passes/InliningPass.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRArgument.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
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
    changed |= runOnFunction(func, M);
  }
  return changed;
}

bool InliningPass::shouldInline(MIRFunction *callee) {
  if (!callee || callee->isDeclaration())
    return false;

  // Do not inline async functions
  if (callee->getType()) {
    std::string retStr = callee->getType()->toString();
    if (retStr.find("Promise") != std::string::npos) {
      return false; // Abort inlining!
    }
  }

  if (callee->isNoInline() || callee->isInterrupt() || callee->isNaked()) {
    return false;
  }

  // Count instructions for heuristic limits
  size_t instCount = 0;
  for (const auto &b : callee->getBlocks()) {
    instCount += b->getInstructions().size();
  }

  return instCount < 75 || callee->isInline();
}

bool InliningPass::runOnFunction(MIRFunction *F, MIRModule &M) {
  if (F->isDeclaration())
    return false;

  bool changed = false;
  bool localChanged = true;

  while (localChanged) {
    localChanged = false;

    // Use indices to iterate to avoid iterator invalidation when adding blocks
    for (size_t bIdx = 0; bIdx < F->getBlocks().size(); ++bIdx) {
      MIRBlock *block = F->getBlocks()[bIdx].get();
      auto &insts = block->getInstructionsMut();

      for (auto it = insts.begin(); it != insts.end(); ++it) {
        if (auto *call = llvm::dyn_cast<CallInst>(it->get())) {
          if (auto *callee =
                  llvm::dyn_cast_or_null<MIRFunction>(call->getCallee())) {
            if (shouldInline(callee)) {
              localChanged = inlineCall(F, block, it, call, M);
              if (localChanged) {
                changed = true;
                break; // Restart loop because CFG changed
              }
            }
          }
        } else if (auto *invoke = llvm::dyn_cast<InvokeInst>(it->get())) {
          if (auto *callee =
                  llvm::dyn_cast_or_null<MIRFunction>(invoke->getCallee())) {
            if (shouldInline(callee)) {
              localChanged = inlineInvoke(F, block, it, invoke, M);
              if (localChanged) {
                changed = true;
                break; // Restart loop because CFG changed
              }
            }
          }
        }
      }
      if (localChanged)
        break; // Break outer loop to restart
    }
  }

  if (changed) {
    F->numberUnnamedValues();
  }

  return changed;
}

// ----------------------------------------------------------------------------
// FULL BLOCK-SPLITTING INLINER
// ----------------------------------------------------------------------------
bool InliningPass::inlineCall(
    MIRFunction *caller, MIRBlock *callBlock,
    std::vector<std::unique_ptr<MIRInst>>::iterator &callIt, CallInst *call,
    MIRModule &M) {

  MIRFunction *callee = llvm::cast<MIRFunction>(call->getCallee());

  std::unordered_map<MIRValue *, MIRValue *> valueMap;
  std::unordered_map<MIRBlock *, MIRBlock *> blockMap;

  // 1. Map Arguments
  for (size_t i = 0; i < call->getArgs().size(); ++i) {
    valueMap[callee->getRawArguments()[i]] = call->getArgs()[i];
  }

  // 2. Clone Blocks
  std::vector<MIRBlock *> clonedBlocks;
  for (const auto &oldBlockPtr : callee->getBlocks()) {
    std::string newName =
        caller->getUniqueName(callee->getName() + "." + oldBlockPtr->getName());
    auto newBlock = std::make_unique<MIRBlock>(newName, caller);

    blockMap[oldBlockPtr.get()] = newBlock.get();
    clonedBlocks.push_back(newBlock.get());
    caller->addBlock(std::move(newBlock));
  }

  // 3. Clone Instructions (Pass 1: Creation)
  std::vector<MIRInst *> allClonedInsts;
  std::vector<ReturnInst *> returns;

  for (const auto &oldBlockPtr : callee->getBlocks()) {
    MIRBlock *newBlock = blockMap[oldBlockPtr.get()];

    for (const auto &oldInstPtr : oldBlockPtr->getInstructions()) {
      auto clonedInst = oldInstPtr->clone();
      clonedInst->setParent(newBlock);
      clonedInst->setBorrowKind(oldInstPtr->getBorrowKind());

      valueMap[oldInstPtr.get()] = clonedInst.get();
      allClonedInsts.push_back(clonedInst.get());

      if (auto *retInst = llvm::dyn_cast<ReturnInst>(clonedInst.get())) {
        returns.push_back(retInst);
      }

      newBlock->addInstruction(std::move(clonedInst));
    }
  }

  // 4. Remap Operands (Pass 2: Patching variables and branches)
  for (MIRInst *inst : allClonedInsts) {
    // Ask the instruction to replace any matching old values with the new ones
    for (auto &[oldVal, newVal] : valueMap) {
      inst->replaceOperand(oldVal, newVal);
    }
    // Do the same for block targets
    for (auto &[oldBlock, newBlock] : blockMap) {
      inst->replaceOperand(oldBlock, newBlock);
    }

    // Explicitly update Phi Nodes and Terminator block targets
    if (auto *phi = llvm::dyn_cast<PhiInst>(inst)) {
      for (auto &[val, incomingBlock] : phi->getIncomingMut()) {
        if (blockMap.count(incomingBlock)) {
          incomingBlock = blockMap[incomingBlock];
        }
      }
    } else if (auto *br = llvm::dyn_cast<BranchInst>(inst)) {
      if (blockMap.count(br->getTarget())) {
        br->setTarget(blockMap[br->getTarget()]);
      }
    } else if (auto *condBr = llvm::dyn_cast<CondBranchInst>(inst)) {
      if (blockMap.count(condBr->getTrueBlock())) {
        condBr->setTrueBlock(blockMap[condBr->getTrueBlock()]);
      }
      if (blockMap.count(condBr->getFalseBlock())) {
        condBr->setFalseBlock(blockMap[condBr->getFalseBlock()]);
      }
    } else if (auto *sw = llvm::dyn_cast<SwitchInst>(inst)) {
      if (blockMap.count(sw->getDefaultBlock())) {
        sw->setDefaultBlock(blockMap[sw->getDefaultBlock()]);
      }
      // Note: adjust the setter/getter here to match your exact SwitchInst API
      for (auto &casePair : sw->getCasesMut()) {
        if (blockMap.count(casePair.second)) {
          casePair.second = blockMap[casePair.second];
        }
      }
    } else if (auto *inv = llvm::dyn_cast<InvokeInst>(inst)) {
      if (blockMap.count(inv->getNormalDest())) {
        inv->setNormalDest(blockMap[inv->getNormalDest()]);
      }
      if (inv->getUnwindDest() && blockMap.count(inv->getUnwindDest())) {
        inv->setUnwindDest(blockMap[inv->getUnwindDest()]);
      }
    } else if (auto *throwInst = llvm::dyn_cast<ThrowInst>(inst)) {
      if (throwInst->getUnwindDest() &&
          blockMap.count(throwInst->getUnwindDest())) {
        throwInst->setUnwindDest(blockMap[throwInst->getUnwindDest()]);
      }
    }
  }

  // 4.5 Rebuild Internal CFG Edges
  for (MIRBlock *newBlock : clonedBlocks) {
    if (newBlock->getInstructions().empty())
      continue;
    MIRInst *term = newBlock->getInstructions().back().get();

    if (auto *br = llvm::dyn_cast<BranchInst>(term)) {
      newBlock->addSuccessor(br->getTarget());
      br->getTarget()->addPredecessor(newBlock);
    } else if (auto *condBr = llvm::dyn_cast<CondBranchInst>(term)) {
      newBlock->addSuccessor(condBr->getTrueBlock());
      condBr->getTrueBlock()->addPredecessor(newBlock);
      newBlock->addSuccessor(condBr->getFalseBlock());
      condBr->getFalseBlock()->addPredecessor(newBlock);
    } else if (auto *invoke = llvm::dyn_cast<InvokeInst>(term)) {
      newBlock->addSuccessor(invoke->getNormalDest());
      invoke->getNormalDest()->addPredecessor(newBlock);
      if (invoke->getUnwindDest()) {
        newBlock->addSuccessor(invoke->getUnwindDest());
        invoke->getUnwindDest()->addPredecessor(newBlock);
      }
    } else if (auto *throwInst = llvm::dyn_cast<ThrowInst>(term)) {
      if (throwInst->getUnwindDest()) {
        newBlock->addSuccessor(throwInst->getUnwindDest());
        throwInst->getUnwindDest()->addPredecessor(newBlock);
      }
    } else if (auto *switchInst = llvm::dyn_cast<SwitchInst>(term)) {
      newBlock->addSuccessor(switchInst->getDefaultBlock());
      switchInst->getDefaultBlock()->addPredecessor(newBlock);
      for (auto &casePair : switchInst->getCases()) {
        newBlock->addSuccessor(casePair.second);
        casePair.second->addPredecessor(newBlock);
      }
    }
  }

  // 5. Split the Caller Block
  auto returnBlock = std::make_unique<MIRBlock>(
      caller->getUniqueName("inline.return"), caller);
  MIRBlock *returnBlockPtr = returnBlock.get();

  auto &callBlockInsts = callBlock->getInstructionsMut();
  auto splitStart = std::next(callIt); // Everything AFTER the call instruction

  // Move remaining instructions to the new return block
  for (auto it = splitStart; it != callBlockInsts.end(); ++it) {
    (*it)->setParent(returnBlockPtr);
    returnBlockPtr->addInstruction(std::move(*it));
  }
  callBlockInsts.erase(splitStart, callBlockInsts.end());
  returnBlockPtr->getSuccessors() = callBlock->getSuccessors();
  for (MIRBlock *succ : returnBlockPtr->getSuccessors()) {
    // Update successors to point back to ReturnBlock instead of CallBlock
    auto &succPreds = succ->getPredecessors();
    std::replace(succPreds.begin(), succPreds.end(), callBlock, returnBlockPtr);

    // Patch Phis in successors
    for (auto &inst : succ->getInstructionsMut()) {
      if (auto *phi = llvm::dyn_cast<PhiInst>(inst.get())) {
        for (auto &[val, incBlock] : phi->getIncomingMut()) {
          if (incBlock == callBlock)
            incBlock = returnBlockPtr;
        }
      } else {
        break; // Phis are always at the top
      }
    }
  }
  callBlock->getSuccessors().clear();

  // 6. Connect Caller to Callee Entry
  MIRBlock *calleeEntry = blockMap[callee->getEntryBlock()];
  std::unique_ptr<MIRInst> savedCall = std::move(callBlockInsts.back());
  callBlockInsts.pop_back();
  callBlock->addInstruction(
      std::make_unique<BranchInst>(calleeEntry, call->getLoc()));
  callBlock->addSuccessor(calleeEntry);
  calleeEntry->addPredecessor(callBlock);

  // 7. Handle Returns & Connect back to the Caller Return Block
  MIRValue *returnValue = nullptr;

  if (!returns.empty()) {
    const hir::HIRType *retTy = call->getType();
    bool hasReturnValue = retTy && retTy->getKind() != hir::TypeKind::Void;

    if (hasReturnValue) {
      auto phi =
          std::make_unique<PhiInst>(retTy, "inline.ret", returns[0]->getLoc());

      for (ReturnInst *ret : returns) {
        MIRBlock *retBlock = ret->getParent();
        if (ret->getReturnValue()) {
          MIRValue *retVal = ret->getReturnValue();

          if (retVal->getType() != retTy) {
            if (retVal->getType()->toString() != retTy->toString()) {
              auto castInst =
                  std::make_unique<CastInst>(Opcode::BitCast, retVal, retTy,
                                             "inline.ret.cast", ret->getLoc());
              retVal = castInst.get();
              auto &retInsts = retBlock->getInstructionsMut();
              // Insert the cast right before the ReturnInst
              retInsts.insert(retInsts.end() - 1, std::move(castInst));
            }
          }

          phi->addIncoming(retVal, retBlock);
        }

        // Replace return with branch to the continuation block
        auto &retInsts = retBlock->getInstructionsMut();
        retInsts.pop_back();
        retInsts.push_back(
            std::make_unique<BranchInst>(returnBlockPtr, ret->getLoc()));

        retBlock->addSuccessor(returnBlockPtr);
        returnBlockPtr->addPredecessor(retBlock);
      }

      returnValue = phi.get();
      returnBlockPtr->getInstructionsMut().insert(
          returnBlockPtr->getInstructionsMut().begin(), std::move(phi));

    } else {
      // Void function, just insert branches
      for (ReturnInst *ret : returns) {
        MIRBlock *retBlock = ret->getParent();
        auto &retInsts = retBlock->getInstructionsMut();
        retInsts.pop_back();
        retInsts.push_back(
            std::make_unique<BranchInst>(returnBlockPtr, ret->getLoc()));

        retBlock->addSuccessor(returnBlockPtr);
        returnBlockPtr->addPredecessor(retBlock);
      }
    }
  }

  // Add the newly created return block to the function
  caller->addBlock(std::move(returnBlock));

  // 9. Replace uses of the call and delete it
  if (returnValue) {
    replaceAllUsesInFunction(caller, call, returnValue);
  }

  return true;
}

// ----------------------------------------------------------------------------
// INVOKE INLINING (Similar block-splitting, but handles unwind dests)
// ----------------------------------------------------------------------------
bool InliningPass::inlineInvoke(
    MIRFunction *caller, MIRBlock *callBlock,
    std::vector<std::unique_ptr<MIRInst>>::iterator &callIt, InvokeInst *invoke,
    MIRModule &M) {

  MIRFunction *callee = llvm::cast<MIRFunction>(invoke->getCallee());

  std::unordered_map<MIRValue *, MIRValue *> valueMap;
  std::unordered_map<MIRBlock *, MIRBlock *> blockMap;

  // 1. Map Arguments
  for (size_t i = 0; i < invoke->getArgs().size(); ++i) {
    valueMap[callee->getRawArguments()[i]] = invoke->getArgs()[i];
  }

  // 2. Clone Blocks
  std::vector<MIRBlock *> clonedBlocks;
  for (const auto &oldBlockPtr : callee->getBlocks()) {
    std::string newName =
        caller->getUniqueName(callee->getName() + "." + oldBlockPtr->getName());
    auto newBlock = std::make_unique<MIRBlock>(newName, caller);
    blockMap[oldBlockPtr.get()] = newBlock.get();
    clonedBlocks.push_back(newBlock.get());
    caller->addBlock(std::move(newBlock));
  }

  // 3. Clone Instructions
  std::vector<MIRInst *> allClonedInsts;
  std::vector<ReturnInst *> returns;
  std::vector<ThrowInst *> throws;

  for (const auto &oldBlockPtr : callee->getBlocks()) {
    MIRBlock *newBlock = blockMap[oldBlockPtr.get()];
    for (const auto &oldInstPtr : oldBlockPtr->getInstructions()) {
      auto clonedInst = oldInstPtr->clone();
      clonedInst->setParent(newBlock);
      clonedInst->setBorrowKind(oldInstPtr->getBorrowKind());

      valueMap[oldInstPtr.get()] = clonedInst.get();
      allClonedInsts.push_back(clonedInst.get());

      if (auto *retInst = llvm::dyn_cast<ReturnInst>(clonedInst.get())) {
        returns.push_back(retInst);
      } else if (auto *throwInst =
                     llvm::dyn_cast<ThrowInst>(clonedInst.get())) {
        throws.push_back(throwInst);
      }

      newBlock->addInstruction(std::move(clonedInst));
    }
  }

  // 4. Remap Operands
  for (MIRInst *inst : allClonedInsts) {
    // Ask the instruction to replace any matching old values with the new ones
    for (auto &[oldVal, newVal] : valueMap) {
      inst->replaceOperand(oldVal, newVal);
    }
    // Do the same for block targets
    for (auto &[oldBlock, newBlock] : blockMap) {
      inst->replaceOperand(oldBlock, newBlock);
    }
    // Explicitly update Phi Nodes and Terminator block targets
    if (auto *phi = llvm::dyn_cast<PhiInst>(inst)) {
      for (auto &[val, incomingBlock] : phi->getIncomingMut()) {
        if (blockMap.count(incomingBlock)) {
          incomingBlock = blockMap[incomingBlock];
        }
      }
    } else if (auto *br = llvm::dyn_cast<BranchInst>(inst)) {
      if (blockMap.count(br->getTarget())) {
        br->setTarget(blockMap[br->getTarget()]);
      }
    } else if (auto *condBr = llvm::dyn_cast<CondBranchInst>(inst)) {
      if (blockMap.count(condBr->getTrueBlock())) {
        condBr->setTrueBlock(blockMap[condBr->getTrueBlock()]);
      }
      if (blockMap.count(condBr->getFalseBlock())) {
        condBr->setFalseBlock(blockMap[condBr->getFalseBlock()]);
      }
    } else if (auto *sw = llvm::dyn_cast<SwitchInst>(inst)) {
      if (blockMap.count(sw->getDefaultBlock())) {
        sw->setDefaultBlock(blockMap[sw->getDefaultBlock()]);
      }
      // Note: adjust the setter/getter here to match your exact SwitchInst API
      for (auto &casePair : sw->getCasesMut()) {
        if (blockMap.count(casePair.second)) {
          casePair.second = blockMap[casePair.second];
        }
      }
    } else if (auto *inv = llvm::dyn_cast<InvokeInst>(inst)) {
      if (blockMap.count(inv->getNormalDest())) {
        inv->setNormalDest(blockMap[inv->getNormalDest()]);
      }
      if (inv->getUnwindDest() && blockMap.count(inv->getUnwindDest())) {
        inv->setUnwindDest(blockMap[inv->getUnwindDest()]);
      }
    } else if (auto *throwInst = llvm::dyn_cast<ThrowInst>(inst)) {
      if (throwInst->getUnwindDest() &&
          blockMap.count(throwInst->getUnwindDest())) {
        throwInst->setUnwindDest(blockMap[throwInst->getUnwindDest()]);
      }
    }
  }

  // 4.5 Rebuild Internal CFG Edges
  for (MIRBlock *newBlock : clonedBlocks) {
    if (newBlock->getInstructions().empty())
      continue;
    MIRInst *term = newBlock->getInstructions().back().get();

    if (auto *br = llvm::dyn_cast<BranchInst>(term)) {
      newBlock->addSuccessor(br->getTarget());
      br->getTarget()->addPredecessor(newBlock);
    } else if (auto *condBr = llvm::dyn_cast<CondBranchInst>(term)) {
      newBlock->addSuccessor(condBr->getTrueBlock());
      condBr->getTrueBlock()->addPredecessor(newBlock);
      newBlock->addSuccessor(condBr->getFalseBlock());
      condBr->getFalseBlock()->addPredecessor(newBlock);
    } else if (auto *invoke = llvm::dyn_cast<InvokeInst>(term)) {
      newBlock->addSuccessor(invoke->getNormalDest());
      invoke->getNormalDest()->addPredecessor(newBlock);
      if (invoke->getUnwindDest()) {
        newBlock->addSuccessor(invoke->getUnwindDest());
        invoke->getUnwindDest()->addPredecessor(newBlock);
      }
    } else if (auto *throwInst = llvm::dyn_cast<ThrowInst>(term)) {
      if (throwInst->getUnwindDest()) {
        newBlock->addSuccessor(throwInst->getUnwindDest());
        throwInst->getUnwindDest()->addPredecessor(newBlock);
      }
    } else if (auto *switchInst = llvm::dyn_cast<SwitchInst>(term)) {
      newBlock->addSuccessor(switchInst->getDefaultBlock());
      switchInst->getDefaultBlock()->addPredecessor(newBlock);
      for (auto &casePair : switchInst->getCases()) {
        newBlock->addSuccessor(casePair.second);
        casePair.second->addPredecessor(newBlock);
      }
    }
  }

  MIRBlock *normalDest = invoke->getNormalDest();
  MIRBlock *unwindDest = invoke->getUnwindDest();

  // 5. Connect Caller to Callee Entry
  MIRBlock *calleeEntry = blockMap[callee->getEntryBlock()];
  auto &callBlockInsts = callBlock->getInstructionsMut();
  std::unique_ptr<MIRInst> savedInvoke = std::move(callBlockInsts.back());
  callBlockInsts.pop_back();
  callBlockInsts.push_back(
      std::make_unique<BranchInst>(calleeEntry, invoke->getLoc()));

  // Fix outgoing CFG
  callBlock->removeSuccessor(normalDest);
  callBlock->removeSuccessor(unwindDest);
  normalDest->removePredecessor(callBlock);
  unwindDest->removePredecessor(callBlock);

  callBlock->addSuccessor(calleeEntry);
  calleeEntry->addPredecessor(callBlock);

  // 6. Handle Returns -> Route to Normal Dest
  MIRValue *returnValue = nullptr;
  if (!returns.empty()) {
    const hir::HIRType *retTy = invoke->getType();
    if (retTy && retTy->getKind() != hir::TypeKind::Void) {
      auto phi =
          std::make_unique<PhiInst>(retTy, "invoke.ret", returns[0]->getLoc());
      for (ReturnInst *ret : returns) {
        MIRBlock *retBlock = ret->getParent();
        if (ret->getReturnValue()) {
          MIRValue *retVal = ret->getReturnValue();

          if (retVal->getType() != retTy) {
            if (retVal->getType()->toString() != retTy->toString()) {
              auto castInst =
                  std::make_unique<CastInst>(Opcode::BitCast, retVal, retTy,
                                             "inline.ret.cast", ret->getLoc());
              retVal = castInst.get();
              auto &retInsts = retBlock->getInstructionsMut();
              // Insert the cast right before the ReturnInst
              retInsts.insert(retInsts.end() - 1, std::move(castInst));
            }
          }

          phi->addIncoming(retVal, retBlock);
        }

        auto &retInsts = retBlock->getInstructionsMut();
        retInsts.pop_back();
        retInsts.push_back(
            std::make_unique<BranchInst>(normalDest, ret->getLoc()));

        retBlock->addSuccessor(normalDest);
        normalDest->addPredecessor(retBlock);
      }
      returnValue = phi.get();
      normalDest->getInstructionsMut().insert(
          normalDest->getInstructionsMut().begin(), std::move(phi));
    } else {
      for (ReturnInst *ret : returns) {
        MIRBlock *retBlock = ret->getParent();
        auto &retInsts = retBlock->getInstructionsMut();
        retInsts.pop_back();
        retInsts.push_back(
            std::make_unique<BranchInst>(normalDest, ret->getLoc()));

        retBlock->addSuccessor(normalDest);
        normalDest->addPredecessor(retBlock);
      }
    }
  } else {
    auto &retInsts = normalDest->getInstructionsMut();
    if (retInsts.empty() ||
        !llvm::isa<UnreachableInst>(retInsts.back().get())) {
      retInsts.insert(retInsts.begin(),
                      std::make_unique<UnreachableInst>(invoke->getLoc()));
    }
  }

  // 7. Handle Throws -> Route to Unwind Dest
  for (ThrowInst *throwInst : throws) {
    MIRBlock *throwBlock = throwInst->getParent();

    // Redirect the throw to our caller's landing pad
    throwInst->setUnwindDest(unwindDest);

    throwBlock->addSuccessor(unwindDest);
    unwindDest->addPredecessor(throwBlock);
  }

  if (returnValue) {
    replaceAllUsesInFunction(caller, invoke, returnValue);
  }

  // 8. Patch existing Phis in normalDest
  for (auto &inst : normalDest->getInstructionsMut()) {
    if (auto *phi = llvm::dyn_cast<PhiInst>(inst.get())) {
      if (phi == returnValue)
        continue; // Skip the return value Phi we just built

      MIRValue *valFromCallBlock = nullptr;
      for (auto &[val, incBlock] : phi->getIncoming()) {
        if (incBlock == callBlock) {
          valFromCallBlock = val;
          break;
        }
      }

      if (valFromCallBlock) {
        phi->removeIncoming(callBlock);
        for (ReturnInst *ret : returns) {
          phi->addIncoming(valFromCallBlock, ret->getParent());
        }
      }
    } else {
      break; // Phis are always at the top
    }
  }

  // 9. Patch existing Phis in unwindDest
  for (auto &inst : unwindDest->getInstructionsMut()) {
    if (auto *phi = llvm::dyn_cast<PhiInst>(inst.get())) {
      MIRValue *valFromCallBlock = nullptr;
      for (auto &[val, incBlock] : phi->getIncoming()) {
        if (incBlock == callBlock) {
          valFromCallBlock = val;
          break;
        }
      }

      if (valFromCallBlock) {
        phi->removeIncoming(callBlock);
        for (ThrowInst *throwInst : throws) {
          phi->addIncoming(valFromCallBlock, throwInst->getParent());
        }
      }
    } else {
      break;
    }
  }

  return true;
}

} // namespace mir
} // namespace moksha
