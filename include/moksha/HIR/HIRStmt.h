#pragma once

#include "moksha/HIR/HIRStmt.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRValue.h"
#include "moksha/Support/SourceLocation.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace moksha {
namespace hir {

// Forward declarations
class HIRFunction;
class HIRStmt;
class HIRExpr;
class HIRVisitor;
class ConstHIRVisitor;
class BlockStmt;
class HIRVarDeclStmt;

// Pointer alias
using HIRStmtPtr = std::unique_ptr<HIRStmt>;

// ============================================================================
// [Base Class] HIRStmt
// ============================================================================
class HIRStmt {
public:
  enum class Kind {
    Block,
    UnsafeBlock,
    Lock,
    ExprStmt,
    Return,
    If,
    Switch,
    While,
    DoWhile,
    For,
    ForIn,
    Break,
    Continue,
    Defer,
    TryCatch,
    Throw,
    VarDecl,
    Unknown
  };

  HIRStmt(Kind k, SourceLocation loc) : kind(k), loc(loc) {
    assert(k != Kind::Unknown && "Invalid HIRStmt kind construction");
  }

  virtual ~HIRStmt() = default;

  HIRStmt(const HIRStmt &) = delete;
  HIRStmt &operator=(const HIRStmt &) = delete;

  [[nodiscard]] Kind getKind() const { return kind; }
  [[nodiscard]] SourceLocation getLoc() const { return loc; }

  void dump(int indent = 0) const;
  virtual void dump(llvm::raw_ostream &os, int indent = 0) const = 0;

  virtual void accept(HIRVisitor &v) = 0;
  virtual void accept(ConstHIRVisitor &v) const = 0;

protected:
  void printIndent(llvm::raw_ostream &os, int indent) const;
  void printLabel(llvm::raw_ostream &os, int indent, const char *label) const;

  Kind kind;
  SourceLocation loc;
};

// ============================================================================
// [Blocks]
// ============================================================================

class BlockStmt : public HIRStmt {
public:
  // Standard constructor for normal blocks
  BlockStmt(std::vector<HIRStmtPtr> stmts, SourceLocation loc);
  ~BlockStmt() override; // Defined in .cpp

  [[nodiscard]] const std::vector<HIRStmtPtr> &getStatements() const;

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) { return S->getKind() == Kind::Block; }

protected:
  BlockStmt(Kind k, std::vector<HIRStmtPtr> stmts, SourceLocation loc);
  std::vector<HIRStmtPtr> statements;
};

class UnsafeBlockStmt : public BlockStmt {
public:
  UnsafeBlockStmt(std::vector<HIRStmtPtr> stmts, SourceLocation loc);
  ~UnsafeBlockStmt() override; // Defined in .cpp

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) {
    return S->getKind() == Kind::UnsafeBlock;
  }
};

class LockStmt : public HIRStmt {
public:
  LockStmt(std::unique_ptr<HIRExpr> mutex, std::unique_ptr<HIRStmt> body,
           bool isAsync, SourceLocation loc);

  ~LockStmt() override;
  [[nodiscard]] const HIRExpr *getMutex() const { return mutex.get(); }
  [[nodiscard]] const HIRStmt *getBody() const { return body.get(); }
  [[nodiscard]] bool isAsyncLock() const { return isAsync; }
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) { return S->getKind() == Kind::Lock; }

private:
  std::unique_ptr<HIRExpr> mutex;
  HIRStmtPtr body;
  bool isAsync;
};

// ============================================================================
// [Simple Statements]
// ============================================================================

class ExprStmt : public HIRStmt {
public:
  ExprStmt(std::unique_ptr<HIRExpr> expr, SourceLocation loc);
  ~ExprStmt() override; // Defined in .cpp

  [[nodiscard]] const HIRExpr *getExpr() const;

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) {
    return S->getKind() == Kind::ExprStmt;
  }

private:
  std::unique_ptr<HIRExpr> expr;
};

class ReturnStmt : public HIRStmt {
public:
  ReturnStmt(std::unique_ptr<HIRExpr> value, SourceLocation loc);
  ~ReturnStmt() override; // Defined in .cpp

  [[nodiscard]] const HIRExpr *getReturnValue() const;

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) { return S->getKind() == Kind::Return; }

private:
  std::unique_ptr<HIRExpr> returnValue;
};

// ============================================================================
// [Control Flow]
// ============================================================================

class IfStmt : public HIRStmt {
public:
  IfStmt(std::unique_ptr<HIRExpr> cond, HIRStmtPtr thenBr, HIRStmtPtr elseBr,
         SourceLocation loc);
  ~IfStmt() override; // Defined in .cpp

  [[nodiscard]] const HIRExpr *getCondition() const;
  [[nodiscard]] const HIRStmt *getThenBranch() const;
  [[nodiscard]] const HIRStmt *getElseBranch() const;

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) { return S->getKind() == Kind::If; }

private:
  std::unique_ptr<HIRExpr> condition;
  HIRStmtPtr thenBranch;
  HIRStmtPtr elseBranch;
};

class WhileStmt : public HIRStmt {
public:
  WhileStmt(std::unique_ptr<HIRExpr> cond, HIRStmtPtr body, SourceLocation loc);
  ~WhileStmt() override; // Defined in .cpp

  [[nodiscard]] const HIRExpr *getCondition() const;
  [[nodiscard]] const HIRStmt *getBody() const;

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) { return S->getKind() == Kind::While; }

private:
  std::unique_ptr<HIRExpr> condition;
  HIRStmtPtr body;
};

class DoWhileStmt : public HIRStmt {
public:
  DoWhileStmt(HIRStmtPtr body, std::unique_ptr<HIRExpr> cond,
              SourceLocation loc);
  ~DoWhileStmt() override; // Defined in .cpp

  [[nodiscard]] const HIRStmt *getBody() const;
  [[nodiscard]] const HIRExpr *getCondition() const;

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) {
    return S->getKind() == Kind::DoWhile;
  }

private:
  HIRStmtPtr body;
  std::unique_ptr<HIRExpr> condition;
};

class ForStmt : public HIRStmt {
public:
  ForStmt(HIRStmtPtr init, std::unique_ptr<HIRExpr> cond,
          std::unique_ptr<HIRExpr> inc, HIRStmtPtr body, SourceLocation loc);
  ~ForStmt() override; // Defined in .cpp

  [[nodiscard]] const HIRStmt *getInit() const { return init.get(); }
  [[nodiscard]] const HIRExpr *getCondition() const { return cond.get(); }
  [[nodiscard]] const HIRExpr *getIncrement() const { return inc.get(); }
  [[nodiscard]] const HIRStmt *getBody() const { return body.get(); }

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) { return S->getKind() == Kind::For; }

private:
  HIRStmtPtr init;
  std::unique_ptr<HIRExpr> cond;
  std::unique_ptr<HIRExpr> inc;
  HIRStmtPtr body;
};

class SwitchCase {
public:
  SwitchCase(std::vector<std::unique_ptr<HIRExpr>> v,
             std::unique_ptr<BlockStmt> b, bool d);
  ~SwitchCase();

  SwitchCase(SwitchCase &&) = default;
  SwitchCase &operator=(SwitchCase &&) = default;
  SwitchCase(const SwitchCase &) = delete;
  SwitchCase &operator=(const SwitchCase &) = delete;

  [[nodiscard]] const std::vector<std::unique_ptr<HIRExpr>> &getValues() const {
    return values;
  }
  [[nodiscard]] const BlockStmt &getBody() const { return *body; }
  [[nodiscard]] bool isDefaultCase() const { return isDefault; }

private:
  std::vector<std::unique_ptr<HIRExpr>> values;
  std::unique_ptr<BlockStmt> body;
  bool isDefault;
};

class SwitchStmt : public HIRStmt {
public:
  SwitchStmt(std::unique_ptr<HIRExpr> cond, std::vector<SwitchCase> cases,
             SourceLocation loc);
  ~SwitchStmt() override; // Defined in .cpp

  [[nodiscard]] const HIRExpr *getCondition() const;
  [[nodiscard]] const std::vector<SwitchCase> &getCases() const;

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) { return S->getKind() == Kind::Switch; }

private:
  std::unique_ptr<HIRExpr> condition;
  std::vector<SwitchCase> cases;
};

// ============================================================================
// [Jumps / Misc]
// ============================================================================

class BreakStmt : public HIRStmt {
public:
  BreakStmt(SourceLocation loc);
  ~BreakStmt() override; // Defined in .cpp

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) { return S->getKind() == Kind::Break; }
};

class ContinueStmt : public HIRStmt {
public:
  ContinueStmt(SourceLocation loc);
  ~ContinueStmt() override; // Defined in .cpp

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) {
    return S->getKind() == Kind::Continue;
  }
};

class DeferStmt : public HIRStmt {
public:
  DeferStmt(HIRStmtPtr stmt, SourceLocation loc);
  ~DeferStmt() override; // Defined in .cpp

  [[nodiscard]] const HIRStmt *getDeferredStmt() const;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) { return S->getKind() == Kind::Defer; }

private:
  HIRStmtPtr deferredStmt;
};

struct HIRCatchClause {
  std::string varName;
  const HIRType *varType;
  std::unique_ptr<HIRStmt> body;
  SourceLocation loc;
};

class TryCatchStmt : public HIRStmt {
public:
  TryCatchStmt(std::unique_ptr<HIRStmt> tryBlock,
               std::vector<HIRCatchClause> catches,
               std::unique_ptr<HIRStmt> finallyBlock, SourceLocation loc)
      : HIRStmt(Kind::TryCatch, loc), tryBlock(std::move(tryBlock)),
        catches(std::move(catches)), finallyBlock(std::move(finallyBlock)) {}

  ~TryCatchStmt() override = default;

  [[nodiscard]] const HIRStmt *getTryBlock() const { return tryBlock.get(); }
  [[nodiscard]] const std::vector<HIRCatchClause> &getCatches() const {
    return catches;
  }
  [[nodiscard]] const HIRStmt *getFinallyBlock() const {
    return finallyBlock.get();
  }
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;

  static bool classof(const HIRStmt *S) {
    return S->getKind() == Kind::TryCatch;
  }

private:
  std::unique_ptr<HIRStmt> tryBlock;
  std::vector<HIRCatchClause> catches;
  std::unique_ptr<HIRStmt> finallyBlock;
};

class HIRThrowStmt : public HIRStmt {
public:
  HIRThrowStmt(std::unique_ptr<HIRExpr> expr, SourceLocation loc);

  [[nodiscard]] const HIRExpr *getExpr() const { return expression.get(); }

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRStmt *S) { return S->getKind() == Kind::Throw; }

private:
  std::unique_ptr<HIRExpr> expression;
};

class HIRVarDeclStmt : public HIRStmt {
public:
  HIRVarDeclStmt(std::string name, const HIRType *type,
                 std::unique_ptr<HIRExpr> init, bool isMutable,
                 bool isThreadLocal, bool isVolatile, int alignment,
                 bool isStatic, bool isUsed, std::string sectionName,
                 SourceLocation loc);

  // Add the explicit destructor declaration
  ~HIRVarDeclStmt() override;

  const HIRType *getType() const { return type; }
  const std::string &getName() const { return name; }
  const HIRExpr *getInit() const { return init.get(); }

  bool isVolatileVar() const { return isVolatile; }
  void setVolatile(bool v) { isVolatile = v; }

  bool isMutableVar() const { return isMutable; }
  void setMutable(bool m) { isMutable = m; }

  bool isStaticVar() const { return isStatic; }
  void setStatic(bool v) { isStatic = v; }

  bool isExternVar() const { return isExtern; }
  void setExtern(bool v) { isExtern = v; }

  bool isUsedVar() const { return isUsed; }
  void setUsed(bool v) { isUsed = v; }

  bool isThreadLocalVar() const { return isThreadLocal; }
  void setThreadLocal(bool v) { isThreadLocal = v; }

  bool isWeakVar() const { return isWeakLinkage; }
  void setWeakVar(bool w) { isWeakLinkage = w; }

  int getAlignment() const { return alignment; }
  void setAlignment(int v) { alignment = v; }

  const std::string &getSection() const { return sectionName; }
  void setSection(std::string s) { sectionName = std::move(s); }

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) {
    return S->getKind() == Kind::VarDecl;
  }

  [[nodiscard]] bool isConstVar() const {
    // If it wasn't explicitly declared with 'mut', it is inherently constant
    if (!isMutable)
      return true;
    // Fallback to checking type-level semantic capabilities ('view'/'const')
    if (type && type->isImmutable())
      return true;
    return false;
  }

private:
  std::string name;
  const HIRType *type;
  std::unique_ptr<HIRExpr> init;
  bool isMutable = false;
  bool isThreadLocal = false;
  bool isVolatile = false;
  bool isWeakLinkage = false;
  int alignment = 0;
  bool isStatic = false;
  bool isUsed = false;
  bool isExtern = false;
  std::string sectionName = "";
};

class ForInStmt : public HIRStmt {
public:
  ForInStmt(std::unique_ptr<HIRVarDeclStmt> var,
            std::unique_ptr<HIRVarDeclStmt> indexVar,
            std::unique_ptr<HIRExpr> collection, HIRStmtPtr body,
            SourceLocation loc);

  ~ForInStmt() override; // Defined in .cpp

  [[nodiscard]] const HIRVarDeclStmt *getVariable() const { return var.get(); }
  [[nodiscard]] const HIRVarDeclStmt *getIndexVariable() const {
    return indexVar.get();
  }
  [[nodiscard]] const HIRExpr *getCollection() const {
    return collection.get();
  }
  [[nodiscard]] const HIRStmt *getBody() const { return body.get(); }

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) { return S->getKind() == Kind::ForIn; }

private:
  std::unique_ptr<HIRVarDeclStmt> var;
  std::unique_ptr<HIRVarDeclStmt> indexVar;
  std::unique_ptr<HIRExpr> collection;
  HIRStmtPtr body;
};

} // namespace hir
} // namespace moksha
