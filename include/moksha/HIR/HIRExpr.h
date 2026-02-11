#pragma once

#include "moksha/HIR/HIRType.h"
#include "moksha/Support/SourceLocation.h"

#include <memory>
#include <vector>
#include <string>
#include <iosfwd>

namespace moksha {
namespace hir {

class HIRVisitor;
class ConstHIRVisitor; // [NEW] Forward declaration
class HIRStmt;

// ============================================================================
// [Enums]
// ============================================================================

enum class BinaryOp {
    Add, Sub, Mul, Div, Mod, Pow,
    Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual,
    And, Or,
    BitAnd, BitOr, BitXor, Shl, Shr
};

enum class UnaryOp {
    Neg, Not, BitNot,
    PreInc, PreDec, PostInc, PostDec
};

enum class CastOp {
    BitCast, IntToFloat, FloatToInt, Truncate,
    SignExtend, ZeroExtend, PointerCast, Dynamic
};

// [NEW] robust ThreadKind
enum class ThreadKind {
    Strong, // Standard joinable thread
    Weak,   // Daemon/Background thread
    Detached // Fire-and-forget
};

// ============================================================================
// [HIR Expression Base]
// ============================================================================

class HIRExpr {
public:
    enum class Kind {
        IntegerLiteral, FloatLiteral, BoolLiteral, StringLiteral, NullLiteral,
        ArrayLiteral,
        Identifier, MemberAccess, Index, This,
        Binary, Unary, Cast, Ternary,
        Call, New, Lambda, Thread
    };

    virtual ~HIRExpr() = default;

    [[nodiscard]] Kind getKind() const { return kind; }
    [[nodiscard]] const HIRType *getType() const { return type; }
    [[nodiscard]] SourceLocation getLoc() const { return loc; }

    // [FIX] Support for mutable AND const visitors
    virtual void accept(HIRVisitor &v) = 0;
    virtual void accept(ConstHIRVisitor &v) const = 0;

protected:
    HIRExpr(Kind k, const HIRType *type, SourceLocation loc)
        : kind(k), type(type), loc(loc) {}

    Kind kind;
    const HIRType *type;
    SourceLocation loc;
};

using HIRExprPtr = std::unique_ptr<HIRExpr>;

// ============================================================================
// [Literals]
// ============================================================================

class HIRIntegerLiteral : public HIRExpr {
public:
    HIRIntegerLiteral(uint64_t val, const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::IntegerLiteral, type, loc), value(val) {}

    [[nodiscard]] uint64_t getValue() const { return value; }
    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override; // [NEW]

private:
    uint64_t value;
};

class HIRFloatLiteral : public HIRExpr {
public:
    HIRFloatLiteral(double val, const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::FloatLiteral, type, loc), value(val) {}

    [[nodiscard]] double getValue() const { return value; }
    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

private:
    double value;
};

class HIRBoolLiteral : public HIRExpr {
public:
    HIRBoolLiteral(bool val, const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::BoolLiteral, type, loc), value(val) {}

    [[nodiscard]] bool getValue() const { return value; }
    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

private:
    bool value;
};

class HIRStringLiteral : public HIRExpr {
public:
    HIRStringLiteral(std::string val, const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::StringLiteral, type, loc), value(std::move(val)) {}

    [[nodiscard]] const std::string &getValue() const { return value; }
    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

private:
    std::string value;
};

class HIRNullLiteral : public HIRExpr {
public:
    HIRNullLiteral(const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::NullLiteral, type, loc) {}

    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;
};

class HIRArrayLiteral : public HIRExpr {
public:
    HIRArrayLiteral(std::vector<HIRExprPtr> elements, const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::ArrayLiteral, type, loc), elements(std::move(elements)) {}

    [[nodiscard]] const std::vector<HIRExprPtr> &getElements() const { return elements; }
    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

private:
    std::vector<HIRExprPtr> elements;
};

// ============================================================================
// [Operations]
// ============================================================================

class HIRBinaryExpr : public HIRExpr {
public:
    HIRBinaryExpr(HIRExprPtr lhs, BinaryOp op, HIRExprPtr rhs, const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::Binary, type, loc), lhs(std::move(lhs)), op(op), rhs(std::move(rhs)) {}

    [[nodiscard]] HIRExpr *getLHS() const { return lhs.get(); }
    [[nodiscard]] HIRExpr *getRHS() const { return rhs.get(); }
    [[nodiscard]] BinaryOp getOp() const { return op; }

    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

private:
    HIRExprPtr lhs;
    BinaryOp op;
    HIRExprPtr rhs;
};

class HIRUnaryExpr : public HIRExpr {
public:
    HIRUnaryExpr(UnaryOp op, HIRExprPtr operand, const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::Unary, type, loc), op(op), operand(std::move(operand)) {}

    [[nodiscard]] HIRExpr *getOperand() const { return operand.get(); }
    [[nodiscard]] UnaryOp getOp() const { return op; }

    [[nodiscard]] bool isPostfix() const {
        return op == UnaryOp::PostInc || op == UnaryOp::PostDec;
    }

    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

private:
    UnaryOp op;
    HIRExprPtr operand;
};

class HIRCastExpr : public HIRExpr {
public:
    HIRCastExpr(HIRExprPtr expr, const HIRType *targetType, CastOp op, SourceLocation loc)
        : HIRExpr(Kind::Cast, targetType, loc), expr(std::move(expr)), op(op) {}

    [[nodiscard]] HIRExpr *getExpr() const { return expr.get(); }
    [[nodiscard]] CastOp getOp() const { return op; }

    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

private:
    HIRExprPtr expr;
    CastOp op;
};

class HIRTernaryExpr : public HIRExpr {
public:
    HIRTernaryExpr(HIRExprPtr cond, HIRExprPtr trueExpr, HIRExprPtr falseExpr, const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::Ternary, type, loc), cond(std::move(cond)),
          trueExpr(std::move(trueExpr)), falseExpr(std::move(falseExpr)) {}

    [[nodiscard]] HIRExpr *getCond() const { return cond.get(); }
    [[nodiscard]] HIRExpr *getTrueExpr() const { return trueExpr.get(); }
    [[nodiscard]] HIRExpr *getFalseExpr() const { return falseExpr.get(); }

    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

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
        : HIRExpr(Kind::Identifier, type, loc), name(std::move(name)) {}

    [[nodiscard]] const std::string &getName() const { return name; }
    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

private:
    std::string name;
};

class HIRMemberExpr : public HIRExpr {
public:
    HIRMemberExpr(HIRExprPtr object, std::string member, const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::MemberAccess, type, loc), object(std::move(object)), member(std::move(member)) {}

    [[nodiscard]] HIRExpr *getObject() const { return object.get(); }
    [[nodiscard]] const std::string &getMemberName() const { return member; }

    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

private:
    HIRExprPtr object;
    std::string member;
};

class HIRIndexExpr : public HIRExpr {
public:
    HIRIndexExpr(HIRExprPtr base, HIRExprPtr index, const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::Index, type, loc), base(std::move(base)), index(std::move(index)) {}

    [[nodiscard]] HIRExpr *getBase() const { return base.get(); }
    [[nodiscard]] HIRExpr *getIndex() const { return index.get(); }

    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

private:
    HIRExprPtr base;
    HIRExprPtr index;
};

class HIRThisExpr : public HIRExpr {
public:
    HIRThisExpr(const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::This, type, loc) {}

    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;
};

// ============================================================================
// [High-Level Constructs]
// ============================================================================

class HIRCallExpr : public HIRExpr {
public:
    HIRCallExpr(HIRExprPtr callee, std::vector<HIRExprPtr> args, const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::Call, type, loc), callee(std::move(callee)), args(std::move(args)) {}

    [[nodiscard]] HIRExpr *getCallee() const { return callee.get(); }
    [[nodiscard]] const std::vector<HIRExprPtr> &getArgs() const { return args; }

    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

private:
    HIRExprPtr callee;
    std::vector<HIRExprPtr> args;
};

class HIRNewExpr : public HIRExpr {
public:
    HIRNewExpr(std::vector<HIRExprPtr> args, const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::New, type, loc), args(std::move(args)) {}

    [[nodiscard]] const std::vector<HIRExprPtr> &getArgs() const { return args; }
    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

private:
    std::vector<HIRExprPtr> args;
};

struct HIRLambdaParam {
    std::string name;
    const HIRType *type;

    HIRLambdaParam(std::string n, const HIRType *t)
        : name(std::move(n)), type(t) {}
};

class HIRLambdaExpr : public HIRExpr {
public:
    HIRLambdaExpr(std::vector<HIRLambdaParam> params, std::unique_ptr<HIRStmt> body, const HIRType *functionType, SourceLocation loc)
        : HIRExpr(Kind::Lambda, functionType, loc), params(std::move(params)), body(std::move(body)) {}

    [[nodiscard]] const std::vector<HIRLambdaParam> &getParams() const { return params; }
    [[nodiscard]] const HIRStmt *getBody() const;

    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

private:
    std::vector<HIRLambdaParam> params;
    std::unique_ptr<HIRStmt> body;
};

class HIRThreadExpr : public HIRExpr {
public:
    // [FIX] Use ThreadKind instead of bool
    HIRThreadExpr(HIRExprPtr task, ThreadKind kind, const HIRType *type, SourceLocation loc)
        : HIRExpr(Kind::Thread, type, loc), task(std::move(task)), kind(kind) {}

    [[nodiscard]] HIRExpr *getTask() const { return task.get(); }
    [[nodiscard]] ThreadKind getThreadKind() const { return kind; }

    void accept(HIRVisitor &v) override;
    void accept(ConstHIRVisitor &v) const override;

private:
    HIRExprPtr task;
    ThreadKind kind;
};

} // namespace hir
} // namespace moksha
