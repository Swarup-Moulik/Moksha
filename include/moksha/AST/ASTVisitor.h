#pragma once
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"

namespace moksha {

class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;

  // --- Types ---
  virtual void visitPrimitiveType(const PrimitiveType *type) = 0;
  virtual void visitPointerType(const PointerType *type) = 0;
  virtual void visitReferenceType(const ReferenceType *type) = 0;
  virtual void visitArrayType(const ArrayType *type) = 0;
  virtual void visitSliceType(const SliceType *type) = 0;
  virtual void visitMapType(const MapType *type) = 0;
  virtual void visitFunctionType(const FunctionType *type) = 0;
  virtual void visitNamedType(const NamedType *type) = 0;
  virtual void visitNullableType(const NullableType *type) = 0;
  virtual void visitAnyType(const AnyType *type) = 0;
  virtual void visitLockType(const LockType *type) = 0;
  virtual void visitViewType(const ViewType *type) = 0;
  virtual void visitMutType(const MutType *type) = 0;
  virtual void visitEnumType(const EnumType *type) = 0;
  virtual void visitNullType(const NullType *type) = 0;
  virtual void visitVolatileType(const VolatileType *type) = 0;
  virtual void visitConstType(const ConstType *type) = 0;
  virtual void visitDecimalType(const DecimalType *type) = 0;
  virtual void visitClosureType(const ClosureType *type) = 0;
  virtual void visitWeakType(const WeakType *type) = 0;

  // --- Expressions ---
  virtual void visitIntegerLiteral(const IntegerLiteral *expr) = 0;
  virtual void visitFloatLiteral(const FloatLiteral *expr) = 0;
  virtual void visitDecimalLiteral(const DecimalLiteral *expr) = 0;
  virtual void visitStringLiteral(const StringLiteral *expr) = 0;
  virtual void visitBoolLiteral(const BoolLiteral *expr) = 0;
  virtual void visitNullLiteral(const NullLiteral *expr) = 0;
  virtual void visitCharLiteral(const CharLiteral *expr) = 0;
  virtual void visitArrayLiteral(const ArrayLiteral *expr) = 0;
  virtual void visitMapLiteral(const MapLiteral *expr) = 0;
  virtual void visitBinaryExpr(const BinaryExpr *expr) = 0;
  virtual void visitUnaryExpr(const UnaryExpr *expr) = 0;
  virtual void visitTernaryExpr(const TernaryExpr *expr) = 0;
  virtual void visitCastExpr(const CastExpr *expr) = 0;
  virtual void visitIdentifierExpr(const IdentifierExpr *expr) = 0;
  virtual void visitCallExpr(const CallExpr *expr) = 0;
  virtual void visitMemberExpr(const MemberExpr *expr) = 0;
  virtual void visitIndexExpr(const IndexExpr *expr) = 0;
  virtual void visitLambdaExpr(const LambdaExpr *expr) = 0;
  virtual void visitNewExpr(const NewExpr *expr) = 0;
  virtual void visitTemplateStringExpr(const TemplateStringExpr *expr) = 0;
  virtual void visitThreadExpr(const ThreadExpr *expr) = 0;
  virtual void visitThisExpr(const ThisExpr *expr) = 0;
  virtual void visitSuperExpr(const SuperExpr *expr) = 0;
  virtual void visitAwaitExpr(const AwaitExpr *expr) = 0;
  virtual void visitSizeOfExpr(const SizeOfExpr *expr) = 0;
  virtual void visitInputExpr(const InputExpr *expr) = 0;

  // --- Declarations ---
  virtual void visitModuleDecl(const ModuleDecl *decl) = 0;
  virtual void visitVariableDecl(const VariableDecl *decl) = 0;
  virtual void visitFunctionDecl(const FunctionDecl *decl) = 0;
  virtual void visitClassDecl(const ClassDecl *decl) = 0;
  virtual void visitGenericDecl(const GenericDecl *decl) = 0;
  virtual void visitImportDecl(const ImportDecl *decl) = 0;
  virtual void visitEnumDecl(const EnumDecl *decl) = 0;
  virtual void visitMacroDecl(const MacroDecl *decl) = 0;
  virtual void visitUsingDecl(const UsingDecl *decl) = 0;

  // --- Statements ---
  virtual void visitBlockStmt(const BlockStmt *stmt) = 0;
  virtual void visitExpressionStmt(const ExpressionStmt *stmt) = 0;
  virtual void visitDeclStmt(const DeclStmt *stmt) = 0;
  virtual void visitReturnStmt(const ReturnStmt *stmt) = 0;
  virtual void visitBreakStmt(const BreakStmt *stmt) = 0;
  virtual void visitContinueStmt(const ContinueStmt *stmt) = 0;
  virtual void visitIfStmt(const IfStmt *stmt) = 0;
  virtual void visitWhileStmt(const WhileStmt *stmt) = 0;
  virtual void visitDoWhileStmt(const DoWhileStmt *stmt) = 0;
  virtual void visitForStmt(const ForStmt *stmt) = 0;
  virtual void visitForInStmt(const ForInStmt *stmt) = 0;
  virtual void visitSwitchStmt(const SwitchStmt *stmt) = 0;
  virtual void visitDeferStmt(const DeferStmt *stmt) = 0;
  virtual void visitUnsafeBlockStmt(const UnsafeBlockStmt *stmt) = 0;
  virtual void visitTryCatchStmt(const TryCatchStmt *stmt) = 0;
  virtual void visitThrowStmt(const ThrowStmt *stmt) = 0;
  virtual void visitAsmStmt(const AsmStmt *stmt) = 0;
  virtual void visitLockStmt(const LockStmt *stmt) = 0;
};

} // namespace moksha
