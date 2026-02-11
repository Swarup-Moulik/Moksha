#include "moksha/HIR/HIRStmt.h"
#include "moksha/HIR/HIRExpr.h"     // Required for std::unique_ptr<HIRExpr> destructor
#include "moksha/HIR/HIRVisitor.h"  // Required for accept()

#include <iostream>

namespace moksha {
namespace hir {

// ============================================================================
// [Base Class]
// ============================================================================

void HIRStmt::printIndent(std::ostream &os, int indent) const {
  for (int i = 0; i < indent; ++i) os << "  ";
}

void HIRStmt::printLabel(std::ostream &os, int indent, const char* label) const {
  printIndent(os, indent);
  os << label << ":\n";
}

// Dump generic implementation
void HIRStmt::dump(int indent) const {
    dump(std::cerr, indent);
}

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

void BlockStmt::dump(std::ostream &os, int indent) const {
    printIndent(os, indent);
    os << "(BlockStmt)\n";
    for (const auto &s : statements) {
        if (s) s->dump(os, indent + 1);
    }
}

void BlockStmt::accept(HIRVisitor &v) { v.visitBlockStmt(*this); }
void BlockStmt::accept(ConstHIRVisitor &v) const { v.visitBlockStmt(*this); }

// --- UnsafeBlock ---

UnsafeBlockStmt::UnsafeBlockStmt(std::vector<HIRStmtPtr> stmts, SourceLocation loc)
    : BlockStmt(Kind::UnsafeBlock, std::move(stmts), loc) {}

void UnsafeBlockStmt::dump(std::ostream &os, int indent) const {
    printIndent(os, indent);
    os << "(UnsafeBlockStmt)\n";
    for (const auto &s : statements) {
        if (s) s->dump(os, indent + 1);
    }
}

void UnsafeBlockStmt::accept(HIRVisitor &v) { v.visitUnsafeBlockStmt(*this); }
void UnsafeBlockStmt::accept(ConstHIRVisitor &v) const { v.visitUnsafeBlockStmt(*this); }

// --- LockStmt ---

LockStmt::LockStmt(std::unique_ptr<HIRExpr> mutex, std::unique_ptr<HIRStmt> body, SourceLocation loc)
    : HIRStmt(Kind::Lock, loc), mutex(std::move(mutex)), body(std::move(body)) {}

void LockStmt::dump(std::ostream &os, int indent) const {
    printIndent(os, indent);
    os << "(LockStmt)\n";

    printLabel(os, indent + 1, "Mutex");
    printIndent(os, indent + 2);
    os << "<expr>\n"; // Structural placeholder until HIRExpr::dump exists

    printLabel(os, indent + 1, "Body");
    if(body) body->dump(os, indent + 2);
}

void LockStmt::accept(HIRVisitor &v) { v.visitLockStmt(*this); }
void LockStmt::accept(ConstHIRVisitor &v) const { v.visitLockStmt(*this); }

// ============================================================================
// [Simple Statements]
// ============================================================================

ExprStmt::ExprStmt(std::unique_ptr<HIRExpr> expr, SourceLocation loc)
    : HIRStmt(Kind::ExprStmt, loc), expr(std::move(expr)) {}

const HIRExpr *ExprStmt::getExpr() const { return expr.get(); }

void ExprStmt::dump(std::ostream &os, int indent) const {
    printIndent(os, indent);
    os << "(ExprStmt)\n";
    printIndent(os, indent + 1);
    os << "<expr>\n";
}

void ExprStmt::accept(HIRVisitor &v) { v.visitExprStmt(*this); }
void ExprStmt::accept(ConstHIRVisitor &v) const { v.visitExprStmt(*this); }

// --- Return ---

ReturnStmt::ReturnStmt(std::unique_ptr<HIRExpr> value, SourceLocation loc)
    : HIRStmt(Kind::Return, loc), returnValue(std::move(value)) {}

const HIRExpr *ReturnStmt::getReturnValue() const { return returnValue.get(); }

void ReturnStmt::dump(std::ostream &os, int indent) const {
    printIndent(os, indent);
    os << "(ReturnStmt)\n";
    if (returnValue) {
        printIndent(os, indent + 1);
        os << "<expr>\n";
    }
}

void ReturnStmt::accept(HIRVisitor &v) { v.visitReturnStmt(*this); }
void ReturnStmt::accept(ConstHIRVisitor &v) const { v.visitReturnStmt(*this); }

// ============================================================================
// [Control Flow]
// ============================================================================

IfStmt::IfStmt(std::unique_ptr<HIRExpr> cond, HIRStmtPtr thenBr, HIRStmtPtr elseBr, SourceLocation loc)
    : HIRStmt(Kind::If, loc), condition(std::move(cond)), thenBranch(std::move(thenBr)), elseBranch(std::move(elseBr)) {}

const HIRExpr *IfStmt::getCondition() const { return condition.get(); }
const HIRStmt *IfStmt::getThenBranch() const { return thenBranch.get(); }
const HIRStmt *IfStmt::getElseBranch() const { return elseBranch.get(); }

void IfStmt::dump(std::ostream &os, int indent) const {
    printIndent(os, indent);
    os << "(IfStmt)\n";

    printLabel(os, indent + 1, "Condition");
    printIndent(os, indent + 2);
    os << "<expr>\n";

    if(thenBranch) {
        printLabel(os, indent+1, "Then");
        thenBranch->dump(os, indent+2);
    }
    if(elseBranch) {
        printLabel(os, indent+1, "Else");
        elseBranch->dump(os, indent+2);
    }
}

void IfStmt::accept(HIRVisitor &v) { v.visitIfStmt(*this); }
void IfStmt::accept(ConstHIRVisitor &v) const { v.visitIfStmt(*this); }

// --- While ---

WhileStmt::WhileStmt(std::unique_ptr<HIRExpr> cond, HIRStmtPtr body, SourceLocation loc)
    : HIRStmt(Kind::While, loc), condition(std::move(cond)), body(std::move(body)) {}

const HIRExpr *WhileStmt::getCondition() const { return condition.get(); }
const HIRStmt *WhileStmt::getBody() const { return body.get(); }

void WhileStmt::dump(std::ostream &os, int indent) const {
    printIndent(os, indent);
    os << "(WhileStmt)\n";

    printLabel(os, indent + 1, "Condition");
    printIndent(os, indent + 2);
    os << "<expr>\n";

    if(body) body->dump(os, indent+1);
}

void WhileStmt::accept(HIRVisitor &v) { v.visitWhileStmt(*this); }
void WhileStmt::accept(ConstHIRVisitor &v) const { v.visitWhileStmt(*this); }

// --- DoWhile ---

DoWhileStmt::DoWhileStmt(HIRStmtPtr body, std::unique_ptr<HIRExpr> cond, SourceLocation loc)
    : HIRStmt(Kind::DoWhile, loc), body(std::move(body)), condition(std::move(cond)) {}

const HIRStmt *DoWhileStmt::getBody() const { return body.get(); }
const HIRExpr *DoWhileStmt::getCondition() const { return condition.get(); }

void DoWhileStmt::dump(std::ostream &os, int indent) const {
    printIndent(os, indent);
    os << "(DoWhileStmt)\n";
    if(body) body->dump(os, indent+1);

    printLabel(os, indent + 1, "Condition");
    printIndent(os, indent + 2);
    os << "<expr>\n";
}

void DoWhileStmt::accept(HIRVisitor &v) { v.visitDoWhileStmt(*this); }
void DoWhileStmt::accept(ConstHIRVisitor &v) const { v.visitDoWhileStmt(*this); }

// --- For ---

ForStmt::ForStmt(HIRStmtPtr init, std::unique_ptr<HIRExpr> cond, std::unique_ptr<HIRExpr> inc, HIRStmtPtr body, SourceLocation loc)
    : HIRStmt(Kind::For, loc), init(std::move(init)), cond(std::move(cond)), inc(std::move(inc)), body(std::move(body)) {}

void ForStmt::dump(std::ostream &os, int indent) const {
    printIndent(os, indent);
    os << "(ForStmt)\n";

    if(init) {
        printLabel(os, indent+1, "Init");
        init->dump(os, indent+2);
    }

    printLabel(os, indent + 1, "Condition");
    printIndent(os, indent + 2);
    os << "<expr>\n";

    printLabel(os, indent + 1, "Increment");
    printIndent(os, indent + 2);
    os << "<expr>\n";

    if(body) {
        printLabel(os, indent+1, "Body");
        body->dump(os, indent+2);
    }
}

void ForStmt::accept(HIRVisitor &v) { v.visitForStmt(*this); }
void ForStmt::accept(ConstHIRVisitor &v) const { v.visitForStmt(*this); }

// --- ForIn ---

ForInStmt::ForInStmt(std::unique_ptr<HIRVarDeclStmt> var,
                     std::unique_ptr<HIRVarDeclStmt> indexVar,
                     std::unique_ptr<HIRExpr> collection,
                     HIRStmtPtr body,
                     SourceLocation loc)
    : HIRStmt(Kind::ForIn, loc), var(std::move(var)), indexVar(std::move(indexVar)),
      collection(std::move(collection)), body(std::move(body)) {}

void ForInStmt::dump(std::ostream &os, int indent) const {
    printIndent(os, indent);
    os << "(ForInStmt)\n";
    if(var) { printLabel(os, indent+1, "Var"); var->dump(os, indent+2); }
    if(collection) {
        printLabel(os, indent + 1, "Collection");
        printIndent(os, indent + 2);
        os << "<expr>\n";
    }
    if(body) { printLabel(os, indent+1, "Body"); body->dump(os, indent+2); }
}

void ForInStmt::accept(HIRVisitor &v) { v.visitForInStmt(*this); }
void ForInStmt::accept(ConstHIRVisitor &v) const { v.visitForInStmt(*this); }

// --- Switch ---

SwitchStmt::SwitchStmt(std::unique_ptr<HIRExpr> cond, std::vector<SwitchCase> cases, SourceLocation loc)
    : HIRStmt(Kind::Switch, loc), condition(std::move(cond)), cases(std::move(cases)) {}

const HIRExpr *SwitchStmt::getCondition() const { return condition.get(); }
const std::vector<SwitchCase> &SwitchStmt::getCases() const { return cases; }

void SwitchStmt::dump(std::ostream &os, int indent) const {
    printIndent(os, indent);
    os << "(SwitchStmt)\n";

    printLabel(os, indent + 1, "Condition");
    printIndent(os, indent + 2);
    os << "<expr>\n";

    for(const auto& c : cases) {
        printIndent(os, indent+1);
        os << (c.isDefaultCase() ? "Default" : "Case") << ":\n";
        c.getBody().dump(os, indent+2);
    }
}

void SwitchStmt::accept(HIRVisitor &v) { v.visitSwitchStmt(*this); }
void SwitchStmt::accept(ConstHIRVisitor &v) const { v.visitSwitchStmt(*this); }

// ============================================================================
// [Jumps / Misc]
// ============================================================================

BreakStmt::BreakStmt(SourceLocation loc) : HIRStmt(Kind::Break, loc) {}
void BreakStmt::dump(std::ostream &os, int indent) const { printIndent(os, indent); os << "(BreakStmt)\n"; }
void BreakStmt::accept(HIRVisitor &v) { v.visitBreakStmt(*this); }
void BreakStmt::accept(ConstHIRVisitor &v) const { v.visitBreakStmt(*this); }

ContinueStmt::ContinueStmt(SourceLocation loc) : HIRStmt(Kind::Continue, loc) {}
void ContinueStmt::dump(std::ostream &os, int indent) const { printIndent(os, indent); os << "(ContinueStmt)\n"; }
void ContinueStmt::accept(HIRVisitor &v) { v.visitContinueStmt(*this); }
void ContinueStmt::accept(ConstHIRVisitor &v) const { v.visitContinueStmt(*this); }

DeferStmt::DeferStmt(HIRStmtPtr stmt, SourceLocation loc)
    : HIRStmt(Kind::Defer, loc), deferredStmt(std::move(stmt)) {}

const HIRStmt *DeferStmt::getDeferredStmt() const { return deferredStmt.get(); }

void DeferStmt::dump(std::ostream &os, int indent) const {
    printIndent(os, indent);
    os << "(DeferStmt)\n";
    if(deferredStmt) deferredStmt->dump(os, indent+1);
}

void DeferStmt::accept(HIRVisitor &v) { v.visitDeferStmt(*this); }
void DeferStmt::accept(ConstHIRVisitor &v) const { v.visitDeferStmt(*this); }

// --- TryCatch ---

TryCatchStmt::TryCatchStmt(HIRStmtPtr tryBody, std::unique_ptr<HIRExpr> catchVar, HIRStmtPtr catchBody, HIRStmtPtr finallyBody, SourceLocation loc)
    : HIRStmt(Kind::TryCatch, loc), tryBody(std::move(tryBody)), catchVar(std::move(catchVar)),
      catchBody(std::move(catchBody)), finallyBody(std::move(finallyBody)) {}

const HIRStmt *TryCatchStmt::getTryBody() const { return tryBody.get(); }
const HIRExpr *TryCatchStmt::getCatchVar() const { return catchVar.get(); }
const HIRStmt *TryCatchStmt::getCatchBody() const { return catchBody.get(); }
const HIRStmt *TryCatchStmt::getFinallyBody() const { return finallyBody.get(); }

bool TryCatchStmt::hasCatch() const { return catchBody != nullptr; }
bool TryCatchStmt::hasFinally() const { return finallyBody != nullptr; }

void TryCatchStmt::dump(std::ostream &os, int indent) const {
  printIndent(os, indent);
  os << "(TryCatchStmt)\n";

  if (tryBody) {
      printLabel(os, indent + 1, "TryBody");
      tryBody->dump(os, indent + 2);
  }

  if (catchVar) {
      printLabel(os, indent + 1, "CatchVar");
      printIndent(os, indent + 2);
      os << "<expr>\n";
  }

  if (catchBody) {
      printLabel(os, indent + 1, "CatchBody");
      catchBody->dump(os, indent + 2);
  }

  if (finallyBody) {
      printLabel(os, indent + 1, "FinallyBody");
      finallyBody->dump(os, indent + 2);
  }
}

void TryCatchStmt::accept(HIRVisitor &v) { v.visitTryCatchStmt(*this); }
void TryCatchStmt::accept(ConstHIRVisitor &v) const { v.visitTryCatchStmt(*this); }

// ============================================================================
// [Variable Declaration]
// ============================================================================

HIRVarDeclStmt::HIRVarDeclStmt(std::string name,
                               std::shared_ptr<const HIRType> type,
                               std::unique_ptr<HIRExpr> init,
                               bool isMutable,
                               SourceLocation loc)
    : HIRStmt(Kind::VarDecl, loc), name(std::move(name)), type(std::move(type)),
      init(std::move(init)), isMutable(isMutable) {}

void HIRVarDeclStmt::dump(std::ostream &os, int indent) const {
    printIndent(os, indent);
    os << "VarDecl: " << name << "\n";
    if (init) {
        printLabel(os, indent + 1, "Init");
        printIndent(os, indent + 2);
        os << "<expr>\n";
    }
}

void HIRVarDeclStmt::accept(HIRVisitor &v) { v.visitVarDeclStmt(*this); }
void HIRVarDeclStmt::accept(ConstHIRVisitor &v) const { v.visitVarDeclStmt(*this); }

} // namespace hir
} // namespace moksha
