#include "moksha/Ownership/ARCInserter.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRArgument.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRGlobal.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "moksha/Support/Diagnostics.h"
#include "llvm/Support/Casting.h"
#include <unordered_set>
#include <vector>

namespace moksha {
namespace mir {

namespace {

class ARCInserter {
public:
  ARCInserter(MIRModule *module, DiagnosticEngine &diags)
      : module(module), diags(diags) {}

  bool run() {
    bool modified = false;
    for (auto &func : module->getFunctions()) {
      modified |= runOnFunction(func.get());
    }

    // Global Teardown (Module Destroy)
    if (MIRFunction *destroyFunc =
            module->getFunction("__moksha_module_destroy")) {
      if (!destroyFunc->getBlocks().empty()) {
        MIRBlock *entry = destroyFunc->getEntryBlock();

        // Find the ReturnInst to insert cleanup code right before it
        auto retIt = entry->getInstructionsMut().end();
        for (auto it = entry->getInstructionsMut().begin();
             it != entry->getInstructionsMut().end(); ++it) {
          if (llvm::isa<ReturnInst>(it->get())) {
            retIt = it;
            break;
          }
        }

        for (auto &globalPtr : module->getGlobalsMut()) {
          MIRGlobal *g = globalPtr.get();

          // Check if the global's underlying type requires ARC
          const hir::HIRType *valTy = nullptr;
          if (auto *ptrTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(g->getType())) {
            valTy = ptrTy->getPointee();
          } else {
            valTy = g->getType();
          }

          if (valTy && isRefCounted(valTy)) {
            // 1. Load the global
            auto loadInst = std::make_unique<LoadInst>(
                g, g->getName() + ".cleanup", SourceLocation());
            loadInst->setParent(entry);
            MIRValue *loadedVal = loadInst.get();

            // 2. Release it
            auto releaseInst = std::make_unique<ARCInst>(
                Opcode::Release, loadedVal, getDropFunc(loadedVal),
                SourceLocation());
            releaseInst->setParent(entry);

            // Insert them before the return statement
            retIt =
                entry->getInstructionsMut().insert(retIt, std::move(loadInst));
            ++retIt; // Move past the load
            retIt = entry->getInstructionsMut().insert(retIt,
                                                       std::move(releaseInst));

            modified = true;
          }
        }
      }
    }
    return modified;
  }

private:
  MIRModule *module;
  DiagnosticEngine &diags;

  MIRFunction *getDropFunc(MIRValue *val) {
    if (!val)
      return nullptr;
    const hir::HIRType *valTy = val->getType();
    if (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(valTy)) {
      valTy = pTy->getPointee();
    }
    if (valTy) {
      std::string typeName = valTy->toString();
      if (typeName.find("shared ") == 0)
        typeName = typeName.substr(7);
      if (typeName.find("struct ") == 0)
        typeName = typeName.substr(7);
      if (typeName.find("class ") == 0)
        typeName = typeName.substr(6);

      std::string dropName = typeName + ".destructor_ret_void";
      return module->getFunction(dropName);
    }
    return nullptr;
  }

  bool isRefCounted(const hir::HIRType *type) {
    if (auto *ptrType = llvm::dyn_cast<const hir::PointerType>(type)) {
      return ptrType->getOwnership() == hir::Ownership::Shared;
    }
    if (llvm::isa<hir::SliceType>(type))
      return true;
    return false;
  }

  MIRValue *getUnderlyingObject(MIRValue *val) {
    while (auto *inst = llvm::dyn_cast<MIRInst>(val)) {
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

  bool runOnFunction(MIRFunction *func) {
    if (func->isDeclaration())
      return false;

    MIRBlock *entryBlock = func->getEntryBlock();
    bool functionModified = false;

    // 1. Identify which parameters actually need ARC
    std::vector<MIRArgument *> refCountedParams;
    for (auto *arg : func->getRawArguments()) {
      if (isRefCounted(arg->getType())) {
        refCountedParams.push_back(arg);
      }
    }

    // 2. Insert Retains at the Entry Block
    if (entryBlock && !refCountedParams.empty()) {
      std::vector<std::unique_ptr<MIRInst>> paramRetains;
      for (auto *arg : refCountedParams) {
        auto retain = std::make_unique<ARCInst>(
            Opcode::Retain, arg, nullptr,
            entryBlock->getInstructions().front()->getLoc());
        retain->setParent(entryBlock);
        paramRetains.push_back(std::move(retain));
        functionModified = true;
      }

      auto &entryInsts = entryBlock->getInstructionsMut();

      auto insertIt = entryInsts.begin();
      while (insertIt != entryInsts.end() &&
             llvm::isa<AllocaInst>(insertIt->get())) {
        ++insertIt;
      }

      entryInsts.insert(insertIt, std::make_move_iterator(paramRetains.begin()),
                        std::make_move_iterator(paramRetains.end()));
    }

    std::unordered_set<MIRValue *> initializedPointers;

    for (auto &blockPtr : func->getBlocks()) {
      MIRBlock *block = blockPtr.get();
      // Need to swap out instructions array to safely mutate while iterating
      auto &instructions = block->getInstructionsMut();
      std::vector<std::unique_ptr<MIRInst>> newInstructions;

      bool blockModified = false;

      for (auto &inst : instructions) {

        // Insert Releases before Returning, Throwing, or Resuming!
        auto op = inst->getOpcode();
        if ((op == Opcode::Return || op == Opcode::Throw ||
             op == Opcode::Resume) &&
            !refCountedParams.empty()) {

          MIRValue *exitVal = nullptr;
          if (op == Opcode::Return) {
            exitVal = static_cast<ReturnInst *>(inst.get())->getReturnValue();
          } else if (op == Opcode::Throw) {
            exitVal = static_cast<ThrowInst *>(inst.get())->getException();
          }

          // Unwrap any bitcasts to find the true base pointer escaping the
          // function
          while (auto *cast = llvm::dyn_cast_or_null<CastInst>(exitVal)) {
            exitVal = cast->getValue();
          }

          for (auto *arg : refCountedParams) {
            // Do NOT release the parameter if we are returning or throwing it!
            // The +1 reference count safely transfers back to the
            // caller/catcher.
            if (arg == exitVal) {
              continue;
            }
            newInstructions.push_back(std::make_unique<ARCInst>(
                Opcode::Release, arg, getDropFunc(arg), inst->getLoc()));
          }
          blockModified = true;
        }

        // --- 1. Handle Assignments (Stores) ---
        if (auto *store = llvm::dyn_cast<StoreInst>(inst.get())) {
          MIRValue *newValue = store->getValue();

          if (newValue && newValue->getType() &&
              isRefCounted(newValue->getType())) {
            MIRValue *ptr = getUnderlyingObject(store->getPointer());
            blockModified = true;

            // A. Do NOT emit a Retain if we are storing a function parameter
            // into its local variable slot.
            if (!llvm::isa<MIRArgument>(newValue)) {
              // A. Retain the new value
              newInstructions.push_back(std::make_unique<ARCInst>(
                  Opcode::Retain, newValue, nullptr, inst->getLoc()));
            }

            // B. If the pointer was already initialized, release the old value
            if (initializedPointers.count(ptr)) {
              auto loadOld =
                  std::make_unique<LoadInst>(ptr, "old_val", inst->getLoc());
              MIRValue *oldValPtr = loadOld.get();
              newInstructions.push_back(std::move(loadOld));
              newInstructions.push_back(std::make_unique<ARCInst>(
                  Opcode::Release, oldValPtr, getDropFunc(oldValPtr),
                  inst->getLoc()));
            }

            // C. Push the actual store and mark as initialized
            initializedPointers.insert(ptr);
            newInstructions.push_back(std::move(inst));
            continue;
          }
        }

        // --- Standard Instructions ---
        newInstructions.push_back(std::move(inst));
      }

      for (auto &newInst : newInstructions) {
        newInst->setParent(block);
      }
      instructions = std::move(newInstructions);

      if (blockModified) {
        functionModified = true;
      }
    }

    return functionModified;
  }
};

} // namespace

bool runARCInsertion(MIRModule *module, DiagnosticEngine &diags) {
  ARCInserter inserter(module, diags);
  return inserter.run();
}

} // namespace mir
} // namespace moksha
