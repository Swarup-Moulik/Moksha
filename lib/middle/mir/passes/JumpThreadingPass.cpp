#include "moksha/MIR/Passes/JumpThreadingPass.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "llvm/Support/Casting.h"

namespace moksha {
namespace mir {

bool JumpThreadingPass::runOnModule(MIRModule &M) {
  bool changed = false;
  for (auto &func : M.getFunctions()) {
    if (func->isDeclaration())
      continue;

    for (auto &block : func->getBlocks()) {
      // Look for a CondBranch at the end of the block
      if (block->getInstructions().empty())
        continue;
      auto *condBr = llvm::dyn_cast_or_null<CondBranchInst>(
          block->getInstructions().back().get());
      if (!condBr)
        continue;

      // Ensure the condition is an ICmp checking a Phi against Null
      auto *icmp = llvm::dyn_cast_or_null<CompareInst>(condBr->getCondition());
      if (!icmp || icmp->getPredicate() != CompareInst::Predicate::NE)
        continue;

      auto *phi = llvm::dyn_cast_or_null<PhiInst>(icmp->getLHS());
      auto *nullConst = llvm::dyn_cast_or_null<ConstantNull>(icmp->getRHS());
      if (!phi || !nullConst || phi->getParent() != block.get())
        continue;

      // Analyze the incoming edges to the Phi
      for (auto &[val, predBlock] : phi->getIncoming()) {
        // If an incoming edge is definitively null...
        if (llvm::isa<ConstantNull>(val)) {
          // The icmp will be FALSE for this edge.
          MIRBlock *bailoutBlock = condBr->getFalseBlock();

          // ========================================================================
          // [CRITICAL FIX]: Abort threading if the target block has Phi nodes!
          // We cannot bypass the current block because the target's Phis depend
          // on it.
          // ========================================================================
          bool targetHasPhis = false;
          if (!bailoutBlock->getInstructions().empty()) {
            if (llvm::isa<PhiInst>(
                    bailoutBlock->getInstructions().front().get())) {
              targetHasPhis = true;
            }
          }
          if (targetHasPhis)
            continue;

          if (predBlock->getInstructions().empty())
            continue;

          MIRInst *term = predBlock->getInstructions().back().get();

          if (auto *predBr = llvm::dyn_cast_or_null<BranchInst>(term)) {
            // Rewrite unconditional branch in predecessor
            auto newBr =
                std::make_unique<BranchInst>(bailoutBlock, predBr->getLoc());
            predBlock->getInstructionsMut().pop_back();
            predBlock->addInstruction(std::move(newBr));

            // Fixup CFG Edges
            predBlock->removeSuccessor(block.get());
            block->removePredecessor(predBlock);

            bool hasBailoutPred = false;
            for (auto *p : bailoutBlock->getPredecessors()) {
              if (p == predBlock) {
                hasBailoutPred = true;
                break;
              }
            }
            if (!hasBailoutPred) {
              predBlock->addSuccessor(bailoutBlock);
              bailoutBlock->addPredecessor(predBlock);
            }

            // Clean up stale Phi edges in the current block
            for (auto &instPtr : block->getInstructionsMut()) {
              if (auto *p = llvm::dyn_cast_or_null<PhiInst>(instPtr.get())) {
                p->removeIncoming(predBlock);
              } else {
                break; // Phis are always at the top
              }
            }

            changed = true;
            break; // Restart analysis as CFG mutated

          } else if (auto *condBrPred =
                         llvm::dyn_cast_or_null<CondBranchInst>(term)) {
            // Handle jumping through conditional branches
            if (condBrPred->getTrueBlock() == block.get()) {
              condBrPred->setTrueBlock(bailoutBlock);
            } else if (condBrPred->getFalseBlock() == block.get()) {
              condBrPred->setFalseBlock(bailoutBlock);
            }

            // Rebuild CFG Edges accurately
            predBlock->getSuccessors().clear();
            predBlock->addSuccessor(condBrPred->getTrueBlock());
            if (condBrPred->getTrueBlock() != condBrPred->getFalseBlock()) {
              predBlock->addSuccessor(condBrPred->getFalseBlock());
            }

            block->removePredecessor(predBlock);

            // Re-add the predecessor link if the other branch STILL points to
            // 'block'
            if (condBrPred->getTrueBlock() == block.get() ||
                condBrPred->getFalseBlock() == block.get()) {
              block->addPredecessor(predBlock);
            } else {
              // Clean up stale Phi edges
              for (auto &instPtr : block->getInstructionsMut()) {
                if (auto *p = llvm::dyn_cast_or_null<PhiInst>(instPtr.get())) {
                  p->removeIncoming(predBlock);
                } else {
                  break;
                }
              }
            }

            // Safely link the bailout block
            bool hasBailoutPred = false;
            for (auto *p : bailoutBlock->getPredecessors()) {
              if (p == predBlock) {
                hasBailoutPred = true;
                break;
              }
            }
            if (!hasBailoutPred) {
              bailoutBlock->addPredecessor(predBlock);
            }

            changed = true;
            break; // Restart analysis as CFG mutated
          }
        }
      }
    }
  }
  return changed;
}

} // namespace mir
} // namespace moksha
