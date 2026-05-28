#pragma once

#include "moksha/AST/Expr.h"
#include "moksha/HIR/HIRStmt.h"
#include <memory>
#include <string>
#include <vector>

namespace moksha {

class ASTVisitor;

enum class StmtKind {
  // Declarations
  ModuleDecl,
  VariableDecl,
  FunctionDecl,
  ClassDecl,
  GenericDecl,
  ImportDecl,
  EnumDecl,
  MacroDecl,
  UsingDecl,
  // Statements
  BlockStmt,
  ExpressionStmt,
  DeclStmt,
  ReturnStmt,
  BreakStmt,
  ContinueStmt,
  IfStmt,
  WhileStmt,
  DoWhileStmt,
  ForStmt,
  ForInStmt,
  SwitchStmt,
  DeferStmt,
  UnsafeBlockStmt,
  TryCatchStmt,
  ThrowStmt,
  LockStmt
};

enum class Visibility { Default, Public, Private, Protected };

enum class AggregateKind { Class, Struct, Union };

enum class IntrinsicKind {
  None,
  AtomicLoad,
  AtomicStore,
  AtomicAdd,
  AtomicCAS,
  AtomicFence,
  Bswap32,
  Clz
};

// --- AST Node Bases ---

class Decl {
public:
  virtual ~Decl() = default;
  [[nodiscard]] SourceLocation getLoc() const { return loc; }
  [[nodiscard]] const std::string &getName() const { return name; }
  [[nodiscard]] StmtKind getKind() const { return kind; }
  [[nodiscard]] Visibility getVisibility() const { return visibility; }

  virtual void accept(ASTVisitor &v) const = 0;
  virtual std::unique_ptr<Decl> clone() const = 0;

protected:
  Decl(StmtKind kind, std::string name, Visibility visibility,
       SourceLocation loc)
      : kind(kind), name(std::move(name)), visibility(visibility), loc(loc) {}
  StmtKind kind;
  std::string name;
  Visibility visibility;
  SourceLocation loc;
};

using DeclPtr = std::unique_ptr<Decl>;

class Stmt {
public:
  virtual ~Stmt() = default;
  [[nodiscard]] SourceLocation getLoc() const { return loc; }
  [[nodiscard]] StmtKind getKind() const { return kind; }
  virtual void accept(ASTVisitor &v) const = 0;
  virtual std::unique_ptr<Stmt> clone() const = 0;
  template <typename T> std::unique_ptr<T> cloneAs() const {
    return std::unique_ptr<T>(static_cast<T *>(clone().release()));
  }

protected:
  Stmt(StmtKind kind, SourceLocation loc) : kind(kind), loc(loc) {}
  StmtKind kind;
  SourceLocation loc;
};

using StmtPtr = std::unique_ptr<Stmt>;

// --- Declarations ---

class ModuleDecl : public Decl {
public:
  // Constructor matching Parser usage (name, decls, loc)
  ModuleDecl(std::string name, std::vector<DeclPtr> decls, SourceLocation loc)
      : Decl(StmtKind::ModuleDecl, std::move(name), Visibility::Default, loc),
        decls(std::move(decls)) {}

  // Full Constructor
  ModuleDecl(std::string name, std::vector<DeclPtr> decls, Visibility vis,
             SourceLocation loc)
      : Decl(StmtKind::ModuleDecl, std::move(name), vis, loc),
        decls(std::move(decls)) {}

  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const std::vector<DeclPtr> &getDecls() const { return decls; }
  std::vector<DeclPtr> &getDeclsMut() { return decls; }
  void addDeclaration(DeclPtr decl) { decls.push_back(std::move(decl)); }
  std::unique_ptr<Decl> clone() const override;
  static bool classof(const Decl *D) {
    return D->getKind() == StmtKind::ModuleDecl;
  }

private:
  std::vector<DeclPtr> decls;
};

class VariableDecl : public Decl {
public:
  // Constructor matching Parser usage (type, name, init, loc)
  VariableDecl(TypePtr type, std::string name, ExprPtr init, SourceLocation loc)
      : Decl(StmtKind::VariableDecl, std::move(name), Visibility::Default, loc),
        type(std::move(type)), initializer(std::move(init)), isConst(false),
        isStatic(false) {}

  // Full Constructor
  VariableDecl(TypePtr type, std::string name, ExprPtr init, bool isConst,
               bool isStatic, bool isShared, Visibility vis, SourceLocation loc)
      : Decl(StmtKind::VariableDecl, std::move(name), vis, loc),
        type(std::move(type)), initializer(std::move(init)), isConst(isConst),
        isStatic(isStatic), isShared(isShared) {}

  void accept(ASTVisitor &v) const override;
  [[nodiscard]] bool isSharedVar() const { return isShared; }
  void setShared(bool s) { isShared = s; }
  [[nodiscard]] const Type *getType() const { return type.get(); }
  [[nodiscard]] const Expr *getInitializer() const { return initializer.get(); }
  ExprPtr &getInitializerMut() { return initializer; }
  [[nodiscard]] bool isConstVar() const {
    if (isConst)
      return true;
    if (type && (type->is<ViewType>() || type->is<ConstType>()))
      return true;
    return false;
  }
  [[nodiscard]] bool isStaticVar() const { return isStatic; }
  [[nodiscard]] bool hasExplicitType() const { return true; }
  std::unique_ptr<Decl> clone() const override;
  static bool classof(const Decl *D) {
    return D->getKind() == StmtKind::VariableDecl;
  }
  bool isVolatileVar() const { return isVolatile; }
  void setVolatile(bool v) { isVolatile = v; }
  bool isExternVar() const { return isExtern; }
  void setExtern(bool e) { isExtern = e; }
  int getAlignment() const { return alignment; }
  void setAlignment(int a) { alignment = a; }
  const std::string &getSection() const { return section; }
  void setSection(std::string s) { section = std::move(s); }
  bool isUsedVar() const { return isUsed; }
  void setUsed(bool u) { isUsed = u; }
  int getBitWidth() const { return bitWidth; }
  void setBitWidth(int bw) { bitWidth = bw; }
  bool isThreadLocalVar() const { return isThreadLocal; }
  void setThreadLocal(bool tl) { isThreadLocal = tl; }
  bool isWeakVar() const { return isWeakLinkage; }
  void setWeakVar(bool w) { isWeakLinkage = w; }
  bool isBitfield() const { return bitWidth != -1; }
  void setBitfield(uint32_t width) {
    bitfieldFlag = true;
    bitWidth = width;
  }
  void setType(TypePtr newType) { type = std::move(newType); }
  void setName(std::string newName) { name = std::move(newName); }
  uint32_t getBitOffset() const { return bitOffset; }
  void setBitOffset(uint32_t offset) { bitOffset = offset; }
  uint32_t getPhysicalIndex() const { return physicalIndex; }
  void setPhysicalIndex(uint32_t index) { physicalIndex = index; }

private:
  TypePtr type;
  ExprPtr initializer;
  bool isConst;
  bool isStatic;
  bool isShared;
  bool isVolatile = false;
  bool isExtern = false;
  int alignment = 0;
  std::string section = "";
  bool isUsed = false;
  bool bitfieldFlag = false;
  int bitWidth = -1;
  bool isThreadLocal = false;
  bool isWeakLinkage = false;
  uint32_t bitOffset = 0;
  uint32_t physicalIndex = 0;
};

class FunctionDecl : public Decl {
public:
  struct Param {
    std::string name;
    TypePtr type;
    SourceLocation loc;
    ExprPtr defaultValue;
    Param clone() const {
      return Param{name, type->clone(), loc,
                   defaultValue ? defaultValue->clone() : nullptr};
    }
  };

  // Constructor matching Parser usage (ret, name, params, body, async, loc)
  FunctionDecl(std::string name, std::vector<Param> params, TypePtr returnType,
               StmtPtr body, bool isAsync, bool isStatic, bool isVariadic,
               bool isWeak, Visibility vis, SourceLocation loc)
      : Decl(StmtKind::FunctionDecl, std::move(name), vis, loc),
        params(std::move(params)), returnType(std::move(returnType)),
        body(std::move(body)), isAsync(isAsync), isStatic(isStatic),
        isVariadic(isVariadic), isWeak(isWeak) {}

  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const Type *getReturnType() const { return returnType.get(); }
  [[nodiscard]] const std::vector<Param> &getParams() const { return params; }
  [[nodiscard]] const Stmt *getBody() const { return body.get(); }
  [[nodiscard]] bool isAsyncFunc() const { return isAsync; }
  [[nodiscard]] bool isStaticFunc() const { return isStatic; }
  [[nodiscard]] bool isWeakFunc() const { return isWeak; }
  bool isVariadicFunc() const { return isVariadic; }
  std::unique_ptr<Decl> clone() const override;
  static bool classof(const Decl *D) {
    return D->getKind() == StmtKind::FunctionDecl;
  }
  [[nodiscard]] bool isBuiltinFunc() const { return isBuiltin; }
  void setBuiltin(bool builtin) { isBuiltin = builtin; }
  [[nodiscard]] IntrinsicKind getIntrinsicKind() const { return intrinsicKind; }
  void setIntrinsicKind(IntrinsicKind kind) { intrinsicKind = kind; }
  bool isExternFunc() const { return isExtern; }
  void setExtern(bool e) { isExtern = e; }
  const std::string &getExternLinkage() const { return externLinkage; }
  void setExternLinkage(std::string l) { externLinkage = std::move(l); }
  bool isInterruptFunc() const { return isInterrupt; }
  void setInterrupt(bool i) { isInterrupt = i; }
  bool isNakedFunc() const { return isNaked; }
  void setNaked(bool n) { isNaked = n; }
  bool isNoReturnFunc() const { return isNoReturn; }
  void setNoReturn(bool nr) { isNoReturn = nr; }
  bool isNoInlineFunc() const { return isNoInline; }
  void setNoInline(bool ni) { isNoInline = ni; }
  bool isInlineFunc() const { return isInline; }
  void setInline(bool i) { isInline = i; }
  bool isPureFunc() const { return isPure; }
  void setPure(bool p) { isPure = p; }
  bool isColdFunc() const { return isCold; }
  void setCold(bool c) { isCold = c; }
  bool isUsedFunc() const { return isUsed; }
  void setUsed(bool u) { isUsed = u; }
  const std::string &getSection() const { return section; }
  void setSection(std::string s) { section = std::move(s); }
  const std::string &getABI() const { return abi; }
  void setABI(std::string abiStr) { abi = std::move(abiStr); }
  bool isVirtualFunc() const { return isVirtual; }
  void setVirtual(bool v) { isVirtual = v; }
  bool isOverrideFunc() const { return isOverride; }
  void setOverride(bool o) { isOverride = o; }
  int getVTableIndex() const { return vtableIndex; }
  void setVTableIndex(int idx) { vtableIndex = idx; }
  void setReturnType(TypePtr newType) { returnType = std::move(newType); }
  void setName(std::string newName) { name = std::move(newName); }
  bool isViewMethod() const { return isView; }
  void setViewMethod(bool v) { isView = v; }
  bool isUnsafeFunc() const { return isUnsafe; }
  void setUnsafe(bool u) { isUnsafe = u; }

private:
  std::vector<Param> params;
  TypePtr returnType;
  StmtPtr body;
  bool isAsync;
  bool isStatic;
  bool isVariadic;
  bool isWeak;
  bool isBuiltin = false;
  IntrinsicKind intrinsicKind = IntrinsicKind::None;
  bool isExtern = false;
  std::string externLinkage = "";
  bool isInterrupt = false;
  bool isNaked = false;
  bool isNoReturn = false;
  bool isNoInline = false;
  bool isInline = false;
  bool isPure = false;
  bool isCold = false;
  bool isUsed = false;
  std::string section = "";
  std::string abi = "C";
  bool isVirtual = false;
  bool isOverride = false;
  int vtableIndex = -1;
  bool isView = false;
  bool isUnsafe = false;
};

class ClassDecl : public Decl {
public:
  // Updated constructor with aggKind (7 arguments total)
  ClassDecl(std::string name, std::vector<std::string> parentNames,
            std::vector<DeclPtr> members, bool isRef, AggregateKind aggKind,
            Visibility vis, SourceLocation loc)
      : Decl(StmtKind::ClassDecl, std::move(name), vis, loc),
        parentNames(std::move(parentNames)), members(std::move(members)),
        isRef(isRef), aggKind(aggKind) {}

  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const std::vector<DeclPtr> &getMembers() const {
    return members;
  }
  [[nodiscard]] bool isReferenceType() const { return isRef; }
  [[nodiscard]] const std::vector<std::string> &getParentNames() const {
    return parentNames;
  }
  [[nodiscard]] AggregateKind getAggregateKind() const { return aggKind; }
  std::unique_ptr<Decl> clone() const override;
  static bool classof(const Decl *D) {
    return D->getKind() == StmtKind::ClassDecl;
  }

  bool isPackedClass() const { return isPacked; }
  void setPacked(bool p) { isPacked = p; }
  int getAlignment() const { return alignment; }
  void setAlignment(int a) { alignment = a; }
  const std::string &getSection() const { return section; }
  void setSection(std::string s) { section = std::move(s); }
  bool hasVTable() const { return hasVTableFlag; }
  void setHasVTable(bool v) { hasVTableFlag = v; }
  void setName(std::string newName) { name = std::move(newName); }

private:
  std::vector<std::string> parentNames;
  std::vector<DeclPtr> members;
  bool isRef;
  AggregateKind aggKind;
  bool isPacked = false;
  int alignment = 0;
  std::string section = "";
  bool hasVTableFlag = false;
};

class GenericDecl : public Decl {
public:
  // Strongly typed parameter to hold lifetime constraints
  struct GenericParam {
    std::string name;
    bool isShared;
    SourceLocation loc;
  };

  GenericDecl(std::string name, std::vector<GenericParam> typeParams,
              DeclPtr innerDecl, SourceLocation loc)
      : Decl(StmtKind::GenericDecl, std::move(name), Visibility::Default, loc),
        typeParams(std::move(typeParams)), innerDecl(std::move(innerDecl)) {}

  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const std::vector<GenericParam> &getTypeParams() const {
    return typeParams;
  }

  [[nodiscard]] const Decl *getInnerDecl() const { return innerDecl.get(); }
  std::unique_ptr<Decl> clone() const override;
  static bool classof(const Decl *D) {
    return D->getKind() == StmtKind::GenericDecl;
  }

private:
  std::vector<GenericParam> typeParams;
  DeclPtr innerDecl;
};

class ImportDecl : public Decl {
public:
  // Constructor matching Parser usage (name, symbols, loc)
  ImportDecl(std::string moduleName, std::vector<std::string> symbols,
             SourceLocation loc)
      : Decl(StmtKind::ImportDecl, std::move(moduleName), Visibility::Default,
             loc),
        symbols(std::move(symbols)) {}

  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const std::string &getModuleName() const { return name; }
  [[nodiscard]] const std::vector<std::string> &getSymbols() const {
    return symbols;
  }
  std::unique_ptr<Decl> clone() const override;
  static bool classof(const Decl *D) {
    return D->getKind() == StmtKind::ImportDecl;
  }

private:
  std::vector<std::string> symbols;
};

class EnumDecl : public Decl {
public:
  struct Case {
    std::string name;
    ExprPtr value;
    Case clone() const { return Case{name, value ? value->clone() : nullptr}; }
  };

  // Constructor matching Parser usage (name, cases, loc)
  EnumDecl(std::string name, std::vector<Case> cases, SourceLocation loc)
      : Decl(StmtKind::EnumDecl, std::move(name), Visibility::Default, loc),
        cases(std::move(cases)) {}

  EnumDecl(std::string name, std::vector<Case> cases, Visibility vis,
           SourceLocation loc)
      : Decl(StmtKind::EnumDecl, std::move(name), vis, loc),
        cases(std::move(cases)) {}

  void accept(ASTVisitor &v) const override;
  const std::vector<Case> &getCases() const { return cases; }
  std::unique_ptr<Decl> clone() const override;
  static bool classof(const Decl *D) {
    return D->getKind() == StmtKind::EnumDecl;
  }

private:
  std::vector<Case> cases;
};

class MacroDecl : public Decl {
public:
  MacroDecl(std::string name, std::vector<std::string> params,
            std::vector<StmtPtr> body, bool isFunction, SourceLocation loc)
      : Decl(StmtKind::MacroDecl, std::move(name), Visibility::Default, loc),
        params(std::move(params)), body(std::move(body)),
        isFunction(isFunction) {}

  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const std::vector<std::string> &getParams() const {
    return params;
  }
  [[nodiscard]] const std::vector<StmtPtr> &getBody() const { return body; }
  [[nodiscard]] bool isFunctionMacro() const { return isFunction; }
  std::unique_ptr<Decl> clone() const override;
  static bool classof(const Decl *D) {
    return D->getKind() == StmtKind::MacroDecl;
  }

private:
  std::vector<std::string> params;
  std::vector<StmtPtr> body;
  bool isFunction;
};

class UsingDecl : public Decl {
public:
  UsingDecl(std::string name, TypePtr targetType, SourceLocation loc)
      : Decl(StmtKind::UsingDecl, std::move(name), Visibility::Default, loc),
        targetType(std::move(targetType)) {}
  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const Type *getTargetType() const { return targetType.get(); }
  std::unique_ptr<Decl> clone() const override;
  static bool classof(const Decl *D) {
    return D->getKind() == StmtKind::UsingDecl;
  }

private:
  TypePtr targetType;
};

// --- Statements (Same as before, included for completeness) ---

class BlockStmt : public Stmt {
public:
  BlockStmt(std::vector<StmtPtr> stmts, SourceLocation loc)
      : Stmt(StmtKind::BlockStmt, loc), statements(std::move(stmts)) {}
  BlockStmt(StmtKind kind, std::vector<StmtPtr> stmts, SourceLocation loc)
      : Stmt(kind, loc), statements(std::move(stmts)) {}

  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const std::vector<StmtPtr> &getStatements() const {
    return statements;
  }
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::BlockStmt ||
           S->getKind() == StmtKind::UnsafeBlockStmt;
  }

private:
  std::vector<StmtPtr> statements;
};

class ExpressionStmt : public Stmt {
public:
  ExpressionStmt(ExprPtr expr, SourceLocation loc)
      : Stmt(StmtKind::ExpressionStmt, loc), expression(std::move(expr)) {}
  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const Expr *getExpr() const { return expression.get(); }
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::ExpressionStmt;
  }

private:
  ExprPtr expression;
};

class DeclStmt : public Stmt {
public:
  DeclStmt(DeclPtr decl, SourceLocation loc)
      : Stmt(StmtKind::DeclStmt, loc), decl(std::move(decl)) {}
  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const Decl *getDecl() const { return decl.get(); }
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::DeclStmt;
  }

private:
  DeclPtr decl;
};

class ReturnStmt : public Stmt {
public:
  ReturnStmt(ExprPtr value, SourceLocation loc)
      : Stmt(StmtKind::ReturnStmt, loc), returnValue(std::move(value)) {}
  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const Expr *getReturnValue() const { return returnValue.get(); }
  ExprPtr &getReturnValueMut() { return returnValue; }
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::ReturnStmt;
  }

private:
  ExprPtr returnValue;
};

class BreakStmt : public Stmt {
public:
  explicit BreakStmt(SourceLocation loc) : Stmt(StmtKind::BreakStmt, loc) {}
  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::BreakStmt;
  }
};

class ContinueStmt : public Stmt {
public:
  explicit ContinueStmt(SourceLocation loc)
      : Stmt(StmtKind::ContinueStmt, loc) {}
  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::ContinueStmt;
  }
};

class IfStmt : public Stmt {
public:
  IfStmt(ExprPtr cond, StmtPtr thenBranch, StmtPtr elseBranch,
         SourceLocation loc)
      : Stmt(StmtKind::IfStmt, loc), condition(std::move(cond)),
        thenStmt(std::move(thenBranch)), elseStmt(std::move(elseBranch)) {}
  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const Expr *getCondition() const { return condition.get(); }
  [[nodiscard]] const Stmt *getThenStmt() const { return thenStmt.get(); }
  [[nodiscard]] const Stmt *getElseStmt() const { return elseStmt.get(); }
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::IfStmt;
  }

private:
  ExprPtr condition;
  StmtPtr thenStmt;
  StmtPtr elseStmt;
};

class WhileStmt : public Stmt {
public:
  WhileStmt(ExprPtr cond, StmtPtr body, SourceLocation loc)
      : Stmt(StmtKind::WhileStmt, loc), condition(std::move(cond)),
        body(std::move(body)) {}
  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const Expr *getCondition() const { return condition.get(); }
  [[nodiscard]] const Stmt *getBody() const { return body.get(); }
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::WhileStmt;
  }

private:
  ExprPtr condition;
  StmtPtr body;
};

class DoWhileStmt : public Stmt {
public:
  DoWhileStmt(StmtPtr body, ExprPtr cond, SourceLocation loc)
      : Stmt(StmtKind::DoWhileStmt, loc), body(std::move(body)),
        condition(std::move(cond)) {}
  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const Stmt *getBody() const { return body.get(); }
  [[nodiscard]] const Expr *getCondition() const { return condition.get(); }
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::DoWhileStmt;
  }

private:
  StmtPtr body;
  ExprPtr condition;
};

class ForStmt : public Stmt {
public:
  ForStmt(StmtPtr init, ExprPtr cond, ExprPtr inc, StmtPtr body,
          SourceLocation loc)
      : Stmt(StmtKind::ForStmt, loc), init(std::move(init)),
        condition(std::move(cond)), increment(std::move(inc)),
        body(std::move(body)) {}
  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const Stmt *getInit() const { return init.get(); }
  [[nodiscard]] const Expr *getCondition() const { return condition.get(); }
  [[nodiscard]] const Expr *getIncrement() const { return increment.get(); }
  [[nodiscard]] const Stmt *getBody() const { return body.get(); }
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::ForStmt;
  }

private:
  StmtPtr init;
  ExprPtr condition;
  ExprPtr increment;
  StmtPtr body;
};

class ForInStmt : public Stmt {
public:
  ForInStmt(DeclPtr var, DeclPtr indexVar, ExprPtr collection, StmtPtr body,
            SourceLocation loc)
      : Stmt(StmtKind::ForInStmt, loc), variable(std::move(var)),
        indexVariable(std::move(indexVar)), collection(std::move(collection)),
        body(std::move(body)) {}
  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const Decl *getVariable() const { return variable.get(); }
  [[nodiscard]] const Decl *getIndexVariable() const {
    return indexVariable.get();
  }
  [[nodiscard]] const Expr *getCollection() const { return collection.get(); }
  [[nodiscard]] const Stmt *getBody() const { return body.get(); }
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::ForInStmt;
  }

private:
  DeclPtr variable;
  DeclPtr indexVariable;
  ExprPtr collection;
  StmtPtr body;
};

class SwitchCase {
public:
  SwitchCase(std::vector<ExprPtr> vals, std::unique_ptr<BlockStmt> b, bool def)
      : values(std::move(vals)), body(std::move(b)), isDefault(def) {}
  SwitchCase(SwitchCase &&) = default;
  SwitchCase &operator=(SwitchCase &&) = default;
  [[nodiscard]] const std::vector<ExprPtr> &getValues() const { return values; }
  [[nodiscard]] const BlockStmt *getBody() const { return body.get(); }
  [[nodiscard]] bool isDefaultCase() const { return isDefault; }
  SwitchCase clone() const;

private:
  std::vector<ExprPtr> values;
  std::unique_ptr<BlockStmt> body;
  bool isDefault;
};

class SwitchStmt : public Stmt {
public:
  SwitchStmt(ExprPtr cond, std::vector<SwitchCase> cases, SourceLocation loc)
      : Stmt(StmtKind::SwitchStmt, loc), condition(std::move(cond)),
        cases(std::move(cases)) {}
  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const Expr *getCondition() const { return condition.get(); }
  [[nodiscard]] const std::vector<SwitchCase> &getCases() const {
    return cases;
  }
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::SwitchStmt;
  }

private:
  ExprPtr condition;
  std::vector<SwitchCase> cases;
};

class DeferStmt : public Stmt {
public:
  DeferStmt(StmtPtr stmt, SourceLocation loc)
      : Stmt(StmtKind::DeferStmt, loc), deferredStmt(std::move(stmt)) {}
  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const Stmt *getDeferredStmt() const {
    return deferredStmt.get();
  }
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::DeferStmt;
  }

private:
  StmtPtr deferredStmt;
};

class UnsafeBlockStmt : public BlockStmt {
public:
  UnsafeBlockStmt(std::vector<StmtPtr> stmts, SourceLocation loc)
      : BlockStmt(StmtKind::UnsafeBlockStmt, std::move(stmts), loc) {}
  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::UnsafeBlockStmt;
  }
};

struct CatchClause {
  DeclPtr var;
  StmtPtr body;
  SourceLocation loc;

  CatchClause(DeclPtr var, StmtPtr body, SourceLocation loc)
      : var(std::move(var)), body(std::move(body)), loc(loc) {}

  CatchClause clone() const {
    return CatchClause(var ? var->clone() : nullptr,
                       body ? body->clone() : nullptr, loc);
  }
};

class TryCatchStmt : public Stmt {
public:
  TryCatchStmt(StmtPtr tryBlock, std::vector<CatchClause> catchClauses,
               StmtPtr finallyBlock, SourceLocation loc)
      : Stmt(StmtKind::TryCatchStmt, loc), tryBody(std::move(tryBlock)),
        catches(std::move(catchClauses)), finallyBody(std::move(finallyBlock)) {
  }

  void accept(ASTVisitor &v) const override;

  [[nodiscard]] const Stmt *getTryBody() const { return tryBody.get(); }
  [[nodiscard]] const std::vector<CatchClause> &getCatches() const {
    return catches;
  }
  [[nodiscard]] const Stmt *getFinallyBody() const { return finallyBody.get(); }

  std::unique_ptr<Stmt> clone() const override {
    std::vector<CatchClause> clonedCatches;
    for (const auto &c : catches)
      clonedCatches.push_back(c.clone());
    return std::make_unique<TryCatchStmt>(
        tryBody ? tryBody->clone() : nullptr, std::move(clonedCatches),
        finallyBody ? finallyBody->clone() : nullptr, loc);
  }

  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::TryCatchStmt;
  }

private:
  StmtPtr tryBody;
  std::vector<CatchClause> catches;
  StmtPtr finallyBody;
};

class ThrowStmt : public Stmt {
public:
  ThrowStmt(ExprPtr expr, SourceLocation loc)
      : Stmt(StmtKind::ThrowStmt, loc), expression(std::move(expr)) {}
  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const Expr *getExpr() const { return expression.get(); }
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::ThrowStmt;
  }

private:
  ExprPtr expression;
};

class LockStmt : public Stmt {
public:
  LockStmt(ExprPtr target, StmtPtr body, bool isAsync, SourceLocation loc)
      : Stmt(StmtKind::LockStmt, loc), target(std::move(target)),
        body(std::move(body)), isAsync(isAsync) {}

  void accept(ASTVisitor &v) const override;
  [[nodiscard]] const Expr *getTarget() const { return target.get(); }
  [[nodiscard]] const Stmt *getBody() const { return body.get(); }
  [[nodiscard]] bool isAsyncLock() const { return isAsync; }
  std::unique_ptr<Stmt> clone() const override;
  static bool classof(const Stmt *S) {
    return S->getKind() == StmtKind::LockStmt;
  }

private:
  ExprPtr target;
  StmtPtr body;
  bool isAsync;
};

} // namespace moksha
