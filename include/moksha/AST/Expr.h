#pragma once

#include "moksha/AST/Type.h"
#include "moksha/Lexer/Lexer.h"
#include <memory>
#include <string>
#include <vector>

namespace moksha {

class Stmt;
class ASTVisitor;

/// Discriminator for LLVM-style RTTI (dyn_cast/isa)
enum class ExprKind {
  IntegerLiteral,
  FloatLiteral,
  StringLiteral,
  BoolLiteral,
  NullLiteral,
  CharLiteral,
  ArrayLiteral,
  MapLiteral,
  BinaryExpr,
  UnaryExpr,
  TernaryExpr,
  CastExpr,
  IdentifierExpr,
  CallExpr,
  MemberExpr,
  IndexExpr,
  LambdaExpr,
  NewExpr,
  TemplateStringExpr,
  ThreadExpr,
  ThisExpr,
  SuperExpr,
  AwaitExpr,
  SizeOfExpr
};

/// Base Expression Node
class Expr {
public:
  virtual ~Expr() = default;
  SourceLocation getLoc() const { return loc; }
  ExprKind getKind() const { return kind; }
  virtual void accept(ASTVisitor &visitor) const = 0;

protected:
  Expr(ExprKind kind, SourceLocation loc) : kind(kind), loc(loc) {}
  ExprKind kind;
  SourceLocation loc;
};

using ExprPtr = std::unique_ptr<Expr>;

// --- Literals ---

class IntegerLiteral : public Expr {
public:
  IntegerLiteral(uint64_t val, NumericSuffix suffix, SourceLocation loc)
      : Expr(ExprKind::IntegerLiteral, loc), value(val), suffix(suffix) {}
  uint64_t getValue() const { return value; }
  void accept(ASTVisitor &v) const override;
  NumericSuffix getSuffix() const { return suffix; }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::IntegerLiteral;
  }

private:
  uint64_t value;
  NumericSuffix suffix;
};

class FloatLiteral : public Expr {
public:
  FloatLiteral(double val, NumericSuffix suffix, SourceLocation loc)
      : Expr(ExprKind::FloatLiteral, loc), value(val), suffix(suffix) {}
  double getValue() const { return value; }
  void accept(ASTVisitor &v) const override;
  NumericSuffix getSuffix() const { return suffix; }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::FloatLiteral;
  }

private:
  double value;
  NumericSuffix suffix;
};

class StringLiteral : public Expr {
public:
  StringLiteral(std::string val, bool isTemplate, SourceLocation loc)
      : Expr(ExprKind::StringLiteral, loc), value(std::move(val)),
        isTemplate(isTemplate) {}
  const std::string &getValue() const { return value; }
  void accept(ASTVisitor &v) const override;

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::StringLiteral;
  }

private:
  std::string value;
  bool isTemplate;
};

class BoolLiteral : public Expr {
public:
  BoolLiteral(bool val, SourceLocation loc)
      : Expr(ExprKind::BoolLiteral, loc), value(val) {}
  bool getValue() const { return value; }
  void accept(ASTVisitor &v) const override;

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::BoolLiteral;
  }

private:
  bool value;
};

class NullLiteral : public Expr {
public:
  explicit NullLiteral(SourceLocation loc) : Expr(ExprKind::NullLiteral, loc) {}
  void accept(ASTVisitor &v) const override;

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::NullLiteral;
  }
};

class CharLiteral : public Expr {
public:
  CharLiteral(char val, SourceLocation loc)
      : Expr(ExprKind::CharLiteral, loc), value(val) {}
  char getValue() const { return value; }
  void accept(ASTVisitor &v) const override;

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::CharLiteral;
  }

private:
  char value;
};

class ArrayLiteral : public Expr {
public:
  ArrayLiteral(std::vector<ExprPtr> elements, SourceLocation loc)
      : Expr(ExprKind::ArrayLiteral, loc), elements(std::move(elements)) {}
  void accept(ASTVisitor &v) const override;
  const std::vector<ExprPtr> &getElements() const { return elements; }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::ArrayLiteral;
  }

private:
  std::vector<ExprPtr> elements;
};

class MapLiteral : public Expr {
public:
  // Store pairs of Key:Value
  using Entry = std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>;

  MapLiteral(std::vector<Entry> entries, SourceLocation loc)
      : Expr(ExprKind::MapLiteral, loc), entries(std::move(entries)) {}

  void accept(ASTVisitor &v) const override;
  const std::vector<Entry> &getEntries() const { return entries; }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::MapLiteral;
  }

private:
  std::vector<Entry> entries;
};

// --- Operations ---

class BinaryExpr : public Expr {
public:
  BinaryExpr(ExprPtr left, TokenKind op, ExprPtr right, SourceLocation loc)
      : Expr(ExprKind::BinaryExpr, loc), lhs(std::move(left)), op(op),
        rhs(std::move(right)) {}
  void accept(ASTVisitor &v) const override;
  const Expr *getLHS() const { return lhs.get(); }
  const Expr *getRHS() const { return rhs.get(); }
  TokenKind getOp() const { return op; }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::BinaryExpr;
  }

private:
  ExprPtr lhs;
  TokenKind op;
  ExprPtr rhs;
};

class UnaryExpr : public Expr {
public:
  UnaryExpr(TokenKind op, ExprPtr operand, bool isPostfix, SourceLocation loc)
      : Expr(ExprKind::UnaryExpr, loc), op(op), operand(std::move(operand)),
        isPostfix(isPostfix) {}
  void accept(ASTVisitor &v) const override;
  const Expr *getOperand() const { return operand.get(); }
  TokenKind getOp() const { return op; }
  bool isPostfixOp() const { return isPostfix; }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::UnaryExpr;
  }

private:
  TokenKind op;
  ExprPtr operand;
  bool isPostfix;
};

class TernaryExpr : public Expr {
public:
  TernaryExpr(ExprPtr cond, ExprPtr trueExpr, ExprPtr falseExpr,
              SourceLocation loc)
      : Expr(ExprKind::TernaryExpr, loc), condition(std::move(cond)),
        trueBranch(std::move(trueExpr)), falseBranch(std::move(falseExpr)) {}
  void accept(ASTVisitor &v) const override;
  const Expr *getCondition() const { return condition.get(); }
  const Expr *getTrueBranch() const { return trueBranch.get(); }
  const Expr *getFalseBranch() const { return falseBranch.get(); }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::TernaryExpr;
  }

private:
  ExprPtr condition, trueBranch, falseBranch;
};

class CastExpr : public Expr {
public:
  CastExpr(TypePtr target, ExprPtr expr, SourceLocation loc)
      : Expr(ExprKind::CastExpr, loc), targetType(std::move(target)),
        expr(std::move(expr)) {}
  void accept(ASTVisitor &v) const override;
  const Type *getTargetType() const { return targetType.get(); }
  const Expr *getExpr() const { return expr.get(); }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::CastExpr;
  }

private:
  TypePtr targetType;
  ExprPtr expr;
};

// --- Variables & Calls ---

class IdentifierExpr : public Expr {
public:
  IdentifierExpr(std::string name, SourceLocation loc)
      : Expr(ExprKind::IdentifierExpr, loc), name(std::move(name)) {}
  void accept(ASTVisitor &v) const override;
  const std::string &getName() const { return name; }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::IdentifierExpr;
  }

private:
  std::string name;
};

class CallExpr : public Expr {
public:
  CallExpr(ExprPtr callee, std::vector<ExprPtr> args, SourceLocation loc)
      : Expr(ExprKind::CallExpr, loc), callee(std::move(callee)),
        args(std::move(args)) {}
  void accept(ASTVisitor &v) const override;
  const Expr *getCallee() const { return callee.get(); }
  const std::vector<ExprPtr> &getArgs() const { return args; }

  void insertFirstArg(ExprPtr arg) {
    args.insert(args.begin(), std::move(arg));
  }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::CallExpr;
  }

private:
  ExprPtr callee;
  std::vector<ExprPtr> args;
};

class MemberExpr : public Expr {
public:
  MemberExpr(ExprPtr object, std::string member, bool isOptional,
             SourceLocation loc)
      : Expr(ExprKind::MemberExpr, loc), object(std::move(object)),
        memberName(std::move(member)), isOptional(isOptional) {}
  void accept(ASTVisitor &v) const override;
  const Expr *getObject() const { return object.get(); }
  const std::string &getName() const { return memberName; }
  bool isOptionalAccess() const { return isOptional; }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::MemberExpr;
  }

private:
  ExprPtr object;
  std::string memberName;
  bool isOptional;
};

class IndexExpr : public Expr {
public:
  IndexExpr(ExprPtr array, ExprPtr index, SourceLocation loc)
      : Expr(ExprKind::IndexExpr, loc), array(std::move(array)),
        index(std::move(index)) {}
  void accept(ASTVisitor &v) const override;
  const Expr *getArray() const { return array.get(); }
  const Expr *getIndex() const { return index.get(); }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::IndexExpr;
  }

private:
  ExprPtr array;
  ExprPtr index;
};

// --- Advanced ---

class LambdaParam {
public:
  LambdaParam(TypePtr t, std::string n)
      : type(std::move(t)), name(std::move(n)) {}
  LambdaParam(LambdaParam &&) = default;
  LambdaParam &operator=(LambdaParam &&) = default;
  const Type *getType() const { return type.get(); }
  const std::string &getName() const { return name; }

private:
  TypePtr type;
  std::string name;
};

class LambdaExpr : public Expr {
public:
  // [FIX] Add destructor declaration
  ~LambdaExpr() override;

  LambdaExpr(std::vector<LambdaParam> params, std::unique_ptr<Stmt> body,
             bool isExprBody, SourceLocation loc)
      : Expr(ExprKind::LambdaExpr, loc), params(std::move(params)),
        body(std::move(body)), isExprBody(isExprBody) {}
  void accept(ASTVisitor &v) const override;
  const std::vector<LambdaParam> &getParams() const { return params; }
  const Stmt *getBody() const { return body.get(); }
  bool isExpressionBody() const { return isExprBody; }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::LambdaExpr;
  }

private:
  std::vector<LambdaParam> params;
  std::unique_ptr<Stmt> body;
  bool isExprBody;
};

class NewExpr : public Expr {
public:
  NewExpr(TypePtr type, std::vector<ExprPtr> args, SourceLocation loc)
      : Expr(ExprKind::NewExpr, loc), type(std::move(type)),
        args(std::move(args)) {}
  void accept(ASTVisitor &v) const override;
  const Type *getType() const { return type.get(); }
  const std::vector<ExprPtr> &getArgs() const { return args; }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::NewExpr;
  }

private:
  TypePtr type;
  std::vector<ExprPtr> args;
};

class TemplateStringExpr : public Expr {
public:
  TemplateStringExpr(std::vector<ExprPtr> parts, SourceLocation loc)
      : Expr(ExprKind::TemplateStringExpr, loc), parts(std::move(parts)) {}
  void accept(ASTVisitor &v) const override;
  const std::vector<ExprPtr> &getParts() const { return parts; }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::TemplateStringExpr;
  }

private:
  std::vector<ExprPtr> parts;
};

class ThreadExpr : public Expr {
public:
  ThreadExpr(bool isWeak, std::unique_ptr<LambdaExpr> body, SourceLocation loc)
      : Expr(ExprKind::ThreadExpr, loc), isWeak(isWeak), body(std::move(body)) {
  }
  void accept(ASTVisitor &v) const override;
  bool isWeakThread() const { return isWeak; }
  const LambdaExpr *getBody() const { return body.get(); }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::ThreadExpr;
  }

private:
  bool isWeak;
  std::unique_ptr<LambdaExpr> body;
};

class ThisExpr : public Expr {
public:
  explicit ThisExpr(SourceLocation loc) : Expr(ExprKind::ThisExpr, loc) {}
  void accept(ASTVisitor &v) const override;
  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::ThisExpr;
  }
};

class SuperExpr : public Expr {
public:
  explicit SuperExpr(SourceLocation loc) : Expr(ExprKind::SuperExpr, loc) {}
  void accept(ASTVisitor &v) const override;
  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::SuperExpr;
  }
};

class AwaitExpr : public Expr {
public:
  AwaitExpr(ExprPtr expr, SourceLocation loc)
      : Expr(ExprKind::AwaitExpr, loc), expr(std::move(expr)) {}

  void accept(ASTVisitor &v) const override;
  const Expr *getExpr() const { return expr.get(); }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::AwaitExpr;
  }

private:
  ExprPtr expr;
};

class SizeOfExpr : public Expr {
public:
  SizeOfExpr(ExprPtr expr, SourceLocation loc)
      : Expr(ExprKind::SizeOfExpr, loc), expression(std::move(expr)) {}
  void accept(ASTVisitor &v) const override;
  const Expr *getExpr() const { return expression.get(); }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::SizeOfExpr;
  }

private:
  ExprPtr expression;
};

} // namespace moksha
