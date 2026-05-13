#pragma once

#include "moksha/AST/ASTContext.h"
#include "moksha/AST/ASTVisitor.h"
#include "moksha/AST/Expr.h"
#include "moksha/HIR/HIRExpr.h"
#include "moksha/HIR/HIRFunction.h"
#include "moksha/HIR/HIRStmt.h"
#include "moksha/Support/SourceLocation.h"
#include <memory>
#include <vector>

namespace moksha {

namespace hir {
class HIRModule;
}

class HIRGen : public ASTVisitor {
public:
  explicit HIRGen(ASTContext &ctx, hir::HIRModule &hirModule);

  std::vector<std::unique_ptr<hir::HIRFunction>> takeFunctions() {
    return std::move(functions);
  }
  std::vector<std::unique_ptr<hir::HIRStmt>> takeGlobals() {
    return std::move(globals);
  }

  std::vector<std::unique_ptr<hir::HIRClass>> takeClasses() {
    return std::move(classes);
  }

  void lowerModule(const ModuleDecl *mod);

  [[nodiscard]] std::unique_ptr<hir::HIRStmt> takeStmt();
  [[nodiscard]] std::unique_ptr<hir::HIRExpr> takeExpr();

  // [FIX] Added Dispatch Helpers
  void visit(const Decl *d);
  void visit(const Stmt *s);
  void visit(const Expr *e);

  // ========================================================================
  // [Declarations]
  // ========================================================================
  void visitFunctionDecl(const FunctionDecl *decl) override;
  void visitModuleDecl(const ModuleDecl *decl) override;
  void visitVariableDecl(const VariableDecl *decl) override;
  void visitMacroDecl(const MacroDecl *) override {}
  void visitUsingDecl(const UsingDecl *) override {}
  void visitClassDecl(const ClassDecl *decl) override;
  void visitEnumDecl(const EnumDecl *) override {}
  void visitImportDecl(const ImportDecl *) override {}
  void visitGenericDecl(const GenericDecl *) override {}

  // ========================================================================
  // [Statements]
  // ========================================================================
  void visitBlockStmt(const BlockStmt *stmt) override;
  void visitReturnStmt(const ReturnStmt *stmt) override;
  void visitIfStmt(const IfStmt *stmt) override;
  void visitWhileStmt(const WhileStmt *stmt) override;
  void visitExpressionStmt(const ExpressionStmt *stmt) override;
  void visitUnsafeBlockStmt(const UnsafeBlockStmt *stmt) override;
  void visitForInStmt(const ForInStmt *stmt) override;
  void visitForStmt(const ForStmt *stmt) override;
  void visitDoWhileStmt(const DoWhileStmt *stmt) override;
  void visitBreakStmt(const BreakStmt *stmt) override;
  void visitContinueStmt(const ContinueStmt *stmt) override;
  void visitSwitchStmt(const SwitchStmt *stmt) override;
  void visitDeferStmt(const DeferStmt *stmt) override;
  void visitTryCatchStmt(const TryCatchStmt *stmt) override;
  void visitThrowStmt(const ThrowStmt *stmt) override;
  void visitDeclStmt(const DeclStmt *stmt) override;
  void visitLockStmt(const LockStmt *stmt) override;

  // ========================================================================
  // [Expressions]
  // ========================================================================
  void visitIntegerLiteral(const IntegerLiteral *expr) override;
  void visitFloatLiteral(const FloatLiteral *expr) override;
  void visitDecimalLiteral(const DecimalLiteral *expr) override;
  void visitBoolLiteral(const BoolLiteral *expr) override;
  void visitStringLiteral(const StringLiteral *expr) override;
  void visitMapLiteral(const MapLiteral *) override;
  void visitSizeOfExpr(const SizeOfExpr *) override;
  void visitBinaryExpr(const BinaryExpr *expr) override;
  void visitCallExpr(const CallExpr *expr) override;
  void visitIdentifierExpr(const IdentifierExpr *expr) override;
  void visitThreadExpr(const ThreadExpr *expr) override;
  void visitUnaryExpr(const UnaryExpr *expr) override;
  void visitMemberExpr(const MemberExpr *expr) override;
  void visitCastExpr(const CastExpr *expr) override;
  void visitBitcastExpr(const BitcastExpr *expr) override;
  void visitCharLiteral(const CharLiteral *expr) override;
  void visitTemplateStringExpr(const TemplateStringExpr *expr) override;
  void visitArrayLiteral(const ArrayLiteral *expr) override;
  void visitIndexExpr(const IndexExpr *expr) override;
  void visitLambdaExpr(const LambdaExpr *expr) override;
  void visitNewExpr(const NewExpr *expr) override;
  void visitNullLiteral(const NullLiteral *expr) override;
  void visitTernaryExpr(const TernaryExpr *expr) override;
  void visitThisExpr(const ThisExpr *expr) override;
  void visitSuperExpr(const SuperExpr *expr) override;
  void visitAwaitExpr(const AwaitExpr *expr) override;
  void visitInputExpr(const InputExpr *expr) override;
  void visitAsmExpr(const AsmExpr *expr) override;

  // ========================================================================
  // [Types] (Stubbed)
  // ========================================================================
  void visitPrimitiveType(const PrimitiveType *) override {}
  void visitPointerType(const PointerType *) override {}
  void visitArrayType(const ArrayType *type) override {}
  void visitSliceType(const SliceType *type) override;
  void visitFunctionType(const FunctionType *) override {}
  void visitNamedType(const NamedType *) override {}
  void visitNullableType(const NullableType *type) override {}
  void visitAnyType(const AnyType *) override;
  void visitMapType(const MapType *) override {}
  void visitReferenceType(const ReferenceType *) override {}
  void visitLockType(const LockType *type) override;
  void visitViewType(const ViewType *type) override;
  void visitMutType(const MutType *type) override;
  void visitConstType(const ConstType *type) override;
  void visitVolatileType(const VolatileType *type) override;
  void visitEnumType(const EnumType *) override {}
  void visitNullType(const NullType *) override;
  void visitDecimalType(const DecimalType *type) override;
  void visitClosureType(const ClosureType *type) override;
  void visitWeakType(const WeakType *type) override;
  void visitPromiseType(const PromiseType *type) override;

private:
  ASTContext &ctx;
  hir::HIRModule &hirModule;

  const hir::HIRType *lowerType(const Type *astType);

  std::vector<std::unique_ptr<hir::HIRStmt>> globals;
  std::unique_ptr<hir::HIRStmt> lastStmt;
  std::unique_ptr<hir::HIRExpr> lastExpr;
  std::vector<std::unique_ptr<hir::HIRFunction>> functions;
  std::vector<std::unique_ptr<hir::HIRClass>> classes;
};
} // namespace moksha
