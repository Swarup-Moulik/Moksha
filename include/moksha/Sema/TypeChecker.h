#pragma once

#include "moksha/AST/ASTVisitor.h"
#include "moksha/AST/Type.h"
#include "moksha/Sema/GenericResolver.h"
#include "moksha/Sema/SymbolTable.h"
#include "moksha/Support/Diagnostics.h"
#include <functional>
#include <map>
#include <set>
#include <unordered_set>
#include <vector>

namespace moksha {

class ASTContext;
class Decl;
class Stmt;
class ClassDecl;

/** @brief Performs semantic analysis and type checking on the AST. */
class TypeChecker : public ASTVisitor {
public:
  TypeChecker(ASTContext &ctx, SymbolTable &sym, DiagnosticEngine &diags);
  std::function<ModuleDecl *(const std::string &)> loadModuleCallback;

  void markModuleAsLoaded(const std::string &modName) {
    loadedModules.insert(modName);
  }

  void check(Decl *decl);
  void check(Stmt *stmt);

  bool hasErrors() const { return hasError; }

private:
  std::set<const Decl *> initializedVars;
  bool isLHSOfAssignment = false;
  std::unordered_set<std::string> loadedModules;

  // Contexts
  ASTContext &context;
  SymbolTable &symbols;
  DiagnosticEngine &Diags;
  GenericResolver resolver;

  // State
  const Type *lastComputedType;
  const Type *currentExpectedReturnType;
  const Type *expectedExprType = nullptr;
  const ClassDecl *currentClassDecl;
  bool hasError;
  int loopDepth = 0;      // Track if we are inside a loop
  int syncLockDepth = 0;  // Track OS locks
  int asyncLockDepth = 0; // Track Coroutine locks
  bool inStaticContext = false;
  bool inInterruptContext = false;
  bool inConstructorContext = false;
  bool inAsyncContext = false;
  bool inUnsafeContext = false;
  std::string currentModuleName = "";
  std::vector<TypePtr> parkedTypes; // Safely stores inferred generic types
  std::map<std::string, std::vector<std::string>> ambiguousImports;
  bool detectInfiniteSize(const Type *t, std::set<std::string> &visited);
  std::set<std::string> activeLocks;
  std::vector<std::vector<std::set<const Decl *>>> loopBreakStates;

  // Helpers
  bool isCompatible(const Type *expected, const Type *actual);
  bool isCastAllowed(const Type *src, const Type *dst);

  const Type *getCommonSuperType(const Type *t1, const Type *t2);
  bool isSubclassOf(const ClassDecl *child, const std::string &parentName);
  bool checkVisibility(const Decl *memberDecl, const ClassDecl *ownerClass,
                       SourceLocation loc);

  std::vector<const ClassDecl *> pendingInstantiations;
  std::vector<const FunctionDecl *> pendingFuncInstantiations;
  void processPendingInstantiations();

  // ASTVisitor Overrides
  void visitIntegerLiteral(const IntegerLiteral *expr) override;
  void visitFloatLiteral(const FloatLiteral *expr) override;
  void visitDecimalLiteral(const DecimalLiteral *expr) override;
  void visitStringLiteral(const StringLiteral *expr) override;
  void visitBoolLiteral(const BoolLiteral *expr) override;
  void visitNullLiteral(const NullLiteral *expr) override;
  void visitCharLiteral(const CharLiteral *expr) override;
  void visitArrayLiteral(const ArrayLiteral *expr) override;
  void visitMapLiteral(const MapLiteral *expr) override;
  void visitIdentifierExpr(const IdentifierExpr *expr) override;
  void visitBinaryExpr(const BinaryExpr *expr) override;
  void visitUnaryExpr(const UnaryExpr *expr) override;
  void visitCallExpr(const CallExpr *expr) override;
  void visitIndexExpr(const IndexExpr *expr) override;
  void visitMemberExpr(const MemberExpr *expr) override;
  void visitCastExpr(const CastExpr *expr) override;
  void visitBitcastExpr(const BitcastExpr *) override;
  void visitTernaryExpr(const TernaryExpr *expr) override;
  void visitNewExpr(const NewExpr *expr) override;
  void visitLambdaExpr(const LambdaExpr *expr) override;
  void visitTemplateStringExpr(const TemplateStringExpr *expr) override;
  void visitThreadExpr(const ThreadExpr *expr) override;
  void visitThisExpr(const ThisExpr *expr) override;
  void visitSuperExpr(const SuperExpr *expr) override;
  void visitAwaitExpr(const AwaitExpr *expr) override;
  void visitSizeOfExpr(const SizeOfExpr *expr) override;
  void visitInputExpr(const InputExpr *expr) override;
  void visitAsmExpr(const AsmExpr *expr) override;
  void visitClosureType(const ClosureType *type) override;

  // Statements
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
  void visitThrowStmt(const ThrowStmt *stmt) override;
  void visitExpressionStmt(const ExpressionStmt *stmt) override;
  void visitDeclStmt(const DeclStmt *stmt) override;
  void visitBreakStmt(const BreakStmt *stmt) override;
  void visitContinueStmt(const ContinueStmt *stmt) override;
  void visitLockStmt(const LockStmt *stmt) override;

  // Top-Level Declarations
  void visitModuleDecl(const ModuleDecl *decl) override;
  void visitClassDecl(const ClassDecl *decl) override;
  void visitGenericDecl(const GenericDecl *decl) override;
  void visitImportDecl(const ImportDecl *decl) override;
  void visitEnumDecl(const EnumDecl *decl) override;
  void visitVariableDecl(const VariableDecl *decl) override;
  void visitFunctionDecl(const FunctionDecl *decl) override;
  void visitMacroDecl(const MacroDecl *decl) override;
  void visitUsingDecl(const UsingDecl *decl) override;

  // Structural visitors
  void visitPrimitiveType(const PrimitiveType *type) override;
  void visitPointerType(const PointerType *type) override;
  void visitReferenceType(const ReferenceType *type) override;
  void visitArrayType(const ArrayType *type) override;
  void visitSliceType(const SliceType *type) override;
  void visitMapType(const MapType *type) override;
  void visitFunctionType(const FunctionType *type) override;
  void visitNullableType(const NullableType *type) override;
  void visitAnyType(const AnyType *type) override;
  void visitNamedType(const NamedType *type) override;
  void visitLockType(const LockType *type) override;
  void visitViewType(const ViewType *type) override;
  void visitMutType(const MutType *type) override;
  void visitEnumType(const EnumType *type) override;
  void visitNullType(const NullType *type) override;
  void visitVolatileType(const VolatileType *type) override;
  void visitConstType(const ConstType *type) override;
  void visitWeakType(const WeakType *type) override;
  void visitDecimalType(const DecimalType *type) override;
  void visitPromiseType(const PromiseType *type) override;
};

} // namespace moksha
