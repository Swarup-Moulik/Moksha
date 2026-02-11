#pragma once

#include "moksha/AST/ASTVisitor.h"
#include "moksha/AST/Type.h"
#include "moksha/Sema/GenericResolver.h"
#include "moksha/Sema/SymbolTable.h"
#include "moksha/Support/Diagnostics.h"
#include <vector>

namespace moksha {

class ASTContext;
class Decl;
class Stmt;

/// \brief Performs semantic analysis and type checking on the AST.
class TypeChecker : public ASTVisitor {
public:
  /// \brief Initialize the TypeChecker with necessary contexts.
  TypeChecker(ASTContext &ctx, SymbolTable &sym, DiagnosticEngine &diags);

  void check(Decl *decl);
  void check(Stmt *stmt);

  bool hasErrors() const { return hasError; }

private:
  // --- Contexts ---
  ASTContext &context;
  SymbolTable &symbols;
  DiagnosticEngine &Diags;
  GenericResolver resolver;

  // --- State ---
  const Type *lastComputedType;
  const Type *currentExpectedReturnType;
  bool hasError;
  int loopDepth; // Track if we are inside a loop
  std::vector<const Type*> lambdaStack;

  // --- Helpers ---
  bool isCompatible(const Type *expected, const Type *actual);
  bool isCastAllowed(const Type *src, const Type *dst);

  /// Helper for numeric promotion (i8 -> i32, etc.)
  const Type *getCommonNumericType(const Type *t1, const Type *t2);
  const Type *getCommonSuperType(const Type *t1, const Type *t2);

  // --- ASTVisitor Overrides ---
  void visitIntegerLiteral(const IntegerLiteral *expr) override;
  void visitFloatLiteral(const FloatLiteral *expr) override;
  void visitStringLiteral(const StringLiteral *expr) override;
  void visitBoolLiteral(const BoolLiteral *expr) override;
  void visitNullLiteral(const NullLiteral *expr) override;
  void visitCharLiteral(const CharLiteral *expr) override;
  void visitArrayLiteral(const ArrayLiteral *expr) override;
  void visitIdentifierExpr(const IdentifierExpr *expr) override;
  void visitBinaryExpr(const BinaryExpr *expr) override;
  void visitUnaryExpr(const UnaryExpr *expr) override;
  void visitCallExpr(const CallExpr *expr) override;
  void visitIndexExpr(const IndexExpr *expr) override;
  void visitMemberExpr(const MemberExpr *expr) override;
  void visitCastExpr(const CastExpr *expr) override;
  void visitTernaryExpr(const TernaryExpr *expr) override;
  void visitNewExpr(const NewExpr *expr) override;
  void visitLambdaExpr(const LambdaExpr *expr) override;
  void visitTemplateStringExpr(const TemplateStringExpr *expr) override;
  void visitThreadExpr(const ThreadExpr *expr) override;

  // Statements
  void visitVariableDecl(const VariableDecl *decl) override;
  void visitFunctionDecl(const FunctionDecl *decl) override;
  void visitReturnStmt(const ReturnStmt *stmt) override;
  void visitBlockStmt(const BlockStmt *stmt) override;
  void visitIfStmt(const IfStmt *stmt) override;
  void visitWhileStmt(const WhileStmt *stmt) override;
  void visitDoWhileStmt(const DoWhileStmt *stmt) override;
  void visitForStmt(const ForStmt *stmt) override;
  void visitForInStmt(const ForInStmt *stmt) override;
  void visitSwitchStmt(const SwitchStmt *stmt) override;
  void visitDeferStmt(const DeferStmt *stmt) override;
  void visitUnsafeBlockStmt(const UnsafeBlockStmt *stmt) override;
  void visitTryCatchStmt(const TryCatchStmt *stmt) override;
  void visitExpressionStmt(const ExpressionStmt *stmt) override;
  void visitDeclStmt(const DeclStmt *stmt) override;
  void visitBreakStmt(const BreakStmt *stmt) override;
  void visitContinueStmt(const ContinueStmt *stmt) override;

  // Top-Level Declarations
  void visitModuleDecl(const ModuleDecl *decl) override;
  void visitClassDecl(const ClassDecl *decl) override;
  void visitGenericDecl(const GenericDecl *decl) override;
  void visitImportDecl(const ImportDecl *decl) override;
  void visitEnumDecl(const EnumDecl *decl) override;

  // Structural visitors
  void visitPrimitiveType(const PrimitiveType *type) override;
  void visitPointerType(const PointerType *type) override;
  void visitReferenceType(const ReferenceType *type) override;
  void visitArrayType(const ArrayType *type) override;
  void visitMapType(const MapType *type) override;
  void visitFunctionType(const FunctionType *type) override;
  void visitNullableType(const NullableType *type) override;
  void visitAnyType(const AnyType *type) override;
  void visitNamedType(const NamedType *type) override;
  void visitLockType(const LockType *type) override;
    void visitViewType(const ViewType *type) override;
    void visitMutType(const MutType *type) override;
};

} // namespace moksha
