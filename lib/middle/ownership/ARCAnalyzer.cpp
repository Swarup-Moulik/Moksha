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
      modified |= runOnFunction(func);
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
    while (auto *inst = llvm::dyn_cast_or_null<MIRInst>(val)) {
      if (inst->getOpcode() == Opcode::BitCast ||
          inst->getOpcode() == Opcode::AnyCast) {
        val = static_cast<CastInst *>(inst)->getValue();
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
          inst->getOpcode() == Opcode::InlineAsm ||
          inst->getOpcode() == Opcode::Spawn ||
          inst->getOpcode() == Opcode::Await) {
        activeRetains.clear();
        continue;
      }

      auto *arc = llvm::dyn_cast_or_null<ARCInst>(inst);
      if (!arc)
        continue;

      // Extract the absolute base memory address
      MIRValue *baseObj = getUnderlyingObject(arc->getObject());

      if (arc->getOpcode() == Opcode::Retain) {
        activeRetains[baseObj].push_back(arc);
      } else if (arc->getOpcode() == Opcode::Release) {
        auto it = activeRetains.find(baseObj);
        if (it != activeRetains.end() && !it->second.empty()) {
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
    MIRValue *directObj = retain->getObject();

    if (directObj && directObj->getType()) {
      std::string tyStr = directObj->getType()->toString();
      std::string className = "";

      size_t qPos = tyStr.find('?');
      if (qPos != std::string::npos) {
        tyStr = tyStr.substr(0, qPos);
      }

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
        std::string dropPrefix = className + ".destructor";
        if (module) {
          for (auto &funcPtr : module->getFunctions()) {
            if (funcPtr->getName().find(dropPrefix) == 0) {
              return false;
            }
          }
        }
      }
    }

    MIRValue *targetVal = getUnderlyingObject(directObj);
    if (MIRFunction *func = block->getParent()) {
      int retainCount = 0;
      for (auto &b : func->getBlocks()) {
        for (auto &i : b->getInstructions()) {
          if (auto *a = llvm::dyn_cast_or_null<ARCInst>(i.get())) {
            if (a->getOpcode() == Opcode::Retain &&
                getUnderlyingObject(a->getObject()) == targetVal) {
              retainCount++;
            }
          }
        }
      }

      if (retainCount <= 1) {
        return false;
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

      if (auto *arc = llvm::dyn_cast_or_null<ARCInst>(inst)) {
        if (getUnderlyingObject(arc->getObject()) == targetVal) {
          return false;
        }
      } else if (auto *store = llvm::dyn_cast_or_null<StoreInst>(inst)) {
        if (getUnderlyingObject(store->getValue()) == targetVal) {
          return false;
        }
      } else if (auto *storeW = llvm::dyn_cast_or_null<StoreWeakInst>(inst)) {
        if (getUnderlyingObject(storeW->getValue()) == targetVal ||
            getUnderlyingObject(storeW->getPointer()) == targetVal) {
          return false;
        }
      } else if (auto *loadW = llvm::dyn_cast_or_null<LoadWeakInst>(inst)) {
        if (getUnderlyingObject(loadW->getPointer()) == targetVal) {
          return false;
        }
      } else if (llvm::isa<CallInst>(inst) || llvm::isa<InvokeInst>(inst) ||
                 llvm::isa<AwaitInst>(inst) || llvm::isa<SpawnInst>(inst)) {
        return false;
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
