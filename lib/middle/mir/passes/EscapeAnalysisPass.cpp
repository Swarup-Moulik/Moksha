#include "moksha/MIR/Passes/EscapeAnalysisPass.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "llvm/Support/Casting.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace moksha {
namespace mir {

// Helper to swap BitCast uses with the new Alloca
static void replaceAllUsesInFunction(MIRFunction *F, MIRValue *oldVal,
                                     MIRValue *newVal) {
  for (auto &blockPtr : F->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructionsMut()) {
      instPtr->replaceOperand(oldVal, newVal);
    }
  }
}

bool EscapeAnalysisPass::runOnModule(MIRModule &M) {
  bool changed = false;
  for (auto &func : M.getFunctions()) {
    changed |= runOnFunction(func.get(), M);
  }
  return changed;
}

bool EscapeAnalysisPass::runOnFunction(MIRFunction *F, MIRModule &M) {
  if (F->isDeclaration())
    return false;

  bool changed = false;
  std::vector<CallInst *> allocations;

  // 1. Build a Def-Use map for the function
  std::unordered_map<MIRValue *, std::vector<MIRInst *>> defUse;
  for (auto &block : F->getBlocks()) {
    for (auto &instPtr : block->getInstructions()) {
      MIRInst *inst = instPtr.get();

      if (auto *store = llvm::dyn_cast<StoreInst>(inst)) {
        defUse[store->getValue()].push_back(inst);
        defUse[store->getPointer()].push_back(inst);
      } else if (auto *load = llvm::dyn_cast<LoadInst>(inst)) {
        defUse[load->getPointer()].push_back(inst);
      } else if (auto *gep = llvm::dyn_cast<GetElementPtrInst>(inst)) {
        defUse[gep->getPointer()].push_back(inst);
      } else if (auto *cast = llvm::dyn_cast<CastInst>(inst)) {
        defUse[cast->getValue()].push_back(inst);
      } else if (auto *call = llvm::dyn_cast<CallInst>(inst)) {
        for (auto *arg : call->getArgs())
          defUse[arg].push_back(inst);

        // Track Heap Allocations
        if (call->getCallee() &&
            call->getCallee()->getName() == "__moksha_alloc") {
          allocations.push_back(call);
        }
      } else if (auto *arc = llvm::dyn_cast<ARCInst>(inst)) {
        defUse[arc->getObject()].push_back(inst);
      } else if (auto *ret = llvm::dyn_cast<ReturnInst>(inst)) {
        if (ret->getReturnValue())
          defUse[ret->getReturnValue()].push_back(inst);
      }
    }
  }

  // 2. Analyze each allocation to see if it escapes
  for (CallInst *alloc : allocations) {
    if (!doesEscape(alloc, defUse)) {

      // Find the bitcast to get the actual target type (e.g., Arc<Cycle>)
      CastInst *bitcast = nullptr;
      for (auto *user : defUse[alloc]) {
        if (auto *cast = llvm::dyn_cast<CastInst>(user)) {
          bitcast = cast;
          break;
        }
      }

      if (!bitcast)
        continue;

      // Extract the underlying allocated type (e.g., Cycle)
      const hir::HIRType *actualType = bitcast->getType();
      const hir::HIRType *pointeeType = nullptr;
      if (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(actualType)) {
        pointeeType = pTy->getPointee();
      } else {
        pointeeType = actualType;
      }

      // 3. Promote to Stack: Create an AllocaInst
      auto alloca = std::make_unique<AllocaInst>(actualType, pointeeType,
                                                 alloc->getName() + ".stack",
                                                 alloc->getLoc(), 8);
      alloca->setBorrowKind(BorrowKind::Mut);
      MIRBlock *entryBlock = F->getEntryBlock();
      alloca->setParent(entryBlock);

      MIRValue *newAllocaPtr = alloca.get();

      // [FIX 1: Prevent Loop Stack Overflow] Hoist Alloca to the Entry Block!
      entryBlock->getInstructionsMut().insert(
          entryBlock->getInstructionsMut().begin(), std::move(alloca));

      // Replace all uses of the old BitCast with the new stack Alloca
      replaceAllUsesInFunction(F, bitcast, newAllocaPtr);

      // Hard Erase the old BitCast
      auto &castInsts = bitcast->getParent()->getInstructionsMut();
      castInsts.erase(std::remove_if(castInsts.begin(), castInsts.end(),
                                     [&](const std::unique_ptr<MIRInst> &i) {
                                       return i.get() == bitcast;
                                     }),
                      castInsts.end());

      std::vector<CallInst *> freeCalls; // Track ALL frees
      if (defUse.count(bitcast)) {
        for (auto *user : defUse[bitcast]) {
          if (auto *call = llvm::dyn_cast<CallInst>(user)) {
            if (call->getCallee() &&
                call->getCallee()->getName() == "__moksha_free") {
              freeCalls.push_back(call); // Don't break!
            }
          }
        }
      }

      // Erase all found frees
      for (CallInst *freeCall : freeCalls) {
        auto &freeInsts = freeCall->getParent()->getInstructionsMut();
        freeInsts.erase(std::remove_if(freeInsts.begin(), freeInsts.end(),
                                       [&](const std::unique_ptr<MIRInst> &i) {
                                         return i.get() == freeCall;
                                       }),
                        freeInsts.end());
      }

      // 5. Hard Erase the original Allocation
      auto &allocInsts = alloc->getParent()->getInstructionsMut();
      allocInsts.erase(std::remove_if(allocInsts.begin(), allocInsts.end(),
                                      [&](const std::unique_ptr<MIRInst> &i) {
                                        return i.get() == alloc;
                                      }),
                       allocInsts.end());

      changed = true;
    }
  }

  return changed;
}

bool EscapeAnalysisPass::doesEscape(
    MIRValue *val,
    const std::unordered_map<MIRValue *, std::vector<MIRInst *>> &defUse) {
  std::vector<MIRValue *> worklist = {val};
  std::unordered_set<MIRValue *> visited;

  while (!worklist.empty()) {
    MIRValue *curr = worklist.back();
    worklist.pop_back();

    if (!visited.insert(curr).second)
      continue;

    auto it = defUse.find(curr);
    if (it == defUse.end())
      continue;

    for (MIRInst *user : it->second) {
      if (auto *store = llvm::dyn_cast<StoreInst>(user)) {
        // If we are storing the pointer INTO something else, it escapes!
        if (store->getValue() == curr)
          return true;
      } else if (auto *call = llvm::dyn_cast<CallInst>(user)) {
        // Passing the pointer to any external/un-inlined function escapes it
        if (call->getCallee() &&
            call->getCallee()->getName() != "__moksha_alloc") {
          return true;
        }
      } else if (llvm::isa<ReturnInst>(user)) {
        return true; // Returning the pointer escapes it
      } else if (auto *gep = llvm::dyn_cast<GetElementPtrInst>(user)) {
        worklist.push_back(gep); // Trace through pointer arithmetic
      } else if (auto *cast = llvm::dyn_cast<CastInst>(user)) {
        worklist.push_back(cast); // Trace through bitcasts
      } else if (auto *ext = llvm::dyn_cast<ExtractValueInst>(user)) {
        if (ext->getIndex() == 0) {
          worklist.push_back(ext);
        }
      } else if (llvm::isa<MakeClosureInst>(user)) {
        return true; // Capturing memory into a closure escapes it
      }
      // LoadInst and ARCInst are safe; they don't leak the pointer identity
    }
  }
  return false;
}

} // namespace mir
} // namespace moksha
