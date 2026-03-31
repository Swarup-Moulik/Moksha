#include "moksha/MIR/Passes/DropElisionPass.h"
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

namespace {

std::unordered_map<MIRBlock *, std::vector<MIRBlock *>>
buildPredecessorMap(MIRFunction *func) {
  std::unordered_map<MIRBlock *, std::vector<MIRBlock *>> preds;
  for (auto &blockPtr : func->getBlocks()) {
    MIRBlock *block = blockPtr.get();
    // Use the native, reliable CFG edges!
    for (auto *succ : block->getSuccessors()) {
      preds[succ].push_back(block);
    }
  }
  return preds;
}

// Traces a value back to its origin AllocaInst
MIRValue *getBaseAlloca(MIRValue *val) {
  while (val) {
    if (llvm::isa<AllocaInst>(val))
      return val;
    if (auto *load = llvm::dyn_cast<LoadInst>(val))
      val = load->getPointer();
    else if (auto *cast = llvm::dyn_cast<CastInst>(val))
      val = cast->getValue();
    else if (auto *ext = llvm::dyn_cast<ExtractValueInst>(val))
      val = ext->getAggregate();
    else
      break;
  }
  return nullptr;
}

} // namespace

bool DropElisionPass::runOnModule(MIRModule &M) {
  bool changed = false;
  for (auto &func : M.getFunctions()) {
    changed |= runOnFunction(func.get());
  }
  return changed;
}

bool DropElisionPass::runOnFunction(MIRFunction *F) {
  if (F->isDeclaration())
    return false;

  bool functionChanged = false;

  // ========================================================================
  // 1. Dataflow Analysis: Track which Allocas are fully moved
  // ========================================================================
  auto preds = buildPredecessorMap(F);
  std::unordered_map<MIRBlock *, std::unordered_set<MIRValue *>> blockMovedIn;
  std::unordered_map<MIRBlock *, std::unordered_set<MIRValue *>> blockMovedOut;
  bool changed = true;

  while (changed) {
    changed = false;
    for (auto &blockPtr : F->getBlocks()) {
      MIRBlock *block = blockPtr.get();

      std::unordered_set<MIRValue *> inSet;
      for (auto *pred : preds[block]) {
        for (auto *movedVal : blockMovedOut[pred]) {
          inSet.insert(movedVal);
        }
      }
      blockMovedIn[block] = inSet;

      std::unordered_set<MIRValue *> currentOut = inSet;

      for (auto &instPtr : block->getInstructions()) {
        MIRInst *inst = instPtr.get();

        if (auto *store = llvm::dyn_cast<StoreInst>(inst)) {
          // KILL: Re-initialization
          if (MIRValue *destAlloca = getBaseAlloca(store->getPointer())) {
            currentOut.erase(destAlloca);
          }

          // GEN: Moving a value
          if (auto *sourceLoad = llvm::dyn_cast<LoadInst>(store->getValue())) {
            if (sourceLoad->getName() != "cleanup_val" &&
                sourceLoad->getName() != "old_val") {
              if (sourceLoad->getBorrowKind() != BorrowKind::View) {

                // [FIX] Do not elide drops for ARC/Shared types! They are
                // copied, not moved.
                bool isShared = false;
                if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(
                        sourceLoad->getType())) {
                  if (ptrTy->getOwnership() == hir::Ownership::Shared)
                    isShared = true;
                } else if (sourceLoad->getType() &&
                           sourceLoad->getType()->toString().find("shared ") !=
                               std::string::npos) {
                  isShared = true;
                }

                if (!isShared) {
                  if (MIRValue *sourceAlloca =
                          getBaseAlloca(sourceLoad->getPointer())) {
                    currentOut.insert(sourceAlloca);
                  }
                }
              }
            }
          }

          // GEN: Consumed by function call
          if (auto *call = llvm::dyn_cast<CallInst>(inst)) {
            for (auto *arg : call->getArgs()) {
              if (auto *argLoad = llvm::dyn_cast<LoadInst>(arg)) {
                if (argLoad->getName() != "cleanup_val") {
                  if (argLoad->getBorrowKind() != BorrowKind::View) {

                    // [FIX] Prevent moving Shared types into functions
                    bool isShared = false;
                    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(
                            argLoad->getType())) {
                      if (ptrTy->getOwnership() == hir::Ownership::Shared)
                        isShared = true;
                    } else if (argLoad->getType() &&
                               argLoad->getType()->toString().find("shared ") !=
                                   std::string::npos) {
                      isShared = true;
                    }

                    if (!isShared) {
                      if (MIRValue *sourceAlloca =
                              getBaseAlloca(argLoad->getPointer())) {
                        currentOut.insert(sourceAlloca);
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }

      if (currentOut != blockMovedOut[block]) {
        blockMovedOut[block] = currentOut;
        changed = true;
      }
    }
  }

  // ========================================================================
  // 2. Elision Sweep: Remove Drops for Moved Variables
  // ========================================================================
  for (auto &blockPtr : F->getBlocks()) {
    MIRBlock *block = blockPtr.get();
    std::unordered_set<MIRValue *> movedAllocas = blockMovedIn[block];

    auto &insts = block->getInstructionsMut();
    auto it = insts.begin();

    while (it != insts.end()) {
      MIRInst *inst = it->get();
      bool elideInstruction = false;

      // Update local state within the block
      if (auto *store = llvm::dyn_cast<StoreInst>(inst)) {
        if (MIRValue *destAlloca = getBaseAlloca(store->getPointer())) {
          movedAllocas.erase(destAlloca);
        }
        if (auto *sourceLoad = llvm::dyn_cast<LoadInst>(store->getValue())) {
          if (sourceLoad->getName() != "cleanup_val" &&
              sourceLoad->getName() != "old_val") {
            // FIX: Do not track as moved if it is just a borrow!
            if (sourceLoad->getBorrowKind() != BorrowKind::View) {
              if (MIRValue *sourceAlloca =
                      getBaseAlloca(sourceLoad->getPointer())) {
                movedAllocas.insert(sourceAlloca);
              }
            }
          }
        }
      } else if (auto *call = llvm::dyn_cast<CallInst>(inst)) {
        for (auto *arg : call->getArgs()) {
          if (auto *argLoad = llvm::dyn_cast<LoadInst>(arg)) {
            if (argLoad->getName() != "cleanup_val") {
              // FIX: Do not track as moved if it is just a borrow!
              if (argLoad->getBorrowKind() != BorrowKind::View) {
                if (MIRValue *sourceAlloca =
                        getBaseAlloca(argLoad->getPointer())) {
                  movedAllocas.insert(sourceAlloca);
                }
              }
            }
          }
        }
      }

      // Check for Elision targets
      if (auto *arc = llvm::dyn_cast<ARCInst>(inst)) {
        if (arc->getOpcode() == Opcode::Release) {
          if (MIRValue *base = getBaseAlloca(arc->getObject())) {
            if (movedAllocas.count(base))
              elideInstruction = true;
          }
        }
      } else if (auto *call = llvm::dyn_cast<CallInst>(inst)) {
        if (call->getCallee()) {
          std::string calleeName = call->getCallee()->getName();

          // [FIX] Elide BOTH the memory free AND the custom class destructor!
          if (calleeName == "__moksha_free" ||
              calleeName.find(".destructor_ret_void") != std::string::npos ||
              calleeName.find(".drop_ret_void") != std::string::npos) {

            if (call->getArgs().size() > 0) {
              if (MIRValue *base = getBaseAlloca(call->getArgs()[0])) {
                if (movedAllocas.count(base))
                  elideInstruction = true;
              }
            }
          }
        }
      }

      if (elideInstruction) {
        it = insts.erase(it); // Safely remove the Destructor AND the Free
        functionChanged = true;
      } else {
        ++it;
      }
    }
  }

  return functionChanged;
}

} // namespace mir
} // namespace moksha
