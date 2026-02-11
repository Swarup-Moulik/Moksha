#ifndef MOKSHA_MIR_MIRARGUMENT_H
#define MOKSHA_MIR_MIRARGUMENT_H

#include "moksha/MIR/MIRValue.h"

namespace moksha {
namespace mir {

class MIRFunction;

class MIRArgument : public MIRValue {
public:
  // [FIX] 1. Use 'const hir::HIRType*'
  // [FIX] 2. Swapped arguments to MIRValue base constructor
  MIRArgument(MIRFunction *parent, const hir::HIRType *type, unsigned index)
      : MIRValue(ValueKind::Argument, type), parent(parent), index(index) {}

  MIRFunction *getParent() const { return parent; }
  unsigned getIndex() const { return index; }

  void dump(std::ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::Argument;
  }

private:
  MIRFunction *parent;
  unsigned index;
};

} // namespace mir
} // namespace moksha

#endif // MOKSHA_MIR_MIRARGUMENT_H
