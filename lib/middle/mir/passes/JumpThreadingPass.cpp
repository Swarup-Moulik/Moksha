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
      if (block->getInstructions().empty())
        continue;
      auto *condBr = llvm::dyn_cast_or_null<CondBranchInst>(
          block->getInstructions().back().get());
      if (!condBr)
        continue;

      auto *icmp = llvm::dyn_cast_or_null<CompareInst>(condBr->getCondition());
      if (!icmp || icmp->getPredicate() != CompareInst::Predicate::NE)
        continue;

      auto *phi = llvm::dyn_cast_or_null<PhiInst>(icmp->getLHS());
      auto *nullConst = llvm::dyn_cast_or_null<ConstantNull>(icmp->getRHS());
      if (!phi || !nullConst || phi->getParent() != block.get())
        continue;

      for (auto &[val, predBlock] : phi->getIncoming()) {
        if (llvm::isa<ConstantNull>(val)) {
          MIRBlock *bailoutBlock = condBr->getFalseBlock();

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
            auto newBr =
                std::make_unique<BranchInst>(bailoutBlock, predBr->getLoc());
            predBlock->getInstructionsMut().pop_back();
            predBlock->addInstruction(std::move(newBr));
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

            for (auto &instPtr : block->getInstructionsMut()) {
              if (auto *p = llvm::dyn_cast_or_null<PhiInst>(instPtr.get())) {
                p->removeIncoming(predBlock);
              } else {
                break;
              }
            }

            changed = true;
            break;

          } else if (auto *condBrPred =
                         llvm::dyn_cast_or_null<CondBranchInst>(term)) {
            if (condBrPred->getTrueBlock() == block.get()) {
              condBrPred->setTrueBlock(bailoutBlock);
            } else if (condBrPred->getFalseBlock() == block.get()) {
              condBrPred->setFalseBlock(bailoutBlock);
            }

            predBlock->getSuccessors().clear();
            predBlock->addSuccessor(condBrPred->getTrueBlock());
            if (condBrPred->getTrueBlock() != condBrPred->getFalseBlock()) {
              predBlock->addSuccessor(condBrPred->getFalseBlock());
            }

            block->removePredecessor(predBlock);

            if (condBrPred->getTrueBlock() == block.get() ||
                condBrPred->getFalseBlock() == block.get()) {
              block->addPredecessor(predBlock);
            } else {
              for (auto &instPtr : block->getInstructionsMut()) {
                if (auto *p = llvm::dyn_cast_or_null<PhiInst>(instPtr.get())) {
                  p->removeIncoming(predBlock);
                } else {
                  break;
                }
              }
            }

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
            break;
          }
        }
      }
    }
  }
  return changed;
}

} // namespace mir
} // namespace moksha
