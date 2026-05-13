#pragma once

#include "moksha/AST/ASTVisitor.h"
#include "moksha/Lexer/Lexer.h"
#include "llvm/Support/raw_ostream.h"
#include <string>

namespace moksha {

class Decl;
class Stmt;
class Expr;
class Type;

/// \brief A visitor that pretty-prints the AST back to source code.
class ASTPrinter : public ASTVisitor {
public:
  explicit ASTPrinter(llvm::raw_ostream &OS);

  // --- Public Entry Points ---
  void print(const Decl *decl);
  void print(const Stmt *stmt);
  void print(const Expr *expr);
  void print(const Type *type);

  // --- Visitor Overrides ---

  // Types
  void visitPrimitiveType(const PrimitiveType *type) override;
  void visitPointerType(const PointerType *type) override;
  void visitArrayType(const ArrayType *type) override;
  void visitSliceType(const SliceType *type) override;
  void visitNamedType(const NamedType *type) override;
  void visitFunctionType(const FunctionType *type) override;
  void visitMapType(const MapType *type) override;
  void visitReferenceType(const ReferenceType *type) override;
  void visitNullableType(const NullableType *type) override;
  void visitAnyType(const AnyType *type) override;
  void visitLockType(const LockType *type) override;
  void visitViewType(const ViewType *type) override;
  void visitMutType(const MutType *type) override;
  void visitEnumType(const EnumType *type) override;
  void visitNullType(const NullType *type) override;
  void visitVolatileType(const VolatileType *type) override;
  void visitConstType(const ConstType *type) override;
  void visitWeakType(const WeakType *type) override;
  void visitDecimalType(const DecimalType *type) override;
  void visitClosureType(const ClosureType *type) override;
  void visitPromiseType(const PromiseType *type) override;

  // Expressions
  void visitIntegerLiteral(const IntegerLiteral *expr) override;
  void visitFloatLiteral(const FloatLiteral *expr) override;
  void visitDecimalLiteral(const DecimalLiteral *expr) override;
  void visitStringLiteral(const StringLiteral *expr) override;
  void visitBoolLiteral(const BoolLiteral *expr) override;
  void visitCharLiteral(const CharLiteral *expr) override;
  void visitNullLiteral(const NullLiteral *expr) override;
  void visitMapLiteral(const MapLiteral *expr) override;
  void visitIdentifierExpr(const IdentifierExpr *expr) override;
  void visitBinaryExpr(const BinaryExpr *expr) override;
  void visitUnaryExpr(const UnaryExpr *expr) override;
  void visitCallExpr(const CallExpr *expr) override;
  void visitMemberExpr(const MemberExpr *expr) override;
  void visitIndexExpr(const IndexExpr *expr) override;
  void visitLambdaExpr(const LambdaExpr *expr) override;
  void visitTernaryExpr(const TernaryExpr *expr) override;
  void visitCastExpr(const CastExpr *expr) override;
  void visitBitcastExpr(const BitcastExpr *expr) override;
  void visitNewExpr(const NewExpr *expr) override;
  void visitTemplateStringExpr(const TemplateStringExpr *expr) override;
  void visitThreadExpr(const ThreadExpr *expr) override;
  void visitArrayLiteral(const ArrayLiteral *expr) override;
  void visitAwaitExpr(const AwaitExpr *expr) override;
  void visitThisExpr(const ThisExpr *expr) override;
  void visitSuperExpr(const SuperExpr *expr) override;
  void visitSizeOfExpr(const SizeOfExpr *expr) override;
  void visitInputExpr(const InputExpr *expr) override;
  void visitAsmExpr(const AsmExpr *expr) override;

  // Statements
  void visitBlockStmt(const BlockStmt *stmt) override;
  void visitExpressionStmt(const ExpressionStmt *stmt) override;
  void visitDeclStmt(const DeclStmt *stmt) override;
  void visitReturnStmt(const ReturnStmt *stmt) override;
  void visitIfStmt(const IfStmt *stmt) override;
  void visitWhileStmt(const WhileStmt *stmt) override;
  void visitDoWhileStmt(const DoWhileStmt *stmt) override;
  void visitForStmt(const ForStmt *stmt) override;
  void visitForInStmt(const ForInStmt *stmt) override;
  void visitSwitchStmt(const SwitchStmt *stmt) override;
  void visitBreakStmt(const BreakStmt *stmt) override;
  void visitContinueStmt(const ContinueStmt *stmt) override;
  void visitDeferStmt(const DeferStmt *stmt) override;
  void visitUnsafeBlockStmt(const UnsafeBlockStmt *stmt) override;
  void visitTryCatchStmt(const TryCatchStmt *stmt) override;
  void visitThrowStmt(const ThrowStmt *stmt) override;
  void visitLockStmt(const LockStmt *stmt) override;

  // Declarations
  void visitModuleDecl(const ModuleDecl *decl) override;
  void visitFunctionDecl(const FunctionDecl *decl) override;
  void visitVariableDecl(const VariableDecl *decl) override;
  void visitClassDecl(const ClassDecl *decl) override;
  void visitGenericDecl(const GenericDecl *decl) override;
  void visitImportDecl(const ImportDecl *decl) override;
  void visitEnumDecl(const EnumDecl *decl) override;
  void visitMacroDecl(const MacroDecl *decl) override;
  void visitUsingDecl(const UsingDecl *decl) override;

private:
  llvm::raw_ostream &OS;
  int indentLevel = 0;

  void printIndent();
  std::string tokenToString(TokenKind kind);
  void printVisibility(Visibility v);
};

/// \brief Helper function to print an AST declaration to a stream.
void printAST(const Decl *decl, llvm::raw_ostream &OS);

} // namespace moksha
