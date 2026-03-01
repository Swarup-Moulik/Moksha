#include "moksha/Ownership/ARCInserter.h"

#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRBuilder.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "moksha/Support/Diagnostics.h"

#include <vector>

namespace moksha {
namespace mir {

namespace {

class ARCInserter {
public:
  // FIX: Just use the default constructor for MIRBuilder
  ARCInserter(MIRModule *module, DiagnosticEngine &diags)
      : module(module), diags(diags), builder() {}

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

  // FIX: Accept const HIRType*
  bool isRefCounted(const hir::HIRType *type) {
    // FIX: String hack for pointer validation
    if (type->toString().find("*") == std::string::npos)
      return false;
    return true;
  }

  MIRValue *getUnderlyingObject(MIRValue *val) {
    while (auto *inst = dynamic_cast<MIRInst *>(val)) {
      if (inst->getOpcode() == Opcode::BitCast) {
        auto *cast = static_cast<CastInst *>(inst);
        val = cast->getValue();
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

    for (auto &block : func->getBlocks()) {

      struct Action {
        MIRInst *target;
        AllocaInst *owner;
        enum { HandleStore, HandleReturn } type;
      };
      std::vector<Action> actions;

      for (auto &inst : block->getInstructions()) {
        if (auto *store = dynamic_cast<StoreInst *>(inst.get())) {
          MIRValue *ptr = getUnderlyingObject(store->getPointer());

          for (auto *owner : refCountedAllocas) {
            if (ptr == owner) {
              actions.push_back({store, owner, Action::HandleStore});
              break;
            }
          }
        } else if (auto *ret = dynamic_cast<ReturnInst *>(inst.get())) {
          actions.push_back({ret, nullptr, Action::HandleReturn});
        }
      }

      for (const auto &action : actions) {
        modified = true;
        MIRInst *inst = action.target;

        // FIX: The compiler specifically says setInsertPoint only takes a
        // MIRBlock*
        builder.setInsertPoint(block.get());

        if (action.type == Action::HandleStore) {
          auto *store = static_cast<StoreInst *>(inst);
          MIRValue *newValue = store->getValue();
          AllocaInst *owner = action.owner;

          if (newValue->getType()->toString().find("*") == std::string::npos) {
            diags.report(inst->getLoc(), DiagID::err_invalid_type)
                << "ARC retain attempted on non-pointer value";
            continue;
          }

          builder.createRetain(newValue);

          // FIX: createLoad only takes 1 required arg (the pointer to load
          // from)
          auto *oldValue = builder.createLoad(owner);
          builder.createRelease(oldValue);

        } else if (action.type == Action::HandleReturn) {
          for (auto *owner : refCountedAllocas) {
            // FIX: createLoad only takes 1 required arg
            auto *val = builder.createLoad(owner);
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
