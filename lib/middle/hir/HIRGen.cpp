#include "moksha/HIR/HIRGen.h"
#include "moksha/AST/ASTContext.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/HIR/HIRExpr.h"
#include "moksha/HIR/HIRFunction.h"
#include "moksha/HIR/HIRModule.h"
#include "moksha/HIR/HIRStmt.h"
#include <memory>
#include <vector>

using namespace moksha;

// Helper to cast generic HIRStmt to specific HIR subclass safely
template <typename T>
std::unique_ptr<T> castToHIR(std::unique_ptr<hir::HIRStmt> stmt) {
  if (!stmt)
    return nullptr;
  return std::unique_ptr<T>(static_cast<T *>(stmt.release()));
}

// [FIX] Constructor Definition (implied missing from previous logs)
HIRGen::HIRGen(ASTContext &ctx) : ctx(ctx) {}

std::unique_ptr<hir::HIRStmt> HIRGen::takeStmt() { return std::move(lastStmt); }
std::unique_ptr<hir::HIRExpr> HIRGen::takeExpr() { return std::move(lastExpr); }

// [FIX] Implement the dispatchers
void HIRGen::visit(const Decl *d) {
  if (d)
    d->accept(*this);
}
void HIRGen::visit(const Stmt *s) {
  if (s)
    s->accept(*this);
}
void HIRGen::visit(const Expr *e) {
  if (e)
    e->accept(*this);
}

void HIRGen::lowerModule(const ModuleDecl *mod) {
  functions.clear();
  globals.clear();

  for (const auto &decl : mod->getDecls()) {
    visit(decl.get());
  }
}

void HIRGen::visitModuleDecl(const ModuleDecl *decl) {
  for (const auto &d : decl->getDecls()) {
    visit(d.get());
  }
}

void HIRGen::visitFunctionDecl(const FunctionDecl *decl) {
  lastStmt = nullptr;

  if (!decl->getBody()) {
    return;
  }

  std::vector<hir::HIRParam> hirParams;
  for (const auto &p : decl->getParams()) {
    // [FIX] Passing nullptr for type because HIRParam expects HIRType*, not
    // AST::Type*
    hirParams.emplace_back(p.name, nullptr, decl->getLoc());
  }

  std::unique_ptr<hir::HIRStmt> body = nullptr;
  if (decl->getBody()) {
    visit(decl->getBody());
    body = takeStmt();
  }

  // [FIX] AST Type to HIR Type conversion stub (passing nullptr)
  const hir::HIRType *retTypeRaw = nullptr;

  std::vector<std::string> typeParams;
  bool isVariadic = false;

  auto func = std::make_unique<hir::HIRFunction>(
      decl->getName(), typeParams, std::move(hirParams), retTypeRaw,
      std::move(body), decl->isAsyncFunc(), isVariadic, decl->getLoc());

  functions.push_back(std::move(func));
}

void HIRGen::visitVariableDecl(const VariableDecl *decl) {
  std::unique_ptr<hir::HIRExpr> init = nullptr;
  if (decl->getInitializer()) {
    visit(decl->getInitializer());
    init = takeExpr();
  }

  // [FIX] Stubbing type conversion
  std::shared_ptr<const hir::HIRType> type = nullptr;

  lastStmt = std::make_unique<hir::HIRVarDeclStmt>(decl->getName(), type,
                                                   std::move(init),
                                                   true, // isMutable
                                                   decl->getLoc());
}

void HIRGen::visitBlockStmt(const BlockStmt *stmt) {
  std::vector<std::unique_ptr<hir::HIRStmt>> hirStmts;
  for (const auto &s : stmt->getStatements()) {
    visit(s.get());
    if (lastStmt) {
      hirStmts.push_back(takeStmt());
    }
  }
  lastStmt =
      std::make_unique<hir::BlockStmt>(std::move(hirStmts), stmt->getLoc());
}

void HIRGen::visitReturnStmt(const ReturnStmt *stmt) {
  std::unique_ptr<hir::HIRExpr> retVal = nullptr;
  if (stmt->getReturnValue()) {
    visit(stmt->getReturnValue());
    retVal = takeExpr();
  }
  lastStmt =
      std::make_unique<hir::ReturnStmt>(std::move(retVal), stmt->getLoc());
}

void HIRGen::visitIfStmt(const IfStmt *stmt) {
  visit(stmt->getCondition());
  auto cond = takeExpr();

  visit(stmt->getThenStmt());
  auto thenBranch = takeStmt();

  std::unique_ptr<hir::HIRStmt> elseBranch = nullptr;
  if (stmt->getElseStmt()) {
    visit(stmt->getElseStmt());
    elseBranch = takeStmt();
  }

  lastStmt =
      std::make_unique<hir::IfStmt>(std::move(cond), std::move(thenBranch),
                                    std::move(elseBranch), stmt->getLoc());
}

void HIRGen::visitWhileStmt(const WhileStmt *stmt) {
  visit(stmt->getCondition());
  auto cond = takeExpr();

  visit(stmt->getBody());
  auto body = takeStmt();

  lastStmt = std::make_unique<hir::WhileStmt>(std::move(cond), std::move(body),
                                              stmt->getLoc());
}

void HIRGen::visitExpressionStmt(const ExpressionStmt *stmt) {
  visit(stmt->getExpr());
  lastStmt = std::make_unique<hir::ExprStmt>(takeExpr(), stmt->getLoc());
}

void HIRGen::visitUnsafeBlockStmt(const UnsafeBlockStmt *stmt) {
  std::vector<std::unique_ptr<hir::HIRStmt>> hirStmts;
  for (const auto &s : stmt->getStatements()) {
    visit(s.get());
    if (lastStmt) {
      hirStmts.push_back(takeStmt());
    }
  }
  lastStmt = std::make_unique<hir::UnsafeBlockStmt>(std::move(hirStmts),
                                                    stmt->getLoc());
}

void HIRGen::visitDoWhileStmt(const DoWhileStmt *stmt) {
  visit(stmt->getBody());
  auto body = takeStmt();

  visit(stmt->getCondition());
  auto cond = takeExpr();

  lastStmt = std::make_unique<hir::DoWhileStmt>(
      std::move(body), std::move(cond), stmt->getLoc());
}

void HIRGen::visitForStmt(const ForStmt *stmt) {
  std::unique_ptr<hir::HIRStmt> init = nullptr;
  if (stmt->getInit()) {
    visit(stmt->getInit());
    init = takeStmt();
  }

  std::unique_ptr<hir::HIRExpr> cond = nullptr;
  if (stmt->getCondition()) {
    visit(stmt->getCondition());
    cond = takeExpr();
  }

  std::unique_ptr<hir::HIRExpr> inc = nullptr;
  if (stmt->getIncrement()) {
    visit(stmt->getIncrement());
    inc = takeExpr();
  }

  visit(stmt->getBody());
  auto body = takeStmt();

  lastStmt = std::make_unique<hir::ForStmt>(std::move(init), std::move(cond),
                                            std::move(inc), std::move(body),
                                            stmt->getLoc());
}

void HIRGen::visitForInStmt(const ForInStmt *stmt) {
  visit(stmt->getVariable());
  auto varDecl = castToHIR<hir::HIRVarDeclStmt>(takeStmt());

  std::unique_ptr<hir::HIRVarDeclStmt> indexDecl = nullptr;
  if (stmt->getIndexVariable()) {
    visit(stmt->getIndexVariable());
    indexDecl = castToHIR<hir::HIRVarDeclStmt>(takeStmt());
  }

  visit(stmt->getCollection());
  auto collection = takeExpr();

  visit(stmt->getBody());
  auto body = takeStmt();

  lastStmt = std::make_unique<hir::ForInStmt>(
      std::move(varDecl), std::move(indexDecl), std::move(collection),
      std::move(body), stmt->getLoc());
}

void HIRGen::visitSwitchStmt(const SwitchStmt *stmt) {
  visit(stmt->getCondition());
  auto cond = takeExpr();

  std::vector<hir::SwitchCase> hirCases;
  for (const auto &c : stmt->getCases()) {
    std::vector<std::unique_ptr<hir::HIRExpr>> vals;
    for (const auto &v : c.getValues()) {
      visit(v.get());
      vals.push_back(takeExpr());
    }

    visit(c.getBody());
    auto caseBody = castToHIR<hir::BlockStmt>(takeStmt());

    hirCases.emplace_back(std::move(vals), std::move(caseBody),
                          c.isDefaultCase());
  }

  lastStmt = std::make_unique<hir::SwitchStmt>(
      std::move(cond), std::move(hirCases), stmt->getLoc());
}

void HIRGen::visitBreakStmt(const BreakStmt *stmt) {
  lastStmt = std::make_unique<hir::BreakStmt>(stmt->getLoc());
}

void HIRGen::visitContinueStmt(const ContinueStmt *stmt) {
  lastStmt = std::make_unique<hir::ContinueStmt>(stmt->getLoc());
}

void HIRGen::visitDeferStmt(const DeferStmt *stmt) {
  visit(stmt->getDeferredStmt());
  lastStmt = std::make_unique<hir::DeferStmt>(takeStmt(), stmt->getLoc());
}

void HIRGen::visitTryCatchStmt(const TryCatchStmt *stmt) {
  visit(stmt->getTryBody());
  auto tryBody = castToHIR<hir::BlockStmt>(takeStmt());

  std::unique_ptr<hir::HIRStmt> catchBody = nullptr;
  std::unique_ptr<hir::HIRExpr> catchVar = nullptr;

  if (stmt->getCatchBody()) {
    visit(stmt->getCatchBody());
    catchBody = takeStmt();
  }

  if (stmt->getCatchVar()) {
    lastExpr = std::make_unique<hir::HIRIdentifierExpr>(
        stmt->getCatchVar()->getName(), nullptr, stmt->getCatchVar()->getLoc());
    catchVar = takeExpr();
  }

  std::unique_ptr<hir::HIRStmt> finallyBody = nullptr;
  if (stmt->getFinallyBody()) {
    visit(stmt->getFinallyBody());
    finallyBody = takeStmt();
  }

  lastStmt = std::make_unique<hir::TryCatchStmt>(
      std::move(tryBody), std::move(catchVar), std::move(catchBody),
      std::move(finallyBody), stmt->getLoc());
}

// --- Expressions ---

void HIRGen::visitIntegerLiteral(const IntegerLiteral *expr) {
  lastExpr = std::make_unique<hir::HIRIntegerLiteral>(expr->getValue(), nullptr,
                                                      expr->getLoc());
}

void HIRGen::visitFloatLiteral(const FloatLiteral *expr) {
  lastExpr = std::make_unique<hir::HIRFloatLiteral>(expr->getValue(), nullptr,
                                                    expr->getLoc());
}

void HIRGen::visitBoolLiteral(const BoolLiteral *expr) {
  lastExpr = std::make_unique<hir::HIRBoolLiteral>(expr->getValue(), nullptr,
                                                   expr->getLoc());
}

void HIRGen::visitStringLiteral(const StringLiteral *expr) {
  lastExpr = std::make_unique<hir::HIRStringLiteral>(expr->getValue(), nullptr,
                                                     expr->getLoc());
}

void HIRGen::visitBinaryExpr(const BinaryExpr *expr) {
  visit(expr->getLHS());
  auto lhs = takeExpr();

  visit(expr->getRHS());
  auto rhs = takeExpr();

  hir::BinaryOp op = static_cast<hir::BinaryOp>(0);

  lastExpr = std::make_unique<hir::HIRBinaryExpr>(
      std::move(lhs), op, std::move(rhs), nullptr, expr->getLoc());
}

void HIRGen::visitUnaryExpr(const UnaryExpr *expr) {
  visit(expr->getOperand());
  auto op = takeExpr();

  hir::UnaryOp uOp = static_cast<hir::UnaryOp>(0);

  lastExpr = std::make_unique<hir::HIRUnaryExpr>(uOp, std::move(op), nullptr,
                                                 expr->getLoc());
}

void HIRGen::visitIdentifierExpr(const IdentifierExpr *expr) {
  lastExpr = std::make_unique<hir::HIRIdentifierExpr>(expr->getName(), nullptr,
                                                      expr->getLoc());
}

void HIRGen::visitCallExpr(const CallExpr *expr) {
  visit(expr->getCallee());
  auto callee = takeExpr();

  std::vector<std::unique_ptr<hir::HIRExpr>> args;
  for (const auto &arg : expr->getArgs()) {
    visit(arg.get());
    args.push_back(takeExpr());
  }

  lastExpr = std::make_unique<hir::HIRCallExpr>(
      std::move(callee), std::move(args), nullptr, expr->getLoc());
}

void HIRGen::visitMemberExpr(const MemberExpr *expr) {
  visit(expr->getObject());
  auto object = takeExpr();
  lastExpr = std::make_unique<hir::HIRMemberExpr>(
      std::move(object), expr->getMemberName(), nullptr, expr->getLoc());
}

void HIRGen::visitThreadExpr(const ThreadExpr *expr) {
  visit(expr->getBody());
  auto bodyLambda = dynamic_cast<hir::HIRLambdaExpr *>(lastExpr.release());
  std::unique_ptr<hir::HIRLambdaExpr> lambdaTask(bodyLambda);

  hir::ThreadKind kind =
      expr->isWeakThread() ? hir::ThreadKind::Weak : hir::ThreadKind::Strong;

  lastExpr = std::make_unique<hir::HIRThreadExpr>(std::move(lambdaTask), kind,
                                                  nullptr, expr->getLoc());
}

void HIRGen::visitNullLiteral(const NullLiteral *expr) {
  lastExpr = std::make_unique<hir::HIRNullLiteral>(nullptr, expr->getLoc());
}

void HIRGen::visitTernaryExpr(const TernaryExpr *expr) {
  visit(expr->getCondition());
  auto cond = takeExpr();
  visit(expr->getTrueBranch());
  auto t = takeExpr();
  visit(expr->getFalseBranch());
  auto f = takeExpr();

  lastExpr = std::make_unique<hir::HIRTernaryExpr>(
      std::move(cond), std::move(t), std::move(f), nullptr, expr->getLoc());
}

void HIRGen::visitCastExpr(const CastExpr *expr) {
  visit(expr->getExpr());
  auto e = takeExpr();

  hir::CastOp castOp = static_cast<hir::CastOp>(0);

  // [FIX] Stubbing type conversion
  const hir::HIRType *targetType = nullptr;

  lastExpr = std::make_unique<hir::HIRCastExpr>(std::move(e), targetType,
                                                castOp, expr->getLoc());
}

void HIRGen::visitCharLiteral(const CharLiteral *expr) {
  lastExpr = std::make_unique<hir::HIRIntegerLiteral>(
      static_cast<uint64_t>(expr->getValue()), nullptr, expr->getLoc());
}

void HIRGen::visitTemplateStringExpr(const TemplateStringExpr *expr) {
  lastExpr = std::make_unique<hir::HIRStringLiteral>("<template_string_stub>",
                                                     nullptr, expr->getLoc());
}

void HIRGen::visitArrayLiteral(const ArrayLiteral *expr) {
  std::vector<std::unique_ptr<hir::HIRExpr>> elements;
  for (const auto &el : expr->getElements()) {
    visit(el.get());
    elements.push_back(takeExpr());
  }
  lastExpr = std::make_unique<hir::HIRArrayLiteral>(std::move(elements),
                                                    nullptr, expr->getLoc());
}

void HIRGen::visitIndexExpr(const IndexExpr *expr) {
  visit(expr->getArray());
  auto base = takeExpr();
  visit(expr->getIndex());
  auto index = takeExpr();
  lastExpr = std::make_unique<hir::HIRIndexExpr>(
      std::move(base), std::move(index), nullptr, expr->getLoc());
}

void HIRGen::visitNewExpr(const NewExpr *expr) {
  std::vector<std::unique_ptr<hir::HIRExpr>> args;
  for (const auto &arg : expr->getArgs()) {
    visit(arg.get());
    args.push_back(takeExpr());
  }

  // [FIX] Stubbing type conversion
  const hir::HIRType *type = nullptr;

  lastExpr =
      std::make_unique<hir::HIRNewExpr>(std::move(args), type, expr->getLoc());
}

void HIRGen::visitLambdaExpr(const LambdaExpr *expr) {
  std::vector<hir::HIRLambdaParam> params;
  for (const auto &p : expr->getParams()) {
    // [FIX] Stubbing type
    params.emplace_back(p.getName(), nullptr);
  }

  visit(expr->getBody());
  auto body = castToHIR<hir::HIRStmt>(takeStmt());

  lastExpr = std::make_unique<hir::HIRLambdaExpr>(
      std::move(params), std::move(body), nullptr, expr->getLoc());
}

void HIRGen::visitThisExpr(const ThisExpr *expr) {
  lastExpr = std::make_unique<hir::HIRThisExpr>(nullptr, expr->getLoc());
}

void HIRGen::visitSuperExpr(const SuperExpr *expr) {
  lastExpr = std::make_unique<hir::HIRIdentifierExpr>("super", nullptr,
                                                      expr->getLoc());
}
