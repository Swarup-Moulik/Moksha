#pragma once

#include "moksha/MIR/MIRFunction.h"
#include "llvm/Support/raw_ostream.h"
#include <iosfwd> // Added
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace moksha {
namespace mir {

class MIRGlobal;
class MIRConstant;

enum class IntrinsicID {
  None,
  Bswap,
  Ctlz,
  Cttz,
  Ctpop,
  Trap,
  // --- Atomics ---
  AtomicLoad,
  AtomicStore,
  AtomicAdd,
  AtomicCAS,
  AtomicFenceAcquire,
  AtomicFenceRelease,
  AtomicFenceSeqCst,
  AtomicThreadFence
};

class MIRModule {
public:
  // [FIX] Removed MIRContext& param
  explicit MIRModule(std::string name);
  ~MIRModule();

  const std::string &getName() const { return name; }

  // Function Management
  void addFunction(std::unique_ptr<MIRFunction> func);
  MIRFunction *getFunction(const std::string &name) const;
  const std::vector<std::unique_ptr<MIRFunction>> &getFunctions() const {
    return functions;
  }

  // Global Variable Management
  void addGlobal(std::unique_ptr<MIRGlobal> global);
  MIRGlobal *getGlobal(const std::string &name) const;
  const std::vector<std::unique_ptr<MIRGlobal>> &getGlobals() const {
    return globals;
  }

  std::vector<std::unique_ptr<MIRGlobal>> &getGlobalsMut() { return globals; }
  std::unordered_map<std::string, MIRGlobal *> &getGlobalMapMut() {
    return globalMap;
  }

  IntrinsicID lookupIntrinsic(const std::string &name) const;

  // Legacy Helpers
  MIRGlobal *findGlobalByName(const std::string &name) const {
    return getGlobal(name);
  }
  MIRFunction *findFunctionByName(const std::string &name) const {
    return getFunction(name);
  }

  void dump(llvm::raw_ostream &os) const;

  // Constant Management
  template <typename T, typename... Args>
  T *getOrInsertConstant(Args &&...args) {
    auto c = std::make_unique<T>(std::forward<Args>(args)...);
    T *ptr = c.get();
    constants.push_back(std::move(c));
    return ptr;
  }

  const hir::HIRType *getPointerType(const hir::HIRType *pointee);
  const hir::HIRType *getArrayType(const hir::HIRType *elemTy, size_t size);

private:
  std::string name;

  std::vector<std::unique_ptr<MIRFunction>> functions;
  std::vector<std::unique_ptr<MIRGlobal>> globals;

  std::unordered_map<std::string, MIRFunction *> functionMap;
  std::unordered_map<std::string, MIRGlobal *> globalMap;

  std::vector<std::unique_ptr<MIRConstant>> constants;

  std::vector<std::unique_ptr<hir::HIRType>> derivedTypes;
  std::unordered_map<const hir::HIRType *, std::unique_ptr<hir::PointerType>>
      pointerTypeCache;
};

} // namespace mir
} // namespace moksha
