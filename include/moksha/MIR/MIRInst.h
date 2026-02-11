#pragma once

#include "moksha/Support/SourceLocation.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRValue.h"
#include <algorithm> // for std::find
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace moksha {
namespace mir {

// Forward declarations
class MIRBlock;
class MIRFunction;

// ============================================================================
// [Constants]
// ============================================================================

class MIRConstant : public MIRValue {
protected:
  MIRConstant(ValueKind k, const hir::HIRType *t) : MIRValue(k, t, "") {}
};

class ConstantInt : public MIRConstant {
public:
  ConstantInt(uint64_t val, const hir::HIRType *t)
      : MIRConstant(ValueKind::ConstantInt, t), value(val) {}

  uint64_t getValue() const { return value; }
  void dump(std::ostream &os) const override;
  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantInt;
  }

private:
  uint64_t value;
};

class ConstantFloat : public MIRConstant {
public:
  ConstantFloat(double val, const hir::HIRType *t)
      : MIRConstant(ValueKind::ConstantFloat, t), value(val) {}

  double getValue() const { return value; }
  void dump(std::ostream &os) const override;
  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantFloat;
  }

private:
  double value;
};

class ConstantBool : public MIRConstant {
public:
  ConstantBool(bool val, const hir::HIRType *t)
      : MIRConstant(ValueKind::ConstantBool, t), value(val) {}

  bool getValue() const { return value; }
  void dump(std::ostream &os) const override;
  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantBool;
  }

private:
  bool value;
};

class ConstantString : public MIRConstant {
public:
  ConstantString(std::string val, const hir::HIRType *t)
      : MIRConstant(ValueKind::ConstantString, t), value(std::move(val)) {}

  const std::string &getValue() const { return value; }
  void dump(std::ostream &os) const override;
  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantString;
  }

private:
  std::string value;
};

class ConstantNull : public MIRConstant {
public:
  explicit ConstantNull(const hir::HIRType *t)
      : MIRConstant(ValueKind::ConstantNull, t) {}

  void dump(std::ostream &os) const override;
  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantNull;
  }
};

// ============================================================================
// [Instruction Opcodes]
// ============================================================================

enum class Opcode {
  Alloca,
  Load,
  Store,
  GetElementPtr,
  InsertValue,
  ExtractValue,
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  FAdd,
  FSub,
  FMul,
  FDiv,
  And,
  Or,
  Xor,
  Shl,
  Shr,
  ICmp,
  FCmp,
  Br,
  CondBr,
  Return,
  Call,
  Switch,
  BitCast,
  IntToFloat,
  FloatToInt,
  Retain,
  Release,
  Phi
};

// ============================================================================
// [Base Instruction]
// ============================================================================

class MIRInst : public MIRValue {
public:
  Opcode getOpcode() const { return opcode; }
  SourceLocation getLoc() const { return loc; }

  MIRBlock *getParent() const { return parent; }
  void setParent(MIRBlock *bb) { parent = bb; }

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::Instruction;
  }

protected:
  MIRInst(Opcode op, const hir::HIRType *type, std::string name,
          SourceLocation loc)
      : MIRValue(ValueKind::Instruction, type, std::move(name)), opcode(op),
        loc(loc), parent(nullptr) {}

  Opcode opcode;
  SourceLocation loc;
  MIRBlock *parent;
};

// ============================================================================
// [Terminators]
// ============================================================================

class BranchInst : public MIRInst {
public:
  explicit BranchInst(MIRBlock *target, SourceLocation loc);
  MIRBlock *getTarget() const { return target; }
  void dump(std::ostream &os) const override;

private:
  MIRBlock *target;
};

class CondBranchInst : public MIRInst {
public:
  CondBranchInst(MIRValue *cond, MIRBlock *trueBlock, MIRBlock *falseBlock,
                 SourceLocation loc);
  MIRValue *getCondition() const { return cond; }
  MIRBlock *getTrueBlock() const { return trueBlock; }
  MIRBlock *getFalseBlock() const { return falseBlock; }
  void dump(std::ostream &os) const override;

private:
  MIRValue *cond;
  MIRBlock *trueBlock;
  MIRBlock *falseBlock;
};

class SwitchInst : public MIRInst {
public:
  SwitchInst(MIRValue *cond, MIRBlock *defaultBlock, SourceLocation loc);
  void addCase(MIRValue *value, MIRBlock *target);
  MIRValue *getCondition() const { return cond; }
  MIRBlock *getDefaultBlock() const { return defaultBlock; }
  const std::vector<std::pair<MIRValue *, MIRBlock *>> &getCases() const {
    return cases;
  }
  void dump(std::ostream &os) const override;

private:
  MIRValue *cond;
  MIRBlock *defaultBlock;
  std::vector<std::pair<MIRValue *, MIRBlock *>> cases;
};

class ReturnInst : public MIRInst {
public:
  explicit ReturnInst(MIRValue *val, SourceLocation loc);
  MIRValue *getReturnValue() const { return val; }
  void dump(std::ostream &os) const override;

private:
  MIRValue *val;
};

// ============================================================================
// [Memory & Aggregates]
// ============================================================================

class AllocaInst : public MIRInst {
public:
  AllocaInst(const hir::HIRType *allocType, std::string name,
             SourceLocation loc, unsigned align = 0);
  const hir::HIRType *getAllocatedType() const { return allocatedType; }
  unsigned getAlignment() const { return alignment; }
  void dump(std::ostream &os) const override;

private:
  const hir::HIRType *allocatedType;
  unsigned alignment;
};

class LoadInst : public MIRInst {
public:
  LoadInst(MIRValue *ptr, std::string name, SourceLocation loc,
           unsigned align = 0);
  MIRValue *getPointer() const { return ptr; }
  unsigned getAlignment() const { return alignment; }
  void dump(std::ostream &os) const override;

private:
  MIRValue *ptr;
  unsigned alignment;
};

class StoreInst : public MIRInst {
public:
  StoreInst(MIRValue *val, MIRValue *ptr, SourceLocation loc,
            unsigned align = 0);
  MIRValue *getValue() const { return val; }
  MIRValue *getPointer() const { return ptr; }
  unsigned getAlignment() const { return alignment; }
  void dump(std::ostream &os) const override;

private:
  MIRValue *val;
  MIRValue *ptr;
  unsigned alignment;
};

class GetElementPtrInst : public MIRInst {
public:
  // UPDATE: Used std::vector&& for move semantics (Performance)
  GetElementPtrInst(MIRValue *ptr, std::vector<MIRValue *> &&indices,
                    const hir::HIRType *resultType, std::string name,
                    SourceLocation loc);

  MIRValue *getPointer() const { return ptr; }
  const std::vector<MIRValue *> &getIndices() const { return indices; }
  void dump(std::ostream &os) const override;

private:
  MIRValue *ptr;
  std::vector<MIRValue *> indices;
};

class InsertValueInst : public MIRInst {
public:
  InsertValueInst(MIRValue *agg, MIRValue *val, uint32_t index,
                  std::string name, SourceLocation loc);
  MIRValue *getAggregate() const { return agg; }
  MIRValue *getInsertedValue() const { return val; }
  uint32_t getIndex() const { return index; }
  void dump(std::ostream &os) const override;

private:
  MIRValue *agg;
  MIRValue *val;
  uint32_t index;
};

class ExtractValueInst : public MIRInst {
public:
  ExtractValueInst(MIRValue *agg, uint32_t index, const hir::HIRType *resType,
                   std::string name, SourceLocation loc);
  MIRValue *getAggregate() const { return agg; }
  uint32_t getIndex() const { return index; }
  void dump(std::ostream &os) const override;

private:
  MIRValue *agg;
  uint32_t index;
};

// ============================================================================
// [ARC, Arithmetic, Logic, Casts]
// ============================================================================

class ARCInst : public MIRInst {
public:
  ARCInst(Opcode op, MIRValue *obj, SourceLocation loc);
  MIRValue *getObject() const { return obj; }
  void dump(std::ostream &os) const override;

private:
  MIRValue *obj;
};

class BinaryInst : public MIRInst {
public:
  BinaryInst(Opcode op, MIRValue *lhs, MIRValue *rhs, std::string name,
             SourceLocation loc);
  MIRValue *getLHS() const { return lhs; }
  MIRValue *getRHS() const { return rhs; }
  void dump(std::ostream &os) const override;

private:
  MIRValue *lhs;
  MIRValue *rhs;
};

class CastInst : public MIRInst {
public:
  CastInst(Opcode op, MIRValue *value, const hir::HIRType *destType,
           std::string name, SourceLocation loc);
  MIRValue *getValue() const { return value; }
  void dump(std::ostream &os) const override;

private:
  MIRValue *value;
};

class CompareInst : public MIRInst {
public:
  enum class Predicate {
    EQ,
    NE,
    LT,
    LE,
    GT,
    GE, // Signed
    ULT,
    ULE,
    UGT,
    UGE // Unsigned
  };
  CompareInst(Predicate pred, MIRValue *lhs, MIRValue *rhs, std::string name,
              SourceLocation loc);
  Predicate getPredicate() const { return pred; }
  MIRValue *getLHS() const { return lhs; }
  MIRValue *getRHS() const { return rhs; }
  void dump(std::ostream &os) const override;

private:
  Predicate pred;
  MIRValue *lhs;
  MIRValue *rhs;
};

class PhiInst : public MIRInst {
public:
  explicit PhiInst(const hir::HIRType *type, std::string name,
                   SourceLocation loc);
  void addIncoming(MIRValue *val, MIRBlock *block);
  const std::vector<std::pair<MIRValue *, MIRBlock *>> &getIncoming() const {
    return incoming;
  }
  void dump(std::ostream &os) const override;

private:
  std::vector<std::pair<MIRValue *, MIRBlock *>> incoming;
};

class CallInst : public MIRInst {
public:
  // UPDATE: Used std::vector&& for move semantics (Performance)
  CallInst(MIRValue *callee, std::vector<MIRValue *> &&args,
           const hir::HIRType *retType, std::string name, bool isVarArg,
           SourceLocation loc);

  MIRValue *getCallee() const { return callee; }
  const std::vector<MIRValue *> &getArgs() const { return args; }
  bool isVariadic() const { return isVarArg; }
  void dump(std::ostream &os) const override;

private:
  MIRValue *callee;
  std::vector<MIRValue *> args;
  bool isVarArg;
};

} // namespace mir
} // namespace moksha
