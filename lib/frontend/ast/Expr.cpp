#include "moksha/AST/Expr.h"
#include "moksha/AST/ASTVisitor.h"
#include "moksha/AST/Stmt.h"

namespace moksha {

void IntegerLiteral::accept(ASTVisitor &v) const {
  v.visitIntegerLiteral(this);
}
void FloatLiteral::accept(ASTVisitor &v) const { v.visitFloatLiteral(this); }
void DecimalLiteral::accept(ASTVisitor &v) const {
  v.visitDecimalLiteral(this);
}
void StringLiteral::accept(ASTVisitor &v) const { v.visitStringLiteral(this); }
void BoolLiteral::accept(ASTVisitor &v) const { v.visitBoolLiteral(this); }
void NullLiteral::accept(ASTVisitor &v) const { v.visitNullLiteral(this); }
void CharLiteral::accept(ASTVisitor &v) const { v.visitCharLiteral(this); }
void ArrayLiteral::accept(ASTVisitor &v) const { v.visitArrayLiteral(this); }
void MapLiteral::accept(ASTVisitor &v) const { v.visitMapLiteral(this); }

void BinaryExpr::accept(ASTVisitor &v) const { v.visitBinaryExpr(this); }
void UnaryExpr::accept(ASTVisitor &v) const { v.visitUnaryExpr(this); }
void TernaryExpr::accept(ASTVisitor &v) const { v.visitTernaryExpr(this); }
void CastExpr::accept(ASTVisitor &v) const { v.visitCastExpr(this); }
void BitcastExpr::accept(ASTVisitor &v) const { v.visitBitcastExpr(this); }

void IdentifierExpr::accept(ASTVisitor &v) const {
  v.visitIdentifierExpr(this);
}
void CallExpr::accept(ASTVisitor &v) const { v.visitCallExpr(this); }
void MemberExpr::accept(ASTVisitor &v) const { v.visitMemberExpr(this); }
void IndexExpr::accept(ASTVisitor &v) const { v.visitIndexExpr(this); }
void AwaitExpr::accept(ASTVisitor &v) const { v.visitAwaitExpr(this); }

LambdaExpr::~LambdaExpr() = default;

void LambdaExpr::accept(ASTVisitor &v) const { v.visitLambdaExpr(this); }
void NewExpr::accept(ASTVisitor &v) const { v.visitNewExpr(this); }
void TemplateStringExpr::accept(ASTVisitor &v) const {
  v.visitTemplateStringExpr(this);
}
void ThreadExpr::accept(ASTVisitor &v) const { v.visitThreadExpr(this); }
void ThisExpr::accept(ASTVisitor &v) const { v.visitThisExpr(this); }
void SuperExpr::accept(ASTVisitor &v) const { v.visitSuperExpr(this); }
void SizeOfExpr::accept(ASTVisitor &v) const { v.visitSizeOfExpr(this); }
void InputExpr::accept(ASTVisitor &v) const { v.visitInputExpr(this); }
void AsmExpr::accept(ASTVisitor &v) const { v.visitAsmExpr(this); }

/** @brief Expr Cloning Implementations */

std::unique_ptr<Expr> IntegerLiteral::clone() const {
  return std::make_unique<IntegerLiteral>(value, suffix, loc);
}
std::unique_ptr<Expr> FloatLiteral::clone() const {
  return std::make_unique<FloatLiteral>(value, suffix, loc);
}
std::unique_ptr<Expr> DecimalLiteral::clone() const {
  auto copy = std::make_unique<DecimalLiteral>(exactValue, loc);
  copy->setType(type);
  return copy;
}
std::unique_ptr<Expr> StringLiteral::clone() const {
  return std::make_unique<StringLiteral>(value, isTemplate, loc);
}
std::unique_ptr<Expr> BoolLiteral::clone() const {
  return std::make_unique<BoolLiteral>(value, loc);
}
std::unique_ptr<Expr> NullLiteral::clone() const {
  return std::make_unique<NullLiteral>(loc);
}
std::unique_ptr<Expr> CharLiteral::clone() const {
  return std::make_unique<CharLiteral>(value, loc);
}
std::unique_ptr<Expr> ArrayLiteral::clone() const {
  std::vector<ExprPtr> clonedElements;
  for (const auto &el : elements)
    clonedElements.push_back(el->clone());
  return std::make_unique<ArrayLiteral>(std::move(clonedElements), loc);
}
std::unique_ptr<Expr> MapLiteral::clone() const {
  std::vector<Entry> clonedEntries;
  for (const auto &entry : entries) {
    clonedEntries.push_back({entry.first->clone(), entry.second->clone()});
  }
  return std::make_unique<MapLiteral>(std::move(clonedEntries), loc);
}
std::unique_ptr<Expr> BinaryExpr::clone() const {
  auto cloned =
      std::make_unique<BinaryExpr>(lhs->clone(), op, rhs->clone(), loc);
  cloned->setType(type);
  cloned->setResolvedOperator(resolvedOperator);
  return cloned;
}

std::unique_ptr<Expr> UnaryExpr::clone() const {
  auto cloned =
      std::make_unique<UnaryExpr>(op, operand->clone(), isPostfix, loc);
  cloned->setType(type);
  cloned->setResolvedOperator(resolvedOperator);
  return cloned;
}
std::unique_ptr<Expr> TernaryExpr::clone() const {
  return std::make_unique<TernaryExpr>(condition->clone(), trueBranch->clone(),
                                       falseBranch->clone(), loc);
}
std::unique_ptr<Expr> CastExpr::clone() const {
  return std::make_unique<CastExpr>(targetType->clone(), expr->clone(), loc);
}
std::unique_ptr<Expr> IdentifierExpr::clone() const {
  return std::make_unique<IdentifierExpr>(name, loc);
}
std::unique_ptr<Expr> CallExpr::clone() const {
  std::vector<ExprPtr> clonedArgs;
  for (const auto &arg : args)
    clonedArgs.push_back(arg->clone());
  return std::make_unique<CallExpr>(callee->clone(), std::move(clonedArgs),
                                    loc);
}
std::unique_ptr<Expr> MemberExpr::clone() const {
  auto cloned = std::make_unique<MemberExpr>(object->clone(), memberName,
                                             isOptionalAccess(), loc);
  cloned->setLayoutInfo(this->getMemberIndex(), this->isBitfield(),
                        this->getBitWidth(), this->getBitOffset());
  cloned->setType(this->getType());
  cloned->setVirtualMethodInfo(isVirtualMethod(), getMemberIndex());
  cloned->setQualifiedParent(this->qualifiedParent);
  cloned->setParentUpcast(this->parentUpcast);
  return cloned;
}
std::unique_ptr<Expr> IndexExpr::clone() const {
  return std::make_unique<IndexExpr>(array->clone(), index->clone(), isOptional,
                                     loc);
}
std::unique_ptr<Expr> NewExpr::clone() const {
  std::vector<ExprPtr> clonedArgs;
  for (const auto &arg : args)
    clonedArgs.push_back(arg->clone());
  return std::make_unique<NewExpr>(type->clone(), std::move(clonedArgs), loc);
}
std::unique_ptr<Expr> TemplateStringExpr::clone() const {
  std::vector<ExprPtr> clonedParts;
  for (const auto &part : parts)
    clonedParts.push_back(part->clone());
  return std::make_unique<TemplateStringExpr>(std::move(clonedParts), loc);
}
std::unique_ptr<Expr> ThisExpr::clone() const {
  return std::make_unique<ThisExpr>(loc);
}
std::unique_ptr<Expr> SuperExpr::clone() const {
  return std::make_unique<SuperExpr>(loc);
}
std::unique_ptr<Expr> AwaitExpr::clone() const {
  return std::make_unique<AwaitExpr>(expr->clone(), loc);
}
std::unique_ptr<Expr> SizeOfExpr::clone() const {
  return std::make_unique<SizeOfExpr>(expression->clone(), loc);
}

LambdaExpr::LambdaExpr(std::vector<LambdaParam> params,
                       std::unique_ptr<Stmt> body, bool isExprBody,
                       CaptureMode mode, SourceLocation loc)
    : Expr(ExprKind::LambdaExpr, loc), params(std::move(params)),
      body(std::move(body)), isExprBody(isExprBody), captureMode(mode) {}

std::unique_ptr<Expr> LambdaExpr::clone() const {
  std::vector<LambdaParam> clonedParams;
  for (const auto &p : params) {
    clonedParams.push_back(p.clone());
  }
  auto cloned = std::make_unique<LambdaExpr>(
      std::move(clonedParams), body->clone(), isExprBody, captureMode, loc);
  cloned->setAsync(this->isAsyncFlag);
  for (const auto &c : captures) {
    cloned->addCapture(c.name, c.type, c.mode);
  }
  cloned->setType(this->type);
  return cloned;
}

std::unique_ptr<Expr> ThreadExpr::clone() const {
  auto clonedBody = body ? body->cloneAs<LambdaExpr>() : nullptr;
  return std::make_unique<ThreadExpr>(isWeak, std::move(clonedBody), loc);
}

} // namespace moksha
