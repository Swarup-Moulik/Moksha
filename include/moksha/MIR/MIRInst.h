#pragma once

#include "moksha/HIR/HIRExpr.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRValue.h"
#include "moksha/Support/SourceLocation.h"
#include "llvm/Support/raw_ostream.h"
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

public:
  static bool classof(const MIRValue *v) {
    return v->getKind() >= ValueKind::ConstantInt &&
           v->getKind() <= ValueKind::ConstantBitCast;
  }
};

class ConstantInt : public MIRConstant {
public:
  ConstantInt(uint64_t val, const hir::HIRType *t)
      : MIRConstant(ValueKind::ConstantInt, t), value(val) {}
  uint64_t getValue() const { return value; }
  void dump(llvm::raw_ostream &os) const override;
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
  void dump(llvm::raw_ostream &os) const override;
  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantFloat;
  }

private:
  double value;
};

class ConstantDecimal : public MIRConstant {
public:
  ConstantDecimal(std::string val, const hir::HIRType *t)
      : MIRConstant(ValueKind::ConstantDecimal, t), value(std::move(val)) {}

  const std::string &getValue() const { return value; }
  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantDecimal;
  }

private:
  std::string value;
};

class ConstantBool : public MIRConstant {
public:
  ConstantBool(bool val, const hir::HIRType *t)
      : MIRConstant(ValueKind::ConstantBool, t), value(val) {}

  bool getValue() const { return value; }
  void dump(llvm::raw_ostream &os) const override;
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
  void dump(llvm::raw_ostream &os) const override;
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

  void dump(llvm::raw_ostream &os) const override;
  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantNull;
  }
};

class ConstantUndef : public MIRConstant {
public:
  ConstantUndef(const hir::HIRType *t)
      : MIRConstant(ValueKind::ConstantUndef, t) {}

  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantUndef;
  }
};

class ConstantArray : public MIRConstant {
public:
  ConstantArray(const hir::HIRType *t, std::vector<MIRValue *> elements)
      : MIRConstant(ValueKind::ConstantArray, t),
        elements(std::move(elements)) {}

  const std::vector<MIRValue *> &getElements() const { return elements; }
  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantArray;
  }

private:
  std::vector<MIRValue *> elements;
};

class ConstantSlice : public MIRConstant {
  std::vector<MIRConstant *> elements;

public:
  ConstantSlice(const hir::HIRType *t, std::vector<MIRConstant *> elems)
      : MIRConstant(ValueKind::ConstantSlice, t), elements(std::move(elems)) {}

  llvm::ArrayRef<MIRConstant *> getElements() const { return elements; }

  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantSlice;
  }
};

class ConstantMap : public MIRConstant {
public:
  ConstantMap(const hir::HIRType *t,
              std::vector<std::pair<MIRValue *, MIRValue *>> entries)
      : MIRConstant(ValueKind::ConstantMap, t), entries(std::move(entries)) {}

  const std::vector<std::pair<MIRValue *, MIRValue *>> &getEntries() const {
    return entries;
  }
  void dump(llvm::raw_ostream &os) const override;
  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantMap;
  }

private:
  std::vector<std::pair<MIRValue *, MIRValue *>> entries;
};

class ConstantStruct : public MIRConstant {
public:
  ConstantStruct(const hir::HIRType *t, std::vector<MIRValue *> fields)
      : MIRConstant(ValueKind::ConstantStruct, t), fields(std::move(fields)) {}

  const std::vector<MIRValue *> &getFields() const { return fields; }
  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantStruct;
  }

private:
  std::vector<MIRValue *> fields;
};

class ConstantBitCast : public MIRConstant {
public:
  ConstantBitCast(MIRValue *value, const hir::HIRType *destType)
      : MIRConstant(ValueKind::ConstantBitCast, destType), value(value) {}

  MIRValue *getValue() const { return value; }
  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantBitCast;
  }

private:
  MIRValue *value;
};

// ============================================================================
// [Instruction Opcodes]
// ============================================================================

enum class MemoryOrder { Relaxed, Consume, Acquire, Release, AcqRel, SeqCst };

enum class AtomicOp {
  Xchg,
  Add,
  Sub,
  And,
  Nand,
  Or,
  Xor,
  Max,
  Min,
  UMax,
  UMin
};

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
  Pow,
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
  Unreachable,
  Call,
  Switch,
  BitCast,
  IntToFloat,
  FloatToInt,
  ZExt,
  SExt,
  Trunc,
  Retain,
  Release,
  StoreWeak,
  LoadWeak,
  Phi,
  Invoke,
  LandingPad,
  Resume,
  Throw,
  InlineAsm,
  MakeClosure,
  Spawn,
  Await,
  AtomicLoad,
  AtomicStore,
  AtomicRMW,
  AtomicCmpXchg,
  Fence,
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
  virtual void replaceOperand(MIRValue *oldVal, MIRValue *newVal) {}

protected:
  MIRInst(Opcode op, const hir::HIRType *type, std::string name,
          SourceLocation loc)
      : MIRValue(ValueKind::Instruction, type, std::move(name)), opcode(op),
        loc(loc), parent(nullptr) {}

  Opcode opcode;
  SourceLocation loc;
  MIRBlock *parent;
};

// --- [MACRO FOR AUTO RTTI GENERATION] ---
#define DECLARE_INST_CLASSOF(OpVal)                                            \
  static bool classof(const MIRValue *v) {                                     \
    if (v->getKind() != ValueKind::Instruction)                                \
      return false;                                                            \
    return static_cast<const MIRInst *>(v)->getOpcode() == Opcode::OpVal;      \
  }

// ============================================================================
// [Terminators]
// ============================================================================

class BranchInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Br)

  explicit BranchInst(MIRBlock *target, SourceLocation loc);
  MIRBlock *getTarget() const { return target; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override;

private:
  MIRBlock *target;
};

class CondBranchInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(CondBr)

  CondBranchInst(MIRValue *cond, MIRBlock *trueBlock, MIRBlock *falseBlock,
                 SourceLocation loc);
  MIRValue *getCondition() const { return cond; }
  MIRBlock *getTrueBlock() const { return trueBlock; }
  MIRBlock *getFalseBlock() const { return falseBlock; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override;

private:
  MIRValue *cond;
  MIRBlock *trueBlock;
  MIRBlock *falseBlock;
};

class SwitchInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Switch)

  SwitchInst(MIRValue *cond, MIRBlock *defaultBlock, SourceLocation loc);
  void addCase(MIRValue *value, MIRBlock *target);
  MIRValue *getCondition() const { return cond; }
  MIRBlock *getDefaultBlock() const { return defaultBlock; }
  const std::vector<std::pair<MIRValue *, MIRBlock *>> &getCases() const {
    return cases;
  }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override;

private:
  MIRValue *cond;
  MIRBlock *defaultBlock;
  std::vector<std::pair<MIRValue *, MIRBlock *>> cases;
};

class ReturnInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Return)

  explicit ReturnInst(MIRValue *val, SourceLocation loc);
  MIRValue *getReturnValue() const { return val; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (val == oldVal)
      val = newVal;
  }

private:
  MIRValue *val;
};

class UnreachableInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Unreachable)

  explicit UnreachableInst(SourceLocation loc)
      : MIRInst(Opcode::Unreachable, nullptr, "", loc) {}

  void dump(llvm::raw_ostream &os) const override;
};

// ============================================================================
// [Memory & Aggregates]
// ============================================================================

class AllocaInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Alloca)

  AllocaInst(const hir::HIRType *ptrType, const hir::HIRType *allocType,
             std::string name, SourceLocation loc, unsigned align);
  const hir::HIRType *getAllocatedType() const { return allocatedType; }
  unsigned getAlignment() const { return alignment; }
  void dump(llvm::raw_ostream &os) const override;

private:
  const hir::HIRType *allocatedType;
  unsigned alignment;
};

class LoadInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Load)

  LoadInst(MIRValue *ptr, std::string name, SourceLocation loc,
           unsigned align = 0);
  MIRValue *getPointer() const { return ptr; }
  bool isVolatile() const { return isVolatileFlag; }
  void setVolatile(bool v) { isVolatileFlag = v; }
  unsigned getAlignment() const { return alignment; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (ptr == oldVal)
      ptr = newVal;
  }

private:
  MIRValue *ptr;
  unsigned alignment;
  bool isVolatileFlag = false;
};

class StoreInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Store)

  StoreInst(MIRValue *val, MIRValue *ptr, SourceLocation loc,
            unsigned align = 0);
  MIRValue *getValue() const { return val; }
  MIRValue *getPointer() const { return ptr; }
  bool isVolatile() const { return isVolatileFlag; }
  void setVolatile(bool v) { isVolatileFlag = v; }
  unsigned getAlignment() const { return alignment; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (val == oldVal)
      val = newVal;
    if (ptr == oldVal)
      ptr = newVal;
  }

private:
  MIRValue *val;
  MIRValue *ptr;
  unsigned alignment;
  bool isVolatileFlag = false;
};

class GetElementPtrInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(GetElementPtr)

  GetElementPtrInst(MIRValue *ptr, std::vector<MIRValue *> &&indices,
                    const hir::HIRType *ptrType, const hir::HIRType *resType,
                    std::string name, SourceLocation loc);

  MIRValue *getPointer() const { return ptr; }
  const std::vector<MIRValue *> &getIndices() const { return indices; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (ptr == oldVal)
      ptr = newVal;
    for (auto &idx : indices) {
      if (idx == oldVal)
        idx = newVal;
    }
  }

private:
  MIRValue *ptr;
  std::vector<MIRValue *> indices;
};

class InsertValueInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(InsertValue)

  InsertValueInst(MIRValue *agg, MIRValue *val, uint32_t index,
                  std::string name, SourceLocation loc);
  MIRValue *getAggregate() const { return agg; }
  MIRValue *getValue() const { return val; }
  uint32_t getIndex() const { return index; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (agg == oldVal)
      agg = newVal;
    if (val == oldVal)
      val = newVal;
  }

private:
  MIRValue *agg;
  MIRValue *val;
  uint32_t index;
};

class ExtractValueInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(ExtractValue)

  ExtractValueInst(MIRValue *agg, uint32_t index, const hir::HIRType *resType,
                   std::string name, SourceLocation loc);
  MIRValue *getAggregate() const { return agg; }
  uint32_t getIndex() const { return index; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (agg == oldVal)
      agg = newVal;
  }

private:
  MIRValue *agg;
  uint32_t index;
};

// ============================================================================
// [ARC, Arithmetic, Logic, Casts]
// ============================================================================

class ARCInst : public MIRInst {
public:
  static bool classof(const MIRValue *v) {
    if (v->getKind() != ValueKind::Instruction)
      return false;
    auto op = static_cast<const MIRInst *>(v)->getOpcode();
    return op == Opcode::Retain || op == Opcode::Release;
  }

  ARCInst(Opcode op, MIRValue *obj, SourceLocation loc);
  MIRValue *getObject() const { return obj; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (obj == oldVal) {
      obj = newVal;
    }
  }

private:
  MIRValue *obj;
};

class StoreWeakInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(StoreWeak)

  StoreWeakInst(MIRValue *val, MIRValue *ptr, SourceLocation loc);

  MIRValue *getValue() const { return val; }
  MIRValue *getPointer() const { return ptr; }

  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (val == oldVal)
      val = newVal;
    if (ptr == oldVal)
      ptr = newVal;
  }

private:
  MIRValue *val;
  MIRValue *ptr;
};

class LoadWeakInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(LoadWeak)

  LoadWeakInst(MIRValue *ptr, const hir::HIRType *resType, std::string name,
               SourceLocation loc);

  MIRValue *getPointer() const { return ptr; }

  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (ptr == oldVal)
      ptr = newVal;
  }

private:
  MIRValue *ptr;
};

class BinaryInst : public MIRInst {
public:
  static bool classof(const MIRValue *v) {
    if (v->getKind() != ValueKind::Instruction)
      return false;
    auto op = static_cast<const MIRInst *>(v)->getOpcode();
    return op >= Opcode::Add && op <= Opcode::Shr;
  }

  BinaryInst(Opcode op, MIRValue *lhs, MIRValue *rhs, std::string name,
             SourceLocation loc);
  MIRValue *getLHS() const { return lhs; }
  MIRValue *getRHS() const { return rhs; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (lhs == oldVal)
      lhs = newVal;
    if (rhs == oldVal)
      rhs = newVal;
  }

private:
  MIRValue *lhs;
  MIRValue *rhs;
};

class CastInst : public MIRInst {
public:
  static bool classof(const MIRValue *v) {
    if (v->getKind() != ValueKind::Instruction)
      return false;
    auto op = static_cast<const MIRInst *>(v)->getOpcode();
    // Update this list to match all the cast opcodes in your enum!
    return op == Opcode::BitCast || op == Opcode::IntToFloat ||
           op == Opcode::FloatToInt || op == Opcode::Trunc ||
           op == Opcode::SExt || op == Opcode::ZExt;
  }

  CastInst(Opcode op, MIRValue *value, const hir::HIRType *destType,
           std::string name, SourceLocation loc);
  MIRValue *getValue() const { return value; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (value == oldVal)
      value = newVal;
  }

private:
  MIRValue *value;
};

class CompareInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(ICmp)

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
  CompareInst(Predicate pred, MIRValue *lhs, MIRValue *rhs,
              const hir::HIRType *resType, std::string name,
              SourceLocation loc);
  Predicate getPredicate() const { return pred; }
  MIRValue *getLHS() const { return lhs; }
  MIRValue *getRHS() const { return rhs; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (lhs == oldVal)
      lhs = newVal;
    if (rhs == oldVal)
      rhs = newVal;
  }

private:
  Predicate pred;
  MIRValue *lhs;
  MIRValue *rhs;
};

class FCmpInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(FCmp)

  // Standard LLVM floating-point predicates
  enum class Predicate {
    OEQ, // Ordered and Equal
    ONE, // Ordered and Not Equal
    OLT, // Ordered and Less Than
    OLE, // Ordered and Less Than or Equal
    OGT, // Ordered and Greater Than
    OGE, // Ordered and Greater Than or Equal
    UEQ, // Unordered or Equal
    UNE, // Unordered or Not Equal
    ULT, // Unordered or Less Than
    ULE, // Unordered or Less Than or Equal
    UGT, // Unordered or Greater Than
    UGE  // Unordered or Greater Than or Equal
  };

  FCmpInst(Predicate pred, MIRValue *lhs, MIRValue *rhs,
           const hir::HIRType *resType, std::string name, SourceLocation loc);

  Predicate getPredicate() const { return pred; }
  MIRValue *getLHS() const { return lhs; }
  MIRValue *getRHS() const { return rhs; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (lhs == oldVal)
      lhs = newVal;
    if (rhs == oldVal)
      rhs = newVal;
  }

private:
  Predicate pred;
  MIRValue *lhs;
  MIRValue *rhs;
};

class PhiInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Phi)

  explicit PhiInst(const hir::HIRType *type, std::string name,
                   SourceLocation loc);
  void addIncoming(MIRValue *val, MIRBlock *block);
  void removeIncoming(MIRBlock *block);
  const std::vector<std::pair<MIRValue *, MIRBlock *>> &getIncoming() const {
    return incoming;
  }
  std::vector<std::pair<MIRValue *, MIRBlock *>> &getIncomingMut() {
    return incoming;
  }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    for (auto &inc : incoming) {
      if (inc.first == oldVal)
        inc.first = newVal;
    }
  }

private:
  std::vector<std::pair<MIRValue *, MIRBlock *>> incoming;
};

class CallInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Call)

  // UPDATE: Used std::vector&& for move semantics (Performance)
  CallInst(MIRValue *callee, std::vector<MIRValue *> &&args,
           const hir::HIRType *retType, std::string name, bool isVarArg,
           SourceLocation loc);

  MIRValue *getCallee() const { return callee; }
  const std::vector<MIRValue *> &getArgs() const { return args; }
  bool isVariadic() const { return isVarArg; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (callee == oldVal)
      callee = newVal;
    for (auto &arg : args) {
      if (arg == oldVal)
        arg = newVal;
    }
  }

private:
  MIRValue *callee;
  std::vector<MIRValue *> args;
  bool isVarArg;
};

// ============================================================================
// [Exceptions & Stack Unwinding]
// ============================================================================

// An InvokeInst is a function call that might throw an exception.
class InvokeInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Invoke)

  InvokeInst(MIRValue *callee, std::vector<MIRValue *> &&args,
             MIRBlock *normalDest, MIRBlock *unwindDest,
             const hir::HIRType *retType, std::string name, SourceLocation loc);

  MIRValue *getCallee() const { return callee; }
  const std::vector<MIRValue *> &getArgs() const { return args; }
  MIRBlock *getNormalDest() const { return normalDest; }
  MIRBlock *getUnwindDest() const { return unwindDest; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override;

private:
  MIRValue *callee;
  std::vector<MIRValue *> args;
  MIRBlock *normalDest;
  MIRBlock *unwindDest;
};

/// Placed at the very top of an 'unwindDest' block. Catches the exception
/// payload.
class LandingPadInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(LandingPad)

  explicit LandingPadInst(const hir::HIRType *catchType, std::string name,
                          SourceLocation loc);
  void dump(llvm::raw_ostream &os) const override;
};

/// If a catch block decides not to handle an exception, ResumeInst continues
/// the unwinding.
class ResumeInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Resume)

  explicit ResumeInst(MIRValue *exception, SourceLocation loc);
  MIRValue *getException() const { return exception; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (exception == oldVal)
      exception = newVal;
  }

private:
  MIRValue *exception;
};

/// High-level representation of throwing an exception (lowers to ABI-specific
/// __cxa_throw).
class ThrowInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Throw)

  ThrowInst(MIRValue *exception, MIRBlock *unwindDest, SourceLocation loc);

  MIRValue *getException() const { return exception; }
  MIRBlock *getUnwindDest() const { return unwindDest; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override;

private:
  MIRValue *exception;
  MIRBlock *unwindDest;
};

// ============================================================================
// [Inline Assembly]
// ============================================================================

/// Passes raw assembly strings directly to the backend.
class InlineAsmInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(InlineAsm)

  InlineAsmInst(std::string asmStr, std::string constraints,
                std::vector<MIRValue *> &&args, const hir::HIRType *retType,
                SourceLocation loc);

  const std::string &getAsmString() const { return asmString; }
  const std::string &getConstraints() const { return constraints; }
  const std::vector<MIRValue *> &getArgs() const { return args; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    for (auto &arg : args) {
      if (arg == oldVal)
        arg = newVal;
    }
  }

private:
  std::string asmString;
  std::string constraints;
  std::vector<MIRValue *> args;
};

// ============================================================================
// [Concurrency & Closures]
// ============================================================================

class MakeClosureInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(MakeClosure)

  MakeClosureInst(MIRValue *funcPtr, std::vector<MIRValue *> captures,
                  const hir::HIRType *closureType, std::string name,
                  SourceLocation loc)
      : MIRInst(Opcode::MakeClosure, closureType, std::move(name), loc),
        funcPtr(funcPtr), captures(std::move(captures)) {}

  MIRValue *getFunctionPointer() const { return funcPtr; }
  const std::vector<MIRValue *> &getCaptures() const { return captures; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (funcPtr == oldVal)
      funcPtr = newVal;
    for (auto &capture : captures) {
      if (capture == oldVal)
        capture = newVal;
    }
  }

private:
  MIRValue *funcPtr;
  std::vector<MIRValue *> captures;
};

class SpawnInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Spawn)

  SpawnInst(MIRValue *closure, hir::ThreadKind threadKind,
            const hir::HIRType *handleType, std::string name,
            SourceLocation loc)
      : MIRInst(Opcode::Spawn, handleType, std::move(name), loc),
        closure(closure), threadKind(threadKind) {}

  MIRValue *getClosure() const { return closure; }
  hir::ThreadKind getThreadKind() const { return threadKind; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (closure == oldVal)
      closure = newVal;
  }

private:
  MIRValue *closure;
  hir::ThreadKind threadKind;
};

class AwaitInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Await)

  AwaitInst(MIRValue *promise, const hir::HIRType *resolvedType,
            std::string name, SourceLocation loc)
      : MIRInst(Opcode::Await, resolvedType, std::move(name), loc),
        promise(promise) {}
  MIRValue *getPromise() const { return promise; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (promise == oldVal)
      promise = newVal;
  }

private:
  MIRValue *promise;
};

class AtomicLoadInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(AtomicLoad)

  AtomicLoadInst(MIRValue *pointer, MemoryOrder order, SourceLocation loc);
  MIRValue *getPointer() const { return pointer; }
  MemoryOrder getOrder() const { return order; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (pointer == oldVal)
      pointer = newVal;
  }

private:
  MIRValue *pointer;
  MemoryOrder order;
};

class AtomicStoreInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(AtomicStore)

  AtomicStoreInst(MIRValue *value, MIRValue *pointer, MemoryOrder order,
                  SourceLocation loc);
  MIRValue *getValue() const { return value; }
  MIRValue *getPointer() const { return pointer; }
  MemoryOrder getOrder() const { return order; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (value == oldVal)
      value = newVal;
    if (pointer == oldVal)
      pointer = newVal;
  }

private:
  MIRValue *value;
  MIRValue *pointer;
  MemoryOrder order;
};

class AtomicRMWInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(AtomicRMW)

  AtomicRMWInst(AtomicOp op, MIRValue *pointer, MIRValue *value,
                MemoryOrder order, SourceLocation loc);
  AtomicOp getAtomicOp() const { return op; }
  MIRValue *getPointer() const { return pointer; }
  MIRValue *getValue() const { return value; }
  MemoryOrder getOrder() const { return order; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (pointer == oldVal)
      pointer = newVal;
    if (value == oldVal)
      value = newVal;
  }

private:
  AtomicOp op;
  MIRValue *pointer;
  MIRValue *value;
  MemoryOrder order;
};

class AtomicCmpXchgInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(AtomicCmpXchg)

  AtomicCmpXchgInst(MIRValue *pointer, MIRValue *expected, MIRValue *desired,
                    MemoryOrder successOrder, MemoryOrder failureOrder,
                    SourceLocation loc);
  MIRValue *getPointer() const { return pointer; }
  MIRValue *getExpected() const { return expected; }
  MIRValue *getDesired() const { return desired; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (pointer == oldVal)
      pointer = newVal;
    if (expected == oldVal)
      expected = newVal;
    if (desired == oldVal)
      desired = newVal;
  }

private:
  MIRValue *pointer;
  MIRValue *expected;
  MIRValue *desired;
  MemoryOrder successOrder;
  MemoryOrder failureOrder;
};

class FenceInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Fence)

  FenceInst(MemoryOrder order, SourceLocation loc);
  MemoryOrder getOrder() const { return order; }
  void dump(llvm::raw_ostream &os) const override;

private:
  MemoryOrder order;
};

} // namespace mir
} // namespace moksha
