#include "moksha/MIR/Passes/Mem2RegPass.h"
#include "moksha/MIR/Analysis/MIRDominance.h"
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

static std::unordered_map<MIRBlock *, std::unordered_set<MIRBlock *>>
computeDominanceFrontiers(MIRFunction *F, const MIRDominance &dom) {
  std::unordered_map<MIRBlock *, std::unordered_set<MIRBlock *>> DF;

  std::unordered_map<MIRBlock *, std::vector<MIRBlock *>> truePreds;
  for (auto &blockPtr : F->getBlocks()) {
    for (auto *succ : blockPtr->getSuccessors()) {
      truePreds[succ].push_back(blockPtr.get());
    }
  }

  for (auto &blockPtr : F->getBlocks()) {
    MIRBlock *b = blockPtr.get();

    if (truePreds[b].size() >= 2) {
      for (MIRBlock *pred : truePreds[b]) {
        MIRBlock *runner = pred;

        while (runner && runner != dom.getIDom(b)) {
          DF[runner].insert(b);
          runner = dom.getIDom(runner);
        }
      }
    }
  }
  return DF;
}

// [Helper] Replace All Uses (Fallback for missing Use-Def Chains)
static void replaceAllUsesInFunction(MIRFunction *F, MIRValue *oldVal,
                                     MIRValue *newVal) {
  for (auto &blockPtr : F->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructionsMut()) {
      instPtr->replaceOperand(oldVal, newVal);
    }
  }
}

bool Mem2RegPass::runOnModule(MIRModule &M) {
  bool changed = false;
  for (auto &func : M.getFunctions()) {
    if (!func->isDeclaration()) {
      changed |= runOnFunction(func, M);
    }
  }
  return changed;
}

bool Mem2RegPass::runOnFunction(MIRFunction *F, MIRModule &M) {
  // PRE-PASS: HEAL CFG BEFORE DOMINANCE ANALYSIS
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
    } else if (auto *switchInst = llvm::dyn_cast_or_null<SwitchInst>(term)) {
      b->addSuccessor(switchInst->getDefaultBlock());
      switchInst->getDefaultBlock()->addPredecessor(b);
      for (auto &casePair : switchInst->getCases()) {
        b->addSuccessor(casePair.second);
        casePair.second->addPredecessor(b);
      }
    } else if (auto *throwInst = llvm::dyn_cast_or_null<ThrowInst>(term)) {
      if (throwInst->getUnwindDest()) {
        b->addSuccessor(throwInst->getUnwindDest());
        throwInst->getUnwindDest()->addPredecessor(b);
      }
    }
  }

  // Phase 1: Identify Promotable Allocas & Record Definitions
  std::vector<AllocaInst *> promotableAllocas;
  std::unordered_map<AllocaInst *, std::vector<MIRBlock *>> defBlocks;
  std::unordered_set<AllocaInst *> promotableSet;

  auto isPromotable = [](const hir::HIRType *ty) {
    if (!ty)
      return false;
    return ty->getKind() == hir::TypeKind::Int ||
           ty->getKind() == hir::TypeKind::Float ||
           ty->getKind() == hir::TypeKind::Bool ||
           ty->getKind() == hir::TypeKind::Pointer;
  };

  // Pass 1.1: Identify escaping uses to filter unpromotable allocas.
  std::unordered_set<AllocaInst *> escapedAllocas;
  auto markEscaped = [&](MIRValue *val) {
    if (!val)
      return;
    if (auto *alloca = llvm::dyn_cast_or_null<AllocaInst>(val)) {
      escapedAllocas.insert(alloca);
    }
  };

  for (auto &blockPtr : F->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructions()) {
      MIRInst *i = instPtr.get();
      if (auto *load = llvm::dyn_cast_or_null<LoadInst>(i)) {
        if (load->isVolatile())
          markEscaped(load->getPointer());
      } else if (auto *store = llvm::dyn_cast_or_null<StoreInst>(i)) {
        markEscaped(store->getValue());
        if (store->isVolatile())
          markEscaped(store->getPointer());
      } else if (auto *gep = llvm::dyn_cast_or_null<GetElementPtrInst>(i)) {
        markEscaped(gep->getPointer());
        for (auto *idx : gep->getIndices())
          markEscaped(idx);
      } else if (auto *call = llvm::dyn_cast_or_null<CallInst>(i)) {
        markEscaped(call->getCallee());
        for (auto *a : call->getArgs())
          markEscaped(a);
      } else if (auto *cast = llvm::dyn_cast_or_null<CastInst>(i)) {
        markEscaped(cast->getValue());
      } else if (auto *phi = llvm::dyn_cast_or_null<PhiInst>(i)) {
        for (auto &p : phi->getIncoming())
          markEscaped(p.first);
      } else if (auto *bin = llvm::dyn_cast_or_null<BinaryInst>(i)) {
        markEscaped(bin->getLHS());
        markEscaped(bin->getRHS());
      } else if (auto *cmp = llvm::dyn_cast_or_null<CompareInst>(i)) {
        markEscaped(cmp->getLHS());
        markEscaped(cmp->getRHS());
      } else if (auto *fcmp = llvm::dyn_cast_or_null<FCmpInst>(i)) {
        markEscaped(fcmp->getLHS());
        markEscaped(fcmp->getRHS());
      } else if (auto *ret = llvm::dyn_cast_or_null<ReturnInst>(i)) {
        markEscaped(ret->getReturnValue());
      } else if (auto *br = llvm::dyn_cast_or_null<CondBranchInst>(i)) {
        markEscaped(br->getCondition());
      } else if (auto *sw = llvm::dyn_cast_or_null<SwitchInst>(i)) {
        markEscaped(sw->getCondition());
        for (auto &c : sw->getCases())
          markEscaped(c.first);
      } else if (auto *inv = llvm::dyn_cast_or_null<InvokeInst>(i)) {
        markEscaped(inv->getCallee());
        for (auto *a : inv->getArgs())
          markEscaped(a);
      } else if (auto *res = llvm::dyn_cast_or_null<ResumeInst>(i)) {
        markEscaped(res->getException());
      } else if (auto *thr = llvm::dyn_cast_or_null<ThrowInst>(i)) {
        markEscaped(thr->getException());
      } else if (auto *ia = llvm::dyn_cast_or_null<InlineAsmInst>(i)) {
        for (auto *a : ia->getArgs())
          markEscaped(a);
      } else if (auto *makeClosure =
                     llvm::dyn_cast_or_null<MakeClosureInst>(i)) {
        for (MIRValue *cap : makeClosure->getCaptures()) {
          MIRValue *tracedCap = cap;
          while (auto *load = llvm::dyn_cast_or_null<LoadInst>(tracedCap)) {
            tracedCap = load->getPointer();
          }
          markEscaped(tracedCap);
        }
      } else if (auto *spawn = llvm::dyn_cast_or_null<SpawnInst>(i)) {
        markEscaped(spawn->getClosure());
      } else if (auto *awaitInst = llvm::dyn_cast_or_null<AwaitInst>(i)) {
        markEscaped(awaitInst->getPromise());
      } else if (auto *ext = llvm::dyn_cast_or_null<ExtractValueInst>(i)) {
        markEscaped(ext->getAggregate());
      } else if (auto *ins = llvm::dyn_cast_or_null<InsertValueInst>(i)) {
        markEscaped(ins->getAggregate());
        markEscaped(ins->getValue());
      } else if (auto *arc = llvm::dyn_cast_or_null<ARCInst>(i)) {
        markEscaped(arc->getObject());
      }
    }
  }

  // Pass 1.2: Record promotable allocas and their def blocks.
  for (auto &blockPtr : F->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructions()) {
      if (auto *alloca = llvm::dyn_cast_or_null<AllocaInst>(instPtr.get())) {
        const hir::HIRType *allocTy = alloca->getAllocatedType();
        if (!isPromotable(allocTy) ||
            (allocTy &&
             (allocTy->getKind() == hir::TypeKind::Weak ||
              allocTy->toString().find("weak ") != std::string::npos))) {
          continue;
        }
        if (escapedAllocas.find(alloca) == escapedAllocas.end()) {
          promotableAllocas.push_back(alloca);
          promotableSet.insert(alloca);
        }
      } else if (auto *store =
                     llvm::dyn_cast_or_null<StoreInst>(instPtr.get())) {
        if (auto *alloca =
                llvm::dyn_cast_or_null<AllocaInst>(store->getPointer())) {
          if (promotableSet.count(alloca)) {
            defBlocks[alloca].push_back(blockPtr.get());
          }
        }
      }
    }
  }

  if (promotableAllocas.empty()) {
    return false;
  }

  // Phase 2: Compute Dominator Tree & Dominance Frontiers
  MIRDominance dom(F);
  dom.analyze();
  auto DF = computeDominanceFrontiers(F, dom);

  // Phase 3: Insert Phi Nodes (Using Iterated Dominance Frontier)
  std::unordered_map<MIRBlock *, std::unordered_map<AllocaInst *, PhiInst *>>
      blockPhis;

  for (AllocaInst *alloca : promotableAllocas) {
    std::vector<MIRBlock *> worklist = defBlocks[alloca];
    std::unordered_set<MIRBlock *> inWorklist(worklist.begin(), worklist.end());
    std::unordered_set<MIRBlock *> hasPhi;

    while (!worklist.empty()) {
      MIRBlock *x = worklist.back();
      worklist.pop_back();

      for (MIRBlock *y : DF[x]) {
        if (hasPhi.insert(y).second) {
          const hir::HIRType *valueType = nullptr;
          if (auto *ptrTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(alloca->getType())) {
            valueType = ptrTy->getPointee();
          }

          auto phi = std::make_unique<PhiInst>(
              valueType, alloca->getName() + ".phi", alloca->getLoc());
          blockPhis[y][alloca] = phi.get();

          phi->setParent(y);
          y->getInstructionsMut().insert(y->getInstructionsMut().begin(),
                                         std::move(phi));

          if (inWorklist.insert(y).second) {
            worklist.push_back(y);
          }
        }
      }
    }
  }

  // Phase 4: Rename Variables & Wire Phi Edges (Dominator Tree DFS)
  std::unordered_map<AllocaInst *, std::vector<MIRValue *>> valueStacks;
  std::vector<MIRInst *> toDelete;

  // Recursive renaming function
  auto renameDFS = [&](auto &self, MIRBlock *b) -> void {
    std::unordered_map<AllocaInst *, int> pushedCounts;

    // a. Push Phi nodes in this block as the new active definitions
    if (blockPhis.count(b)) {
      for (auto &[alloca, phi] : blockPhis[b]) {
        valueStacks[alloca].push_back(phi);
        pushedCounts[alloca]++;
      }
    }

    // b. Process normal instructions (Loads/Stores)
    for (auto &instPtr : b->getInstructionsMut()) {
      MIRInst *inst = instPtr.get();

      if (auto *load = llvm::dyn_cast_or_null<LoadInst>(inst)) {
        if (auto *alloca =
                llvm::dyn_cast_or_null<AllocaInst>(load->getPointer())) {
          if (promotableSet.count(alloca)) {
            MIRValue *activeVal = valueStacks[alloca].empty()
                                      ? nullptr
                                      : valueStacks[alloca].back();
            if (!activeVal) {
              activeVal = M.getOrInsertConstant<ConstantUndef>(load->getType());
            }
            replaceAllUsesInFunction(F, load, activeVal);
            toDelete.push_back(load);
          }
        }
      } else if (auto *store = llvm::dyn_cast_or_null<StoreInst>(inst)) {
        if (auto *alloca =
                llvm::dyn_cast_or_null<AllocaInst>(store->getPointer())) {
          if (promotableSet.count(alloca)) {
            valueStacks[alloca].push_back(store->getValue());
            pushedCounts[alloca]++;
            toDelete.push_back(store);
          }
        }
      } else if (auto *alloca = llvm::dyn_cast_or_null<AllocaInst>(inst)) {
        if (promotableSet.count(alloca)) {
          toDelete.push_back(alloca);
        }
      }
    }

    // c. Fill in incoming values for Phi nodes in successor blocks
    for (MIRBlock *succ : b->getSuccessors()) {
      if (blockPhis.count(succ)) {
        for (auto &[alloca, phi] : blockPhis[succ]) {
          if (!valueStacks[alloca].empty()) {
            phi->addIncoming(valueStacks[alloca].back(), b);
          } else {
            const hir::HIRType *valTy = phi->getType();
            MIRValue *undefVal = M.getOrInsertConstant<ConstantUndef>(valTy);
            phi->addIncoming(undefVal, b);
          }
        }
      }
    }

    // d. Recurse into blocks immediately dominated by this block
    for (MIRBlock *child : dom.getChildren(b)) {
      self(self, child);
    }

    // e. Pop definitions created in this block before returning up the tree
    for (auto &[alloca, count] : pushedCounts) {
      for (int i = 0; i < count; ++i) {
        valueStacks[alloca].pop_back();
      }
    }
  };

  if (F->getEntryBlock()) {
    renameDFS(renameDFS, F->getEntryBlock());
  }

  // Phase 5: Cleanup Dead Instructions
  for (auto &blockPtr : F->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructions()) {
      if (auto *load = llvm::dyn_cast_or_null<LoadInst>(instPtr.get())) {
        if (auto *alloca =
                llvm::dyn_cast_or_null<AllocaInst>(load->getPointer())) {
          if (promotableSet.count(alloca)) {
            toDelete.push_back(load);
          }
        }
      } else if (auto *store =
                     llvm::dyn_cast_or_null<StoreInst>(instPtr.get())) {
        if (auto *alloca =
                llvm::dyn_cast_or_null<AllocaInst>(store->getPointer())) {
          if (promotableSet.count(alloca)) {
            toDelete.push_back(store);
          }
        }
      }
    }
  }

  std::unordered_set<MIRInst *> deadSet(toDelete.begin(), toDelete.end());

  for (auto &blockPtr : F->getBlocks()) {
    auto &insts = blockPtr->getInstructionsMut();
    insts.erase(std::remove_if(insts.begin(), insts.end(),
                               [&](const std::unique_ptr<MIRInst> &inst) {
                                 return deadSet.count(inst.get()) > 0;
                               }),
                insts.end());
  }

  return true;
}

} // namespace mir
} // namespace moksha
