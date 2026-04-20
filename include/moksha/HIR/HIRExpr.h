#pragma once

#include "moksha/AST/Expr.h"
#include "moksha/HIR/HIRParam.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/Support/SourceLocation.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace moksha {
namespace hir {

class HIRVisitor;
class ConstHIRVisitor;
class HIRStmt;

// ============================================================================
// [Enums]
// ============================================================================

enum class BinaryOp {
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Pow,
  Assign,
  Equal,
  NotEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  And,
  Or,
  BitAnd,
  BitOr,
  BitXor,
  Shl,
  Shr,
  NullCoalesce,
  Range
};

enum class UnaryOp { Neg, Not, BitNot, PreInc, PreDec, PostInc, PostDec };

enum class CastOp {
  BitCast,
  IntToFloat,
  FloatToInt,
  Truncate,      // Integer narrowing (e.g., long -> int)
  SignExtend,    // Signed Integer widening (e.g., short -> int)
  ZeroExtend,    // Unsigned Integer widening (e.g., unsigned short -> unsigned
                 // int)
  FloatExtend,   // Float widening (e.g., quarter -> half -> float -> double)
  FloatTruncate, // Float narrowing (e.g., double -> float)
  PointerCast,
  AnyCast
};

// [NEW] robust ThreadKind
enum class ThreadKind {
  Strong,  // Standard joinable thread
  Weak,    // Daemon/Background thread
  Detached // Fire-and-forget
};

enum class ValueCategory {
  LValue, // Represents a memory location (e.g., variables, dereferences)
  RValue  // Represents a temporary value (e.g., literals, arithmetic results)
};

// Define the explicit capture modes for HIR
enum class CaptureMode {
  Snapshot, // Default: by-value copy ()
  View,     // Immutable borrow &()
  Mut,      // Mutable borrow &mut ()
  Move      // Ownership transfer move ()
};

// ============================================================================
// [HIR Expression Base]
// ============================================================================

class HIRExpr {
public:
  enum class Kind {
    IntegerLiteral,
    FloatLiteral,
    DecimalLiteral,
    BoolLiteral,
    StringLiteral,
    TemplateString,
    NullLiteral,
    ArrayLiteral,
    Spread,
    MapLiteral,
    Identifier,
    MemberAccess,
    Index,
    This,
    Binary,
    Unary,
    Cast,
    Ternary,
    Call,
    New,
    Lambda,
    Thread,
    SizeOf,
    Await,
    Super,
    Deref,
    AddressOf,
    Member,
    Input
  };

  virtual ~HIRExpr() = default;

  [[nodiscard]] Kind getKind() const { return kind; }
  [[nodiscard]] const HIRType *getType() const { return type; }
  [[nodiscard]] SourceLocation getLoc() const { return loc; }
  [[nodiscard]] ValueCategory getValueCategory() const { return valCategory; }
  [[nodiscard]] bool isLValue() const {
    return valCategory == ValueCategory::LValue;
  }

  // Support for mutable AND const visitors
  virtual void accept(HIRVisitor &v) = 0;
  virtual void accept(ConstHIRVisitor &v) const = 0;
  virtual void dump(llvm::raw_ostream &os, int indent = 0) const = 0;

protected:
  HIRExpr(Kind kind, const HIRType *type, ValueCategory vc, SourceLocation loc)
      : kind(kind), type(type), valCategory(vc), loc(loc) {}

  Kind kind;
  const HIRType *type;
  ValueCategory valCategory;
  SourceLocation loc;
};

using HIRExprPtr = std::unique_ptr<HIRExpr>;

// ============================================================================
// [Literals]
// ============================================================================

class HIRIntegerLiteral : public HIRExpr {
public:
  HIRIntegerLiteral(uint64_t value, const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::IntegerLiteral, type, ValueCategory::RValue, loc),
        value(value) {}

  [[nodiscard]] uint64_t getValue() const { return value; }
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override; // [NEW]
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) {
    return E->getKind() == Kind::IntegerLiteral;
  }

private:
  uint64_t value;
};

class HIRFloatLiteral : public HIRExpr {
public:
  HIRFloatLiteral(double value, const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::FloatLiteral, type, ValueCategory::RValue, loc),
        value(value) {}

  [[nodiscard]] double getValue() const { return value; }
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) {
    return E->getKind() == Kind::FloatLiteral;
  }

private:
  double value;
};

class HIRDecimalLiteral : public HIRExpr {
public:
  HIRDecimalLiteral(std::string value, const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::DecimalLiteral, type, ValueCategory::RValue, loc),
        value(std::move(value)) {}

  const std::string &getValue() const { return value; }

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;

  static bool classof(const HIRExpr *E) {
    return E->getKind() == Kind::DecimalLiteral;
  }

private:
  std::string value;
};

class HIRBoolLiteral : public HIRExpr {
public:
  HIRBoolLiteral(bool value, const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::BoolLiteral, type, ValueCategory::RValue, loc),
        value(value) {}

  [[nodiscard]] bool getValue() const { return value; }
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) {
    return E->getKind() == Kind::BoolLiteral;
  }

private:
  bool value;
};

class HIRStringLiteral : public HIRExpr {
public:
  HIRStringLiteral(std::string value, const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::StringLiteral, type, ValueCategory::RValue, loc),
        value(std::move(value)) {}

  [[nodiscard]] const std::string &getValue() const { return value; }
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) {
    return E->getKind() == Kind::StringLiteral;
  }

private:
  std::string value;
};

class HIRTemplateStringExpr : public HIRExpr {
public:
  HIRTemplateStringExpr(std::vector<std::unique_ptr<HIRExpr>> parts,
                        const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::TemplateString, type, ValueCategory::RValue, loc),
        parts(std::move(parts)) {}

  [[nodiscard]] const std::vector<std::unique_ptr<HIRExpr>> &getParts() const {
    return parts;
  }

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) {
    return E->getKind() == Kind::TemplateString;
  }

private:
  std::vector<std::unique_ptr<HIRExpr>> parts;
};

class HIRNullLiteral : public HIRExpr {
public:
  HIRNullLiteral(const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::NullLiteral, type, ValueCategory::RValue, loc) {}

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) {
    return E->getKind() == Kind::NullLiteral;
  }
};

class HIRArrayLiteral : public HIRExpr {
public:
  HIRArrayLiteral(std::vector<HIRExprPtr> elements, const HIRType *type,
                  SourceLocation loc)
      : HIRExpr(Kind::ArrayLiteral, type, ValueCategory::RValue, loc),
        elements(std::move(elements)) {}

  [[nodiscard]] const std::vector<HIRExprPtr> &getElements() const {
    return elements;
  }
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) {
    return E->getKind() == Kind::ArrayLiteral;
  }

private:
  std::vector<HIRExprPtr> elements;
};

class HIRMapLiteral : public HIRExpr {
public:
  using Entry = std::pair<std::unique_ptr<HIRExpr>, std::unique_ptr<HIRExpr>>;

  HIRMapLiteral(std::vector<Entry> entries, const HIRType *type,
                SourceLocation loc)
      : HIRExpr(Kind::MapLiteral, type, ValueCategory::RValue, loc),
        entries(std::move(entries)) {}

  [[nodiscard]] const std::vector<Entry> &getEntries() const { return entries; }

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) {
    return E->getKind() == Kind::MapLiteral;
  }

private:
  std::vector<Entry> entries;
};

// ============================================================================
// [Operations]
// ============================================================================

class HIRBinaryExpr : public HIRExpr {
public:
  HIRBinaryExpr(BinaryOp op, HIRExprPtr lhs, HIRExprPtr rhs,
                const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::Binary, type, ValueCategory::RValue, loc),
        lhs(std::move(lhs)), op(op), rhs(std::move(rhs)) {}

  [[nodiscard]] HIRExpr *getLHS() const { return lhs.get(); }
  [[nodiscard]] HIRExpr *getRHS() const { return rhs.get(); }
  [[nodiscard]] BinaryOp getOp() const { return op; }

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::Binary; }

private:
  HIRExprPtr lhs;
  BinaryOp op;
  HIRExprPtr rhs;
};

class HIRUnaryExpr : public HIRExpr {
public:
  HIRUnaryExpr(UnaryOp op, HIRExprPtr operand, const HIRType *type,
               SourceLocation loc)
      : HIRExpr(Kind::Unary, type, ValueCategory::RValue, loc), op(op),
        operand(std::move(operand)) {}

  [[nodiscard]] HIRExpr *getOperand() const { return operand.get(); }
  [[nodiscard]] UnaryOp getOp() const { return op; }

  [[nodiscard]] bool isPostfix() const {
    return op == UnaryOp::PostInc || op == UnaryOp::PostDec;
  }

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::Unary; }

private:
  UnaryOp op;
  HIRExprPtr operand;
};

class HIRCastExpr : public HIRExpr {
public:
  HIRCastExpr(CastOp op, HIRExprPtr operand, const HIRType *targetType,
              SourceLocation loc)
      : HIRExpr(Kind::Cast, targetType, ValueCategory::RValue, loc),
        expr(std::move(operand)), op(op) {} // Match order: expr then op

  [[nodiscard]] HIRExpr *getExpr() const { return expr.get(); }
  [[nodiscard]] CastOp getOp() const { return op; }

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::Cast; }

private:
  HIRExprPtr expr; // Declare expr first
  CastOp op;
};

class HIRTernaryExpr : public HIRExpr {
public:
  HIRTernaryExpr(HIRExprPtr cond, HIRExprPtr trueExpr, HIRExprPtr falseExpr,
                 const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::Ternary, type, ValueCategory::RValue, loc),
        cond(std::move(cond)), trueExpr(std::move(trueExpr)),
        falseExpr(std::move(falseExpr)) {}

  const HIRExpr *getCond() const { return cond.get(); }
  const HIRExpr *getTrueExpr() const { return trueExpr.get(); }
  const HIRExpr *getFalseExpr() const { return falseExpr.get(); }
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) {
    return E->getKind() == Kind::Ternary;
  }

private:
  HIRExprPtr cond;
  HIRExprPtr trueExpr;
  HIRExprPtr falseExpr;
};

// ============================================================================
// [Variables & Access]
// ============================================================================

class HIRIdentifierExpr : public HIRExpr {
public:
  HIRIdentifierExpr(std::string name, const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::Identifier, type, ValueCategory::LValue, loc),
        name(std::move(name)) {}

  [[nodiscard]] const std::string &getName() const { return name; }
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) {
    return E->getKind() == Kind::Identifier;
  }

private:
  std::string name;
};

class HIRMemberExpr : public HIRExpr {
public:
  HIRMemberExpr(
      std::unique_ptr<HIRExpr> object, std::string member,
      std::unordered_map<std::string, const HIRType *> genericBindings,
      const HIRType *type, SourceLocation loc, FieldInfo info = FieldInfo())
      : HIRExpr(Kind::Member, type, ValueCategory::LValue, loc),
        object(std::move(object)), member(std::move(member)),
        info(std::move(info)), genericBindings(std::move(genericBindings)) {}

  HIRMemberExpr(std::unique_ptr<HIRExpr> object, std::string member,
                const HIRType *type, SourceLocation loc,
                FieldInfo info = FieldInfo())
      : HIRExpr(Kind::Member, type, ValueCategory::LValue, loc),
        object(std::move(object)), member(std::move(member)),
        info(std::move(info)) {}

  [[nodiscard]] HIRExpr *getObject() const { return object.get(); }
  [[nodiscard]] const std::string &getMemberName() const { return member; }
  [[nodiscard]] const FieldInfo &getMemberInfo() const { return info; }
  const std::unordered_map<std::string, const HIRType *> &
  getGenericBindings() const {
    return genericBindings;
  }
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::Member; }

  bool isVirtualMethod() const { return virtualMethod; }
  uint32_t getVTableIndex() const { return vtableIndex; }

  void setVirtualMethodInfo(bool isVirtual, uint32_t vtableIdx) {
    virtualMethod = isVirtual;
    vtableIndex = vtableIdx;
  }

private:
  HIRExprPtr object;
  std::string member;
  FieldInfo info;
  bool virtualMethod = false;
  uint32_t vtableIndex = 0;
  std::unordered_map<std::string, const HIRType *> genericBindings;
};

class HIRIndexExpr : public HIRExpr {
public:
  HIRIndexExpr(std::unique_ptr<HIRExpr> base, std::unique_ptr<HIRExpr> index,
               bool isOptional, const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::Index, type, ValueCategory::LValue, loc),
        base(std::move(base)), index(std::move(index)), isOptional(isOptional) {
  }

  [[nodiscard]] HIRExpr *getBase() const { return base.get(); }
  [[nodiscard]] HIRExpr *getIndex() const { return index.get(); }
  [[nodiscard]] bool isOptionalAccess() const { return isOptional; }

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::Index; }

private:
  HIRExprPtr base;
  HIRExprPtr index;
  bool isOptional;
};

class HIRThisExpr : public HIRExpr {
public:
  HIRThisExpr(const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::This, type, ValueCategory::RValue, loc) {}

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::This; }
};

class HIRSuperExpr : public HIRExpr {
public:
  // SuperExpr is usually a pointer to the parent class type
  HIRSuperExpr(const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::Super, type, ValueCategory::RValue, loc) {}

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::Super; }
};

// ============================================================================
// [High-Level Constructs]
// ============================================================================

class HIRCallExpr : public HIRExpr {
public:
  HIRCallExpr(HIRExprPtr callee, std::vector<HIRExprPtr> args,
              std::unordered_map<std::string, const HIRType *> genericBindings,
              const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::Call, type, ValueCategory::RValue, loc),
        callee(std::move(callee)), args(std::move(args)) {}

  HIRCallExpr(HIRExprPtr callee, std::vector<HIRExprPtr> args,
              const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::Call, type, ValueCategory::RValue, loc),
        callee(std::move(callee)), args(std::move(args)) {}

  [[nodiscard]] HIRExpr *getCallee() const { return callee.get(); }
  [[nodiscard]] const std::vector<HIRExprPtr> &getArgs() const { return args; }
  const std::unordered_map<std::string, const HIRType *> &
  getGenericBindings() const {
    return genericBindings;
  }
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::Call; }

private:
  HIRExprPtr callee;
  std::vector<HIRExprPtr> args;
  std::unordered_map<std::string, const HIRType *> genericBindings;
};

class HIRNewExpr : public HIRExpr {
public:
  HIRNewExpr(const HIRType *allocatedType, std::vector<HIRExprPtr> args,
             std::unordered_map<std::string, const HIRType *> genericBindings,
             const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::New, type, ValueCategory::RValue, loc),
        allocatedType(allocatedType), args(std::move(args)),
        genericBindings(std::move(genericBindings)) {}

  HIRNewExpr(const HIRType *allocatedType, std::vector<HIRExprPtr> args,
             const HIRType *type, SourceLocation loc)
      : HIRExpr(Kind::New, type, ValueCategory::RValue, loc),
        allocatedType(allocatedType), args(std::move(args)) {}

  [[nodiscard]] const std::vector<HIRExprPtr> &getArgs() const { return args; }
  [[nodiscard]] const HIRType *getAllocatedType() const {
    return allocatedType;
  }
  const std::unordered_map<std::string, const HIRType *> &
  getGenericBindings() const {
    return genericBindings;
  }
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::New; }

private:
  const HIRType *allocatedType;
  std::vector<HIRExprPtr> args;
  std::unordered_map<std::string, const HIRType *> genericBindings;
};

// ============================================================================
// [Lambda & Closure Implementation]
// ============================================================================

// [NEW] Define how a variable is captured by the closure
enum class CaptureKind {
  ByValue,    // Deep copy or Arc<T> clone (Required for Threads!)
  ByReference // Weak pointer / raw reference
};

// [NEW] Struct to hold capture metadata
struct HIRCapture {
  std::string name;
  const HIRType *type;
  CaptureKind kind;

  HIRCapture(std::string n, const HIRType *t, CaptureKind k)
      : name(std::move(n)), type(t), kind(k) {}
};

struct HIRLambdaParam {
  std::string name;
  const HIRType *type;
  std::unique_ptr<HIRExpr> defaultValue;

  HIRLambdaParam(std::string n, const HIRType *t,
                 std::unique_ptr<HIRExpr> defVal = nullptr)
      : name(std::move(n)), type(t), defaultValue(std::move(defVal)) {}

  const std::string &getName() const { return name; }
  const HIRType *getType() const { return type; }
  const HIRExpr *getDefaultValue() const { return defaultValue.get(); }
};

class HIRLambdaExpr : public HIRExpr {
public:
  HIRLambdaExpr(std::vector<HIRLambdaParam> params,
                std::vector<HIRCapture> captures, std::unique_ptr<HIRStmt> body,
                const HIRType *type, CaptureMode mode, SourceLocation loc);

  ~HIRLambdaExpr() override;

  const std::vector<HIRLambdaParam> &getParams() const { return params; }
  const std::vector<HIRCapture> &getCaptures() const { return captures; }
  const HIRStmt *getBody() const;
  CaptureMode getCaptureMode() const { return captureMode; }
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;

  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::Lambda; }

private:
  std::vector<HIRLambdaParam> params;
  std::vector<HIRCapture> captures;
  std::unique_ptr<HIRStmt> body;
  CaptureMode captureMode;
};

class HIRThreadExpr : public HIRExpr {
public:
  HIRThreadExpr(HIRExprPtr task, ThreadKind kind, const HIRType *type,
                SourceLocation loc)
      : HIRExpr(Kind::Thread, type, ValueCategory::RValue, loc),
        task(std::move(task)), kind(kind) {}

  [[nodiscard]] HIRExpr *getTask() const { return task.get(); }
  [[nodiscard]] ThreadKind getThreadKind() const { return kind; }
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::Thread; }

private:
  HIRExprPtr task;
  ThreadKind kind;
};

class HIRSizeOfExpr : public HIRExpr {
public:
  HIRSizeOfExpr(const HIRType *targetType, const HIRType *usizeType,
                SourceLocation loc)
      : HIRExpr(Kind::SizeOf, usizeType, ValueCategory::RValue, loc),
        targetType(targetType) {}

  const HIRType *getTargetType() const { return targetType; }

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;

  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::SizeOf; }

private:
  const HIRType *targetType;
};

class HIRAwaitExpr : public HIRExpr {
public:
  HIRAwaitExpr(std::unique_ptr<HIRExpr> expr, const HIRType *type,
               SourceLocation loc)
      : HIRExpr(Kind::Await, type, ValueCategory::RValue, loc),
        expression(std::move(expr)) {}

  [[nodiscard]] const HIRExpr *getExpr() const { return expression.get(); }

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::Await; }

private:
  std::unique_ptr<HIRExpr> expression;
};

// ============================================================================
// [Pointer Operations]
// ============================================================================

class HIRDerefExpr : public HIRExpr {
public:
  // Dereferencing a pointer gives you a memory location (L-Value!)
  HIRDerefExpr(std::unique_ptr<HIRExpr> pointerExpr, const HIRType *derefedType,
               SourceLocation loc)
      : HIRExpr(Kind::Deref, derefedType, ValueCategory::LValue, loc),
        pointer(std::move(pointerExpr)) {}

  [[nodiscard]] const HIRExpr *getPointer() const { return pointer.get(); }

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::Deref; }

private:
  std::unique_ptr<HIRExpr> pointer;
};

class HIRAddressOfExpr : public HIRExpr {
public:
  HIRAddressOfExpr(std::unique_ptr<HIRExpr> operandExpr, const HIRType *ptrType,
                   bool isMutBorrow, SourceLocation loc)
      : HIRExpr(Kind::AddressOf, ptrType, ValueCategory::RValue, loc),
        operand(std::move(operandExpr)), isMutBorrow(isMutBorrow) {}

  [[nodiscard]] const HIRExpr *getOperand() const { return operand.get(); }
  bool isMutableBorrow() const { return isMutBorrow; }
  void setMutableBorrow(bool mut) { isMutBorrow = mut; }
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRExpr *E) {
    return E->getKind() == Kind::AddressOf;
  }

private:
  std::unique_ptr<HIRExpr> operand;
  bool isMutBorrow;
};

/// Represents the spread operator (...) in array or map literals.
class HIRSpreadExpr : public HIRExpr {
public:
  HIRSpreadExpr(std::unique_ptr<HIRExpr> iterable, const HIRType *type,
                SourceLocation loc)
      : HIRExpr(Kind::Spread, type, ValueCategory::RValue, loc),
        iterable(std::move(iterable)) {}

  [[nodiscard]] const HIRExpr *getIterable() const { return iterable.get(); }

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;

  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::Spread; }

private:
  std::unique_ptr<HIRExpr> iterable;
};

class HIRInputExpr : public HIRExpr {
public:
  // prompt can be null if it's just `input()` without arguments
  HIRInputExpr(std::unique_ptr<HIRExpr> prompt, const HIRType *type,
               SourceLocation loc)
      : HIRExpr(Kind::Input, type, ValueCategory::RValue, loc),
        prompt(std::move(prompt)) {}

  [[nodiscard]] const HIRExpr *getPrompt() const { return prompt.get(); }

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;

  static bool classof(const HIRExpr *E) { return E->getKind() == Kind::Input; }

private:
  std::unique_ptr<HIRExpr> prompt;
};

} // namespace hir
} // namespace moksha
