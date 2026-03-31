#include "moksha/MIR/Passes/ConstantFoldingPass.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRGlobal.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "llvm/Support/Casting.h"
#include <unordered_set>

namespace moksha {
namespace mir {

// Helper to replace all uses of a value within a function
static void replaceAllUsesInFunction(MIRFunction *F, MIRValue *oldVal,
                                     MIRValue *newVal) {
  for (auto &blockPtr : F->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructionsMut()) {
      instPtr->replaceOperand(oldVal, newVal);
    }
  }
}

bool ConstantFoldingPass::runOnModule(MIRModule &M) {
  bool changed = false;

  // Global Constant Propagation
  std::unordered_set<MIRGlobal *> mutatedGlobals;

  // 1. Scan for any potential mutation of a global
  for (auto &func : M.getFunctions()) {
    if (func->isDeclaration())
      continue;
    for (auto &block : func->getBlocks()) {
      for (auto &inst : block->getInstructions()) {

        // Check Store Instructions
        if (auto *store = llvm::dyn_cast<StoreInst>(inst.get())) {
          // Direct mutation: Storing TO the global
          if (auto *g =
                  llvm::dyn_cast_or_null<MIRGlobal>(store->getPointer())) {
            mutatedGlobals.insert(g);
          }
          // Indirect mutation: Storing the global's address INTO something else
          if (auto *g = llvm::dyn_cast_or_null<MIRGlobal>(store->getValue())) {
            mutatedGlobals.insert(g);
          }
        }
        // Indirect mutation: Passing the global to a function
        else if (auto *call = llvm::dyn_cast<CallInst>(inst.get())) {
          for (MIRValue *arg : call->getArgs()) {
            if (auto *g = llvm::dyn_cast_or_null<MIRGlobal>(arg)) {
              mutatedGlobals.insert(g);
            }
          }
        }
        // Indirect mutation: Casting the global to another pointer type
        else if (auto *cast = llvm::dyn_cast<CastInst>(inst.get())) {
          if (auto *g = llvm::dyn_cast_or_null<MIRGlobal>(cast->getValue())) {
            mutatedGlobals.insert(g);
          }
        }
        // Indirect mutation: Pointer arithmetic on the global
        else if (auto *gep = llvm::dyn_cast<GetElementPtrInst>(inst.get())) {
          if (auto *g = llvm::dyn_cast_or_null<MIRGlobal>(gep->getPointer())) {
            mutatedGlobals.insert(g);
          }
        }
      }
    }
  }

  // 2. Replace loads of unmodified globals with their constant initializer
  for (auto &globalPtr : M.getGlobalsMut()) {
    MIRGlobal *g = globalPtr.get();
    if (g->getInitializer() && !g->isVolatile() &&
        mutatedGlobals.find(g) == mutatedGlobals.end()) {
      for (auto &func : M.getFunctions()) {
        if (func->isDeclaration())
          continue;
        for (auto &block : func->getBlocks()) {
          auto &insts = block->getInstructionsMut();
          for (auto it = insts.begin(); it != insts.end();) {
            if (auto *load = llvm::dyn_cast<LoadInst>(it->get())) {
              if (!load->isVolatile() && load->getPointer() == g) {
                MIRValue *replacement = g->getInitializer();

                if (replacement->getType() != load->getType()) {
                  if (llvm::isa<ConstantNull>(replacement)) {
                    replacement =
                        M.getOrInsertConstant<ConstantNull>(load->getType());
                  } else {
                    replacement = M.getOrInsertConstant<ConstantBitCast>(
                        replacement, load->getType());
                  }
                }

                replaceAllUsesInFunction(func.get(), load, replacement);
                it = insts.erase(it); // Remove the LoadInst
                changed = true;
                continue;
              }
            }
            ++it;
          }
        }
      }
    }
  }

  for (auto &func : M.getFunctions()) {
    if (func->isDeclaration())
      continue;

    for (auto &block : func->getBlocks()) {
      auto &insts = block->getInstructionsMut();

      // 1. Fold individual instructions (Slice access, arithmetic, etc.)
      for (auto it = insts.begin(); it != insts.end();) {
        MIRInst *inst = it->get();
        MIRValue *folded = nullptr;

        // Fold Slice Property Accesses
        if (auto *ext = llvm::dyn_cast<ExtractValueInst>(inst)) {
          if (auto *constSlice =
                  llvm::dyn_cast_or_null<ConstantSlice>(ext->getAggregate())) {

            if (ext->getIndex() == 1) { // Index 1 is the .length/size
              folded = M.getOrInsertConstant<ConstantInt>(
                  constSlice->getElements().size(), ext->getType());
            } else if (ext->getIndex() == 0) { // Index 0 is the data pointer
              // To fold the pointer, we create a ConstantArray of the elements
              // and return a BitCast of that array to the expected pointer type
              if (auto *sliceTy =
                      llvm::dyn_cast<hir::SliceType>(constSlice->getType())) {
                auto *elemTy = sliceTy->getElementType();
                auto *arrayTy =
                    M.getArrayType(elemTy, constSlice->getElements().size());

                // 1. Get the elements from the slice
                auto elementsRef = constSlice->getElements();

                // 2. Convert ArrayRef<MIRConstant*> to std::vector<MIRValue*>
                std::vector<mir::MIRValue *> elementsVec(elementsRef.begin(),
                                                         elementsRef.end());

                // 3. Pass the Type FIRST, then the vector!
                auto *constArray = M.getOrInsertConstant<ConstantArray>(
                    arrayTy, std::move(elementsVec));

                folded = M.getOrInsertConstant<ConstantBitCast>(constArray,
                                                                ext->getType());
              }
            }
          }
        }

        // Fold Compare Instructions (e.g., icmp ne null, null)
        if (auto *icmp = llvm::dyn_cast<CompareInst>(inst)) {
          // If comparing the exact same SSA value, or two nulls
          if (icmp->getLHS() == icmp->getRHS() ||
              (llvm::isa<ConstantNull>(icmp->getLHS()) &&
               llvm::isa<ConstantNull>(icmp->getRHS()))) {

            if (icmp->getPredicate() == CompareInst::Predicate::EQ) {
              folded =
                  M.getOrInsertConstant<ConstantBool>(true, icmp->getType());
            } else if (icmp->getPredicate() == CompareInst::Predicate::NE) {
              folded =
                  M.getOrInsertConstant<ConstantBool>(false, icmp->getType());
            }
          }
        }

        if (folded) {
          replaceAllUsesInFunction(func.get(), inst, folded);
          it = insts.erase(it); // Remove the redundant instruction
          changed = true;
          continue; // Don't increment iterator
        }
        ++it;
      }

      // 2. Fold Control Flow (Branches)
      if (block->getInstructions().empty())
        continue;

      MIRInst *terminator = block->getInstructions().back().get();
      if (auto *condBr = llvm::dyn_cast_or_null<CondBranchInst>(terminator)) {
        if (auto *constBool =
                llvm::dyn_cast_or_null<ConstantBool>(condBr->getCondition())) {

          MIRBlock *target = constBool->getValue() ? condBr->getTrueBlock()
                                                   : condBr->getFalseBlock();
          MIRBlock *deadTarget = constBool->getValue() ? condBr->getFalseBlock()
                                                       : condBr->getTrueBlock();

          // Rewrite to unconditional branch
          auto newBr = std::make_unique<BranchInst>(target, condBr->getLoc());
          block->getInstructionsMut().pop_back();
          block->addInstruction(std::move(newBr));

          // Clean up dead CFG edges
          if (deadTarget != target) {
            for (auto &instPtr : deadTarget->getInstructionsMut()) {
              if (auto *phi = llvm::dyn_cast_or_null<PhiInst>(instPtr.get())) {
                phi->removeIncoming(block.get());
              } else {
                break;
              }
            }
            block->removeSuccessor(deadTarget);
            deadTarget->removePredecessor(block.get());
          }
          changed = true;
        }
      }
    }
  }
  return changed;
}

} // namespace mir
} // namespace moksha
