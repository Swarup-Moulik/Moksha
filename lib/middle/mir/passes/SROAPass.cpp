#include "moksha/MIR/Passes/SROAPass.h"
#include "moksha/HIR/HIRModule.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <unordered_map>
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
      instPtr->replaceOperand(oldVal, newVal);
    }
  }
}

// ============================================================================
// [Pass Implementation]
// ============================================================================

bool SROAPass::runOnModule(MIRModule &M) {
  bool changed = false;
  for (auto &func : M.getFunctions()) {
    if (!func->isDeclaration()) {
      changed |= runOnFunction(func, M);
    }
  }
  return changed;
}

bool SROAPass::runOnFunction(MIRFunction *F, MIRModule &M) {
  bool changed = false;
  std::vector<AllocaInst *> structAllocas;

  // ------------------------------------------------------------------------
  // Phase 1: Find all Struct Allocas
  // ------------------------------------------------------------------------
  for (auto &blockPtr : F->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructions()) {
      if (auto *alloca = llvm::dyn_cast<AllocaInst>(instPtr.get())) {
        const hir::HIRType *allocTy = alloca->getAllocatedType();
        if (allocTy && allocTy->getKind() == hir::TypeKind::Struct) {
          structAllocas.push_back(alloca);
        }
      }
    }
  }

  if (structAllocas.empty()) {
    return false; // Nothing to shatter
  }

  // ------------------------------------------------------------------------
  // Phase 2: Build Local Def-Use Map
  // ------------------------------------------------------------------------
  std::unordered_map<MIRValue *, std::vector<MIRInst *>> uses;
  for (auto &blockPtr : F->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructions()) {
      MIRInst *inst = instPtr.get();
      if (auto *load = llvm::dyn_cast<LoadInst>(inst)) {
        uses[load->getPointer()].push_back(inst);
      } else if (auto *store = llvm::dyn_cast<StoreInst>(inst)) {
        uses[store->getValue()].push_back(inst);
        uses[store->getPointer()].push_back(inst);
      } else if (auto *gep = llvm::dyn_cast<GetElementPtrInst>(inst)) {
        uses[gep->getPointer()].push_back(inst);
      } else if (auto *cast = llvm::dyn_cast<CastInst>(inst)) {
        uses[cast->getValue()].push_back(inst);
      } else if (auto *call = llvm::dyn_cast<CallInst>(inst)) {
        for (auto *arg : call->getArgs()) {
          uses[arg].push_back(inst);
        }
      } else if (auto *mc = llvm::dyn_cast<MakeClosureInst>(inst)) {
        for (auto *cap : mc->getCaptures()) {
          uses[cap].push_back(inst);
        }
      }
    }
  }

  std::vector<MIRInst *> toDelete;

  // ------------------------------------------------------------------------
  // Phase 3: Evaluate and Shatter
  // ------------------------------------------------------------------------
  for (auto *alloca : structAllocas) {
    auto &allocUses = uses[alloca];
    bool canShatter = true;
    std::vector<GetElementPtrInst *> geps;

    // Check if the struct is ONLY used by constant GEPs
    for (auto *user : allocUses) {
      if (auto *gep = llvm::dyn_cast<GetElementPtrInst>(user)) {
        // Ensure GEP has {0, index} format for structs
        if (gep->getIndices().size() == 2) {
          auto *baseIdx = llvm::dyn_cast<ConstantInt>(gep->getIndices()[0]);
          auto *fieldIdx = llvm::dyn_cast<ConstantInt>(gep->getIndices()[1]);

          if (baseIdx && baseIdx->getValue() == 0 && fieldIdx) {
            geps.push_back(gep);
            continue;
          }
        }
      }

      canShatter = false;
      break;
    }

    if (!canShatter || geps.empty()) {
      continue;
    }

    // --- SHATTER IT! ---
    auto *structTy = llvm::cast<hir::StructType>(alloca->getAllocatedType());
    std::unordered_map<int, AllocaInst *> newAllocas;

    // 3a. Create a new scalar alloca for each field in the struct
    for (size_t i = 0; i < structTy->getFields().size(); ++i) {
      const auto *fieldTy = structTy->getFields()[i];

      // Just use the numeric index for the shattered variable name
      std::string fieldName = std::to_string(i);

      // Generate the PointerType required by AllocaInst
      const hir::HIRType *ptrTy = M.getPointerType(fieldTy);

      auto newAlloca = std::make_unique<AllocaInst>(
          ptrTy, fieldTy, alloca->getName() + "." + fieldName, alloca->getLoc(),
          0);

      newAllocas[i] = newAlloca.get();

      newAlloca->setParent(F->getEntryBlock());

      // Push the new allocas to the very top of the entry block
      F->getEntryBlock()->getInstructionsMut().insert(
          F->getEntryBlock()->getInstructionsMut().begin(),
          std::move(newAlloca));
    }

    // 3b. Redirect all GEPs to point directly to the new scalar allocas
    for (auto *gep : geps) {
      auto *idxVal = llvm::cast<ConstantInt>(gep->getIndices()[1]);
      int fieldIdx = idxVal->getValue();

      AllocaInst *replacement = newAllocas[fieldIdx];

      // [FIX] Handle nested struct accesses (Deep GEPs)
      if (gep->getIndices().size() == 2) {
        replaceAllUsesInFunction(F, gep, replacement);
        toDelete.push_back(gep);
      } else {
        // Rewrite the GEP to use the new base and shift the remaining indices
        std::vector<MIRValue *> newIdx;
        auto *i32Ty =
            reinterpret_cast<hir::HIRModule *>(&M)->getIntType(32, true);

        newIdx.push_back(
            M.getOrInsertConstant<ConstantInt>(0, i32Ty)); // New Base 0
        for (size_t i = 2; i < gep->getIndices().size(); ++i) {
          newIdx.push_back(gep->getIndices()[i]);
        }

        gep->setPointer(replacement); // Change base to the shattered field
        gep->setIndices(newIdx);      // Apply remaining path
      }
    }

    toDelete.push_back(alloca);
    changed = true;
  }

  // ------------------------------------------------------------------------
  // Phase 4: Cleanup Dead Instructions
  // ------------------------------------------------------------------------
  std::unordered_set<MIRInst *> deadSet(toDelete.begin(), toDelete.end());

  for (auto &blockPtr : F->getBlocks()) {
    auto &insts = blockPtr->getInstructionsMut();
    insts.erase(std::remove_if(insts.begin(), insts.end(),
                               [&](const std::unique_ptr<MIRInst> &inst) {
                                 return deadSet.count(inst.get()) > 0;
                               }),
                insts.end());
  }

  return changed;
}

} // namespace mir
} // namespace moksha
