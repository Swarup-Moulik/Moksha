#include "moksha/Ownership/BorrowChecker.h"
#include "moksha/HIR/HIRExpr.h"
#include "moksha/HIR/HIRFunction.h"
#include "moksha/HIR/HIRStmt.h"
#include <algorithm>
#include <functional>
#include <map>

using namespace moksha;
using namespace moksha::ownership;

static std::map<std::string, std::string> pointerProvenance;
static int unsafeBlockDepth = 0;
static int currentTaskDepth = 0;
static std::map<std::string, int> varTaskDepth;

static bool hasView(const hir::HIRType *t) {
  if (!t)
    return false;

  // [FIX] Check semantic ownership first.
  // Borrowed ownership in HIR corresponds to 'view' semantics.
  if (t->getOwnership() == hir::Ownership::Borrowed)
    return true;

  // Fallback: Check for "view" in the string (for AST-heavy types)
  if (t->toString().find("view") != std::string::npos)
    return true;

  // Recursive check for wrapped types (like pointers)
  if (auto ptr = llvm::dyn_cast<hir::PointerType>(t))
    return hasView(ptr->getPointee());

  if (auto nullT = llvm::dyn_cast<hir::HIRNullableType>(t))
    return hasView(nullT->getInner());

  return false;
}

BorrowChecker::BorrowChecker(DiagnosticEngine &diags) : diags(diags) {}

void BorrowChecker::pushScope() {
  scopeStack.emplace_back();
  cleanupStack.emplace_back();
}

void BorrowChecker::popScope() {
  // Execute cleanups in REVERSE order (LIFO)
  auto &cleanups = cleanupStack.back();
  for (auto it = cleanups.rbegin(); it != cleanups.rend(); ++it) {
    (*it)();
  }

  cleanupStack.pop_back();
  scopeStack.pop_back();
}

StorageInfo *BorrowChecker::lookupStorage(const std::string &name) {
  for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
    if (it->find(name) != it->end()) {
      return &it->at(name);
    }
  }
  return nullptr;
}

bool BorrowChecker::canBorrow(BorrowState current, BorrowState requested) {
  switch (current) {
  case BorrowState::Unborrowed:
    return true;
  case BorrowState::ViewBorrowed:
    return requested == BorrowState::ViewBorrowed;
  case BorrowState::MutBorrowed:
    return false;
  case BorrowState::LockAcquired:
    return requested == BorrowState::ViewBorrowed;
  }
  return false;
}

void BorrowChecker::checkModule(const hir::HIRModule &module) {
  pointerProvenance.clear();
  unsafeBlockDepth = 0;
  currentTaskDepth = 0;
  varTaskDepth.clear();
  pushScope();
  for (const auto &global : module.getGlobals()) {
    global->accept(*this);
  }
  for (const auto &func : module.getFunctions()) {
    func->accept(*this);
  }
  for (auto *cls : module.getClasses()) {
    for (const auto &method : cls->getMethods()) {
      if (method)
        method->accept(*this);
    }
  }
  popScope();
}

void BorrowChecker::visitFunction(const hir::HIRFunction &func) {
  pushScope();

  // 1. Add function parameters to the local scope
  for (const auto &param : func.getParams()) {
    scopeStack.back().insert(
        {param.name, StorageInfo{param.type, BorrowState::Unborrowed, 0}});
  }

  // 2. Traverse the function body to check all statements
  if (func.getBody()) {
    func.getBody()->accept(*this);
  }

  popScope();
}

void BorrowChecker::visitExprStmt(const hir::ExprStmt &stmt) {
  if (stmt.getExpr())
    stmt.getExpr()->accept(*this);
}

void BorrowChecker::visitReturnStmt(const hir::ReturnStmt &stmt) {
  if (stmt.getReturnValue()) {
    stmt.getReturnValue()->accept(*this);

    // Escape Analysis: Prevent returning borrows to local variables
    if (auto *addrOf =
            llvm::dyn_cast<hir::HIRAddressOfExpr>(stmt.getReturnValue())) {
      if (auto *ident =
              llvm::dyn_cast<hir::HIRIdentifierExpr>(addrOf->getOperand())) {

        // If the variable exists in the current scope stack, it is local and
        // will die!
        StorageInfo *storage = lookupStorage(ident->getName());
        if (storage) {
          diags.report(stmt.getLoc(), DiagID::err_borrow_violation)
              << "Escape Analysis: Cannot return a borrow to local variable '"
              << ident->getName() << "' as it will become a dangling pointer.";
        }
      }
    }
  }
}

void BorrowChecker::visitVarDeclStmt(const hir::HIRVarDeclStmt &stmt) {
  varTaskDepth[stmt.getName()] = currentTaskDepth;
  if (stmt.getInit()) {
    stmt.getInit()->accept(*this);

    std::string targetName;
    if (auto *addrOf = llvm::dyn_cast<hir::HIRAddressOfExpr>(stmt.getInit())) {
      if (auto *id =
              llvm::dyn_cast<hir::HIRIdentifierExpr>(addrOf->getOperand()))
        targetName = id->getName();
    } else if (auto *cast = llvm::dyn_cast<hir::HIRCastExpr>(stmt.getInit())) {
      if (auto *addrOf =
              llvm::dyn_cast<hir::HIRAddressOfExpr>(cast->getExpr())) {
        if (auto *id =
                llvm::dyn_cast<hir::HIRIdentifierExpr>(addrOf->getOperand()))
          targetName = id->getName();
      }
    } else if (auto *id =
                   llvm::dyn_cast<hir::HIRIdentifierExpr>(stmt.getInit())) {
      targetName = pointerProvenance[id->getName()]; // Chain provenance
    }

    if (!targetName.empty()) {
      pointerProvenance[stmt.getName()] = targetName;
    }

    if (auto *ident = llvm::dyn_cast<hir::HIRIdentifierExpr>(stmt.getInit())) {
      StorageInfo *storage = lookupStorage(ident->getName());
      if (storage) {
        // Check if we are declaring a view pointer
        bool isTargetView = hasView(stmt.getType());

        // Allow aliasing a lock ONLY if the new variable is a view
        if (storage->currentState == BorrowState::MutBorrowed ||
            (storage->currentState == BorrowState::LockAcquired &&
             !isTargetView)) {
          diags.report(stmt.getLoc(), DiagID::err_borrow_violation)
              << "Cannot alias '" << ident->getName()
              << "' because it is exclusively borrowed or locked.";
        }
      }
    }
  }

  scopeStack.back().insert(
      {stmt.getName(),
       StorageInfo{stmt.getType(), BorrowState::Unborrowed, 0}});
}

void BorrowChecker::visitLockStmt(const hir::LockStmt &stmt) {
  stmt.getMutex()->accept(*this);

  if (auto idExpr =
          dynamic_cast<const hir::HIRIdentifierExpr *>(stmt.getMutex())) {
    StorageInfo *storage = lookupStorage(idExpr->getName());

    if (storage) {
      if (!canBorrow(storage->currentState, BorrowState::LockAcquired)) {
        // FIX: Updated diagnostic API
        diags.report(stmt.getLoc(), DiagID::err_invalid_type)
            << "cannot acquire lock; storage is already exclusively borrowed "
               "or locked in this scope";
        return;
      }

      BorrowState prevState = storage->currentState;
      storage->currentState = BorrowState::LockAcquired;

      pushScope();
      if (stmt.getBody())
        stmt.getBody()->accept(*this);
      popScope();

      storage->currentState = prevState;
    }
  } else {
    pushScope();
    if (stmt.getBody())
      stmt.getBody()->accept(*this);
    popScope();
  }
}

void BorrowChecker::visitBlockStmt(const hir::BlockStmt &stmt) {
  pushScope();
  for (const auto &s : stmt.getStatements()) {
    s->accept(*this);
  }
  popScope();
}

void BorrowChecker::visitAddressOfExpr(const hir::HIRAddressOfExpr &expr) {
  // 1. Traverse child first
  if (expr.getOperand())
    expr.getOperand()->accept(*this);

  // 2. Extract the variable being borrowed
  if (auto *ident = llvm::dyn_cast<hir::HIRIdentifierExpr>(expr.getOperand())) {

    // Thread Escape Analysis
    if (varTaskDepth.count(ident->getName()) &&
        varTaskDepth[ident->getName()] < currentTaskDepth) {
      // Allow global variables to be borrowed (they live forever at scope level
      // 0)
      bool isGlobal =
          (!scopeStack.empty() && scopeStack.front().count(ident->getName()));
      if (!isGlobal) {
        diags.report(expr.getLoc(), DiagID::err_borrow_violation)
            << "Thread Escape: Cannot borrow local variable '"
            << ident->getName()
            << "' inside a thread block. The thread may outlive the stack "
               "frame.";
        return;
      }
    }

    StorageInfo *storage = lookupStorage(ident->getName());
    if (!storage)
      return;

    // 3. Determine requested borrow type
    BorrowState requested = expr.isMutableBorrow() ? BorrowState::MutBorrowed
                                                   : BorrowState::ViewBorrowed;

    // 4. Enforce aliasing rules
    if (!canBorrow(storage->currentState, requested)) {
      std::string reqStr =
          (requested == BorrowState::MutBorrowed) ? "mutable" : "view";
      diags.report(
          expr.getLoc(),
          DiagID::err_borrow_violation) // Ensure you define this DiagID!
          << "Cannot borrow '" << ident->getName() << "' as " << reqStr
          << " because it is already borrowed.";
      return;
    }

    // 5. Apply the borrow and queue the cleanup!
    BorrowState prevState = storage->currentState;
    if (requested == BorrowState::ViewBorrowed) {
      storage->currentState = BorrowState::ViewBorrowed;
      storage->activeViewCount++;

      cleanupStack.back().push_back([storage, prevState]() {
        storage->activeViewCount--;
        if (storage->activeViewCount == 0) {
          storage->currentState = prevState;
        }
      });
    } else {
      storage->currentState = BorrowState::MutBorrowed;

      cleanupStack.back().push_back(
          [storage, prevState]() { storage->currentState = prevState; });
    }
  }
}

void BorrowChecker::visitIfStmt(const hir::IfStmt &stmt) {
  if (stmt.getCondition())
    stmt.getCondition()->accept(*this);

  // 1. THEN Branch
  pushScope();
  if (stmt.getThenBranch()) {
    // Unwrap block to prevent double-scoping
    if (auto *block = llvm::dyn_cast<hir::BlockStmt>(stmt.getThenBranch())) {
      for (const auto &s : block->getStatements()) {
        if (s)
          s->accept(*this);
      }
    } else {
      stmt.getThenBranch()->accept(*this);
    }
  }
  popScope(); // Safely executes LIFO cleanups!

  // 2. ELSE Branch
  pushScope();
  if (stmt.getElseBranch()) {
    if (auto *block = llvm::dyn_cast<hir::BlockStmt>(stmt.getElseBranch())) {
      for (const auto &s : block->getStatements()) {
        if (s)
          s->accept(*this);
      }
    } else {
      stmt.getElseBranch()->accept(*this);
    }
  }
  popScope(); // Safely executes LIFO cleanups!
}

void BorrowChecker::visitWhileStmt(const hir::WhileStmt &stmt) {
  if (stmt.getCondition())
    stmt.getCondition()->accept(*this);

  pushScope();
  if (stmt.getBody())
    stmt.getBody()->accept(*this);
  popScope();
}

void BorrowChecker::visitTryCatchStmt(const hir::TryCatchStmt &stmt) {
  pushScope();
  if (stmt.getTryBody())
    stmt.getTryBody()->accept(*this);
  popScope();

  pushScope();
  if (stmt.getCatchBody())
    stmt.getCatchBody()->accept(*this);
  popScope();
}

void BorrowChecker::visitBinaryExpr(const hir::HIRBinaryExpr &expr) {
  if (expr.getLHS())
    expr.getLHS()->accept(*this);
  if (expr.getRHS())
    expr.getRHS()->accept(*this);

  if (expr.getOp() == hir::BinaryOp::Assign) {
    std::string lhsName;
    const hir::HIRIdentifierExpr *lhsIdent = nullptr;

    if (auto *id = llvm::dyn_cast<hir::HIRIdentifierExpr>(expr.getLHS())) {
      lhsIdent = id;
      lhsName = id->getName();
    } else if (auto *deref = llvm::dyn_cast<hir::HIRDerefExpr>(expr.getLHS())) {
      lhsIdent = llvm::dyn_cast<hir::HIRIdentifierExpr>(deref->getPointer());

      // Provenance Validation: Block unsafe tainted pointers mutating immutable
      // data
      if (lhsIdent && unsafeBlockDepth == 0) {
        std::string ptrName = lhsIdent->getName();
        if (pointerProvenance.count(ptrName)) {
          std::string target = pointerProvenance[ptrName];
          StorageInfo *targetStorage = lookupStorage(target);
          if (targetStorage && hasView(targetStorage->baseType)) {
            diags.report(expr.getLoc(), DiagID::err_borrow_violation)
                << "The safe world cannot use `" << ptrName
                << "` because its provenance traces back to immutable storage. "
                   "Unsafe capabilities cannot escape the block.";
          }
        }
      }
    }

    // Maintain Pointer Provenance dynamically
    if (!lhsName.empty()) {
      std::string targetName;
      if (auto *addrOf = llvm::dyn_cast<hir::HIRAddressOfExpr>(expr.getRHS())) {
        if (auto *id =
                llvm::dyn_cast<hir::HIRIdentifierExpr>(addrOf->getOperand()))
          targetName = id->getName();
      } else if (auto *cast = llvm::dyn_cast<hir::HIRCastExpr>(expr.getRHS())) {
        if (auto *addrOf =
                llvm::dyn_cast<hir::HIRAddressOfExpr>(cast->getExpr())) {
          if (auto *id =
                  llvm::dyn_cast<hir::HIRIdentifierExpr>(addrOf->getOperand()))
            targetName = id->getName();
        }
      } else if (auto *id =
                     llvm::dyn_cast<hir::HIRIdentifierExpr>(expr.getRHS())) {
        targetName =
            pointerProvenance[id->getName()]; // Chain provenance tracking
      }

      if (!targetName.empty()) {
        pointerProvenance[lhsName] = targetName;
      } else {
        pointerProvenance.erase(lhsName); // Clear taint if assigned safe value
      }
    }

    if (lhsIdent) {
      StorageInfo *storage = lookupStorage(lhsIdent->getName());
      // LOCK ELEVATION: Allow mutation if state is Unborrowed OR
      // LockAcquired
      if (storage && storage->currentState != BorrowState::Unborrowed &&
          storage->currentState != BorrowState::LockAcquired) {
        diags.report(expr.getLoc(), DiagID::err_invalid_type)
            << "Cannot mutate '" << lhsIdent->getName()
            << "' directly while it is actively borrowed.";
      }
    }

    // Handle aliasing on the RHS
    if (auto *rhsIdent =
            llvm::dyn_cast<hir::HIRIdentifierExpr>(expr.getRHS())) {
      StorageInfo *storage = lookupStorage(rhsIdent->getName());
      if (storage) {
        bool isLHSView = false;
        if (lhsIdent) {
          if (auto *lhsStorage = lookupStorage(lhsIdent->getName()))
            isLHSView = hasView(lhsStorage->baseType);
        }

        if (storage->currentState == BorrowState::MutBorrowed ||
            (storage->currentState == BorrowState::LockAcquired &&
             !isLHSView)) {
          diags.report(expr.getLoc(), DiagID::err_borrow_violation)
              << "Cannot alias '" << rhsIdent->getName()
              << "' because it is exclusively borrowed or locked.";
        }
      }
    }
  }
}

void BorrowChecker::visitTemplateStringExpr(
    const hir::HIRTemplateStringExpr &expr) {
  for (const auto &part : expr.getParts()) {
    if (part)
      part->accept(*this);
  }
}

void BorrowChecker::visitSpreadExpr(const hir::HIRSpreadExpr &expr) {
  if (expr.getIterable())
    expr.getIterable()->accept(*this);
}

// ============================================================================
// [Control Flow & Loops]
// ============================================================================

void BorrowChecker::visitUnsafeBlockStmt(const hir::UnsafeBlockStmt &stmt) {
  unsafeBlockDepth++;
  pushScope();
  for (const auto &s : stmt.getStatements()) {
    if (s)
      s->accept(*this);
  }
  popScope();
  unsafeBlockDepth--;
}

void BorrowChecker::visitDoWhileStmt(const hir::DoWhileStmt &stmt) {
  pushScope();
  // Traverse body first, then condition (matching execution order)
  if (stmt.getBody())
    stmt.getBody()->accept(*this);
  if (stmt.getCondition())
    stmt.getCondition()->accept(*this);
  popScope();
}

void BorrowChecker::visitForStmt(const hir::ForStmt &stmt) {
  pushScope();
  if (stmt.getInit())
    stmt.getInit()->accept(*this);
  if (stmt.getCondition())
    stmt.getCondition()->accept(*this);
  if (stmt.getIncrement())
    stmt.getIncrement()->accept(*this);
  if (stmt.getBody())
    stmt.getBody()->accept(*this);
  popScope();
}

void BorrowChecker::visitForInStmt(const hir::ForInStmt &stmt) {
  pushScope();
  if (stmt.getCollection())
    stmt.getCollection()->accept(*this);

  // Register the loop variables in the new scope
  if (stmt.getVariable()) {
    stmt.getVariable()->accept(*this);
  }
  if (stmt.getIndexVariable()) {
    stmt.getIndexVariable()->accept(*this);
  }

  if (stmt.getBody())
    stmt.getBody()->accept(*this);
  popScope();
}

void BorrowChecker::visitSwitchStmt(const hir::SwitchStmt &stmt) {
  if (stmt.getCondition())
    stmt.getCondition()->accept(*this);

  for (const auto &c : stmt.getCases()) {
    for (const auto &val : c.getValues()) {
      if (val)
        val->accept(*this);
    }
    c.getBody().accept(*this);
  }
}

void BorrowChecker::visitDeferStmt(const hir::DeferStmt &stmt) {
  if (stmt.getDeferredStmt())
    stmt.getDeferredStmt()->accept(*this);
}

// ============================================================================
// [Remaining Statements & Expressions]
// ============================================================================

void BorrowChecker::visitThrowStmt(const hir::HIRThrowStmt &stmt) {
  if (stmt.getExpr()) {
    stmt.getExpr()->accept(*this);
  }
}

void BorrowChecker::visitLambdaExpr(const hir::HIRLambdaExpr &expr) {
  pushScope(); // Lambdas create a hard boundary for variables!

  // Register lambda parameters into the new scope
  for (const auto &param : expr.getParams()) {
    scopeStack.back().insert(
        {param.name, StorageInfo{param.type, BorrowState::Unborrowed, 0}});
  }

  // Traverse the body of the lambda
  if (expr.getBody()) {
    expr.getBody()->accept(*this);
  }

  popScope();
}

void BorrowChecker::visitThreadExpr(const hir::HIRThreadExpr &expr) {
  currentTaskDepth++;
  if (expr.getTask()) {
    expr.getTask()->accept(*this);
  }
  currentTaskDepth--;
}

// ============================================================================
// [Expressions]
// ============================================================================

void BorrowChecker::visitCallExpr(const hir::HIRCallExpr &expr) {
  if (expr.getCallee()) {
    expr.getCallee()->accept(*this);
  }
  // Temporary Scope for Function Arguments
  pushScope();
  for (const auto &arg : expr.getArgs()) {
    if (arg) {
      arg->accept(*this);
    }
  }
  popScope();
}

void BorrowChecker::visitUnaryExpr(const hir::HIRUnaryExpr &expr) {
  if (expr.getOperand())
    expr.getOperand()->accept(*this);
}

void BorrowChecker::visitCastExpr(const hir::HIRCastExpr &expr) {
  if (expr.getExpr())
    expr.getExpr()->accept(*this);
}

void BorrowChecker::visitTernaryExpr(const hir::HIRTernaryExpr &expr) {
  if (expr.getCond())
    expr.getCond()->accept(*this);
  if (expr.getTrueExpr())
    expr.getTrueExpr()->accept(*this);
  if (expr.getFalseExpr())
    expr.getFalseExpr()->accept(*this);
}

void BorrowChecker::visitIndexExpr(const hir::HIRIndexExpr &expr) {
  if (expr.getBase())
    expr.getBase()->accept(*this);
  if (expr.getIndex())
    expr.getIndex()->accept(*this);
}

void BorrowChecker::visitMemberExpr(const hir::HIRMemberExpr &expr) {
  if (expr.getObject())
    expr.getObject()->accept(*this);
}

void BorrowChecker::visitDerefExpr(const hir::HIRDerefExpr &expr) {
  if (expr.getPointer())
    expr.getPointer()->accept(*this);
}

void BorrowChecker::visitAwaitExpr(const hir::HIRAwaitExpr &expr) {
  if (expr.getExpr()) {
    expr.getExpr()->accept(*this);
  }

  // If we have more than 1 scope, we have local variables
  if (scopeStack.size() > 1) {
    auto it = scopeStack.begin();
    ++it; // Skip the global scope (globals are safe to hold across await)

    for (; it != scopeStack.end(); ++it) {
      for (const auto &pair : *it) {
        if (pair.second.currentState != BorrowState::Unborrowed) {
          diags.report(expr.getLoc(), DiagID::err_borrow_violation)
              << "Async Borrow: Cannot hold a borrow across an `await` "
                 "suspension point. "
              << "Local variable '" << pair.first << "' is actively borrowed.";
        }
      }
    }
  }
}
