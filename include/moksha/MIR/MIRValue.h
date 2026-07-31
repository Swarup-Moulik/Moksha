#pragma once

#include "moksha/HIR/HIRType.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <string>

namespace moksha {
namespace mir {

/** @brief Represents the linkage of a symbol. */
enum class Linkage { Internal, External, Weak, LinkOnce };

/** @brief Represents the calling convention of a function. */
enum class CallingConv { C, StdCall, FastCall, VectorCall, SysV64, Win64, Interrupt };

/** @brief Represents the kind of a value. */
enum class ValueKind {
  Instruction,
  Argument,
  Global,
  ConstantInt,
  ConstantFloat,
  ConstantBool,
  ConstantString,
  ConstantNull,
  ConstantUndef,
  ConstantDecimal,
  ConstantArray,
  ConstantSlice,
  ConstantMap,
  ConstantStruct,
  ConstantUnion,
  ConstantBitCast,
  ConstantAnyCast,
  ConstantArrayToSlice,
  ConstantSliceToArray,
  ConstantUpcast,
  BasicBlock,
  Function
};

/** @brief Represents the borrow kind of a value. */
enum class BorrowKind {
  None, // Standard by-value data (e.g., int, bool)
  View, // Read-only shared borrow (*view)
  Mut,  // Exclusive mutable borrow (*mut)
  Lock  // Thread-safe mutex borrow (*lock)
};

/** @brief Base class for all MIR values. */
class MIRValue {
public:
  virtual ~MIRValue() = default;

  ValueKind getKind() const { return kind; }
  const hir::HIRType *getType() const { return type; }
  void setType(const hir::HIRType *t) { type = t; }
  const std::string &getName() const { return name; }
  void setName(std::string n) { name = std::move(n); }
  BorrowKind getBorrowKind() const { return borrowKind; }
  void setBorrowKind(BorrowKind bk) { borrowKind = bk; }

  virtual void dump(llvm::raw_ostream &os) const = 0;

protected:
  MIRValue(ValueKind k, const hir::HIRType *t, std::string n = "")
      : kind(k), type(t), name(std::move(n)) {}

  ValueKind kind;
  const hir::HIRType *type;
  std::string name;
  BorrowKind borrowKind;
};

} // namespace mir
} // namespace moksha
