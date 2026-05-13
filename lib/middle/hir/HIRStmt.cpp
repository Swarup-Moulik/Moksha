#include "moksha/HIR/HIRStmt.h"
#include "moksha/HIR/HIRExpr.h"
#include "moksha/HIR/HIRVisitor.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>

namespace moksha {
namespace hir {

// ============================================================================
// [Base Class]
// ============================================================================

void HIRStmt::printIndent(llvm::raw_ostream &os, int indent) const {
  for (int i = 0; i < indent; ++i)
    os << "  ";
}

void HIRStmt::printLabel(llvm::raw_ostream &os, int indent,
                         const char *label) const {
  printIndent(os, indent);
  os << label << ":\n";
}

// Dump generic implementation
void HIRStmt::dump(int indent) const { dump(llvm::errs(), indent); }

// ============================================================================
// [Blocks]
// ============================================================================

BlockStmt::BlockStmt(std::vector<HIRStmtPtr> stmts, SourceLocation loc)
    : HIRStmt(Kind::Block, loc), statements(std::move(stmts)) {}

BlockStmt::BlockStmt(Kind k, std::vector<HIRStmtPtr> stmts, SourceLocation loc)
    : HIRStmt(k, loc), statements(std::move(stmts)) {}

const std::vector<HIRStmtPtr> &BlockStmt::getStatements() const {
  return statements;
}

void BlockStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(BlockStmt)\n";
  for (const auto &stmt : statements) {
    if (stmt) {
      stmt->dump(os, indent + 1);
    }
  }
}
void BlockStmt::accept(HIRVisitor &v) { v.visitBlockStmt(*this); }
void BlockStmt::accept(ConstHIRVisitor &v) const { v.visitBlockStmt(*this); }

// --- UnsafeBlock ---

UnsafeBlockStmt::UnsafeBlockStmt(std::vector<HIRStmtPtr> stmts,
                                 SourceLocation loc)
    : BlockStmt(Kind::UnsafeBlock, std::move(stmts), loc) {}

void UnsafeBlockStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(UnsafeBlockStmt)\n";
  for (const auto &s : statements) {
    if (s)
      s->dump(os, indent + 1);
  }
}

void UnsafeBlockStmt::accept(HIRVisitor &v) { v.visitUnsafeBlockStmt(*this); }
void UnsafeBlockStmt::accept(ConstHIRVisitor &v) const {
  v.visitUnsafeBlockStmt(*this);
}

// --- LockStmt ---

LockStmt::LockStmt(std::unique_ptr<HIRExpr> mutex,
                   std::unique_ptr<HIRStmt> body, bool isAsync,
                   SourceLocation loc)
    : HIRStmt(Kind::Lock, loc), mutex(std::move(mutex)), body(std::move(body)),
      isAsync(isAsync) {}

void LockStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << (isAsync ? "(AsyncLockStmt)\n" : "(LockStmt)\n");
  printLabel(os, indent + 1, "Mutex");
  printIndent(os, indent + 2);
  if (mutex)
    mutex->dump(os, indent + 2);

  printLabel(os, indent + 1, "Body");
  if (body)
    body->dump(os, indent + 2);
}

void LockStmt::accept(HIRVisitor &v) { v.visitLockStmt(*this); }
void LockStmt::accept(ConstHIRVisitor &v) const { v.visitLockStmt(*this); }

// ============================================================================
// [Simple Statements]
// ============================================================================

ExprStmt::ExprStmt(std::unique_ptr<HIRExpr> expr, SourceLocation loc)
    : HIRStmt(Kind::ExprStmt, loc), expr(std::move(expr)) {}

const HIRExpr *ExprStmt::getExpr() const { return expr.get(); }

void ExprStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(ExprStmt)\n";
  if (expr) {
    expr->dump(os, indent + 1);
  }
}

void ExprStmt::accept(HIRVisitor &v) { v.visitExprStmt(*this); }
void ExprStmt::accept(ConstHIRVisitor &v) const { v.visitExprStmt(*this); }

// --- Return ---

ReturnStmt::ReturnStmt(std::unique_ptr<HIRExpr> value, SourceLocation loc)
    : HIRStmt(Kind::Return, loc), returnValue(std::move(value)) {}

const HIRExpr *ReturnStmt::getReturnValue() const { return returnValue.get(); }

void ReturnStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(ReturnStmt)\n";

  if (returnValue) {
    returnValue->dump(os, indent + 1);
  }
}

void ReturnStmt::accept(HIRVisitor &v) { v.visitReturnStmt(*this); }
void ReturnStmt::accept(ConstHIRVisitor &v) const { v.visitReturnStmt(*this); }

// ============================================================================
// [Control Flow]
// ============================================================================

IfStmt::IfStmt(std::unique_ptr<HIRExpr> cond, HIRStmtPtr thenBr,
               HIRStmtPtr elseBr, SourceLocation loc)
    : HIRStmt(Kind::If, loc), condition(std::move(cond)),
      thenBranch(std::move(thenBr)), elseBranch(std::move(elseBr)) {}

const HIRExpr *IfStmt::getCondition() const { return condition.get(); }
const HIRStmt *IfStmt::getThenBranch() const { return thenBranch.get(); }
const HIRStmt *IfStmt::getElseBranch() const { return elseBranch.get(); }

void IfStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(IfStmt)\n";

  printLabel(os, indent + 1, "Condition");
  printIndent(os, indent + 2);
  if (condition)
    condition->dump(os, indent + 2);

  if (thenBranch) {
    printLabel(os, indent + 1, "Then");
    thenBranch->dump(os, indent + 2);
  }
  if (elseBranch) {
    printLabel(os, indent + 1, "Else");
    elseBranch->dump(os, indent + 2);
  }
}

void IfStmt::accept(HIRVisitor &v) { v.visitIfStmt(*this); }
void IfStmt::accept(ConstHIRVisitor &v) const { v.visitIfStmt(*this); }

// --- While ---

WhileStmt::WhileStmt(std::unique_ptr<HIRExpr> cond, HIRStmtPtr body,
                     SourceLocation loc)
    : HIRStmt(Kind::While, loc), condition(std::move(cond)),
      body(std::move(body)) {}

const HIRExpr *WhileStmt::getCondition() const { return condition.get(); }
const HIRStmt *WhileStmt::getBody() const { return body.get(); }

void WhileStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(WhileStmt)\n";

  printLabel(os, indent + 1, "Condition");
  printIndent(os, indent + 2);
  if (condition)
    condition->dump(os, indent + 2);

  if (body)
    body->dump(os, indent + 1);
}

void WhileStmt::accept(HIRVisitor &v) { v.visitWhileStmt(*this); }
void WhileStmt::accept(ConstHIRVisitor &v) const { v.visitWhileStmt(*this); }

// --- DoWhile ---

DoWhileStmt::DoWhileStmt(HIRStmtPtr body, std::unique_ptr<HIRExpr> cond,
                         SourceLocation loc)
    : HIRStmt(Kind::DoWhile, loc), body(std::move(body)),
      condition(std::move(cond)) {}

const HIRStmt *DoWhileStmt::getBody() const { return body.get(); }
const HIRExpr *DoWhileStmt::getCondition() const { return condition.get(); }

void DoWhileStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(DoWhileStmt)\n";
  if (body)
    body->dump(os, indent + 1);

  printLabel(os, indent + 1, "Condition");
  printIndent(os, indent + 2);
  if (condition)
    condition->dump(os, indent + 2);
}

void DoWhileStmt::accept(HIRVisitor &v) { v.visitDoWhileStmt(*this); }
void DoWhileStmt::accept(ConstHIRVisitor &v) const {
  v.visitDoWhileStmt(*this);
}

// --- For ---

ForStmt::ForStmt(HIRStmtPtr init, std::unique_ptr<HIRExpr> cond,
                 std::unique_ptr<HIRExpr> inc, HIRStmtPtr body,
                 SourceLocation loc)
    : HIRStmt(Kind::For, loc), init(std::move(init)), cond(std::move(cond)),
      inc(std::move(inc)), body(std::move(body)) {}

void ForStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(ForStmt)\n";

  if (init) {
    printLabel(os, indent + 1, "Init");
    init->dump(os, indent + 2);
  }

  printLabel(os, indent + 1, "Condition");
  printIndent(os, indent + 2);
  if (cond)
    cond->dump(os, indent + 2);

  printLabel(os, indent + 1, "Increment");
  printIndent(os, indent + 2);
  if (inc)
    inc->dump(os, indent + 2);

  if (body) {
    printLabel(os, indent + 1, "Body");
    body->dump(os, indent + 2);
  }
}

void ForStmt::accept(HIRVisitor &v) { v.visitForStmt(*this); }
void ForStmt::accept(ConstHIRVisitor &v) const { v.visitForStmt(*this); }

// --- ForIn ---

ForInStmt::ForInStmt(std::unique_ptr<HIRVarDeclStmt> var,
                     std::unique_ptr<HIRVarDeclStmt> indexVar,
                     std::unique_ptr<HIRExpr> collection, HIRStmtPtr body,
                     SourceLocation loc)
    : HIRStmt(Kind::ForIn, loc), var(std::move(var)),
      indexVar(std::move(indexVar)), collection(std::move(collection)),
      body(std::move(body)) {}

void ForInStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(ForInStmt)\n";
  if (var) {
    printLabel(os, indent + 1, "Var");
    var->dump(os, indent + 2);
  }
  if (indexVar) {
    printLabel(os, indent + 1, "IndexVar");
    indexVar->dump(os, indent + 2);
  }
  if (collection) {
    printLabel(os, indent + 1, "Collection");
    collection->dump(os, indent + 2);
  }
  if (body) {
    printLabel(os, indent + 1, "Body");
    body->dump(os, indent + 2);
  }
}

void ForInStmt::accept(HIRVisitor &v) { v.visitForInStmt(*this); }
void ForInStmt::accept(ConstHIRVisitor &v) const { v.visitForInStmt(*this); }

// --- Switch ---

SwitchStmt::SwitchStmt(std::unique_ptr<HIRExpr> cond,
                       std::vector<SwitchCase> cases, SourceLocation loc)
    : HIRStmt(Kind::Switch, loc), condition(std::move(cond)),
      cases(std::move(cases)) {}

const HIRExpr *SwitchStmt::getCondition() const { return condition.get(); }
const std::vector<SwitchCase> &SwitchStmt::getCases() const { return cases; }

void SwitchStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(SwitchStmt)\n";

  printLabel(os, indent + 1, "Condition");
  if (condition)
    condition->dump(os, indent + 2);

  for (const auto &c : cases) {
    printIndent(os, indent + 1);
    os << (c.isDefaultCase() ? "Default" : "Case") << ":\n";
    c.getBody().dump(os, indent + 2);
  }
}

void SwitchStmt::accept(HIRVisitor &v) { v.visitSwitchStmt(*this); }
void SwitchStmt::accept(ConstHIRVisitor &v) const { v.visitSwitchStmt(*this); }

// ============================================================================
// [Jumps / Misc]
// ============================================================================

BreakStmt::BreakStmt(SourceLocation loc) : HIRStmt(Kind::Break, loc) {}
void BreakStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(BreakStmt)\n";
}
void BreakStmt::accept(HIRVisitor &v) { v.visitBreakStmt(*this); }
void BreakStmt::accept(ConstHIRVisitor &v) const { v.visitBreakStmt(*this); }

ContinueStmt::ContinueStmt(SourceLocation loc) : HIRStmt(Kind::Continue, loc) {}
void ContinueStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(ContinueStmt)\n";
}
void ContinueStmt::accept(HIRVisitor &v) { v.visitContinueStmt(*this); }
void ContinueStmt::accept(ConstHIRVisitor &v) const {
  v.visitContinueStmt(*this);
}

DeferStmt::DeferStmt(HIRStmtPtr stmt, SourceLocation loc)
    : HIRStmt(Kind::Defer, loc), deferredStmt(std::move(stmt)) {}

const HIRStmt *DeferStmt::getDeferredStmt() const { return deferredStmt.get(); }

void DeferStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(DeferStmt)\n";
  if (deferredStmt)
    deferredStmt->dump(os, indent + 1);
}

void DeferStmt::accept(HIRVisitor &v) { v.visitDeferStmt(*this); }
void DeferStmt::accept(ConstHIRVisitor &v) const { v.visitDeferStmt(*this); }

// --- TryCatch ---

void TryCatchStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(TryCatchStmt)\n";

  if (tryBlock) {
    printLabel(os, indent + 1, "TryBlock");
    tryBlock->dump(os, indent + 2);
  }

  for (size_t i = 0; i < catches.size(); ++i) {
    printIndent(os, indent + 1);
    os << "CatchClause (" << catches[i].varName;
    if (catches[i].varType) {
      os << ": " << catches[i].varType->toString();
    }
    os << "):\n";

    if (catches[i].body) {
      catches[i].body->dump(os, indent + 2);
    }
  }

  if (finallyBlock) {
    printLabel(os, indent + 1, "FinallyBlock");
    finallyBlock->dump(os, indent + 2);
  }
}

void TryCatchStmt::accept(HIRVisitor &v) { v.visitTryCatchStmt(*this); }
void TryCatchStmt::accept(ConstHIRVisitor &v) const {
  v.visitTryCatchStmt(*this);
}

// ============================================================================
// [Variable Declaration]
// ============================================================================

HIRVarDeclStmt::HIRVarDeclStmt(std::string name, const HIRType *type,
                               std::unique_ptr<HIRExpr> init, bool isMutable,
                               bool isThreadLocal, bool isVolatile,
                               int alignment, bool isStatic, bool isUsed,
                               std::string sectionName, SourceLocation loc)
    : HIRStmt(Kind::VarDecl, loc), name(std::move(name)), type(std::move(type)),
      init(std::move(init)), isMutable(isMutable), isThreadLocal(isThreadLocal),
      isVolatile(isVolatile), alignment(alignment), isStatic(isStatic),
      isUsed(isUsed), sectionName(std::move(sectionName)) {}

void HIRVarDeclStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "VarDecl: " << name;

  if (isThreadLocal)
    os << " [thread_local]";
  if (isVolatile)
    os << " [volatile]";
  if (alignment > 0)
    os << " [align(" << alignment << ")]";
  if (isStatic)
    os << " [static]";
  if (isUsed)
    os << " [used]";
  if (isWeakLinkage)
    os << " [weak]";
  if (!sectionName.empty())
    os << " [section(\"" << sectionName << "\")]";

  os << "\n";

  if (init) {
    printLabel(os, indent + 1, "Init");
    init->dump(os, indent + 2);
  }
}

void HIRVarDeclStmt::accept(HIRVisitor &v) { v.visitVarDeclStmt(*this); }
void HIRVarDeclStmt::accept(ConstHIRVisitor &v) const {
  v.visitVarDeclStmt(*this);
}

HIRThrowStmt::HIRThrowStmt(std::unique_ptr<HIRExpr> expr, SourceLocation loc)
    : HIRStmt(Kind::Throw, loc), expression(std::move(expr)) {}

void HIRThrowStmt::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "ThrowStmt\n";
}
void HIRThrowStmt::accept(HIRVisitor &v) { v.visitThrowStmt(*this); }
void HIRThrowStmt::accept(ConstHIRVisitor &v) const { v.visitThrowStmt(*this); }

// ============================================================================
// [SwitchCase Implementation]
// ============================================================================
SwitchCase::SwitchCase(std::vector<std::unique_ptr<HIRExpr>> v,
                       std::unique_ptr<BlockStmt> b, bool d)
    : values(std::move(v)), body(std::move(b)), isDefault(d) {}

SwitchCase::~SwitchCase() = default;

// ============================================================================
// [Destructors for Forward-Declared unique_ptr compatibility]
// ============================================================================
BlockStmt::~BlockStmt() = default;
UnsafeBlockStmt::~UnsafeBlockStmt() = default;
LockStmt::~LockStmt() = default;
ExprStmt::~ExprStmt() = default;
ReturnStmt::~ReturnStmt() = default;
IfStmt::~IfStmt() = default;
WhileStmt::~WhileStmt() = default;
DoWhileStmt::~DoWhileStmt() = default;
ForStmt::~ForStmt() = default;
SwitchStmt::~SwitchStmt() = default;
BreakStmt::~BreakStmt() = default;
ContinueStmt::~ContinueStmt() = default;
DeferStmt::~DeferStmt() = default;
HIRVarDeclStmt::~HIRVarDeclStmt() = default;
ForInStmt::~ForInStmt() = default;

} // namespace hir
} // namespace moksha
