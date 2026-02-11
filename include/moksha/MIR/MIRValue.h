#ifndef MOKSHA_MIR_MIRVALUE_H
#define MOKSHA_MIR_MIRVALUE_H

#include "moksha/HIR/HIRType.h"
#include <iostream>
#include <string>

namespace moksha {
namespace mir {

enum class Linkage { Internal, External };

enum class ValueKind {
  Instruction,
  Argument,
  Global,
  ConstantInt,
  ConstantFloat,
  ConstantBool,
  ConstantString,
  ConstantNull,
  BasicBlock,
  Function
};

class MIRValue {
public:
  virtual ~MIRValue() = default;

  ValueKind getKind() const { return kind; }
  const hir::HIRType *getType() const { return type; }
  const std::string &getName() const { return name; }
  void setName(std::string n) { name = std::move(n); }

  virtual void dump(std::ostream &os) const = 0;

protected:
  MIRValue(ValueKind k, const hir::HIRType *t, std::string n = "")
      : kind(k), type(t), name(std::move(n)) {}

  ValueKind kind;
  const hir::HIRType *type;
  std::string name;
};

} // namespace mir
} // namespace moksha

#endif
