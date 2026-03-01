#include "moksha/Ownership/ARCAnalyzer.h"

#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "moksha/Support/Diagnostics.h"

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

  bool optimizeBlock(MIRBlock *block) {
    bool modified = false;
    auto &insts = block->getInstructions();

    std::unordered_map<MIRValue *, ARCInst *> activeRetains;
    std::vector<MIRInst *> toRemove;

    for (auto &instPtr : insts) {
      MIRInst *inst = instPtr.get();

      auto *arc = dynamic_cast<ARCInst *>(inst);
      if (!arc)
        continue;

      MIRValue *val = arc->getObject();

      if (arc->getOpcode() == Opcode::Retain) {
        activeRetains[val] = arc;
      } else if (arc->getOpcode() == Opcode::Release) {
        auto it = activeRetains.find(val);
        if (it != activeRetains.end()) {
          ARCInst *retainInst = it->second;

          if (isSafeToRemovePair(retainInst, arc, block)) {
            toRemove.push_back(retainInst);
            toRemove.push_back(arc);
            modified = true;
          }
          activeRetains.erase(it);
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

  bool isSafeToRemovePair(ARCInst *retain, ARCInst *release, MIRBlock *block) {
    bool inRange = false;
    MIRValue *targetVal = retain->getObject();

    for (auto &instPtr : block->getInstructions()) {
      MIRInst *inst = instPtr.get();

      if (inst == retain) {
        inRange = true;
        continue;
      }
      if (inst == release) {
        break;
      }
      if (!inRange)
        continue;

      if (auto *arc = dynamic_cast<ARCInst *>(inst)) {
        if (arc->getOpcode() == Opcode::Release &&
            arc->getObject() == targetVal) {
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
