#pragma once

#include "moksha/HIR/HIRExpr.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRValue.h"
#include "moksha/Support/SourceLocation.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
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

/** @brief Represents a constant value in MIR. */
class MIRConstant : public MIRValue {
protected:
  MIRConstant(ValueKind k, const hir::HIRType *t) : MIRValue(k, t, "") {}

public:
  static bool classof(const MIRValue *v) {
    return v->getKind() >= ValueKind::ConstantInt &&
           v->getKind() <= ValueKind::ConstantUpcast;
    ;
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
  std::vector<MIRValue *> elements;

public:
  ConstantSlice(const hir::HIRType *t, std::vector<MIRValue *> elems)
      : MIRConstant(ValueKind::ConstantSlice, t), elements(std::move(elems)) {}

  llvm::ArrayRef<MIRValue *> getElements() const { return elements; }

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

class ConstantUnion : public MIRConstant {
public:
  ConstantUnion(const hir::HIRType *t, MIRConstant *activeField)
      : MIRConstant(ValueKind::ConstantUnion, t), activeField(activeField) {}

  MIRConstant *getActiveField() const { return activeField; }

  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantUnion;
  }

private:
  MIRConstant *activeField;
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

class ConstantUpcast : public MIRConstant {
public:
  ConstantUpcast(MIRValue *value, const hir::HIRType *destType)
      : MIRConstant(ValueKind::ConstantUpcast, destType), value(value) {}

  MIRValue *getValue() const { return value; }
  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantUpcast;
  }

private:
  MIRValue *value;
};

class ConstantAnyCast : public MIRConstant {
public:
  ConstantAnyCast(MIRValue *value, const hir::HIRType *destType)
      : MIRConstant(ValueKind::ConstantAnyCast, destType), value(value) {}

  MIRValue *getValue() const { return value; }
  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantAnyCast;
  }

private:
  MIRValue *value;
};

class ConstantArrayToSlice : public MIRConstant {
public:
  ConstantArrayToSlice(MIRValue *value, const hir::HIRType *destType)
      : MIRConstant(ValueKind::ConstantArrayToSlice, destType), value(value) {}

  MIRValue *getValue() const { return value; }
  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantArrayToSlice;
  }

private:
  MIRValue *value;
};

class ConstantSliceToArray : public MIRConstant {
public:
  ConstantSliceToArray(MIRValue *value, const hir::HIRType *destType)
      : MIRConstant(ValueKind::ConstantSliceToArray, destType), value(value) {}

  MIRValue *getValue() const { return value; }
  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::ConstantSliceToArray;
  }

private:
  MIRValue *value;
};

// Instruction Opcodes
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
  PtrToInt,
  IntToPtr,
  AnyCast,
  ArrayToSlice,
  SliceToArray,
  Upcast,
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
  MakeShared,
  Spawn,
  Await,
  AtomicLoad,
  AtomicStore,
  AtomicRMW,
  AtomicCmpXchg,
  Fence,
};

// Base Instruction
class MIRInst : public MIRValue {
public:
  Opcode getOpcode() const { return opcode; }
  SourceLocation getLoc() const { return loc; }

  MIRBlock *getParent() const { return parent; }
  void setParent(MIRBlock *bb) { parent = bb; }

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::Instruction;
  }

  virtual std::unique_ptr<MIRInst> clone() const = 0;
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

// MACRO FOR AUTO RTTI GENERATION
#define DECLARE_INST_CLASSOF(OpVal)                                            \
  static bool classof(const MIRValue *v) {                                     \
    if (v->getKind() != ValueKind::Instruction)                                \
      return false;                                                            \
    return static_cast<const MIRInst *>(v)->getOpcode() == Opcode::OpVal;      \
  }

// Terminators
class BranchInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Br)

  explicit BranchInst(MIRBlock *target, SourceLocation loc);
  MIRBlock *getTarget() const { return target; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override;
  void setTarget(MIRBlock *b) { target = b; }
  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<BranchInst>(target, loc);
  }

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
  void setTrueBlock(MIRBlock *b) { trueBlock = b; }
  void setFalseBlock(MIRBlock *b) { falseBlock = b; }
  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<CondBranchInst>(cond, trueBlock, falseBlock, loc);
  }

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
  void setDefaultBlock(MIRBlock *b) { defaultBlock = b; }
  std::vector<std::pair<MIRValue *, MIRBlock *>> &getCasesMut() {
    return cases;
  }
  std::unique_ptr<MIRInst> clone() const override {
    auto cloned = std::make_unique<SwitchInst>(cond, defaultBlock, loc);
    for (const auto &c : cases)
      cloned->addCase(c.first, c.second);
    return cloned;
  }

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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<ReturnInst>(val, loc);
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<UnreachableInst>(loc);
  }
};

// Memory & Aggregates
class AllocaInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Alloca)

  AllocaInst(const hir::HIRType *ptrType, const hir::HIRType *allocType,
             std::string name, SourceLocation loc, unsigned align);
  const hir::HIRType *getAllocatedType() const { return allocatedType; }
  unsigned getAlignment() const { return alignment; }
  void dump(llvm::raw_ostream &os) const override;

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<AllocaInst>(getType(), allocatedType, getName(),
                                        loc, alignment);
  }

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

  std::unique_ptr<MIRInst> clone() const override {
    auto cloned = std::make_unique<LoadInst>(ptr, getName(), loc, alignment);
    cloned->setVolatile(isVolatileFlag);
    return cloned;
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

  std::unique_ptr<MIRInst> clone() const override {
    auto cloned = std::make_unique<StoreInst>(val, ptr, loc, alignment);
    cloned->setVolatile(isVolatileFlag);
    return cloned;
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
  void setPointer(MIRValue *newPtr) { ptr = newPtr; }
  void setIndices(std::vector<MIRValue *> newIdx) {
    indices = std::move(newIdx);
  }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (ptr == oldVal)
      ptr = newVal;
    for (auto &idx : indices) {
      if (idx == oldVal)
        idx = newVal;
    }
  }

  std::unique_ptr<MIRInst> clone() const override {
    std::vector<MIRValue *> idxs = indices;
    return std::make_unique<GetElementPtrInst>(
        ptr, std::move(idxs), ptr->getType(), getType(), getName(), loc);
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<InsertValueInst>(agg, val, index, getName(), loc);
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<ExtractValueInst>(agg, index, getType(), getName(),
                                              loc);
  }

private:
  MIRValue *agg;
  uint32_t index;
};

// ARC, Arithmetic, Logic, Casts
class ARCInst : public MIRInst {
public:
  static bool classof(const MIRValue *v) {
    if (v->getKind() != ValueKind::Instruction)
      return false;
    auto op = static_cast<const MIRInst *>(v)->getOpcode();
    return op == Opcode::Retain || op == Opcode::Release;
  }

  ARCInst(Opcode op, MIRValue *obj, MIRFunction *dropFunc = nullptr,
          SourceLocation loc = {});

  Opcode getOpcode() const { return opcode; }
  MIRValue *getObject() const { return object; }

  MIRFunction *getDropFunc() const { return dropFunc; }

  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (object == oldVal)
      object = newVal;
  }

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<ARCInst>(opcode, object, dropFunc, loc);
  }

private:
  Opcode opcode;
  MIRValue *object;
  MIRFunction *dropFunc;
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<StoreWeakInst>(val, ptr, loc);
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<LoadWeakInst>(ptr, getType(), getName(), loc);
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<BinaryInst>(getOpcode(), lhs, rhs, getName(), loc);
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
    return op == Opcode::BitCast || op == Opcode::IntToFloat ||
           op == Opcode::FloatToInt || op == Opcode::Trunc ||
           op == Opcode::SExt || op == Opcode::ZExt || op == Opcode::PtrToInt ||
           op == Opcode::IntToPtr || op == Opcode::AnyCast ||
           op == Opcode::ArrayToSlice || op == Opcode::SliceToArray ||
           op == Opcode::Upcast;
  }
  CastInst(Opcode op, MIRValue *value, const hir::HIRType *destType,
           std::string name, SourceLocation loc);
  MIRValue *getValue() const { return value; }
  int32_t getByteOffset() const { return byteOffset; }
  void setByteOffset(int32_t offset) { byteOffset = offset; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (value == oldVal)
      value = newVal;
  }

  std::unique_ptr<MIRInst> clone() const override {
    auto cloned = std::make_unique<CastInst>(getOpcode(), value, getType(),
                                             getName(), loc);
    cloned->setByteOffset(byteOffset);
    return cloned;
  }

private:
  MIRValue *value;
  int32_t byteOffset = 0;
};

class CompareInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(ICmp)

  enum class Predicate { EQ, NE, LT, LE, GT, GE, ULT, ULE, UGT, UGE };
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<CompareInst>(pred, lhs, rhs, getType(), getName(),
                                         loc);
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
    OEQ,
    ONE,
    OLT,
    OLE,
    OGT,
    OGE,
    UEQ,
    UNE,
    ULT,
    ULE,
    UGT,
    UGE
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<FCmpInst>(pred, lhs, rhs, getType(), getName(),
                                      loc);
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
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override;

  std::unique_ptr<MIRInst> clone() const override {
    auto cloned = std::make_unique<PhiInst>(getType(), getName(), loc);
    for (const auto &inc : incoming)
      cloned->addIncoming(inc.first, inc.second);
    return cloned;
  }

private:
  std::vector<std::pair<MIRValue *, MIRBlock *>> incoming;
};

class CallInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Call)

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

  std::unique_ptr<MIRInst> clone() const override {
    std::vector<MIRValue *> argsCopy = args;
    return std::make_unique<CallInst>(callee, std::move(argsCopy), getType(),
                                      getName(), isVarArg, loc);
  }

private:
  MIRValue *callee;
  std::vector<MIRValue *> args;
  bool isVarArg;
};

// Exceptions & Stack Unwinding
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
  void setNormalDest(MIRBlock *b) { normalDest = b; }
  void setUnwindDest(MIRBlock *b) { unwindDest = b; }
  std::unique_ptr<MIRInst> clone() const override {
    std::vector<MIRValue *> argsCopy = args;
    return std::make_unique<InvokeInst>(callee, std::move(argsCopy), normalDest,
                                        unwindDest, getType(), getName(), loc);
  }

private:
  MIRValue *callee;
  std::vector<MIRValue *> args;
  MIRBlock *normalDest;
  MIRBlock *unwindDest;
};

class LandingPadInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(LandingPad)
  explicit LandingPadInst(const hir::HIRType *resultType, std::string name,
                          SourceLocation loc);
  void addCatchType(const hir::HIRType *catchType) {
    catchTypes.push_back(catchType);
  }

  const std::vector<const hir::HIRType *> &getCatchTypes() const {
    return catchTypes;
  }

  void dump(llvm::raw_ostream &os) const override;

  std::unique_ptr<MIRInst> clone() const override {
    auto cloned = std::make_unique<LandingPadInst>(getType(), getName(), loc);
    for (const auto *ct : catchTypes) {
      cloned->addCatchType(ct);
    }
    return cloned;
  }

private:
  std::vector<const hir::HIRType *> catchTypes;
};

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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<ResumeInst>(exception, loc);
  }

private:
  MIRValue *exception;
};

class ThrowInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(Throw)

  ThrowInst(MIRValue *exception, MIRBlock *unwindDest, SourceLocation loc);

  MIRValue *getException() const { return exception; }
  MIRBlock *getUnwindDest() const { return unwindDest; }
  void setUnwindDest(MIRBlock *newDest) { unwindDest = newDest; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override;
  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<ThrowInst>(exception, unwindDest, loc);
  }

private:
  MIRValue *exception;
  MIRBlock *unwindDest;
};

// Inline Assembly
class InlineAsmInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(InlineAsm)

  InlineAsmInst(std::string asmStr, std::string constraints,
                std::vector<MIRValue *> &&args, bool isVolatile,
                const hir::HIRType *retType, SourceLocation loc);

  const std::string &getAsmString() const { return asmString; }
  const std::string &getConstraints() const { return constraints; }
  const std::vector<MIRValue *> &getArgs() const { return args; }
  bool getIsVolatile() const { return isVolatile; }

  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    for (auto &arg : args) {
      if (arg == oldVal)
        arg = newVal;
    }
  }

  std::unique_ptr<MIRInst> clone() const override {
    std::vector<MIRValue *> argsCopy = args;
    return std::make_unique<InlineAsmInst>(asmString, constraints,
                                           std::move(argsCopy), isVolatile,
                                           getType(), loc);
  }

private:
  std::string asmString;
  std::string constraints;
  std::vector<MIRValue *> args;
  bool isVolatile;
};

// Concurrency & Closures
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<MakeClosureInst>(funcPtr, captures, getType(),
                                             getName(), loc);
  }

private:
  MIRValue *funcPtr;
  std::vector<MIRValue *> captures;
};

class MakeSharedInst : public MIRInst {
public:
  DECLARE_INST_CLASSOF(MakeShared)

  MakeSharedInst(MIRValue *operand, const hir::HIRType *allocatedType,
                 SourceLocation loc)
      : MIRInst(Opcode::MakeShared, allocatedType, "", loc), operand(operand) {}

  MIRValue *getOperand() const { return operand; }

  void dump(llvm::raw_ostream &os) const override;

  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (operand == oldVal)
      operand = newVal;
  }

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<MakeSharedInst>(operand, getType(), loc);
  }

private:
  MIRValue *operand;
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<SpawnInst>(closure, threadKind, getType(),
                                       getName(), loc);
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<AwaitInst>(promise, getType(), getName(), loc);
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<AtomicLoadInst>(pointer, order, loc);
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<AtomicStoreInst>(value, pointer, order, loc);
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<AtomicRMWInst>(op, pointer, value, order, loc);
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
  MemoryOrder getSuccessOrder() const { return successOrder; }
  MemoryOrder getFailureOrder() const { return failureOrder; }
  void dump(llvm::raw_ostream &os) const override;
  void replaceOperand(MIRValue *oldVal, MIRValue *newVal) override {
    if (pointer == oldVal)
      pointer = newVal;
    if (expected == oldVal)
      expected = newVal;
    if (desired == oldVal)
      desired = newVal;
  }

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<AtomicCmpXchgInst>(pointer, expected, desired,
                                               successOrder, failureOrder, loc);
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

  std::unique_ptr<MIRInst> clone() const override {
    return std::make_unique<FenceInst>(order, loc);
  }

private:
  MemoryOrder order;
};

} // namespace mir
} // namespace moksha
