#include "moksha/AST/Expr.h"
#include "moksha/AST/ASTVisitor.h"

namespace moksha {

void IntegerLiteral::accept(ASTVisitor &v) const {
  v.visitIntegerLiteral(this);
}
void FloatLiteral::accept(ASTVisitor &v) const { v.visitFloatLiteral(this); }
void StringLiteral::accept(ASTVisitor &v) const { v.visitStringLiteral(this); }
void BoolLiteral::accept(ASTVisitor &v) const { v.visitBoolLiteral(this); }
void NullLiteral::accept(ASTVisitor &v) const { v.visitNullLiteral(this); }
void CharLiteral::accept(ASTVisitor &v) const { v.visitCharLiteral(this); }
void ArrayLiteral::accept(ASTVisitor &v) const { v.visitArrayLiteral(this); }

void BinaryExpr::accept(ASTVisitor &v) const { v.visitBinaryExpr(this); }
void UnaryExpr::accept(ASTVisitor &v) const { v.visitUnaryExpr(this); }
void TernaryExpr::accept(ASTVisitor &v) const { v.visitTernaryExpr(this); }
void CastExpr::accept(ASTVisitor &v) const { v.visitCastExpr(this); }

void IdentifierExpr::accept(ASTVisitor &v) const {
  v.visitIdentifierExpr(this);
}
void CallExpr::accept(ASTVisitor &v) const { v.visitCallExpr(this); }
void MemberExpr::accept(ASTVisitor &v) const { v.visitMemberExpr(this); }
void IndexExpr::accept(ASTVisitor &v) const { v.visitIndexExpr(this); }

void LambdaExpr::accept(ASTVisitor &v) const { v.visitLambdaExpr(this); }
void NewExpr::accept(ASTVisitor &v) const { v.visitNewExpr(this); }
void TemplateStringExpr::accept(ASTVisitor &v) const {
  v.visitTemplateStringExpr(this);
}
void ThreadExpr::accept(ASTVisitor &v) const { v.visitThreadExpr(this); }
void ThisExpr::accept(ASTVisitor &v) const { v.visitThisExpr(this); }
void SuperExpr::accept(ASTVisitor &v) const { v.visitSuperExpr(this); }

} // namespace moksha
