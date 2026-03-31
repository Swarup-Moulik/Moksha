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
      auto *condBr =
          llvm::dyn_cast<CondBranchInst>(block->getInstructions().back().get());
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

          // Thread the jump: Change the predecessor to branch directly to the
          // bailout
          if (predBlock->getInstructions().empty())
            continue;
          if (auto *predBr = llvm::dyn_cast<BranchInst>(
                  predBlock->getInstructions().back().get())) {

            // Rewrite unconditional branch in predecessor
            auto newBr =
                std::make_unique<BranchInst>(bailoutBlock, predBr->getLoc());
            predBlock->getInstructionsMut().pop_back();
            predBlock->addInstruction(std::move(newBr));

            // Fixup CFG Edges
            predBlock->removeSuccessor(block.get());
            block->removePredecessor(predBlock);
            predBlock->addSuccessor(bailoutBlock);
            bailoutBlock->addPredecessor(predBlock);

            // Clean up stale Phi edges to prevent the Infinite Loop!
            for (auto &instPtr : block->getInstructionsMut()) {
              if (auto *p = llvm::dyn_cast_or_null<PhiInst>(instPtr.get())) {
                p->removeIncoming(predBlock);
              } else {
                break; // Phis are always at the top of the block, so we can
                       // break early
              }
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
