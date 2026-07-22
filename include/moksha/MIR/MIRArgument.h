#pragma once

#include "moksha/MIR/MIRValue.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {
namespace mir {

class MIRFunction;

/** @brief Represents a function argument in MIR. */
class MIRArgument : public MIRValue {
public:
  MIRArgument(MIRFunction *parent, const hir::HIRType *type, unsigned index,
              std::string name = "")
      : MIRValue(ValueKind::Argument, type, std::move(name)), parent(parent),
        index(index) {}

  MIRFunction *getParent() const { return parent; }
  unsigned getIndex() const { return index; }

  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::Argument;
  }

private:
  MIRFunction *parent;
  unsigned index;
};

} // namespace mir
} // namespace moksha
