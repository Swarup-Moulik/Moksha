#pragma once

#include "moksha/HIR/HIRModule.h"
#include "moksha/HIR/HIRVisitor.h"
#include "moksha/Support/Diagnostics.h"
#include <functional>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

namespace moksha {
namespace ownership {

enum class BorrowState { Unborrowed, ViewBorrowed, MutBorrowed, LockAcquired };

struct StorageInfo {
  const hir::HIRType *baseType;
  BorrowState currentState;
  int activeViewCount;
};

class BorrowChecker : public hir::ConstHIRVisitor {
public:
  explicit BorrowChecker(DiagnosticEngine &diags);

  void checkModule(const hir::HIRModule &module);

  // --- Core Overrides ---
  void visitVarDeclStmt(const hir::HIRVarDeclStmt &stmt) override;
  void visitLockStmt(const hir::LockStmt &stmt) override;
  void visitBlockStmt(const hir::BlockStmt &stmt) override;
  void visitAddressOfExpr(const hir::HIRAddressOfExpr &expr) override;
  void visitIfStmt(const hir::IfStmt &stmt) override;
  void visitWhileStmt(const hir::WhileStmt &stmt) override;
  void visitTryCatchStmt(const hir::TryCatchStmt &stmt) override;

  // --- Moved out of header to fix "incomplete type" ---
  void visitExprStmt(const hir::ExprStmt &stmt) override;
  void visitReturnStmt(const hir::ReturnStmt &stmt) override;

  // --- Control Flow Overrides ---
  void visitUnsafeBlockStmt(const hir::UnsafeBlockStmt &stmt) override;
  void visitDoWhileStmt(const hir::DoWhileStmt &stmt) override;
  void visitForStmt(const hir::ForStmt &stmt) override;
  void visitForInStmt(const hir::ForInStmt &stmt) override;
  void visitSwitchStmt(const hir::SwitchStmt &stmt) override;
  void visitDeferStmt(const hir::DeferStmt &stmt) override;
  void visitBreakStmt(const hir::BreakStmt &) override {}
  void visitContinueStmt(const hir::ContinueStmt &) override {}
  void visitAsmStmt(const hir::HIRAsmStmt &) override {}
  void visitThrowStmt(const hir::HIRThrowStmt &stmt) override;

  void visitIntegerLiteral(const hir::HIRIntegerLiteral &) override {}
  void visitFloatLiteral(const hir::HIRFloatLiteral &) override {}
  void visitBoolLiteral(const hir::HIRBoolLiteral &) override {}
  void visitStringLiteral(const hir::HIRStringLiteral &) override {}
  void visitTemplateStringExpr(const hir::HIRTemplateStringExpr &expr) override;
  void visitNullLiteral(const hir::HIRNullLiteral &) override {}
  void visitArrayLiteral(const hir::HIRArrayLiteral &) override {}
  void visitMapLiteral(const hir::HIRMapLiteral &) override {}

  void visitBinaryExpr(const hir::HIRBinaryExpr &expr) override;
  void visitUnaryExpr(const hir::HIRUnaryExpr &expr) override;
  void visitCastExpr(const hir::HIRCastExpr &expr) override;
  void visitTernaryExpr(const hir::HIRTernaryExpr &expr) override;
  void visitIdentifierExpr(const hir::HIRIdentifierExpr &) override {}
  void visitMemberExpr(const hir::HIRMemberExpr &expr) override;
  void visitIndexExpr(const hir::HIRIndexExpr &expr) override;
  void visitCallExpr(const hir::HIRCallExpr &expr) override;
  void visitDerefExpr(const hir::HIRDerefExpr &expr) override;
  void visitThisExpr(const hir::HIRThisExpr &) override {}
  void visitNewExpr(const hir::HIRNewExpr &) override {}
  void visitLambdaExpr(const hir::HIRLambdaExpr &expr) override;
  void visitThreadExpr(const hir::HIRThreadExpr &expr) override;
  void visitSizeOfExpr(const hir::HIRSizeOfExpr &) override {}
  void visitAwaitExpr(const hir::HIRAwaitExpr &expr) override;
  void visitSuperExpr(const hir::HIRSuperExpr &) override {}
  void visitFunction(const hir::HIRFunction &func) override;
  void visitSpreadExpr(const hir::HIRSpreadExpr &expr) override;

private:
  DiagnosticEngine &diags;

  using Scope = std::unordered_map<std::string, StorageInfo>;
  std::list<Scope> scopeStack;
  std::vector<std::vector<std::function<void()>>> cleanupStack;

  void pushScope();
  void popScope();

  StorageInfo *lookupStorage(const std::string &name);
  bool canBorrow(BorrowState current, BorrowState requested);
};

} // namespace ownership
} // namespace moksha
