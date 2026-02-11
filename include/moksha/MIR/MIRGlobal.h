#ifndef MOKSHA_MIR_MIRGLOBAL_H
#define MOKSHA_MIR_MIRGLOBAL_H

#include "moksha/MIR/MIRValue.h"
#include <string>

namespace moksha {
namespace mir {

class MIRGlobal : public MIRValue {
public:
  // [FIX] 1. Use 'const hir::HIRType*'
  // [FIX] 2. Swapped arguments to MIRValue base constructor
  MIRGlobal(std::string name, const hir::HIRType *type, bool isConstant = false,
            Linkage linkage = Linkage::External)
      : MIRValue(ValueKind::Global, type,
                 std::move(name)), // Passed name to MIRValue
        isConstantFlag(isConstant), linkage(linkage) {}

  // Removed redundant 'name' member since MIRValue already has one.
  // const std::string &getName() const { return name; } // MIRValue::getName()
  // handles this

  bool isConstant() const { return isConstantFlag; }
  Linkage getLinkage() const { return linkage; }

  void dump(std::ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::Global;
  }

private:
  // std::string name; // [FIX] Removed, handled by base class
  bool isConstantFlag;
  Linkage linkage;
};

} // namespace mir
} // namespace moksha

#endif // MOKSHA_MIR_MIRGLOBAL_H
