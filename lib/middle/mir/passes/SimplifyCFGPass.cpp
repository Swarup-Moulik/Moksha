#include "moksha/MIR/Passes/SimplifyCFGPass.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <unordered_set>
#include <vector>

namespace moksha {
namespace mir {

/** @brief Replaces all uses of `oldVal` with `newVal` in the function `F`. */
static void replaceAllUsesInFunction(MIRFunction *F, MIRValue *oldVal,
                                     MIRValue *newVal) {
  for (auto &blockPtr : F->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructionsMut()) {
      if (instPtr) {
        instPtr->replaceOperand(oldVal, newVal);
      }
    }
  }
}

/** @brief Replaces the target block in the terminator `term`. */
static void replaceTargetBlock(MIRInst *term, MIRBlock *oldTarget,
                               MIRBlock *newTarget) {
  if (auto *br = llvm::dyn_cast_or_null<BranchInst>(term)) {
    if (br->getTarget() == oldTarget)
      br->setTarget(newTarget);
  } else if (auto *condBr = llvm::dyn_cast_or_null<CondBranchInst>(term)) {
    if (condBr->getTrueBlock() == oldTarget)
      condBr->setTrueBlock(newTarget);
    if (condBr->getFalseBlock() == oldTarget)
      condBr->setFalseBlock(newTarget);
  } else if (auto *invoke = llvm::dyn_cast_or_null<InvokeInst>(term)) {
    if (invoke->getNormalDest() == oldTarget)
      invoke->setNormalDest(newTarget);
    if (invoke->getUnwindDest() == oldTarget)
      invoke->setUnwindDest(newTarget);
  } else if (auto *throwInst = llvm::dyn_cast_or_null<ThrowInst>(term)) {
    if (throwInst->getUnwindDest() == oldTarget)
      throwInst->setUnwindDest(newTarget);
  } else if (auto *switchInst = llvm::dyn_cast_or_null<SwitchInst>(term)) {
    if (switchInst->getDefaultBlock() == oldTarget)
      switchInst->setDefaultBlock(newTarget);
    for (auto &casePair : switchInst->getCasesMut()) {
      if (casePair.second == oldTarget)
        casePair.second = newTarget;
    }
  }
}

bool SimplifyCFGPass::runOnModule(MIRModule &M) {
  bool changed = false;
  for (auto &func : M.getFunctions()) {
    if (!func->isDeclaration()) {
      changed |= runOnFunction(func);
    }
  }
  return changed;
}

bool SimplifyCFGPass::runOnFunction(MIRFunction *F) {
  // 1. THE CFG HEALER (Must run FIRST!)
  for (auto &blockPtr : F->getBlocks()) {
    blockPtr->getSuccessors().clear();
    blockPtr->getPredecessors().clear();
  }

  for (auto &blockPtr : F->getBlocks()) {
    MIRBlock *b = blockPtr.get();
    if (b->getInstructions().empty())
      continue;

    MIRInst *term = b->getInstructions().back().get();
    if (auto *br = llvm::dyn_cast_or_null<BranchInst>(term)) {
      b->addSuccessor(br->getTarget());
      br->getTarget()->addPredecessor(b);
    } else if (auto *condBr = llvm::dyn_cast_or_null<CondBranchInst>(term)) {
      b->addSuccessor(condBr->getTrueBlock());
      condBr->getTrueBlock()->addPredecessor(b);
      b->addSuccessor(condBr->getFalseBlock());
      condBr->getFalseBlock()->addPredecessor(b);
    } else if (auto *invoke = llvm::dyn_cast_or_null<InvokeInst>(term)) {
      b->addSuccessor(invoke->getNormalDest());
      invoke->getNormalDest()->addPredecessor(b);
      if (invoke->getUnwindDest()) {
        b->addSuccessor(invoke->getUnwindDest());
        invoke->getUnwindDest()->addPredecessor(b);
      }
    } else if (auto *throwInst = llvm::dyn_cast_or_null<ThrowInst>(term)) {
      if (throwInst->getUnwindDest()) {
        b->addSuccessor(throwInst->getUnwindDest());
        throwInst->getUnwindDest()->addPredecessor(b);
      }
    } else if (auto *switchInst = llvm::dyn_cast_or_null<SwitchInst>(term)) {
      b->addSuccessor(switchInst->getDefaultBlock());
      switchInst->getDefaultBlock()->addPredecessor(b);
      for (auto &casePair : switchInst->getCases()) {
        b->addSuccessor(casePair.second);
        casePair.second->addPredecessor(b);
      }
    }

    for (auto &instPtr : b->getInstructions()) {
      if (auto *phi = llvm::dyn_cast_or_null<PhiInst>(instPtr.get())) {
        std::vector<MIRBlock *> stalePhiBlocks;
        for (auto &[val, predBlock] : phi->getIncoming()) {
          if (std::find(b->getPredecessors().begin(),
                        b->getPredecessors().end(),
                        predBlock) == b->getPredecessors().end()) {
            stalePhiBlocks.push_back(predBlock);
          }
        }

        std::vector<MIRBlock *> missingCFGBlocks;
        for (MIRBlock *cfgPred : b->getPredecessors()) {
          bool found = false;
          for (auto &[val, predBlock] : phi->getIncoming()) {
            if (predBlock == cfgPred) {
              found = true;
              break;
            }
          }
          if (!found)
            missingCFGBlocks.push_back(cfgPred);
        }

        if (stalePhiBlocks.size() == 1 && !missingCFGBlocks.empty()) {
          MIRValue *rescuedVal = nullptr;
          for (auto &[val, predBlock] : phi->getIncoming()) {
            if (predBlock == stalePhiBlocks[0]) {
              rescuedVal = val;
              break;
            }
          }
          if (rescuedVal) {
            phi->removeIncoming(stalePhiBlocks[0]);
            for (MIRBlock *missingBlock : missingCFGBlocks) {
              phi->addIncoming(rescuedVal, missingBlock);
            }
          }
        }
      } else {
        break;
      }
    }
  }

  // 2. SIMPLIFICATION LOOP (Runs safely on a sanitized graph)
  bool changed = false;
  bool localChanged = true;

  while (localChanged) {
    localChanged = false;
    localChanged |= foldBranches(F);
    localChanged |= bypassEmptyBlocks(F);
    localChanged |= mergeBlocks(F);
    changed |= localChanged;
  }

  // 3. DEAD BLOCK SWEEP
  size_t beforeSize = F->getBlocks().size();
  removeDeadBlocks(F);

  if (F->getBlocks().size() != beforeSize) {
    changed = true;
  }

  return changed;
}

// 1. Branch Folding
bool SimplifyCFGPass::foldBranches(MIRFunction *F) {
  bool changed = false;
  for (auto &blockPtr : F->getBlocks()) {
    MIRBlock *b = blockPtr.get();
    if (b->getInstructions().empty())
      continue;

    auto *condBr = llvm::dyn_cast_or_null<CondBranchInst>(
        b->getInstructions().back().get());

    if (condBr && condBr->getTrueBlock() == condBr->getFalseBlock()) {
      MIRBlock *target = condBr->getTrueBlock();
      auto newBr = std::make_unique<BranchInst>(target, condBr->getLoc());
      b->getInstructionsMut().pop_back();
      b->addInstruction(std::move(newBr));

      b->removeSuccessor(target);
      target->removePredecessor(b);

      for (auto &inst : target->getInstructionsMut()) {
        if (auto *phi = llvm::dyn_cast_or_null<PhiInst>(inst.get())) {
          auto &incoming = phi->getIncomingMut();
          for (auto it = incoming.begin(); it != incoming.end(); ++it) {
            if (it->second == b) {
              incoming.erase(it);
              break;
            }
          }
        } else {
          break;
        }
      }

      changed = true;
    }
  }
  return changed;
}

// 2. Empty Block Bypassing
bool SimplifyCFGPass::bypassEmptyBlocks(MIRFunction *F) {
  for (auto &blockPtr : F->getBlocks()) {
    MIRBlock *B = blockPtr.get();

    if (B == F->getEntryBlock())
      continue;

    if (B->getPredecessors().empty())
      continue;

    if (B->getInstructions().size() != 1)
      continue;

    auto *br =
        llvm::dyn_cast_or_null<BranchInst>(B->getInstructions().front().get());
    if (!br)
      continue;

    MIRBlock *C = br->getTarget();
    if (B == C)
      continue;

    bool cHasPhi = false;
    for (auto &inst : C->getInstructions()) {
      if (!inst)
        continue;

      if (llvm::isa<PhiInst>(inst.get())) {
        cHasPhi = true;
        break;
      }
    }

    bool canBypass = true;

    // Prevent bypassing if the predecessor is an InvokeInst
    for (MIRBlock *A : B->getPredecessors()) {
      if (A->getInstructionsMut().empty())
        continue;

      auto *term = A->getInstructionsMut().back().get();
      if (term && llvm::isa<InvokeInst>(term)) {
        canBypass = false;
        break;
      }
    }

    if (cHasPhi) {
      for (MIRBlock *A : B->getPredecessors()) {
        if (std::find(C->getPredecessors().begin(), C->getPredecessors().end(),
                      A) != C->getPredecessors().end()) {
          canBypass = false;
          break;
        }
      }
    }

    if (!canBypass)
      continue;

    std::vector<MIRBlock *> preds = B->getPredecessors();

    for (MIRBlock *A : preds) {
      auto *term = A->getInstructionsMut().back().get();
      replaceTargetBlock(term, B, C);
      A->removeSuccessor(B);
      A->addSuccessor(C);
      C->addPredecessor(A);

      for (auto &inst : C->getInstructionsMut()) {
        if (!inst)
          continue;
        if (auto *phi = llvm::dyn_cast_or_null<PhiInst>(inst.get())) {
          MIRValue *valForB = nullptr;
          for (auto &[val, pred] : phi->getIncoming()) {
            if (pred == B) {
              valForB = val;
              break;
            }
          }
          if (valForB) {
            phi->addIncoming(valForB, A);
          }
        } else {
          break;
        }
      }
    }

    C->removePredecessor(B);
    for (auto &inst : C->getInstructionsMut()) {
      if (!inst)
        continue;
      if (auto *phi = llvm::dyn_cast_or_null<PhiInst>(inst.get())) {
        phi->removeIncoming(B);
      } else {
        break;
      }
    }
    B->getPredecessors().clear();
    B->getSuccessors().clear();

    return true;
  }
  return false;
}

// 3. Block Merging
bool SimplifyCFGPass::mergeBlocks(MIRFunction *F) {
  for (auto &blockPtr : F->getBlocks()) {
    MIRBlock *A = blockPtr.get();
    if (A->getInstructions().empty())
      continue;

    auto *br =
        llvm::dyn_cast_or_null<BranchInst>(A->getInstructions().back().get());
    if (!br)
      continue;

    MIRBlock *B = br->getTarget();

    if (B == A || B == F->getEntryBlock())
      continue;

    if (B->getPredecessors().size() == 1 && B->getPredecessors()[0] == A) {
      A->getInstructionsMut().pop_back();
      for (auto &inst : B->getInstructionsMut()) {
        if (!inst)
          continue;
        if (auto *phi = llvm::dyn_cast_or_null<PhiInst>(inst.get())) {
          MIRValue *incomingVal = nullptr;
          for (auto &[val, pred] : phi->getIncoming()) {
            if (pred == A) {
              incomingVal = val;
              break;
            }
          }
          if (incomingVal) {
            replaceAllUsesInFunction(F, phi, incomingVal);
          }
        } else {
          inst->setParent(A);
          A->getInstructionsMut().push_back(std::move(inst));
        }
      }
      B->getInstructionsMut().clear();
      A->removeSuccessor(B);
      for (MIRBlock *succ : B->getSuccessors()) {
        A->addSuccessor(succ);
        succ->removePredecessor(B);
        succ->addPredecessor(A);
        for (auto &inst : succ->getInstructionsMut()) {
          if (!inst)
            continue;
          if (auto *phi = llvm::dyn_cast_or_null<PhiInst>(inst.get())) {
            MIRValue *valForB = nullptr;
            for (auto &[val, pred] : phi->getIncoming()) {
              if (pred == B) {
                valForB = val;
                break;
              }
            }
            if (valForB) {
              phi->removeIncoming(B);
              phi->addIncoming(valForB, A);
            }
          } else {
            break;
          }
        }
      }

      B->getSuccessors().clear();
      B->getPredecessors().clear();

      return true;
    }
  }
  return false;
}

// 4. Dead Block Cleanup
void SimplifyCFGPass::removeDeadBlocks(MIRFunction *F) {
  auto &blocks = F->getBlocksMut();
  bool changed = true;

  while (changed) {
    changed = false;
    std::unordered_map<MIRBlock *, std::vector<MIRBlock *>> truePreds;
    for (auto &blockPtr : blocks) {
      for (MIRBlock *succ : blockPtr->getSuccessors()) {
        truePreds[succ].push_back(blockPtr.get());
      }
    }

    for (auto it = blocks.begin(); it != blocks.end();) {
      MIRBlock *block = it->get();

      if (block != F->getEntryBlock() && truePreds[block].empty()) {

        // 1. Sever outgoing CFG edges to prevent dangling pointers in live
        // blocks!
        for (MIRBlock *succ : block->getSuccessors()) {
          succ->removePredecessor(block);

          // 2. Remove incoming phi entries from this dead block
          for (auto &inst : succ->getInstructionsMut()) {
            if (!inst)
              continue;
            if (auto *phi = llvm::dyn_cast_or_null<PhiInst>(inst.get())) {
              phi->removeIncoming(block);
            } else {
              break;
            }
          }
        }

        block->getSuccessors().clear();

        // 3. Now it is safe to erase the block
        it = blocks.erase(it);
        changed = true;
      } else {
        ++it;
      }
    }
  }
}

} // namespace mir
} // namespace moksha
