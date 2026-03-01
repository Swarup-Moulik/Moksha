#pragma once

#include "moksha/HIR/HIRType.h"
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
    AsmStmt,
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
           SourceLocation loc);
  ~LockStmt() override; // Defined in .cpp

  [[nodiscard]] const HIRExpr *getMutex() const { return mutex.get(); }
  [[nodiscard]] const HIRStmt *getBody() const { return body.get(); }

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) { return S->getKind() == Kind::Lock; }

private:
  std::unique_ptr<HIRExpr> mutex;
  HIRStmtPtr body;
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

class TryCatchStmt : public HIRStmt {
public:
  TryCatchStmt(HIRStmtPtr tryBody, std::unique_ptr<HIRExpr> catchVar,
               HIRStmtPtr catchBody, HIRStmtPtr finallyBody,
               SourceLocation loc);
  ~TryCatchStmt() override; // Defined in .cpp

  [[nodiscard]] const HIRStmt *getTryBody() const;
  [[nodiscard]] const HIRExpr *getCatchVar() const;
  [[nodiscard]] const HIRStmt *getCatchBody() const;
  [[nodiscard]] const HIRStmt *getFinallyBody() const;

  [[nodiscard]] bool hasCatch() const;
  [[nodiscard]] bool hasFinally() const;

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) {
    return S->getKind() == Kind::TryCatch;
  }

private:
  HIRStmtPtr tryBody;
  std::unique_ptr<HIRExpr> catchVar;
  HIRStmtPtr catchBody;
  HIRStmtPtr finallyBody;
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
                 SourceLocation loc);
  ~HIRVarDeclStmt() override;

  [[nodiscard]] const std::string &getName() const { return name; }
  [[nodiscard]] const HIRType *getType() const { return type.get(); }
  [[nodiscard]] const HIRExpr *getInit() const { return init.get(); }
  [[nodiscard]] bool isMutableVar() const { return isMutable; }
  bool isThreadLocalVar() const { return isThreadLocal; }
  bool isVolatileVar() const { return isVolatile; }
  int getAlignment() const { return alignment; }

  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  static bool classof(const HIRStmt *S) {
    return S->getKind() == Kind::VarDecl;
  }

private:
  std::string name;
  std::shared_ptr<const HIRType> type;
  std::unique_ptr<HIRExpr> init;
  bool isMutable;
  bool isThreadLocal;
  bool isVolatile;
  int alignment;
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

class HIRAsmStmt : public HIRStmt {
public:
  HIRAsmStmt(std::string assemblyStr, std::string constraints,
             SourceLocation loc)
      : HIRStmt(Kind::AsmStmt, loc), assemblyStr(std::move(assemblyStr)),
        constraints(std::move(constraints)) {}

  const std::string &getAssemblyStr() const { return assemblyStr; }
  const std::string &getConstraints() const { return constraints; }

  void accept(HIRVisitor &v) override;
  void accept(ConstHIRVisitor &v) const override;
  void dump(llvm::raw_ostream &os, int indent = 0) const override;
  static bool classof(const HIRStmt *S) {
    return S->getKind() == Kind::AsmStmt;
  }

private:
  std::string assemblyStr;
  std::string constraints;
};

} // namespace hir
} // namespace moksha
