#include "moksha/MIR/Passes/DeadCodeEliminationPass.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRGlobal.h"
#include "moksha/MIR/MIRModule.h"
#include "llvm/Support/Casting.h"
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace moksha {
namespace mir {

bool DeadCodeEliminationPass::runOnModule(MIRModule &M) {
  bool changed = false;

  // ========================================================================
  // STEP 1: Unreachable Block Elimination
  // ========================================================================
  for (auto &func : M.getFunctions()) {
    if (func->isDeclaration() || func->getBlocks().empty())
      continue;

    std::unordered_set<MIRBlock *> reachable;
    std::vector<MIRBlock *> worklist;

    MIRBlock *entry = func->getEntryBlock();
    if (!entry)
      continue;
    reachable.insert(entry);
    worklist.push_back(entry);

    while (!worklist.empty()) {
      MIRBlock *curr = worklist.back();
      worklist.pop_back();
      if (!curr)
        continue;
      for (MIRBlock *succ : curr->getSuccessors()) {
        if (succ && reachable.insert(succ).second) {
          worklist.push_back(succ);
        }
      }
    }

    std::unordered_map<MIRBlock *, std::unordered_set<MIRBlock *>> livePreds;
    for (MIRBlock *b : reachable) {
      for (MIRBlock *succ : b->getSuccessors()) {
        livePreds[succ].insert(b);
      }
    }

    for (MIRBlock *b : reachable) {
      for (auto &inst : b->getInstructionsMut()) {
        if (auto *phi = llvm::dyn_cast_or_null<PhiInst>(inst.get())) {
          auto &incoming = phi->getIncomingMut();
          for (auto it = incoming.begin(); it != incoming.end();) {
            if (livePreds[b].find(it->second) == livePreds[b].end()) {
              it = incoming.erase(it);
              changed = true;
            } else {
              ++it;
            }
          }
        }
      }
    }

    auto &blocks = func->getBlocksMut();
    for (auto it = blocks.begin(); it != blocks.end();) {
      if (reachable.find(it->get()) == reachable.end()) {
        MIRBlock *deadBlock = it->get();

        // Sever outgoing CFG edges ONLY for live blocks!
        for (MIRBlock *succ : deadBlock->getSuccessors()) {
          if (reachable.find(succ) != reachable.end()) {
            succ->removePredecessor(deadBlock);
          }
        }

        it = blocks.erase(it); // Now safe to pop the dead block
        changed = true;
      } else {
        ++it;
      }
    }
  }

  // ========================================================================
  // STEP 1.5: Trivial Phi Node Elimination
  // ========================================================================
  auto replaceAllUsesLocally = [&](MIRFunction *f, MIRValue *oldVal,
                                   MIRValue *newVal) {
    for (auto &b : f->getBlocks()) {
      for (auto &i : b->getInstructionsMut()) {
        i->replaceOperand(oldVal, newVal);
      }
    }
  };

  for (auto &func : M.getFunctions()) {
    if (func->isDeclaration())
      continue;

    for (auto &block : func->getBlocks()) {
      auto &insts = block->getInstructionsMut();
      auto it = insts.begin();

      while (it != insts.end()) {
        if (auto *phi = llvm::dyn_cast_or_null<PhiInst>(it->get())) {
          auto &incoming = phi->getIncoming();

          // If the Phi has been reduced to a single valid incoming edge
          if (incoming.size() == 1) {
            MIRValue *resolvedVal = incoming.front().first;

            // Break self-referential dead cycles to prevent use-after-free
            if (resolvedVal == phi) {
              resolvedVal =
                  M.getOrInsertConstant<ConstantUndef>(phi->getType());
            }

            replaceAllUsesLocally(func.get(), phi, resolvedVal);
            it = insts.erase(it);
            changed = true;
            continue;
          }
        }
        ++it;
      }
    }
  }

  // ========================================================================
  // STEP 2: Instruction & Global Dead Code Elimination (Mark and Sweep)
  // ========================================================================
  std::unordered_set<MIRValue *> alive;
  std::vector<MIRValue *> aliveWorklist;

  auto markAlive = [&](MIRValue *v) {
    if (!v)
      return;
    // If it's a new value we haven't marked yet, queue it to trace its operands
    if (alive.insert(v).second) {
      aliveWorklist.push_back(v);
    }
  };

  // Define what "Roots" the program has (instructions that DO things)
  auto hasSideEffects = [](MIRInst *i) -> bool {
    if (!i)
      return false;
    switch (i->getOpcode()) {
    case Opcode::Store:
    case Opcode::StoreWeak:
    case Opcode::Call:
    case Opcode::Invoke:
    case Opcode::Return:
    case Opcode::Unreachable:
    case Opcode::Br:
    case Opcode::CondBr:
    case Opcode::Switch:
    case Opcode::Throw:
    case Opcode::Resume:
    case Opcode::InlineAsm:
    case Opcode::Spawn:
    case Opcode::Await:
    case Opcode::AtomicStore:
    case Opcode::AtomicRMW:
    case Opcode::AtomicCmpXchg:
    case Opcode::Fence:
    case Opcode::Retain:
    case Opcode::Release:
    case Opcode::LandingPad:
      return true;
    case Opcode::Load:
      return llvm::cast<LoadInst>(i)->isVolatile(); // Volatile loads are roots!
    case Opcode::AtomicLoad:
      return true; // Atomics carry memory barriers
    default:
      return false; // Pure Math, Phis, GEPs, Allocas, Casts have NO side
                    // effects
    }
  };

  // Trace an alive instruction back to its operand dependencies
  auto extractOperands = [&](MIRValue *val) {
    if (auto *i = llvm::dyn_cast_or_null<MIRInst>(val)) {
      if (auto *load = llvm::dyn_cast_or_null<LoadInst>(i))
        markAlive(load->getPointer());
      else if (auto *store = llvm::dyn_cast_or_null<StoreInst>(i)) {
        markAlive(store->getValue());
        markAlive(store->getPointer());
      } else if (auto *storeWeak = llvm::dyn_cast_or_null<StoreWeakInst>(i)) {
        markAlive(storeWeak->getValue());
        markAlive(storeWeak->getPointer());
      } else if (auto *loadWeak = llvm::dyn_cast_or_null<LoadWeakInst>(i)) {
        markAlive(loadWeak->getPointer());
      } else if (auto *gep = llvm::dyn_cast_or_null<GetElementPtrInst>(i)) {
        markAlive(gep->getPointer());
        for (auto *idx : gep->getIndices())
          markAlive(idx);
      } else if (auto *call = llvm::dyn_cast_or_null<CallInst>(i)) {
        markAlive(call->getCallee());
        for (auto *a : call->getArgs())
          markAlive(a);
      } else if (auto *cast = llvm::dyn_cast_or_null<CastInst>(i)) {
        markAlive(cast->getValue());
      } else if (auto *phi = llvm::dyn_cast_or_null<PhiInst>(i)) {
        for (auto &p : phi->getIncoming())
          markAlive(p.first);
      } else if (auto *bin = llvm::dyn_cast_or_null<BinaryInst>(i)) {
        markAlive(bin->getLHS());
        markAlive(bin->getRHS());
      } else if (auto *cmp = llvm::dyn_cast_or_null<CompareInst>(i)) {
        markAlive(cmp->getLHS());
        markAlive(cmp->getRHS());
      } else if (auto *fcmp = llvm::dyn_cast_or_null<FCmpInst>(i)) {
        markAlive(fcmp->getLHS());
        markAlive(fcmp->getRHS());
      } else if (auto *ret = llvm::dyn_cast_or_null<ReturnInst>(i)) {
        markAlive(ret->getReturnValue());
      } else if (auto *br = llvm::dyn_cast_or_null<CondBranchInst>(i)) {
        markAlive(br->getCondition());
      } else if (auto *sw = llvm::dyn_cast_or_null<SwitchInst>(i)) {
        markAlive(sw->getCondition());
        for (auto &c : sw->getCases())
          markAlive(c.first);
      } else if (auto *inv = llvm::dyn_cast_or_null<InvokeInst>(i)) {
        markAlive(inv->getCallee());
        for (auto *a : inv->getArgs())
          markAlive(a);
      } else if (auto *res = llvm::dyn_cast_or_null<ResumeInst>(i)) {
        markAlive(res->getException());
      } else if (auto *thr = llvm::dyn_cast_or_null<ThrowInst>(i)) {
        markAlive(thr->getException());
      } else if (auto *ia = llvm::dyn_cast_or_null<InlineAsmInst>(i)) {
        for (auto *a : ia->getArgs())
          markAlive(a);
      } else if (auto *spawn = llvm::dyn_cast_or_null<SpawnInst>(i)) {
        markAlive(spawn->getClosure());
      } else if (auto *awaitInst = llvm::dyn_cast_or_null<AwaitInst>(i)) {
        markAlive(awaitInst->getPromise());
      } else if (auto *al = llvm::dyn_cast_or_null<AtomicLoadInst>(i)) {
        markAlive(al->getPointer());
      } else if (auto *as = llvm::dyn_cast_or_null<AtomicStoreInst>(i)) {
        markAlive(as->getValue());
        markAlive(as->getPointer());
      } else if (auto *armw = llvm::dyn_cast_or_null<AtomicRMWInst>(i)) {
        markAlive(armw->getValue());
        markAlive(armw->getPointer());
      } else if (auto *acx = llvm::dyn_cast_or_null<AtomicCmpXchgInst>(i)) {
        markAlive(acx->getPointer());
        markAlive(acx->getExpected());
        markAlive(acx->getDesired());
      } else if (auto *ext = llvm::dyn_cast_or_null<ExtractValueInst>(i)) {
        markAlive(ext->getAggregate());
      } else if (auto *ins = llvm::dyn_cast_or_null<InsertValueInst>(i)) {
        markAlive(ins->getAggregate());
        markAlive(ins->getValue());
      } else if (auto *arc = llvm::dyn_cast_or_null<ARCInst>(i)) {
        markAlive(arc->getObject());
      } else if (auto *mc = llvm::dyn_cast_or_null<MakeClosureInst>(i)) {
        markAlive(mc->getFunctionPointer());
        for (auto *c : mc->getCaptures())
          markAlive(c);
      }
    } else if (auto *g = llvm::dyn_cast_or_null<MIRGlobal>(val)) {
      markAlive(g->getInitializer());
    } else if (auto *ca = llvm::dyn_cast_or_null<ConstantArray>(val)) {
      for (auto *e : ca->getElements())
        markAlive(e);
    } else if (auto *cm = llvm::dyn_cast_or_null<ConstantMap>(val)) {
      for (auto &p : cm->getEntries()) {
        markAlive(p.first);
        markAlive(p.second);
      }
    } else if (auto *cs = llvm::dyn_cast_or_null<ConstantStruct>(val)) {
      for (auto *e : cs->getFields())
        markAlive(e);
    } else if (auto *cbc = llvm::dyn_cast_or_null<ConstantBitCast>(val)) {
      markAlive(cbc->getValue());
    }
  };

  // MARK PHASE: Seed Exported Globals
  for (auto &globalPtr : M.getGlobalsMut()) {
    if (globalPtr->getLinkage() != Linkage::Internal) {
      markAlive(globalPtr.get());
    }

    // Also keep __attribute__((used)) globals alive if you support that flag
    if (globalPtr->isUsed()) {
      markAlive(globalPtr.get());
    }
  }

  // MARK PHASE: Seed the worklist with all side-effecting instructions in all
  // blocks
  for (auto &func : M.getFunctions()) {

    // [FIX] Force-keep all instructions in system initialization/teardown
    // functions! This ensures DCE never deletes your global destructors!
    bool isSystemRoot = (func->getName() == "__moksha_module_init" ||
                         func->getName() == "__moksha_module_destroy");

    for (auto &block : func->getBlocks()) {
      if (!block)
        continue;
      for (auto &inst : block->getInstructions()) {
        if (!inst)
          continue;
        if (isSystemRoot || hasSideEffects(inst.get())) {
          markAlive(inst.get());
        }
      }
    }
  }

  // Propagate liveness up the dependency chain
  while (!aliveWorklist.empty()) {
    MIRValue *curr = aliveWorklist.back();
    aliveWorklist.pop_back();
    extractOperands(curr);
  }

  // SWEEP PHASE 1: Remove Dead Instructions
  for (auto &func : M.getFunctions()) {
    for (auto &block : func->getBlocks()) {
      auto &insts = block->getInstructionsMut();
      auto it = insts.begin();
      while (it != insts.end()) {
        if (alive.find(it->get()) == alive.end()) {
          it = insts.erase(it); // Pop the dead instruction
          changed = true;
        } else {
          ++it;
        }
      }
    }
  }

  // SWEEP PHASE 2: Remove Dead Globals
  auto &globals = M.getGlobalsMut();
  auto &globalMap = M.getGlobalMapMut();

  for (auto it = globals.begin(); it != globals.end();) {
    if (alive.find(it->get()) == alive.end()) {
      globalMap.erase((*it)->getName());
      it = globals.erase(it); // Pop the dead global
      changed = true;
    } else {
      ++it;
    }
  }

  return changed;
} // End of runOnModule

} // namespace mir
} // namespace moksha
