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
      modified |= runOnFunction(func);
    }

    // Global Teardown (Module Destroy)
    if (MIRFunction *destroyFunc =
            module->getFunction("__moksha_module_destroy")) {
      if (!destroyFunc->getBlocks().empty()) {
        MIRBlock *entry = destroyFunc->getEntryBlock();
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

          if (g->isConstant()) {
            continue;
          }

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
            retIt =
                entry->getInstructionsMut().insert(retIt, std::move(loadInst));
            ++retIt;
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
    if (auto *nullableTy =
            llvm::dyn_cast_or_null<hir::HIRNullableType>(valTy)) {
      valTy = nullableTy->getInner();
    }
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
    if (!type)
      return false;

    // 1. Unwrap Nullable Optionals (e.g., Node?)
    if (auto *nullableTy = llvm::dyn_cast_or_null<const hir::HIRNullableType>(type)) {
      type = nullableTy->getInner();
    }

    // 2. Safely capture Reference Classes and explicitly tagged pointers
    if (auto *ptrType = llvm::dyn_cast_or_null<const hir::PointerType>(type)) {
      if (ptrType->getOwnership() == hir::Ownership::Borrowed ||
          ptrType->getOwnership() == hir::Ownership::None) {
        return false;
      }

      if (ptrType->getOwnership() == hir::Ownership::Shared ||
          ptrType->getOwnership() == hir::Ownership::Owned) {
        return true;
      }
      if (ptrType->getPointee() &&
          ptrType->getPointee()->getKind() == hir::TypeKind::Struct) {
        return true;
      }
      return false;
    }

    // 3. Catch native built-in managed types
    auto kind = type->getKind();
    if (kind == hir::TypeKind::Any || kind == hir::TypeKind::Slice ||
        kind == hir::TypeKind::String || kind == hir::TypeKind::Map ||
        kind == hir::TypeKind::Closure || kind == hir::TypeKind::Promise) {
      return true;
    }

    return false;
  }

  // Analyzes the actual memory location rather than the loaded SSA instance.
  MIRValue *getUnderlyingObject(MIRValue *val) {
    while (auto *inst = llvm::dyn_cast_or_null<MIRInst>(val)) {
      if (auto *cast = llvm::dyn_cast_or_null<CastInst>(inst)) {
        val = cast->getValue();
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
      if (arg->getName() == "this") {
        continue;
      }
      if (isRefCounted(arg->getType())) {
        refCountedParams.push_back(arg);
      }
    }

    std::unordered_set<MIRValue *> initializedPointers;

    for (auto &blockPtr : func->getBlocks()) {
      MIRBlock *block = blockPtr.get();
      auto &instructions = block->getInstructionsMut();
      std::vector<std::unique_ptr<MIRInst>> newInstructions;

      bool blockModified = false;

      MIRValue *exitVal = nullptr;
      MIRValue *baseExitVal = nullptr;
      if (!instructions.empty()) {
        if (auto *retInst =
                llvm::dyn_cast_or_null<ReturnInst>(instructions.back().get())) {
          exitVal = retInst->getReturnValue();
          if (exitVal && exitVal->getType() &&
              isRefCounted(exitVal->getType())) {
            baseExitVal = getUnderlyingObject(exitVal);
          }
        }
      }

      bool elidedFrontendRelease = false;

      for (auto &inst : instructions) {
        auto op = inst->getOpcode();
        if (op == Opcode::Release && baseExitVal) {
          auto *arcInst = static_cast<ARCInst *>(inst.get());
          if (getUnderlyingObject(arcInst->getObject()) == baseExitVal) {
            elidedFrontendRelease = true;
            blockModified = true;
            continue;
          }
        }

        if (op == Opcode::Return && baseExitVal) {
          bool isParam = false;
          for (auto *arg : refCountedParams) {
            if (arg == baseExitVal)
              isParam = true;
          }
          if (!elidedFrontendRelease && !isParam) {
            newInstructions.push_back(std::make_unique<ARCInst>(
                Opcode::Retain, exitVal, nullptr, inst->getLoc()));
            blockModified = true;
          }
        }

        if (auto *store = llvm::dyn_cast_or_null<StoreInst>(inst.get())) {
          MIRValue *newValue = store->getValue();

          if (newValue && newValue->getType() &&
              isRefCounted(newValue->getType())) {
            MIRValue *ptr = getUnderlyingObject(store->getPointer());
            blockModified = true;

            bool isFreshAllocation = false;
            MIRValue *traceVal = newValue;

            while (traceVal) {
              if (auto *cast = llvm::dyn_cast_or_null<CastInst>(traceVal)) {
                traceVal = cast->getValue();
              } else {
                break;
              }
            }

            if (auto call = llvm::dyn_cast_or_null<CallInst>(traceVal)) {
              if (call->getCallee() &&
                  call->getCallee()->getName() == "__moksha_alloc") {
                isFreshAllocation = true;
              }
            } else if (auto invoke = llvm::dyn_cast_or_null<InvokeInst>(traceVal)) {
              if (invoke->getCallee() &&
                  invoke->getCallee()->getName() == "__moksha_alloc") {
                isFreshAllocation = true;
              }
            }

            if (!isFreshAllocation) {
              newInstructions.push_back(std::make_unique<ARCInst>(
                  Opcode::Retain, newValue, nullptr, inst->getLoc()));
            }

            if (initializedPointers.count(ptr)) {
              auto loadOld =
                  std::make_unique<LoadInst>(ptr, "old_val", inst->getLoc());
              MIRValue *oldValPtr = loadOld.get();
              newInstructions.push_back(std::move(loadOld));
              newInstructions.push_back(std::make_unique<ARCInst>(
                  Opcode::Release, oldValPtr, getDropFunc(oldValPtr),
                  inst->getLoc()));
            }

            initializedPointers.insert(ptr);
            newInstructions.push_back(std::move(inst));
            continue;
          }
        }

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
