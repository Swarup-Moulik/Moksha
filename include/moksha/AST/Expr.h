#pragma once

#include "moksha/AST/Type.h"
#include "moksha/Lexer/Lexer.h"
#include <memory>
#include <string>
#include <vector>

namespace moksha {

/** @brief Forward declarations */
class Stmt;
class ASTVisitor;
class FunctionDecl;
class ClassDecl;

/** @brief It contains the kind of expression nodes */
enum class ExprKind {
  IntegerLiteral,
  FloatLiteral,
  DecimalLiteral,
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
  BitcastExpr,
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
  SizeOfExpr,
  InputExpr,
  AsmExpr
};

/** @brief Base Expression Node */
class Expr {
public:
  virtual ~Expr() = default;
  SourceLocation getLoc() const { return loc; }
  ExprKind getKind() const { return kind; }

  virtual const Type *getType() const { return type; }
  void setType(const Type *newType) { type = newType; }
  virtual void accept(ASTVisitor &visitor) const = 0;
  virtual std::unique_ptr<Expr> clone() const = 0;
  template <typename T> std::unique_ptr<T> cloneAs() const {
    return std::unique_ptr<T>(static_cast<T *>(clone().release()));
  }

protected:
  Expr(ExprKind kind, SourceLocation loc) : kind(kind), loc(loc) {}
  ExprKind kind;
  SourceLocation loc;
  const Type *type = nullptr;
};

using ExprPtr = std::unique_ptr<Expr>;

/** @brief Literal expression nodes */

class IntegerLiteral : public Expr {
public:
  IntegerLiteral(uint64_t val, NumericSuffix suffix, SourceLocation loc)
      : Expr(ExprKind::IntegerLiteral, loc), value(val), suffix(suffix) {}
  uint64_t getValue() const { return value; }
  void accept(ASTVisitor &v) const override;
  NumericSuffix getSuffix() const { return suffix; }
  std::unique_ptr<Expr> clone() const override;
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
  std::unique_ptr<Expr> clone() const override;
  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::FloatLiteral;
  }

private:
  double value;
  NumericSuffix suffix;
};

class DecimalLiteral : public Expr {
public:
  DecimalLiteral(std::string exactValue, SourceLocation loc)
      : Expr(ExprKind::DecimalLiteral, loc), exactValue(std::move(exactValue)) {
  }

  const std::string &getValue() const { return exactValue; }

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Expr> clone() const override;

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::DecimalLiteral;
  }

private:
  std::string exactValue;
};

class StringLiteral : public Expr {
public:
  StringLiteral(std::string val, bool isTemplate, SourceLocation loc)
      : Expr(ExprKind::StringLiteral, loc), value(std::move(val)),
        isTemplate(isTemplate) {}
  const std::string &getValue() const { return value; }
  bool isTemplateString() const { return isTemplate; }
  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Expr> clone() const override;
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
  std::unique_ptr<Expr> clone() const override;
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
  std::unique_ptr<Expr> clone() const override;
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
  std::unique_ptr<Expr> clone() const override;
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
  std::unique_ptr<Expr> clone() const override;
  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::ArrayLiteral;
  }

private:
  std::vector<ExprPtr> elements;
};

class MapLiteral : public Expr {
public:
  using Entry = std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>;

  MapLiteral(std::vector<Entry> entries, SourceLocation loc)
      : Expr(ExprKind::MapLiteral, loc), entries(std::move(entries)) {}

  void accept(ASTVisitor &v) const override;
  const std::vector<Entry> &getEntries() const { return entries; }
  std::unique_ptr<Expr> clone() const override;
  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::MapLiteral;
  }

private:
  std::vector<Entry> entries;
};

/** @brief Operation expression nodes */

class BinaryExpr : public Expr {
public:
  BinaryExpr(ExprPtr left, TokenKind op, ExprPtr right, SourceLocation loc)
      : Expr(ExprKind::BinaryExpr, loc), lhs(std::move(left)), op(op),
        rhs(std::move(right)) {}
  void accept(ASTVisitor &v) const override;
  const Expr *getLHS() const { return lhs.get(); }
  const Expr *getRHS() const { return rhs.get(); }
  ExprPtr &getLHSMut() { return lhs; }
  ExprPtr &getRHSMut() { return rhs; }
  TokenKind getOp() const { return op; }
  std::unique_ptr<Expr> clone() const override;
  void setResolvedOperator(const FunctionDecl *func) {
    resolvedOperator = func;
  }
  const FunctionDecl *getResolvedOperator() const { return resolvedOperator; }
  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::BinaryExpr;
  }

private:
  ExprPtr lhs;
  TokenKind op;
  ExprPtr rhs;
  const FunctionDecl *resolvedOperator = nullptr;
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
  std::unique_ptr<Expr> clone() const override;
  void setResolvedOperator(const FunctionDecl *func) {
    resolvedOperator = func;
  }
  const FunctionDecl *getResolvedOperator() const { return resolvedOperator; }
  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::UnaryExpr;
  }

private:
  TokenKind op;
  ExprPtr operand;
  bool isPostfix;
  const FunctionDecl *resolvedOperator = nullptr;
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
  std::unique_ptr<Expr> clone() const override;
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
  std::unique_ptr<Expr> clone() const override;
  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::CastExpr;
  }

private:
  TypePtr targetType;
  ExprPtr expr;
};

class BitcastExpr : public Expr {
public:
  BitcastExpr(TypePtr target, ExprPtr expr, SourceLocation loc)
      : Expr(ExprKind::BitcastExpr, loc), targetType(std::move(target)),
        expr(std::move(expr)) {}

  void accept(ASTVisitor &v) const override;

  const Type *getTargetType() const { return targetType.get(); }
  const Expr *getExpr() const { return expr.get(); }

  std::unique_ptr<Expr> clone() const override {
    return std::make_unique<BitcastExpr>(targetType->clone(), expr->clone(),
                                         loc);
  }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::BitcastExpr;
  }

private:
  TypePtr targetType;
  ExprPtr expr;
};

/** @brief Variables & Calls */

class IdentifierExpr : public Expr {
public:
  IdentifierExpr(std::string name, SourceLocation loc)
      : Expr(ExprKind::IdentifierExpr, loc), name(std::move(name)) {}
  void accept(ASTVisitor &v) const override;
  const std::string &getName() const { return name; }
  void setName(std::string newName) { name = std::move(newName); }
  std::unique_ptr<Expr> clone() const override;
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
  std::vector<ExprPtr> &getArgsMut() { return args; }
  std::unique_ptr<Expr> clone() const override;
  void insertFirstArg(std::unique_ptr<Expr> arg) {
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
  std::unique_ptr<Expr> clone() const override;
  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::MemberExpr;
  }
  uint32_t getMemberIndex() const { return memberIndex; }
  bool isBitfield() const { return bitfield; }
  uint32_t getBitWidth() const { return bitWidth; }
  uint32_t getBitOffset() const { return bitOffset; }
  void setLayoutInfo(uint32_t index, bool isBf = false, uint32_t width = 0,
                     uint32_t offset = 0) {
    memberIndex = index;
    bitfield = isBf;
    bitWidth = width;
    bitOffset = offset;
  }
  bool isVirtualMethod() const { return virtualMethod; }
  void setVirtualMethodInfo(bool isVirtual, uint32_t vtableIdx) {
    virtualMethod = isVirtual;
    memberIndex = vtableIdx;
  }
  void setType(const Type *t) { type = t; }
  const ClassDecl *getQualifiedParent() const { return qualifiedParent; }
  void setQualifiedParent(const ClassDecl *parent) { qualifiedParent = parent; }
  bool isParentUpcast() const { return parentUpcast; }
  void setParentUpcast(bool val) { parentUpcast = val; }

private:
  ExprPtr object;
  std::string memberName;
  bool isOptional;
  uint32_t memberIndex = 0;
  bool bitfield = false;
  uint32_t bitWidth = 0;
  uint32_t bitOffset = 0;
  bool virtualMethod = false;
  const ClassDecl *qualifiedParent = nullptr;
  bool parentUpcast = false;
};

class IndexExpr : public Expr {
public:
  IndexExpr(ExprPtr array, ExprPtr index, bool isOptional, SourceLocation loc)
      : Expr(ExprKind::IndexExpr, loc), array(std::move(array)),
        index(std::move(index)), isOptional(isOptional) {}

  void accept(ASTVisitor &v) const override;

  const Expr *getArray() const { return array.get(); }
  const Expr *getIndex() const { return index.get(); }
  bool isOptionalAccess() const { return isOptional; }

  std::unique_ptr<Expr> clone() const override;

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::IndexExpr;
  }

private:
  ExprPtr array;
  ExprPtr index;
  bool isOptional;
};

/** @brief Advanced expression nodes */

class LambdaParam {
public:
  LambdaParam(TypePtr t, std::string n, ExprPtr defVal = nullptr)
      : type(std::move(t)), name(std::move(n)),
        defaultValue(std::move(defVal)) {}
  LambdaParam(LambdaParam &&) = default;
  LambdaParam &operator=(LambdaParam &&) = default;
  const Type *getType() const { return type.get(); }
  const std::string &getName() const { return name; }
  const Expr *getDefaultValue() const { return defaultValue.get(); }
  LambdaParam clone() const {
    return LambdaParam(type->clone(), name,
                       defaultValue ? defaultValue->clone() : nullptr);
  }

private:
  TypePtr type;
  std::string name;
  ExprPtr defaultValue;
};

enum class CaptureMode {
  Snapshot, // closure -> "take a snapshot" (value copy)
  View,     // &closure -> "look at it" (immutable borrow)
  Mut,      // &mut closure -> "modify it" (mutable borrow)
  Move      // move closure -> "take it completely" (ownership transfer)
};

struct ASTCapture {
  std::string name;
  const Type *type;
  CaptureMode mode;
};

class LambdaExpr : public Expr {
public:
  ~LambdaExpr() override;

  LambdaExpr(std::vector<LambdaParam> params, std::unique_ptr<Stmt> body,
             bool isExprBody, CaptureMode mode, SourceLocation loc);

  void accept(ASTVisitor &v) const override;
  const std::vector<LambdaParam> &getParams() const { return params; }
  const Stmt *getBody() const { return body.get(); }
  bool isExpressionBody() const { return isExprBody; }
  std::unique_ptr<Expr> clone() const override;
  const std::vector<ASTCapture> &getCaptures() const { return captures; }

  void addCapture(std::string name, const Type *type, CaptureMode mode) const {
    captures.push_back({std::move(name), type, mode});
  }
  CaptureMode getCaptureMode() const { return captureMode; }
  bool isAsyncLambda() const { return isAsyncFlag; }
  void setAsync(bool async) { isAsyncFlag = async; }
  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::LambdaExpr;
  }

private:
  std::vector<LambdaParam> params;
  std::unique_ptr<Stmt> body;
  bool isExprBody;
  mutable std::vector<ASTCapture> captures;
  CaptureMode captureMode;
  bool isAsyncFlag = false;
};

class NewExpr : public Expr {
public:
  NewExpr(TypePtr type, std::vector<ExprPtr> args, SourceLocation loc)
      : Expr(ExprKind::NewExpr, loc), type(std::move(type)),
        args(std::move(args)) {}
  void accept(ASTVisitor &v) const override;
  const Type *getType() const override { return type.get(); }
  const std::vector<ExprPtr> &getArgs() const { return args; }
  std::vector<ExprPtr> &getArgsMut() { return args; }
  std::unique_ptr<Expr> clone() const override;
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
  std::unique_ptr<Expr> clone() const override;
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
  std::unique_ptr<Expr> clone() const override;
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
  std::unique_ptr<Expr> clone() const override;
  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::ThisExpr;
  }
};

class SuperExpr : public Expr {
public:
  explicit SuperExpr(SourceLocation loc) : Expr(ExprKind::SuperExpr, loc) {}
  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Expr> clone() const override;
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
  std::unique_ptr<Expr> clone() const override;
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
  std::unique_ptr<Expr> clone() const override;
  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::SizeOfExpr;
  }

private:
  ExprPtr expression;
};

class InputExpr : public Expr {
public:
  InputExpr(ExprPtr prompt, SourceLocation loc)
      : Expr(ExprKind::InputExpr, loc), prompt(std::move(prompt)) {}

  void accept(ASTVisitor &v) const override;
  const Expr *getPrompt() const { return prompt.get(); }
  std::unique_ptr<Expr> clone() const override;

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::InputExpr;
  }

private:
  ExprPtr prompt;
};

class AsmExpr : public Expr {
public:
  /** @brief Helper struct to map a constraint string to an expression */
  struct AsmOperand {
    std::string constraint;
    std::unique_ptr<Expr> expr;

    AsmOperand(std::string c, std::unique_ptr<Expr> e)
        : constraint(std::move(c)), expr(std::move(e)) {}

    AsmOperand(AsmOperand &&) = default;
    AsmOperand &operator=(AsmOperand &&) = default;

    AsmOperand clone() const {
      return AsmOperand(constraint, expr ? expr->clone() : nullptr);
    }
  };

  AsmExpr(std::string assemblyStr, std::vector<AsmOperand> outputs,
          std::vector<AsmOperand> inputs, std::vector<AsmOperand> inouts,
          std::vector<std::string> clobbers, bool isVolatile,
          TypePtr returnType, SourceLocation loc)
      : Expr(ExprKind::AsmExpr, loc), assemblyStr(std::move(assemblyStr)),
        outputs(std::move(outputs)), inputs(std::move(inputs)),
        inouts(std::move(inouts)), clobbers(std::move(clobbers)),
        isVolatile(isVolatile) {
    if (returnType)
      this->type = returnType.get();
  }

  void accept(ASTVisitor &v) const override;

  const std::string &getAssemblyStr() const { return assemblyStr; }
  const std::vector<AsmOperand> &getOutputs() const { return outputs; }
  const std::vector<AsmOperand> &getInputs() const { return inputs; }
  const std::vector<AsmOperand> &getInouts() const { return inouts; }
  const std::vector<std::string> &getClobbers() const { return clobbers; }
  bool getIsVolatile() const { return isVolatile; }

  std::unique_ptr<Expr> clone() const override {
    std::vector<AsmOperand> outClone, inClone, inoutClone;
    for (const auto &o : outputs)
      outClone.push_back(o.clone());
    for (const auto &i : inputs)
      inClone.push_back(i.clone());
    for (const auto &io : inouts)
      inoutClone.push_back(io.clone());

    auto expr = std::make_unique<AsmExpr>(
        assemblyStr, std::move(outClone), std::move(inClone),
        std::move(inoutClone), clobbers, isVolatile, nullptr, loc);
    expr->setType(this->type);
    return expr;
  }

  static bool classof(const Expr *E) {
    return E->getKind() == ExprKind::AsmExpr;
  }

private:
  std::string assemblyStr;
  std::vector<AsmOperand> outputs;
  std::vector<AsmOperand> inputs;
  std::vector<AsmOperand> inouts;
  std::vector<std::string> clobbers;
  bool isVolatile;
};

} // namespace moksha
