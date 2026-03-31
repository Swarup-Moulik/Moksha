#pragma once

#include "moksha/AST/ASTContext.h"
#include "moksha/AST/ASTVisitor.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/Support/Diagnostics.h"
#include "moksha/Support/SourceLocation.h"
#include "llvm/ADT/StringRef.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace moksha {

/// Represents a single macro parameter.
struct MacroParam {
  std::string name;
  SourceLocation loc;

  MacroParam(llvm::StringRef n, SourceLocation l) : name(n.str()), loc(l) {}
};

/// Base class for all macro definitions.
class Macro {
public:
  enum class Kind {
    ObjectLike,  // Simple constant-like macros
    FunctionLike // Parameterized macros
  };

  Macro(Kind k, llvm::StringRef n, SourceLocation l)
      : kind(k), name(n.str()), loc(l) {}
  virtual ~Macro() = default;

  /// Expands the macro into a sequence of AST statements or expressions.
  virtual std::vector<std::unique_ptr<Stmt>>
  expand(const std::vector<std::unique_ptr<Expr>> &args,
         ASTContext &ctx) const = 0;

  Kind getKind() const { return kind; }
  llvm::StringRef getName() const { return name; }
  SourceLocation getLoc() const { return loc; }

protected:
  Kind kind;
  std::string name;
  SourceLocation loc;
};

/// Object-like macro (constant replacement)
class ObjectMacro : public Macro {
public:
  ObjectMacro(llvm::StringRef n, std::unique_ptr<Expr> val, SourceLocation l)
      : Macro(Kind::ObjectLike, n, l), value(std::move(val)) {}
  const Expr *getValue() const { return value.get(); }

  std::vector<std::unique_ptr<Stmt>>
  expand(const std::vector<std::unique_ptr<Expr>> &args,
         ASTContext &ctx) const override;

private:
  std::unique_ptr<Expr> value;
};

/// Function-like macro (parameterized)
class FunctionMacro : public Macro {
public:
  FunctionMacro(llvm::StringRef n, std::vector<MacroParam> params,
                std::vector<std::unique_ptr<Stmt>> body, SourceLocation l)
      : Macro(Kind::FunctionLike, n, l), params(std::move(params)),
        body(std::move(body)) {}

  const std::vector<MacroParam> &getParams() const { return params; }

  std::vector<std::unique_ptr<Stmt>>
  expand(const std::vector<std::unique_ptr<Expr>> &args,
         ASTContext &ctx) const override;

private:
  std::vector<MacroParam> params;
  std::vector<std::unique_ptr<Stmt>> body;
};

/// Macro Table: stores all defined macros for lookup
class MacroTable {
public:
  void addMacro(std::unique_ptr<Macro> macro) {
    table[macro->getName().str()] = std::move(macro);
  }

  const Macro *lookup(llvm::StringRef name) const {
    auto it = table.find(name.str());
    if (it != table.end())
      return it->second.get();
    return nullptr;
  }

  bool contains(llvm::StringRef name) const {
    return table.count(name.str()) > 0;
  }

private:
  std::unordered_map<std::string, std::unique_ptr<Macro>> table;
};

class MacroExpander : public ASTVisitor {
  ASTContext &ctx;
  DiagnosticEngine &Diags;
  MacroTable macros;

public:
  MacroExpander(ASTContext &ctx, DiagnosticEngine &Diags)
      : ctx(ctx), Diags(Diags) {}

  void visitModuleDecl(const ModuleDecl *decl) override;
  void visitBlockStmt(const BlockStmt *stmt) override;
  void visitFunctionDecl(const FunctionDecl *decl) override;
  void visitIfStmt(const IfStmt *stmt) override;
  void visitWhileStmt(const WhileStmt *stmt) override;
  void visitDoWhileStmt(const DoWhileStmt *stmt) override;
  void visitForStmt(const ForStmt *stmt) override;
  void visitForInStmt(const ForInStmt *stmt) override;
  void visitSwitchStmt(const SwitchStmt *stmt) override;
  void visitTryCatchStmt(const TryCatchStmt *stmt) override;
  void visitClassDecl(const ClassDecl *decl) override;
  void visitGenericDecl(const GenericDecl *decl) override;

  // --- Empty Stubs for ASTVisitor Pure Virtuals ---
  void visitPrimitiveType(const PrimitiveType *) override {}
  void visitPointerType(const PointerType *) override {}
  void visitReferenceType(const ReferenceType *) override {}
  void visitArrayType(const ArrayType *) override {}
  void visitSliceType(const SliceType *type) override;
  void visitMapType(const MapType *) override {}
  void visitFunctionType(const FunctionType *) override {}
  void visitNamedType(const NamedType *) override {}
  void visitNullableType(const NullableType *) override {}
  void visitAnyType(const AnyType *) override {}
  void visitLockType(const LockType *) override {}
  void visitViewType(const ViewType *) override {}
  void visitMutType(const MutType *) override {}
  void visitEnumType(const EnumType *) override {}
  void visitNullType(const NullType *) override {}
  void visitVolatileType(const VolatileType *type) override;
  void visitConstType(const ConstType *type) override;
  void visitWeakType(const WeakType *type) override {}
  void visitDecimalType(const DecimalType *) override {}
  void visitClosureType(const ClosureType *type) override;

  void visitIntegerLiteral(const IntegerLiteral *) override {}
  void visitFloatLiteral(const FloatLiteral *) override {}
  void visitDecimalLiteral(const DecimalLiteral *) override {}
  void visitStringLiteral(const StringLiteral *) override {}
  void visitBoolLiteral(const BoolLiteral *) override {}
  void visitNullLiteral(const NullLiteral *) override {}
  void visitCharLiteral(const CharLiteral *) override {}
  void visitArrayLiteral(const ArrayLiteral *) override {}
  void visitMapLiteral(const MapLiteral *) override {}
  void visitBinaryExpr(const BinaryExpr *) override {}
  void visitUnaryExpr(const UnaryExpr *) override {}
  void visitTernaryExpr(const TernaryExpr *) override {}
  void visitCastExpr(const CastExpr *) override {}
  void visitIdentifierExpr(const IdentifierExpr *) override {}
  void visitCallExpr(const CallExpr *) override {}
  void visitMemberExpr(const MemberExpr *) override {}
  void visitIndexExpr(const IndexExpr *) override {}
  void visitLambdaExpr(const LambdaExpr *) override {}
  void visitNewExpr(const NewExpr *) override {}
  void visitTemplateStringExpr(const TemplateStringExpr *) override {}
  void visitThreadExpr(const ThreadExpr *) override {}
  void visitThisExpr(const ThisExpr *) override {}
  void visitSuperExpr(const SuperExpr *) override {}
  void visitAwaitExpr(const AwaitExpr *) override {}
  void visitSizeOfExpr(const SizeOfExpr *) override {}
  void visitInputExpr(const InputExpr *expr) override;

  void visitVariableDecl(const VariableDecl *) override {}
  void visitImportDecl(const ImportDecl *) override {}
  void visitEnumDecl(const EnumDecl *) override {}
  void visitMacroDecl(const MacroDecl *) override {}
  void visitUsingDecl(const UsingDecl *) override {}

  void visitExpressionStmt(const ExpressionStmt *) override {}
  void visitDeclStmt(const DeclStmt *) override {}
  void visitReturnStmt(const ReturnStmt *) override {}
  void visitBreakStmt(const BreakStmt *) override {}
  void visitContinueStmt(const ContinueStmt *) override {}
  void visitDeferStmt(const DeferStmt *) override {}
  void visitUnsafeBlockStmt(const UnsafeBlockStmt *) override {}
  void visitThrowStmt(const ThrowStmt *) override {}
  void visitAsmStmt(const AsmStmt *) override {}
  void visitLockStmt(const LockStmt *stmt) override;
};

} // namespace moksha
