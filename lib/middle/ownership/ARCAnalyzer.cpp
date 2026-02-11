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

/// \brief Performs local ARC optimizations within functions.
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

    // Run repeatedly until no more pairs can be removed (convergence).
    // This handles nested pairs: Retain(x) ... Retain(x) ... Release(x) ...
    // Release(x) The inner pair is removed first, exposing the outer pair for
    // the next iteration.
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

  /// \brief Scans a basic block to remove redundant Retain/Release pairs.
  bool optimizeBlock(MIRBlock *block) {
    bool modified = false;
    auto &insts = block->getInstructions();

    // Map: Tracks the most recent 'Retain' instruction for a specific Value.
    // Key: The value being retained. Value: The Retain instruction itself.
    std::unordered_map<MIRValue *, ARCInst *> activeRetains;

    // List of instructions identified for removal in this pass.
    std::vector<MIRInst *> toRemove;

    // Forward Scan
    for (auto &instPtr : insts) {
      MIRInst *inst = instPtr.get();

      // We only care about ARC instructions (Retain/Release)
      auto *arc = dynamic_cast<ARCInst *>(inst);
      if (!arc)
        continue;

      MIRValue *val = arc->getOperand();

      if (arc->getOpcode() == Opcode::Retain) {
        // We found a Retain.
        // Overwrite any previous Retain for this value.
        // This effectively pairs the Release with the *nearest* preceding
        // Retain, which handles nested scopes correctly.
        activeRetains[val] = arc;
      } else if (arc->getOpcode() == Opcode::Release) {
        // We found a Release. Check if we have a matching active Retain.
        auto it = activeRetains.find(val);
        if (it != activeRetains.end()) {
          ARCInst *retainInst = it->second;

          // Found a Retain...Release pair.
          // Check if it is safe to remove (i.e., no intervening releases of the
          // same value).
          if (isSafeToRemovePair(retainInst, arc, block)) {
            toRemove.push_back(retainInst);
            toRemove.push_back(arc);
            modified = true;
          }

          // Whether we removed it or not, this specific Retain instance is now
          // "consumed" or invalid for further pairing in this forward pass.
          activeRetains.erase(it);
        }
      }
    }

    // Remove the identified instructions
    if (!toRemove.empty()) {
      auto &instructions = block->getInstructions();

      // [Optimization] Use a set for O(1) lookup during removal
      std::unordered_set<MIRInst *> toRemoveSet(toRemove.begin(),
                                                toRemove.end());

      // Standard erase-remove idiom for unique_ptr vector
      auto newEnd = std::remove_if(instructions.begin(), instructions.end(),
                                   [&](const std::unique_ptr<MIRInst> &ptr) {
                                     return toRemoveSet.count(ptr.get());
                                   });

      instructions.erase(newEnd, instructions.end());
    }

    return modified;
  }

  /// \brief Checks if the instruction sequence between Retain and Release is
  /// safe.
  /// \return false if an intervening instruction interferes with the
  /// optimization.
  bool isSafeToRemovePair(ARCInst *retain, ARCInst *release, MIRBlock *block) {
    bool inRange = false;
    MIRValue *targetVal = retain->getOperand();

    for (auto &instPtr : block->getInstructions()) {
      MIRInst *inst = instPtr.get();

      // Start checking after the Retain
      if (inst == retain) {
        inRange = true;
        continue;
      }
      // Stop checking at the Release
      if (inst == release) {
        break;
      }
      if (!inRange)
        continue;

      // --- Safety Checks ---

      // 1. Check for Intervening Releases
      // If we remove the outer Retain, this inner Release might cause a
      // premature deallocation (Double Free or Use-After-Free).
      if (auto *arc = dynamic_cast<ARCInst *>(inst)) {
        if (arc->getOpcode() == Opcode::Release &&
            arc->getOperand() == targetVal) {
          return false;
        }
      }

      // 2. Check for Function Calls (Ownership Consumption)
      // NOTE: We currently assume a "Borrow" calling convention (+0).
      // If your ABI changes to "Consume" (+1), you must check CallInsts here.
    }

    return true;
  }
};

} // namespace

// ============================================================================
// [Entry Points]
// ============================================================================

bool runARCOptimization(MIRModule *module, DiagnosticEngine &diags) {
  ARCOptimizer optimizer(module, diags);
  return optimizer.run();
}

bool runARCOptimization(MIRFunction *function, DiagnosticEngine &diags) {
  // Optimization requires module-level context (e.g., types), but works
  // per-function logic.
  if (!function->getParent())
    return false;
  ARCOptimizer optimizer(function->getParent(), diags);
  return optimizer.runOnFunction(function);
}

} // namespace mir
} // namespace moksha
