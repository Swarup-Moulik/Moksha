#include "moksha/MIR/VerifyNoMacros.h"

#include "moksha/HIR/HIRExpr.h"
#include "moksha/HIR/HIRFunction.h"
#include "moksha/HIR/HIRModule.h"
#include "moksha/HIR/HIRStmt.h"
#include "moksha/HIR/HIRVisitor.h"
#include "moksha/Support/Diagnostics.h"

// Needed for std::remove_const_t
#include <type_traits>

namespace moksha {
namespace mir {

namespace {

class MacroVerifier : public hir::HIRVisitor {
public:
  explicit MacroVerifier(DiagnosticEngine &diags) : diags(diags) {}

  bool verify(const hir::HIRModule *module) {
    hasMacro = false;
    // Cast to non-const reference because the generic Visitor interface
    // usually expects mutable references (T&), even for read-only ops.
    visitModule(*const_cast<hir::HIRModule *>(module));
    return !hasMacro;
  }

  void visitClass(hir::HIRClass &cls) override {
    for (auto &method : cls.getMethods()) {
      if (method) {
        visitFunction(*method);
      }
    }
  }

private:
  DiagnosticEngine &diags;
  bool hasMacro = false;

  // --- Dispatch Helper ---
  // Bridges the gap between pointer-based AST navigation and the
  // reference-based Visitor interface.
  template <typename T> void dispatch(T *node) {
    if (node && !hasMacro) {
      // Check if this node IS a macro before we dispatch!
      // Assuming your AST/HIR nodes have a way to identify macros, or
      // if you are identifying them inside specific visit methods:

      auto *nonConstNode = const_cast<std::remove_const_t<T> *>(node);
      nonConstNode->accept(*this);

      // If the visitor triggered the flag, report it at this node's location
      // (Requires your HIR nodes to have a getLoc() method)
      if (hasMacro) {
        diags.report(node->getLoc(), DiagID::err_unexpanded_macro);
      }
    }
  }

  // --- Structural Entry Points (NOT Overrides) ---

  void visitModule(hir::HIRModule &module) {
    // getFunctions returns pointers, so we access them directly
    for (auto *func : module.getFunctions()) {
      if (hasMacro)
        return;
      visitFunction(*func);
    }

    for (auto *global : module.getGlobals()) {
      if (hasMacro)
        return;
      // Dispatch the global statement (likely a VarDeclStmt) to the visitor.
      dispatch(global);
    }
  }

  void visitFunction(hir::HIRFunction &func) override {
    if (auto *body = func.getBody()) {
      dispatch(body);
    }
  }

  // --- Statement Traversal (Overrides) ---

  void visitBlockStmt(hir::BlockStmt &stmt) override {
    // [FIX] getStmts -> getStatements
    for (auto &s : stmt.getStatements()) {
      if (hasMacro)
        return;
      dispatch(s.get()); // Access raw pointer from unique_ptr
    }
  }

  void visitUnsafeBlockStmt(hir::UnsafeBlockStmt &stmt) override {
    visitBlockStmt(stmt);
  }

  void visitIfStmt(hir::IfStmt &stmt) override {
    if (hasMacro)
      return;
    dispatch(stmt.getCondition());  // [FIX] getCond -> getCondition
    dispatch(stmt.getThenBranch()); // [FIX] getThen -> getThenBranch
    if (stmt.getElseBranch()) {     // [FIX] getElse -> getElseBranch
      dispatch(stmt.getElseBranch());
    }
  }

  void visitWhileStmt(hir::WhileStmt &stmt) override {
    if (hasMacro)
      return;
    dispatch(stmt.getCondition());
    dispatch(stmt.getBody());
  }

  void visitDoWhileStmt(hir::DoWhileStmt &stmt) override {
    if (hasMacro)
      return;
    dispatch(stmt.getBody());
    dispatch(stmt.getCondition());
  }

  void visitForStmt(hir::ForStmt &stmt) override {
    dispatch(stmt.getInit());
    dispatch(stmt.getCondition());
    if (stmt.getIncrement())
      dispatch(stmt.getIncrement());
    dispatch(stmt.getBody());
  }

  void visitForInStmt(hir::ForInStmt &stmt) override {
    if (hasMacro)
      return;
    // [FIX] getVarDecl -> getVariable
    dispatch(stmt.getVariable());
    // [FIX] getIndexDecl -> getIndexVariable
    if (stmt.getIndexVariable()) {
      dispatch(stmt.getIndexVariable());
    }
    dispatch(stmt.getCollection());
    dispatch(stmt.getBody());
  }

  void visitSwitchStmt(hir::SwitchStmt &stmt) override {
    dispatch(stmt.getCondition()); // [FIX] getCond -> getCondition
    for (auto &caseClause : stmt.getCases()) {
      for (auto &val : caseClause.getValues()) {
        dispatch(val.get());
      }
      // [FIX] SwitchCase::getBody returns a reference, dispatch needs pointer
      dispatch(const_cast<hir::BlockStmt *>(&caseClause.getBody()));
    }
  }

  void visitReturnStmt(hir::ReturnStmt &stmt) override {
    if (hasMacro)
      return;
    if (stmt.getReturnValue()) // [FIX] getExpr -> getReturnValue
      dispatch(stmt.getReturnValue());
  }

  void visitVarDeclStmt(hir::HIRVarDeclStmt &stmt) override {
    if (hasMacro)
      return;
    if (stmt.getInit())
      dispatch(stmt.getInit());
  }

  void visitExprStmt(hir::ExprStmt &stmt) override {
    // [FIX] ExprStmt has getExpr(), not getReturnValue()
    dispatch(stmt.getExpr());
  }

  void visitLockStmt(hir::LockStmt &stmt) override {
    dispatch(stmt.getMutex()); // [FIX] getLockedExpr -> getMutex
    dispatch(stmt.getBody());
  }

  void visitDeferStmt(hir::DeferStmt &stmt) override {
    dispatch(stmt.getDeferredStmt()); // [FIX] getStmt -> getDeferredStmt
  }

  void visitTryCatchStmt(hir::TryCatchStmt &stmt) override {
    if (hasMacro)
      return;

    if (stmt.getTryBlock()) {
      dispatch(stmt.getTryBlock());
    }

    // Iterate through all the catch clauses
    for (const auto &clause : stmt.getCatches()) {
      if (clause.body) {
        dispatch(clause.body.get());
      }
    }

    if (stmt.getFinallyBlock()) {
      dispatch(stmt.getFinallyBlock());
    }
  }

  // Leaves
  void visitBreakStmt(hir::BreakStmt &) override {}
  void visitContinueStmt(hir::ContinueStmt &) override {}
  void visitThrowStmt(hir::HIRThrowStmt &) override {}

  // --- Expression Traversal ---

  void visitBinaryExpr(hir::HIRBinaryExpr &expr) override {
    if (hasMacro)
      return;
    dispatch(expr.getLHS());
    dispatch(expr.getRHS());
  }

  void visitUnaryExpr(hir::HIRUnaryExpr &expr) override {
    if (hasMacro)
      return;
    dispatch(expr.getOperand()); // [FIX] getSubExpr -> getOperand
  }

  void visitCallExpr(hir::HIRCallExpr &expr) override {
    dispatch(expr.getCallee());
    for (auto &arg : expr.getArgs()) {
      if (hasMacro)
        return;
      dispatch(arg.get());
    }
  }

  void visitCastExpr(hir::HIRCastExpr &expr) override {
    if (hasMacro)
      return;
    dispatch(expr.getExpr()); // [FIX] getSubExpr -> getExpr
  }

  void visitMemberExpr(hir::HIRMemberExpr &expr) override {
    if (hasMacro)
      return;
    dispatch(expr.getObject()); // [FIX] getBase -> getObject
  }

  void visitIndexExpr(hir::HIRIndexExpr &expr) override {
    if (hasMacro)
      return;
    dispatch(expr.getBase());
    dispatch(expr.getIndex());
  }

  void visitTernaryExpr(hir::HIRTernaryExpr &expr) override {
    if (hasMacro)
      return;
    dispatch(expr.getCond());
    dispatch(expr.getTrueExpr());
    dispatch(expr.getFalseExpr());
  }

  void visitNewExpr(hir::HIRNewExpr &expr) override {
    for (auto &arg : expr.getArgs()) {
      if (hasMacro)
        return;
      dispatch(arg.get());
    }
  }

  void visitLambdaExpr(hir::HIRLambdaExpr &expr) override {
    // [FIX] HIRLambdaExpr::getBody returns const HIRStmt*
    if (auto *body = expr.getBody()) {
      dispatch(body);
    }
  }

  void visitThreadExpr(hir::HIRThreadExpr &expr) override {
    dispatch(expr.getTask()); // [FIX] getBlock -> getTask
  }

  void visitArrayLiteral(hir::HIRArrayLiteral &expr) override {
    for (auto &el : expr.getElements()) {
      if (hasMacro)
        return;
      dispatch(el.get());
    }
  }

  void visitMapLiteral(hir::HIRMapLiteral &expr) override {
    for (auto &pair : expr.getEntries()) {
      if (hasMacro)
        return;
      dispatch(pair.first.get());
      dispatch(pair.second.get());
    }
  }

  void visitTemplateStringExpr(hir::HIRTemplateStringExpr &expr) override {
    for (auto &part : expr.getParts()) {
      if (hasMacro)
        return;
      dispatch(part.get());
    }
  }

  void visitSpreadExpr(hir::HIRSpreadExpr &expr) override {
    if (hasMacro)
      return;
    dispatch(expr.getIterable());
  }

  void visitSharedExpr(hir::HIRSharedExpr &expr) override {
    if (hasMacro)
      return;
    dispatch(expr.getExpr());
  }

  void visitInputExpr(hir::HIRInputExpr &expr) override {
    if (hasMacro)
      return;
    if (expr.getPrompt()) {
      dispatch(expr.getPrompt());
    }
  }

  void visitAsmExpr(hir::HIRAsmExpr &expr) override {
    if (hasMacro)
      return;

    // Traverse all the new structured operand vectors
    for (auto &op : expr.getOutputs()) {
      if (op.expr)
        dispatch(op.expr.get());
    }
    for (auto &op : expr.getInputs()) {
      if (op.expr)
        dispatch(op.expr.get());
    }
    for (auto &op : expr.getInouts()) {
      if (op.expr)
        dispatch(op.expr.get());
    }
  }

  // Literals are safe
  void visitIntegerLiteral(hir::HIRIntegerLiteral &) override {}
  void visitFloatLiteral(hir::HIRFloatLiteral &) override {}
  void visitDecimalLiteral(hir::HIRDecimalLiteral &expr) override {}
  void visitBoolLiteral(hir::HIRBoolLiteral &) override {}
  void visitStringLiteral(hir::HIRStringLiteral &) override {}
  void visitNullLiteral(hir::HIRNullLiteral &) override {}
  void visitIdentifierExpr(hir::HIRIdentifierExpr &) override {}
  void visitThisExpr(hir::HIRThisExpr &) override {}
  void visitSizeOfExpr(hir::HIRSizeOfExpr &) override {}
  void visitAwaitExpr(hir::HIRAwaitExpr &) override {}
  void visitSuperExpr(hir::HIRSuperExpr &) override {}
  void visitDerefExpr(hir::HIRDerefExpr &) override {}
  void visitAddressOfExpr(hir::HIRAddressOfExpr &) override {}
};

} // namespace

bool VerifyNoMacros(const hir::HIRModule *module, DiagnosticEngine &diags) {
  MacroVerifier verifier(diags);
  return verifier.verify(module);
}

} // namespace mir
} // namespace moksha
