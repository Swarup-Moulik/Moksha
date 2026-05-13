#pragma once

#include "moksha/MIR/MIRValue.h"
#include "llvm/Support/raw_ostream.h"
#include <string>

namespace moksha {
namespace mir {

class MIRConstant; // Forward declaration

class MIRGlobal : public MIRValue {
public:
  MIRGlobal(std::string name, const hir::HIRType *type,
            MIRConstant *initializer = nullptr, bool isConstant = false,
            Linkage linkage = Linkage::External)
      : MIRValue(ValueKind::Global, type, std::move(name)),
        initializer(initializer), isConstantFlag(isConstant), linkage(linkage),
        alignment(0), isUsedFlag(false) {}

  bool isConstant() const { return isConstantFlag; }
  Linkage getLinkage() const { return linkage; }
  void setLinkage(Linkage l) { linkage = l; }
  bool isWeak() const { return linkage == Linkage::Weak; }
  bool isExtern() const { return isExternFlag; }
  void setExtern(bool e) { isExternFlag = e; }

  // Initializer Management
  MIRConstant *getInitializer() const { return initializer; }
  void setInitializer(MIRConstant *init) { initializer = init; }
  bool hasInitializer() const { return initializer != nullptr; }

  uint32_t getAlignment() const { return alignment; }
  void setAlignment(uint32_t align) { alignment = align; }

  bool isUsed() const { return isUsedFlag; }
  void setUsed(bool u) { isUsedFlag = u; }

  bool isThreadLocal() const { return isThreadLocalFlag; }
  void setThreadLocal(bool tl) { isThreadLocalFlag = tl; }

  bool isVolatile() const { return isVolatileFlag; }
  void setVolatile(bool v) { isVolatileFlag = v; }

  const std::string &getSection() const { return sectionName; }
  void setSection(std::string s) { sectionName = std::move(s); }

  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::Global;
  }

private:
  MIRConstant *initializer;
  bool isConstantFlag;
  Linkage linkage;
  bool isExternFlag = false;
  uint32_t alignment;
  bool isUsedFlag;
  bool isThreadLocalFlag = false;
  bool isVolatileFlag = false;
  std::string sectionName;
};

} // namespace mir
} // namespace moksha
