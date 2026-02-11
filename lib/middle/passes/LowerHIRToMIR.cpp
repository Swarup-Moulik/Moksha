#include "moksha/MIR/LowerHIRToMIR.h"
#include "moksha/HIR/HIRExpr.h"
#include "moksha/HIR/HIRFunction.h"
#include "moksha/HIR/HIRModule.h"
#include "moksha/HIR/HIRStmt.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/HIR/HIRVisitor.h"
#include "moksha/MIR/MIRArgument.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRBuilder.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRGlobal.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"

// [FIX] Stub DiagnosticEngine if the header is unavailable in this context
namespace {
struct DiagnosticEngine {
  template <typename... Args> void report(Args... args) {}
};
} // namespace

#include <iostream>
#include <memory>
#include <stack>
#include <unordered_map>
#include <vector>

namespace moksha {
namespace mir {

namespace {

// Helper to check for terminators
static MIRInst *getTerminator(MIRBlock *block) {
  if (!block || block->getInstructions().empty())
    return nullptr;
  MIRInst *last = block->getInstructions().back().get();
  if (last->getOpcode() == Opcode::Br || last->getOpcode() == Opcode::CondBr ||
      last->getOpcode() == Opcode::Return ||
      last->getOpcode() == Opcode::Switch) {
    return last;
  }
  return nullptr;
}

class HIRToMIRConverter : public hir::ConstHIRVisitor {
public:
  HIRToMIRConverter(const hir::HIRModule *hirModule, DiagnosticEngine &diags)
      : hirModule(hirModule), diags(diags) {
    mirModule = std::make_unique<MIRModule>(hirModule->getName().str());
    builder = std::make_unique<MIRBuilder>();
  }

  std::unique_ptr<MIRModule> run() {
    for (const auto *func : hirModule->getFunctions()) {
      createFunctionDecl(func);
    }
    for (const auto *func : hirModule->getFunctions()) {
      if (!func->isExtern()) {
        // [FIX] Dereference pointer to pass as reference
        visitFunction(*func);
      }
    }
    return std::move(mirModule);
  }

private:
  const hir::HIRModule *hirModule;
  std::unique_ptr<MIRModule> mirModule;
  DiagnosticEngine &diags;
  std::unique_ptr<MIRBuilder> builder;

  MIRFunction *currFunc = nullptr;
  MIRValue *lastExprValue = nullptr;
  std::unordered_map<std::string, MIRValue *> symbolMap;

  // Type conversion (Identity for now)
  const hir::HIRType *getMIRType(const hir::HIRType *t) { return t; }

  MIRBlock *newBlock(const std::string &name) {
    auto block = std::make_unique<MIRBlock>(name, currFunc);
    MIRBlock *ptr = block.get();
    currFunc->addBlock(std::move(block));
    return ptr;
  }

  void createFunctionDecl(const hir::HIRFunction *hirFunc) {
    auto mirFunc = std::make_unique<MIRFunction>(
        hirFunc->getReturnType(), hirFunc->getName(), Linkage::External);

    unsigned idx = 0;
    for (const auto &p : hirFunc->getParams()) {
      mirFunc->addArgument(
          std::make_unique<MIRArgument>(mirFunc.get(), p.getType(), idx++));
    }
    mirModule->addFunction(std::move(mirFunc));
  }

  // --- Visitor Implementations ---

  // [FIX] Signature updated to match ConstHIRVisitor (reference instead of
  // pointer)
  void visitFunction(const hir::HIRFunction &func) override {
    currFunc = mirModule->getFunction(func.getName());
    symbolMap.clear();

    MIRBlock *entryBlock = newBlock("entry");
    builder->setInsertPoint(entryBlock);

    const auto &mirArgs = currFunc->getArguments();
    const auto &hirParams = func.getParams();

    for (size_t i = 0; i < mirArgs.size(); ++i) {
      MIRArgument *arg = mirArgs[i].get();
      auto *alloca = builder->insert(std::make_unique<AllocaInst>(
          arg->getType(), hirParams[i].getName() + ".addr",
          hirParams[i].getLoc()));

      builder->insert(
          std::make_unique<StoreInst>(arg, alloca, hirParams[i].getLoc()));
      symbolMap[hirParams[i].getName()] = alloca;
    }

    if (func.getBody()) {
      visit(func.getBody());
    }

    if (!getTerminator(builder->getInsertBlock())) {
      builder->createRetVoid();
    }
  }

  void visitBlockStmt(const hir::BlockStmt &stmt) override {
    for (const auto &s : stmt.getStatements()) {
      visit(s.get());
      if (builder->getInsertBlock() && getTerminator(builder->getInsertBlock()))
        break;
    }
  }

  void visitReturnStmt(const hir::ReturnStmt &stmt) override {
    MIRValue *val = nullptr;
    if (stmt.getReturnValue()) {
      visit(stmt.getReturnValue());
      val = lastExprValue;
    }
    if (val)
      builder->createRet(val, stmt.getLoc());
    else
      builder->createRetVoid(stmt.getLoc());
  }

  void visitIfStmt(const hir::IfStmt &stmt) override {
    visit(stmt.getCondition());
    MIRValue *cond = lastExprValue;

    MIRBlock *thenBlock = newBlock("if.then");
    MIRBlock *elseBlock = newBlock("if.else");
    MIRBlock *mergeBlock = newBlock("if.end");

    builder->createCondBr(cond, thenBlock, elseBlock);

    builder->setInsertPoint(thenBlock);
    visit(stmt.getThenBranch());
    if (!getTerminator(builder->getInsertBlock()))
      builder->createBr(mergeBlock);

    builder->setInsertPoint(elseBlock);
    if (stmt.getElseBranch())
      visit(stmt.getElseBranch());
    if (!getTerminator(builder->getInsertBlock()))
      builder->createBr(mergeBlock);

    builder->setInsertPoint(mergeBlock);
  }

  // Stubs
  void visitWhileStmt(const hir::WhileStmt &) override {}
  void visitDoWhileStmt(const hir::DoWhileStmt &) override {}
  void visitForStmt(const hir::ForStmt &) override {}
  void visitForInStmt(const hir::ForInStmt &) override {}
  void visitSwitchStmt(const hir::SwitchStmt &) override {}
  void visitBreakStmt(const hir::BreakStmt &) override {}
  void visitContinueStmt(const hir::ContinueStmt &) override {}
  void visitDeferStmt(const hir::DeferStmt &) override {}
  void visitTryCatchStmt(const hir::TryCatchStmt &) override {}
  void visitLockStmt(const hir::LockStmt &) override {}
  void visitExprStmt(const hir::ExprStmt &) override {}

  void visitUnsafeBlockStmt(const hir::UnsafeBlockStmt &stmt) override {
    visitBlockStmt(stmt);
  }

  void visitVarDeclStmt(const hir::HIRVarDeclStmt &stmt) override {
    auto *alloca = builder->insert(std::make_unique<AllocaInst>(
        stmt.getType(), stmt.getName(), stmt.getLoc()));
    symbolMap[stmt.getName()] = alloca;
    if (stmt.getInit()) {
      visit(stmt.getInit());
      builder->insert(
          std::make_unique<StoreInst>(lastExprValue, alloca, stmt.getLoc()));
    }
  }

  void visitBinaryExpr(const hir::HIRBinaryExpr &expr) override {
    visit(expr.getLHS());
    MIRValue *lhs = lastExprValue;
    visit(expr.getRHS());
    MIRValue *rhs = lastExprValue;
    lastExprValue = builder->insert(
        std::make_unique<BinaryInst>(Opcode::Add, lhs, rhs, "", expr.getLoc()));
  }

  void visitIntegerLiteral(const hir::HIRIntegerLiteral &expr) override {
    lastExprValue = new ConstantInt(expr.getValue(), nullptr);
  }

  // Expression Stubs
  void visitFloatLiteral(const hir::HIRFloatLiteral &) override {}
  void visitBoolLiteral(const hir::HIRBoolLiteral &) override {}
  void visitStringLiteral(const hir::HIRStringLiteral &) override {}
  void visitNullLiteral(const hir::HIRNullLiteral &) override {}
  void visitArrayLiteral(const hir::HIRArrayLiteral &) override {}

  void visitIdentifierExpr(const hir::HIRIdentifierExpr &expr) override {
    if (symbolMap.count(expr.getName())) {
      auto *ptr = symbolMap[expr.getName()];
      lastExprValue =
          builder->insert(std::make_unique<LoadInst>(ptr, "", expr.getLoc()));
    }
  }

  void visitCallExpr(const hir::HIRCallExpr &) override {}
  void visitUnaryExpr(const hir::HIRUnaryExpr &) override {}
  void visitMemberExpr(const hir::HIRMemberExpr &) override {}
  void visitIndexExpr(const hir::HIRIndexExpr &) override {}
  void visitTernaryExpr(const hir::HIRTernaryExpr &) override {}
  void visitCastExpr(const hir::HIRCastExpr &) override {}
  void visitNewExpr(const hir::HIRNewExpr &) override {}
  void visitLambdaExpr(const hir::HIRLambdaExpr &) override {}
  void visitThreadExpr(const hir::HIRThreadExpr &) override {}
  void visitThisExpr(const hir::HIRThisExpr &) override {}

  void visit(const hir::HIRStmt *stmt) {
    if (stmt)
      stmt->accept(*this);
  }
  void visit(const hir::HIRExpr *expr) {
    if (expr)
      expr->accept(*this);
  }
};

} // namespace

std::unique_ptr<MIRModule> LowerHIRToMIR(const hir::HIRModule *hirModule,
                                         DiagnosticEngine &diags) {
  return HIRToMIRConverter(hirModule, diags).run();
}

} // namespace mir
} // namespace moksha
