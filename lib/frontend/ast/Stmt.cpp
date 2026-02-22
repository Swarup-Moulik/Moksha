#include "moksha/AST/Stmt.h"
#include "moksha/AST/ASTVisitor.h"

namespace moksha {

// --- Declarations ---
void ModuleDecl::accept(ASTVisitor &v) const { v.visitModuleDecl(this); }
void VariableDecl::accept(ASTVisitor &v) const { v.visitVariableDecl(this); }
void FunctionDecl::accept(ASTVisitor &v) const { v.visitFunctionDecl(this); }
void ClassDecl::accept(ASTVisitor &v) const { v.visitClassDecl(this); }
void GenericDecl::accept(ASTVisitor &v) const { v.visitGenericDecl(this); }
void ImportDecl::accept(ASTVisitor &v) const { v.visitImportDecl(this); }
void EnumDecl::accept(ASTVisitor &v) const { v.visitEnumDecl(this); }
void MacroDecl::accept(ASTVisitor &v) const { v.visitMacroDecl(this); }
void UsingDecl::accept(ASTVisitor &v) const { v.visitUsingDecl(this); }

// --- Statements ---
void BlockStmt::accept(ASTVisitor &v) const { v.visitBlockStmt(this); }
void ExpressionStmt::accept(ASTVisitor &v) const {
  v.visitExpressionStmt(this);
}
void DeclStmt::accept(ASTVisitor &v) const { v.visitDeclStmt(this); }
void ReturnStmt::accept(ASTVisitor &v) const { v.visitReturnStmt(this); }
void BreakStmt::accept(ASTVisitor &v) const { v.visitBreakStmt(this); }
void ContinueStmt::accept(ASTVisitor &v) const { v.visitContinueStmt(this); }
void IfStmt::accept(ASTVisitor &v) const { v.visitIfStmt(this); }
void WhileStmt::accept(ASTVisitor &v) const { v.visitWhileStmt(this); }
void DoWhileStmt::accept(ASTVisitor &v) const { v.visitDoWhileStmt(this); }
void ForStmt::accept(ASTVisitor &v) const { v.visitForStmt(this); }
void ForInStmt::accept(ASTVisitor &v) const { v.visitForInStmt(this); }
void SwitchStmt::accept(ASTVisitor &v) const { v.visitSwitchStmt(this); }
void DeferStmt::accept(ASTVisitor &v) const { v.visitDeferStmt(this); }
void UnsafeBlockStmt::accept(ASTVisitor &v) const {
  v.visitUnsafeBlockStmt(this);
}
void TryCatchStmt::accept(ASTVisitor &v) const { v.visitTryCatchStmt(this); }
void ThrowStmt::accept(ASTVisitor &v) const { v.visitThrowStmt(this); }
void AsmStmt::accept(ASTVisitor &v) const { v.visitAsmStmt(this); }

} // namespace moksha
