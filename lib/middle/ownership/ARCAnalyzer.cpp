#include "moksha/Ownership/ARCAnalyzer.h"
#include "moksha/MIR/MIRArgument.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "moksha/Support/Diagnostics.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace moksha {
namespace mir {

namespace {

class ARCOptimizer {
public:
  ARCOptimizer(MIRModule *module, DiagnosticEngine &diags)
      : module(module), diags(diags) {}

  bool run() {
    if (!module)
      return false;
    bool modified = false;
    for (auto &func : module->getFunctions()) {
      modified |= runOnFunction(func.get());
    }
    return modified;
  }

  bool runOnFunction(MIRFunction *func) {
    if (func->isDeclaration())
      return false;

    bool modified = false;
    bool localChanged = true;

    while (localChanged) {
      localChanged = false;
      for (auto &block : func->getBlocks()) {
        localChanged |= optimizeBlock(block.get());
      }
      modified |= localChanged;
    }
    return modified;
  }

private:
  MIRModule *module;
  DiagnosticEngine &diags;

  // Analyzes the actual memory location rather than the loaded SSA instance.
  MIRValue *getUnderlyingObject(MIRValue *val) {
    while (auto *inst = llvm::dyn_cast<MIRInst>(val)) {
      if (inst->getOpcode() == Opcode::BitCast) {
        val = static_cast<CastInst *>(inst)->getValue();
        continue;
      } else if (inst->getOpcode() == Opcode::Load) {
        val = static_cast<LoadInst *>(inst)->getPointer();
        continue;
      } else if (inst->getOpcode() == Opcode::ExtractValue) {
        auto *ext = static_cast<ExtractValueInst *>(inst);
        if (ext->getIndex() == 0) {
          val = ext->getAggregate();
          continue;
        }
        break;
      }
      break;
    }
    return val;
  }

  bool optimizeBlock(MIRBlock *block) {
    bool modified = false;
    auto &insts = block->getInstructions();

    std::unordered_map<MIRValue *, std::vector<ARCInst *>> activeRetains;
    std::vector<MIRInst *> toRemove;

    for (auto &instPtr : insts) {
      MIRInst *inst = instPtr.get();
      if (!inst)
        continue;

      // Optimization Safety Barrier
      if (inst->getOpcode() == Opcode::Call ||
          inst->getOpcode() == Opcode::Invoke ||
          inst->getOpcode() == Opcode::InlineAsm) {
        activeRetains.clear();
        continue;
      }

      auto *arc = llvm::dyn_cast<ARCInst>(inst);
      if (!arc)
        continue;

      // Extract the absolute base memory address
      MIRValue *baseObj = getUnderlyingObject(arc->getObject());

      if (arc->getOpcode() == Opcode::Retain) {
        activeRetains[baseObj].push_back(arc); // Push to stack
      } else if (arc->getOpcode() == Opcode::Release) {
        auto it = activeRetains.find(baseObj);
        if (it != activeRetains.end() && !it->second.empty()) {
          // Pop the most recent retain
          ARCInst *retainInst = it->second.back();
          it->second.pop_back();

          if (isSafeToRemovePair(block, retainInst, arc)) {
            toRemove.push_back(retainInst);
            toRemove.push_back(arc);
            modified = true;
          }
        }
      }
    }

    if (!toRemove.empty()) {
      auto &instructions = const_cast<std::vector<std::unique_ptr<MIRInst>> &>(
          block->getInstructions());
      std::unordered_set<MIRInst *> toRemoveSet(toRemove.begin(),
                                                toRemove.end());

      auto newEnd = std::remove_if(instructions.begin(), instructions.end(),
                                   [&](const std::unique_ptr<MIRInst> &ptr) {
                                     return toRemoveSet.count(ptr.get());
                                   });

      instructions.erase(newEnd, instructions.end());
    }

    return modified;
  }

  bool isSafeToRemovePair(MIRBlock *block, ARCInst *retain, ARCInst *release) {
    bool inRange = false;

    // [FIX] 1. Extract the true type using the direct object BEFORE stripping!
    MIRValue *directObj = retain->getObject();
    if (directObj && directObj->getType()) {
      std::string tyStr = directObj->getType()->toString();
      std::string className = "";

      if (!tyStr.empty() && tyStr[0] == '*') {
        className = tyStr.substr(1);
      } else if (tyStr.find("shared ") == 0) {
        className = tyStr.substr(7);
      }

      if (className.find("struct ") == 0)
        className = className.substr(7);
      if (className.find("class ") == 0)
        className = className.substr(6);

      if (!className.empty()) {
        std::string dropFuncName = className + ".destructor";
        // If the module contains a drop function for this type, abort elision!
        if (module && module->getFunction(dropFuncName)) {
          return false;
        }
      }
    }

    // [FIX] 2. Now strip the object to check for intermediate memory
    // manipulations
    MIRValue *targetVal = getUnderlyingObject(directObj);

    // We must preserve at least one retain/release pair to ensure the memory is
    // freed.
    if (MIRFunction *func = block->getParent()) {
      int retainCount = 0;
      for (auto &b : func->getBlocks()) {
        for (auto &i : b->getInstructions()) {
          if (auto *a = llvm::dyn_cast<ARCInst>(i.get())) {
            if (a->getOpcode() == Opcode::Retain &&
                getUnderlyingObject(a->getObject()) == targetVal) {
              retainCount++;
            }
          }
        }
      }

      // If this is the only retain left in the function for this object,
      // DO NOT elide IF it is a true root allocation!
      if (retainCount <= 1) {
        bool isRoot = false;

        // 1. Is it a direct allocation?
        if (auto *call = llvm::dyn_cast<CallInst>(targetVal)) {
          if (call->getCallee() &&
              call->getCallee()->getName() == "__moksha_alloc") {
            isRoot = true;
          }
        }
        // 2. Is it a local stack variable or function argument?
        else if (llvm::isa<MIRArgument>(targetVal) ||
                 llvm::isa<AllocaInst>(targetVal)) {
          isRoot = true;
        }

        // If it's a Phi node, Load, or something else, it's just an alias,
        // so it doesn't need root protection.
        if (isRoot) {
          return false;
        }
      }
    }

    for (auto &instPtr : block->getInstructions()) {
      MIRInst *inst = instPtr.get();
      if (!inst)
        continue;

      if (inst == retain) {
        inRange = true;
        continue;
      }
      if (inst == release) {
        break;
      }
      if (!inRange)
        continue;

      // Ensure no intermediate instructions manipulate the exact same ARC
      // memory
      if (auto *arc = llvm::dyn_cast<ARCInst>(inst)) {
        if (getUnderlyingObject(arc->getObject()) == targetVal) {
          return false;
        }
      } else if (auto *store = llvm::dyn_cast<StoreInst>(inst)) {
        if (getUnderlyingObject(store->getValue()) == targetVal) {
          return false;
        }
      } else if (auto *storeW = llvm::dyn_cast<StoreWeakInst>(inst)) {
        if (getUnderlyingObject(storeW->getValue()) == targetVal ||
            getUnderlyingObject(storeW->getPointer()) == targetVal) {
          return false;
        }
      } else if (auto *loadW = llvm::dyn_cast<LoadWeakInst>(inst)) {
        if (getUnderlyingObject(loadW->getPointer()) == targetVal) {
          return false;
        }
      }
    }

    return true;
  }
};

} // namespace

bool runARCOptimization(MIRModule *module, DiagnosticEngine &diags) {
  ARCOptimizer optimizer(module, diags);
  return optimizer.run();
}

bool runARCOptimization(MIRFunction *function, DiagnosticEngine &diags) {
  ARCOptimizer optimizer(nullptr, diags);
  return optimizer.runOnFunction(function);
}

} // namespace mir
} // namespace moksha
