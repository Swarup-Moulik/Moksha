#include "moksha/MIR/MIRModule.h"
#include "moksha/HIR/HIRFunction.h"
#include "moksha/HIR/HIRStmt.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRGlobal.h"
#include "llvm/ADT/ArrayRef.h"
#include <iostream>
#include <unordered_map>

namespace moksha {
namespace mir {

// The Scalable Registry
static const std::unordered_map<std::string, IntrinsicID> IntrinsicTable = {
    {"llvm.bswap.i16", IntrinsicID::Bswap},
    {"llvm.bswap.i32", IntrinsicID::Bswap},
    {"llvm.bswap.i64", IntrinsicID::Bswap},
    {"llvm.ctlz.i32", IntrinsicID::Ctlz},
    {"llvm.cttz.i32", IntrinsicID::Cttz},
    {"llvm.ctpop.i32", IntrinsicID::Ctpop},
    {"llvm.trap", IntrinsicID::Trap},
    // --- Atomics Registry ---
    {"atomic_load", IntrinsicID::AtomicLoad},
    {"atomic_store", IntrinsicID::AtomicStore},
    {"atomic_add", IntrinsicID::AtomicAdd},
    {"atomic_cas", IntrinsicID::AtomicCAS},
    {"atomic_fence_acquire", IntrinsicID::AtomicFenceAcquire},
    {"atomic_fence_release", IntrinsicID::AtomicFenceRelease},
    {"atomic_fence_seqcst", IntrinsicID::AtomicFenceSeqCst},
    {"atomic_thread_fence", IntrinsicID::AtomicThreadFence},
};

IntrinsicID MIRModule::lookupIntrinsic(const std::string &name) const {
  auto it = IntrinsicTable.find(name);
  return it != IntrinsicTable.end() ? it->second : IntrinsicID::None;
}

MIRModule::MIRModule(std::string name) : name(std::move(name)) {}

MIRModule::~MIRModule() = default;

// Populate the functionMap
void MIRModule::addFunction(std::unique_ptr<MIRFunction> func) {
  functionMap[func->getName()] = func.get();
  functions.push_back(std::move(func));
  functionsDirty = true;
}

llvm::ArrayRef<MIRFunction *> MIRModule::getFunctions() const {
  if (functionsDirty) {
    functionCache.clear();
    functionCache.reserve(functions.size());
    for (const auto &f : functions) {
      functionCache.push_back(f.get());
    }
    functionsDirty = false;
  }
  return functionCache;
}

// Populate the globalMap
void MIRModule::addGlobal(std::unique_ptr<MIRGlobal> global) {
  globalMap[global->getName()] = global.get();
  globals.push_back(std::move(global));
  globalsDirty = true;
}

llvm::ArrayRef<MIRGlobal *> MIRModule::getGlobals() const {
  if (globalsDirty) {
    globalCache.clear();
    globalCache.reserve(globals.size());
    for (const auto &g : globals) {
      globalCache.push_back(g.get());
    }
    globalsDirty = false;
  }
  return globalCache;
}

// Class Management
void MIRModule::addClass(std::unique_ptr<hir::HIRClass> cls) {
  classes.push_back(std::move(cls));
  classesDirty = true;
}

llvm::ArrayRef<hir::HIRClass *> MIRModule::getClasses() const {
  if (classesDirty) {
    classCache.clear();
    classCache.reserve(classes.size());
    for (const auto &c : classes) {
      classCache.push_back(c.get());
    }
    classesDirty = false;
  }
  return classCache;
}

void MIRModule::dump(llvm::raw_ostream &os) const {
  os << "; ModuleID = '" << getName() << "'\n\n";

  for (const auto &g : getGlobals()) {
    g->dump(os);
    os << "\n";
  }
  if (!getGlobals().empty())
    os << "\n";

  for (const auto &f : getFunctions()) {
    f->dump(os);
    os << "\n";
  }
}

const hir::HIRType *MIRModule::getPointerType(const hir::HIRType *pointeeType) {
  if (!pointeeType)
    return nullptr;

  // Return cached instance if it exists to preserve pointer equality
  auto it = pointerTypeCache.find(pointeeType);
  if (it != pointerTypeCache.end()) {
    return it->second.get();
  }

  auto newPtrTy =
      std::make_unique<hir::PointerType>(pointeeType, hir::Ownership::None);
  const hir::HIRType *rawPtr = newPtrTy.get();

  pointerTypeCache[pointeeType] = std::move(newPtrTy);

  return rawPtr;
}

const hir::HIRType *MIRModule::getArrayType(const hir::HIRType *elemTy,
                                            size_t size) {
  // Allocate the new array type
  auto newArrayTy = std::make_unique<hir::ArrayType>(elemTy, size);
  const hir::HIRType *rawPtr = newArrayTy.get();
  derivedTypes.push_back(std::move(newArrayTy));

  return rawPtr;
}

MIRFunction *MIRModule::getFunction(const std::string &name) const {
  auto it = functionMap.find(name);
  return it != functionMap.end() ? it->second : nullptr;
}

MIRGlobal *MIRModule::getGlobal(const std::string &name) const {
  auto it = globalMap.find(name);
  return it != globalMap.end() ? it->second : nullptr;
}

} // namespace mir
} // namespace moksha
