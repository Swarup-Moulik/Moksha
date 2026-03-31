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

// ============================================================================
// [Helper] Replace All Uses
// ============================================================================
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

// ============================================================================
// [Pass Implementation]
// ============================================================================

bool SimplifyCFGPass::runOnModule(MIRModule &M) {
  bool changed = false;
  for (auto &func : M.getFunctions()) {
    if (!func->isDeclaration()) {
      changed |= runOnFunction(func.get());
    }
  }
  return changed;
}

bool SimplifyCFGPass::runOnFunction(MIRFunction *F) {
  bool changed = false;
  bool localChanged = true;

  // Run the simplification loops until the control flow graph stabilizes
  while (localChanged) {
    localChanged = false;
    localChanged |= foldBranches(F);
    localChanged |= bypassEmptyBlocks(F);
    localChanged |= mergeBlocks(F);
    changed |= localChanged;
  }

  // Once stabilized, sweep out any orphaned/unreachable blocks
  if (changed) {
    removeDeadBlocks(F);
  }

  return changed;
}

// ----------------------------------------------------------------------------
// 1. Branch Folding
// ----------------------------------------------------------------------------
// Converts `br bool %cond, label %X, label %X` into `br label %X`
bool SimplifyCFGPass::foldBranches(MIRFunction *F) {
  bool changed = false;
  for (auto &blockPtr : F->getBlocks()) {
    MIRBlock *b = blockPtr.get();
    if (b->getInstructions().empty())
      continue;

    auto *condBr =
        llvm::dyn_cast<CondBranchInst>(b->getInstructions().back().get());

    if (condBr && condBr->getTrueBlock() == condBr->getFalseBlock()) {
      MIRBlock *target = condBr->getTrueBlock();

      // Replace with unconditional branch
      auto newBr = std::make_unique<BranchInst>(target, condBr->getLoc());
      b->getInstructionsMut().pop_back();
      b->addInstruction(std::move(newBr));

      changed = true;
    }
  }
  return changed;
}

// ----------------------------------------------------------------------------
// 2. Empty Block Bypassing
// ----------------------------------------------------------------------------
// If Block B is empty and unconditionally jumps to C, rewrite A to jump to C.
bool SimplifyCFGPass::bypassEmptyBlocks(MIRFunction *F) {
  for (auto &blockPtr : F->getBlocks()) {
    MIRBlock *B = blockPtr.get();

    // Never bypass the entry block
    if (B == F->getEntryBlock())
      continue;

    // [FIX] If the block is already dead (0 predecessors), ignore it!
    // Otherwise, the pass will infinitely loop trying to bypass an orphaned
    // block.
    if (B->getPredecessors().empty())
      continue;

    // An empty block has exactly one instruction: an unconditional jump
    if (B->getInstructions().size() != 1)
      continue;

    auto *br = llvm::dyn_cast<BranchInst>(B->getInstructions().front().get());
    if (!br)
      continue;

    MIRBlock *C = br->getTarget();
    if (B == C)
      continue; // Ignore infinite self-loops

    // Bypass B: Wire all of B's predecessors directly to C
    // Copy predecessors to avoid iterator invalidation during mutation
    std::vector<MIRBlock *> preds = B->getPredecessors();

    for (MIRBlock *A : preds) {
      // 1. Rewrite A's terminator to point to C instead of B.
      auto *term = A->getInstructionsMut().back().get();
      term->replaceOperand(B, C);

      // 2. Update CFG Edges
      A->removeSuccessor(B);
      A->addSuccessor(C);
      C->addPredecessor(A);

      // 3. Update Phi Nodes in C: Inherit values coming from B, but attribute
      // them to A
      for (auto &inst : C->getInstructionsMut()) {
        if (auto *phi = llvm::dyn_cast<PhiInst>(inst.get())) {
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
          break; // Phis are always at the top of the block
        }
      }
    }

    // Disconnect B from C
    C->removePredecessor(B);
    for (auto &inst : C->getInstructionsMut()) {
      if (auto *phi = llvm::dyn_cast<PhiInst>(inst.get())) {
        phi->removeIncoming(B);
      } else {
        break;
      }
    }
    B->getPredecessors().clear();

    // Return immediately to restart the scan (since the graph mutated)
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------------
// 3. Block Merging
// ----------------------------------------------------------------------------
// If A unconditionally jumps to B, and A is B's ONLY predecessor, merge them.
bool SimplifyCFGPass::mergeBlocks(MIRFunction *F) {
  for (auto &blockPtr : F->getBlocks()) {
    MIRBlock *A = blockPtr.get();
    if (A->getInstructions().empty())
      continue;

    auto *br = llvm::dyn_cast<BranchInst>(A->getInstructions().back().get());
    if (!br)
      continue;

    MIRBlock *B = br->getTarget();

    // Cannot merge into the entry block, and cannot merge self-loops
    if (B == A || B == F->getEntryBlock())
      continue;

    // We can only merge if A is B's EXCLUSIVE predecessor
    if (B->getPredecessors().size() == 1 && B->getPredecessors()[0] == A) {

      // 1. Delete the jump from A to B
      A->getInstructionsMut().pop_back();

      // 2. Move instructions from B into A
      for (auto &inst : B->getInstructionsMut()) {
        if (auto *phi = llvm::dyn_cast<PhiInst>(inst.get())) {
          // Because B only has 1 predecessor (A), this Phi is purely redundant.
          // Extract the single incoming value, replace all uses, and drop the
          // Phi!
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

      // 3. Update CFG Edges: A inherits B's successors
      A->removeSuccessor(B);
      for (MIRBlock *succ : B->getSuccessors()) {
        A->addSuccessor(succ);
        succ->removePredecessor(B);
        succ->addPredecessor(A);

        // 4. Update Phis in the successors to expect edges from A instead of B
        for (auto &inst : succ->getInstructionsMut()) {
          if (auto *phi = llvm::dyn_cast<PhiInst>(inst.get())) {
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

      return true; // Return immediately to restart scan
    }
  }
  return false;
}

// ----------------------------------------------------------------------------
// 4. Dead Block Cleanup
// ----------------------------------------------------------------------------
// Deletes blocks that have zero predecessors (except the Entry Block)
void SimplifyCFGPass::removeDeadBlocks(MIRFunction *F) {
  // NOTE: Requires `std::vector<std::unique_ptr<MIRBlock>>& getBlocksMut();`
  // to be defined in `MIRFunction.h`.
  auto &blocks = F->getBlocksMut();

  blocks.erase(std::remove_if(blocks.begin(), blocks.end(),
                              [&](const std::unique_ptr<MIRBlock> &block) {
                                return block.get() != F->getEntryBlock() &&
                                       block->getPredecessors().empty();
                              }),
               blocks.end());
}

} // namespace mir
} // namespace moksha
