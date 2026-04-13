#include "moksha/AST/Stmt.h"
#include "moksha/AST/ASTVisitor.h"

namespace moksha {
// --- Declarations ---
void ModuleDecl::accept(ASTVisitor &v) const { v.visitModuleDecl(this); }
void VariableDecl::accept(ASTVisitor &v) const { v.visitVariableDecl(this); }
void FunctionDecl::accept(ASTVisitor &v) const { v.visitFunctionDecl(this); }
void ClassDecl::accept(ASTVisitor &v) const { v.visitClassDecl(this); }
void GenericDecl::accept(ASTVisitor &v) const { v.visitGenericDecl(this); }
void ImportDecl::accept(ASTVisitor &v) const { v.visitImportDecl(this); }
void EnumDecl::accept(ASTVisitor &v) const { v.visitEnumDecl(this); }
void MacroDecl::accept(ASTVisitor &v) const { v.visitMacroDecl(this); }
void UsingDecl::accept(ASTVisitor &v) const { v.visitUsingDecl(this); }

// --- Statements ---
void BlockStmt::accept(ASTVisitor &v) const { v.visitBlockStmt(this); }
void ExpressionStmt::accept(ASTVisitor &v) const {
  v.visitExpressionStmt(this);
}
void DeclStmt::accept(ASTVisitor &v) const { v.visitDeclStmt(this); }
void ReturnStmt::accept(ASTVisitor &v) const { v.visitReturnStmt(this); }
void BreakStmt::accept(ASTVisitor &v) const { v.visitBreakStmt(this); }
void ContinueStmt::accept(ASTVisitor &v) const { v.visitContinueStmt(this); }
void IfStmt::accept(ASTVisitor &v) const { v.visitIfStmt(this); }
void WhileStmt::accept(ASTVisitor &v) const { v.visitWhileStmt(this); }
void DoWhileStmt::accept(ASTVisitor &v) const { v.visitDoWhileStmt(this); }
void ForStmt::accept(ASTVisitor &v) const { v.visitForStmt(this); }
void ForInStmt::accept(ASTVisitor &v) const { v.visitForInStmt(this); }
void SwitchStmt::accept(ASTVisitor &v) const { v.visitSwitchStmt(this); }
void DeferStmt::accept(ASTVisitor &v) const { v.visitDeferStmt(this); }
void UnsafeBlockStmt::accept(ASTVisitor &v) const {
  v.visitUnsafeBlockStmt(this);
}
void TryCatchStmt::accept(ASTVisitor &v) const { v.visitTryCatchStmt(this); }
void ThrowStmt::accept(ASTVisitor &v) const { v.visitThrowStmt(this); }
void AsmStmt::accept(ASTVisitor &v) const { v.visitAsmStmt(this); }
void LockStmt::accept(ASTVisitor &v) const { v.visitLockStmt(this); }

// --- Helper Struct Clones ---
SwitchCase SwitchCase::clone() const {
  std::vector<ExprPtr> clonedVals;
  for (const auto &v : values)
    clonedVals.push_back(v->clone());
  auto clonedBody = body ? body->cloneAs<BlockStmt>() : nullptr;
  return SwitchCase(std::move(clonedVals), std::move(clonedBody), isDefault);
}

// --- Declaration Cloning ---

std::unique_ptr<Decl> ModuleDecl::clone() const {
  std::vector<DeclPtr> clonedDecls;
  for (const auto &d : decls)
    clonedDecls.push_back(d->clone());
  return std::make_unique<ModuleDecl>(name, std::move(clonedDecls), visibility,
                                      loc);
}

std::unique_ptr<Decl> VariableDecl::clone() const {
  auto cloned = std::make_unique<VariableDecl>(
      type->clone(), name, initializer ? initializer->clone() : nullptr,
      isConstVar(), isStaticVar(), isSharedVar(), visibility, loc);
  cloned->setVolatile(isVolatileVar());
  cloned->setExtern(isExternVar());
  cloned->setAlignment(getAlignment());
  cloned->setSection(getSection());
  cloned->setUsed(isUsedVar());
  cloned->setBitWidth(getBitWidth());
  cloned->setThreadLocal(isThreadLocalVar());
  cloned->setBitfield(this->getBitWidth());
  cloned->setBitOffset(this->getBitOffset());
  cloned->setPhysicalIndex(this->getPhysicalIndex());
  return cloned;
}

std::unique_ptr<Decl> FunctionDecl::clone() const {
  std::vector<Param> clonedParams;
  for (const auto &p : params)
    clonedParams.push_back(p.clone());
  auto cloned = std::make_unique<FunctionDecl>(
      name, std::move(clonedParams), returnType->clone(),
      body ? body->clone() : nullptr, isAsyncFunc(), isStaticFunc(),
      isVariadicFunc(), isWeakFunc(), visibility, loc);
  cloned->setBuiltin(isBuiltinFunc());
  cloned->setIntrinsicKind(getIntrinsicKind());
  cloned->setExtern(isExternFunc());
  cloned->setExternLinkage(getExternLinkage());
  cloned->setInterrupt(isInterruptFunc());
  cloned->setNaked(isNakedFunc());
  cloned->setNoReturn(isNoReturnFunc());
  cloned->setNoInline(isNoInlineFunc());
  cloned->setUsed(isUsedFunc());
  cloned->setSection(getSection());
  cloned->setVirtual(isVirtualFunc());
  cloned->setOverride(isOverrideFunc());
  cloned->setVTableIndex(getVTableIndex());
  cloned->setInline(isInlineFunc());
  cloned->setPure(isPureFunc());
  cloned->setCold(isColdFunc());
  return cloned;
}

std::unique_ptr<Decl> ClassDecl::clone() const {
  std::vector<DeclPtr> clonedMembers;
  for (const auto &m : members)
    clonedMembers.push_back(m->clone());
  auto cloned = std::make_unique<ClassDecl>(
      name, parentNames, std::move(clonedMembers), isReferenceType(),
      getAggregateKind(), visibility, loc);
  cloned->setPacked(isPackedClass());
  cloned->setAlignment(getAlignment());
  cloned->setSection(getSection());
  cloned->setHasVTable(hasVTable());
  return cloned;
}

std::unique_ptr<Decl> GenericDecl::clone() const {
  return std::make_unique<GenericDecl>(name, typeParams, innerDecl->clone(),
                                       loc);
}

std::unique_ptr<Decl> ImportDecl::clone() const {
  return std::make_unique<ImportDecl>(name, symbols, loc);
}

std::unique_ptr<Decl> EnumDecl::clone() const {
  std::vector<Case> clonedCases;
  for (const auto &c : cases)
    clonedCases.push_back(c.clone());
  return std::make_unique<EnumDecl>(name, std::move(clonedCases), visibility,
                                    loc);
}

std::unique_ptr<Decl> MacroDecl::clone() const {
  std::vector<StmtPtr> clonedBody;
  for (const auto &s : body)
    clonedBody.push_back(s->clone());
  return std::make_unique<MacroDecl>(name, params, std::move(clonedBody),
                                     isFunctionMacro(), loc);
}

std::unique_ptr<Decl> UsingDecl::clone() const {
  return std::make_unique<UsingDecl>(name, targetType->clone(), loc);
}

// --- Statement Cloning ---

std::unique_ptr<Stmt> BlockStmt::clone() const {
  std::vector<StmtPtr> clonedStmts;
  for (const auto &s : statements)
    clonedStmts.push_back(s->clone());
  return std::make_unique<BlockStmt>(std::move(clonedStmts), loc);
}

std::unique_ptr<Stmt> ExpressionStmt::clone() const {
  return std::make_unique<ExpressionStmt>(expression->clone(), loc);
}

std::unique_ptr<Stmt> DeclStmt::clone() const {
  return std::make_unique<DeclStmt>(decl->clone(), loc);
}

std::unique_ptr<Stmt> ReturnStmt::clone() const {
  return std::make_unique<ReturnStmt>(
      returnValue ? returnValue->clone() : nullptr, loc);
}

std::unique_ptr<Stmt> BreakStmt::clone() const {
  return std::make_unique<BreakStmt>(loc);
}
std::unique_ptr<Stmt> ContinueStmt::clone() const {
  return std::make_unique<ContinueStmt>(loc);
}

std::unique_ptr<Stmt> IfStmt::clone() const {
  return std::make_unique<IfStmt>(condition->clone(), thenStmt->clone(),
                                  elseStmt ? elseStmt->clone() : nullptr, loc);
}

std::unique_ptr<Stmt> WhileStmt::clone() const {
  return std::make_unique<WhileStmt>(condition->clone(), body->clone(), loc);
}

std::unique_ptr<Stmt> DoWhileStmt::clone() const {
  return std::make_unique<DoWhileStmt>(body->clone(), condition->clone(), loc);
}

std::unique_ptr<Stmt> ForStmt::clone() const {
  return std::make_unique<ForStmt>(
      init ? init->clone() : nullptr, condition ? condition->clone() : nullptr,
      increment ? increment->clone() : nullptr, body->clone(), loc);
}

std::unique_ptr<Stmt> ForInStmt::clone() const {
  return std::make_unique<ForInStmt>(
      variable->clone(), indexVariable ? indexVariable->clone() : nullptr,
      collection->clone(), body->clone(), loc);
}

std::unique_ptr<Stmt> SwitchStmt::clone() const {
  std::vector<SwitchCase> clonedCases;
  for (const auto &c : cases)
    clonedCases.push_back(c.clone());
  return std::make_unique<SwitchStmt>(condition->clone(),
                                      std::move(clonedCases), loc);
}

std::unique_ptr<Stmt> DeferStmt::clone() const {
  return std::make_unique<DeferStmt>(deferredStmt->clone(), loc);
}

std::unique_ptr<Stmt> UnsafeBlockStmt::clone() const {
  std::vector<StmtPtr> clonedStmts;
  for (const auto &s : getStatements())
    clonedStmts.push_back(s->clone());
  return std::make_unique<UnsafeBlockStmt>(std::move(clonedStmts), loc);
}

std::unique_ptr<Stmt> TryCatchStmt::clone() const {
  return std::make_unique<TryCatchStmt>(
      tryBody->clone(), catchVar ? catchVar->clone() : nullptr,
      catchBody ? catchBody->clone() : nullptr,
      finallyBody ? finallyBody->clone() : nullptr, loc);
}

std::unique_ptr<Stmt> ThrowStmt::clone() const {
  return std::make_unique<ThrowStmt>(expression ? expression->clone() : nullptr,
                                     loc);
}

std::unique_ptr<Stmt> AsmStmt::clone() const {
  return std::make_unique<AsmStmt>(getAssemblyStr(), getConstraints(), loc);
}

std::unique_ptr<Stmt> LockStmt::clone() const {
  return std::make_unique<LockStmt>(target ? target->clone() : nullptr,
                                    body ? body->clone() : nullptr, loc);
}

std::unique_ptr<Expr> InputExpr::clone() const {
  return std::make_unique<InputExpr>(prompt ? prompt->clone() : nullptr, loc);
}

} // namespace moksha
