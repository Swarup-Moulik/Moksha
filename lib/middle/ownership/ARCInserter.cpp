#include "moksha/Middle/Ownership/ARCInserter.h"

#include "moksha/Middle/MIR/MIRBlock.h"
#include "moksha/Middle/MIR/MIRBuilder.h"
#include "moksha/Middle/MIR/MIRFunction.h"
#include "moksha/Middle/MIR/MIRInst.h"
#include "moksha/Middle/MIR/MIRModule.h"
#include "moksha/Support/Diagnostics.h"

#include <vector>

namespace moksha {
namespace mir {

namespace {

// ARCInserter is a conservative, correctness-first pass.
// It may introduce redundant retains/releases.
// Canonicalization and elision are handled by ARCOptimizer.
class ARCInserter {
public:
  ARCInserter(MIRModule *module, DiagnosticEngine &diags)
      : module(module), diags(diags), builder(module->getContext()) {}

  bool run() {
    bool modified = false;
    for (auto &func : module->getFunctions()) {
      modified |= runOnFunction(func.get());
    }
    return modified;
  }

private:
  MIRModule *module;
  DiagnosticEngine &diags;
  MIRBuilder builder;

  // --- Helpers ---

  /// \brief Determines if a type requires Reference Counting.
  bool isRefCounted(MIRType *type) {
    // [Fix #4] Conservative Safety
    // We strictly only manage pointers.
    if (!type->isPointer())
      return false;

    // TODO: Add specific checks for your language's managed types
    // (e.g., Classes, Strings, Arrays).
    // For now, we assume all pointers are managed for this prototype.
    return true;
  }

  /// \brief Strips casts to find the underlying object allocation.
  /// [Fix #2] Robust pointer matching.
  MIRValue *getUnderlyingObject(MIRValue *val) {
    // Simple look-through for BitCasts
    while (auto *inst = dynamic_cast<MIRInst *>(val)) {
      if (inst->getOpcode() == Opcode::BitCast) {
        auto *cast = static_cast<CastInst *>(inst);
        val = cast->getOperand();
        continue;
      }
      break;
    }
    return val;
  }

  bool runOnFunction(MIRFunction *func) {
    if (func->isDeclaration())
      return false;

    bool modified = false;
    std::vector<AllocaInst *> refCountedAllocas;

    // 1. Identify all local variables (Allocas) that hold ref-counted types.
    MIRBlock *entry = func->getEntryBlock();
    for (auto &inst : entry->getInstructions()) {
      if (auto *alloca = dynamic_cast<AllocaInst *>(inst.get())) {
        if (isRefCounted(alloca->getAllocatedType())) {
          refCountedAllocas.push_back(alloca);
        }
      }
    }

    if (refCountedAllocas.empty())
      return false;

    // 2. Iterate blocks to collect actions
    for (auto &block : func->getBlocks()) {

      // [Fix #1] Track owner in the Action struct
      struct Action {
        MIRInst *target;   // The instruction triggering the action
        AllocaInst *owner; // The specific alloca being accessed (for Stores)
        enum { HandleStore, HandleReturn } type;
      };
      std::vector<Action> actions;

      for (auto &inst : block->getInstructions()) {
        if (auto *store = dynamic_cast<StoreInst *>(inst.get())) {
          // [Fix #2] Use helper to handle casts so we find the real owner
          MIRValue *ptr = getUnderlyingObject(store->getPointer());

          for (auto *owner : refCountedAllocas) {
            if (ptr == owner) {
              actions.push_back({store, owner, Action::HandleStore});
              break;
            }
          }
        } else if (auto *ret = dynamic_cast<ReturnInst *>(inst.get())) {
          // For returns, we don't need a specific owner, we release all.
          actions.push_back({ret, nullptr, Action::HandleReturn});
        }
      }

      // 3. Apply ARC Insertions
      for (const auto &action : actions) {
        modified = true;
        MIRInst *inst = action.target;
        builder.setInsertPoint(inst);

        if (action.type == Action::HandleStore) {
          auto *store = static_cast<StoreInst *>(inst);
          MIRValue *newValue = store->getValue();
          AllocaInst *owner = action.owner;

          // [Fix #5] Safety Check
          if (!newValue->getType()->isPointer()) {
            diags.report(inst->getLoc(), DiagID::err_invalid_type)
                << "ARC retain attempted on non-pointer value";
            continue;
          }

          // A. Retain New Value
          builder.createRetain(newValue);

          // B. Release Old Value
          // [Fix #1] Load using the OWNER'S allocated type.
          // [Refinement] Use 'owner' directly (canonical address) instead of
          // store->getPointer() to ensure we access the alloca correctly
          // regardless of how it was accessed in the store.
          auto *oldValue = builder.createLoad(owner->getAllocatedType(), owner);
          builder.createRelease(oldValue);

        } else if (action.type == Action::HandleReturn) {
          // [Fix #3] Documentation
          // NOTE: This pass releases all locals at every return.
          // This is safe but redundant if there are multiple exits.
          // The ARCOptimizer is responsible for removing redundant releases
          // later.

          for (auto *owner : refCountedAllocas) {
            auto *val = builder.createLoad(owner->getAllocatedType(), owner);
            builder.createRelease(val);
          }
        }
      }
    }

    return modified;
  }
};

} // namespace

bool runARCInsertion(MIRModule *module, DiagnosticEngine &diags) {
  ARCInserter inserter(module, diags);
  return inserter.run();
}

} // namespace mir
} // namespace moksha
