#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "moksha/Macros/Macro.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {

using llvm::dyn_cast;
using llvm::isa;

/** @brief Substitutes macro parameters with their corresponding arguments. */
class MacroSubstituter {
  const std::vector<MacroParam> &params;
  const std::vector<std::unique_ptr<Expr>> &args;
  ASTContext &ctx;
  bool isHygieneEnabled;
  std::unordered_map<std::string, std::string> localRenameMap;

public:
  MacroSubstituter(const std::vector<MacroParam> &params,
                   const std::vector<std::unique_ptr<Expr>> &args,
                   ASTContext &ctx, bool enableHygiene = false)
      : params(params), args(args), ctx(ctx), isHygieneEnabled(enableHygiene) {}

  TypePtr cloneType(const Type *t) {
    if (!t)
      return nullptr;
    SourceLocation loc = t->getLoc();
    if (auto *p = dyn_cast<PrimitiveType>(t))
      return std::make_unique<PrimitiveType>(p->getScalar(), loc);
    if (isa<AnyType>(t))
      return std::make_unique<AnyType>(loc);
    if (isa<NullType>(t))
      return std::make_unique<NullType>(loc);
    if (auto *p = dyn_cast<PointerType>(t))
      return std::make_unique<PointerType>(cloneType(p->getPointee()), loc);
    if (auto *r = dyn_cast<ReferenceType>(t))
      return std::make_unique<ReferenceType>(cloneType(r->getInner()), loc);
    if (auto *n = dyn_cast<NullableType>(t))
      return std::make_unique<NullableType>(cloneType(n->getInner()), loc);
    if (auto *l = dyn_cast<LockType>(t))
      return std::make_unique<LockType>(cloneType(l->getInner()), loc);
    if (auto *v = dyn_cast<ViewType>(t))
      return std::make_unique<ViewType>(cloneType(v->getInner()), loc);
    if (auto *m = dyn_cast<MutType>(t))
      return std::make_unique<MutType>(cloneType(m->getInner()), loc);
    if (auto *a = dyn_cast<ArrayType>(t))
      return std::make_unique<ArrayType>(cloneType(a->getElementType()),
                                         cloneExpr(a->getSizeExpr()), loc);
    if (auto *s = dyn_cast<SliceType>(t))
      return std::make_unique<SliceType>(cloneType(s->getElementType()), loc);
    if (auto *m = dyn_cast<MapType>(t))
      return std::make_unique<MapType>(cloneType(m->getKeyType()),
                                       cloneType(m->getValueType()), loc);
    if (auto *f = dyn_cast<FunctionType>(t)) {
      std::vector<TypePtr> newParams;
      for (const auto &pt : f->getParamTypes())
        newParams.push_back(cloneType(pt.get()));
      return std::make_unique<FunctionType>(
          cloneType(f->getReturnType()), std::move(newParams),
          f->isVariadicFunc(), f->isInterruptFunc(), loc);
    }
    if (auto *nt = dyn_cast<NamedType>(t)) {
      std::vector<NamedType::GenericArg> newArgs;
      for (const auto &arg : nt->getGenericArgs()) {
        newArgs.push_back({cloneType(arg.type.get()), arg.variance});
      }
      return std::make_unique<NamedType>(nt->getName(), std::move(newArgs),
                                         loc);
    }
    if (auto *c = dyn_cast<ClosureType>(t)) {
      std::vector<TypePtr> newParams;
      for (const auto &pt : c->getParamTypes()) {
        newParams.push_back(cloneType(pt.get()));
      }
      return std::make_unique<ClosureType>(cloneType(c->getReturnType()),
                                           std::move(newParams), loc);
    }
    if (auto *p = dyn_cast<PromiseType>(t))
      return std::make_unique<PromiseType>(cloneType(p->getInner()), loc);
    if (auto *c = dyn_cast<ConstType>(t))
      return std::make_unique<ConstType>(cloneType(c->getInner()), loc);
    if (auto *v = dyn_cast<VolatileType>(t))
      return std::make_unique<VolatileType>(cloneType(v->getInner()), loc);
    if (auto *w = dyn_cast<WeakType>(t))
      return std::make_unique<WeakType>(cloneType(w->getInner()), loc);
    if (auto *e = dyn_cast<EnumType>(t))
      return std::make_unique<EnumType>(e->getName(), e->getMembers(), loc);
    if (auto *d = dyn_cast<DecimalType>(t))
      return std::make_unique<DecimalType>(d->getPrecision(), d->getScale(),
                                           loc);
    return std::make_unique<AnyType>(loc);
  }

  std::unique_ptr<Expr> cloneExpr(const Expr *e) {
    if (!e)
      return nullptr;
    SourceLocation loc = e->getLoc();

    // Literals
    if (auto *l = dyn_cast<IntegerLiteral>(e))
      return std::make_unique<IntegerLiteral>(l->getValue(),
                                              NumericSuffix::None, loc);
    if (auto *l = dyn_cast<FloatLiteral>(e))
      return std::make_unique<FloatLiteral>(l->getValue(), NumericSuffix::None,
                                            loc);
    if (auto *l = dyn_cast<DecimalLiteral>(e))
      return std::make_unique<DecimalLiteral>(l->getValue(), loc);
    if (auto *l = dyn_cast<StringLiteral>(e))
      return std::make_unique<StringLiteral>(l->getValue(), false, loc);
    if (auto *l = dyn_cast<BoolLiteral>(e))
      return std::make_unique<BoolLiteral>(l->getValue(), loc);
    if (auto *l = dyn_cast<CharLiteral>(e))
      return std::make_unique<CharLiteral>(l->getValue(), loc);
    if (isa<NullLiteral>(e))
      return std::make_unique<NullLiteral>(loc);
    if (auto *arr = dyn_cast<ArrayLiteral>(e)) {
      std::vector<ExprPtr> newElems;
      for (const auto &el : arr->getElements())
        newElems.push_back(cloneExpr(el.get()));
      return std::make_unique<ArrayLiteral>(std::move(newElems), loc);
    }
    if (auto *map = dyn_cast<MapLiteral>(e)) {
      std::vector<MapLiteral::Entry> newEntries;
      for (const auto &entry : map->getEntries()) {
        newEntries.push_back(
            {cloneExpr(entry.first.get()), cloneExpr(entry.second.get())});
      }
      return std::make_unique<MapLiteral>(std::move(newEntries), loc);
    }

    // Identifiers & Macro Argument Substitution
    if (auto *id = dyn_cast<IdentifierExpr>(e)) {
      std::string name = id->getName();

      // Only apply hygiene map if enabled
      if (isHygieneEnabled && localRenameMap.count(name)) {
        return std::make_unique<IdentifierExpr>(localRenameMap[name], loc);
      }

      // Check if this identifier is one of the macro's parameters
      for (size_t i = 0; i < params.size(); ++i) {
        if (params[i].name == name) {
          if (i < args.size() && args[i]) {
            std::vector<MacroParam> shadowedParams = params;
            shadowedParams.erase(shadowedParams.begin() + i);
            MacroSubstituter subSubstituter(shadowedParams, args, ctx,
                                            isHygieneEnabled);
            return subSubstituter.cloneExpr(args[i].get());
          }
        }
      }
      return std::make_unique<IdentifierExpr>(name, loc);
    }

    // Object & Class context
    if (isa<ThisExpr>(e))
      return std::make_unique<ThisExpr>(loc);
    if (isa<SuperExpr>(e))
      return std::make_unique<SuperExpr>(loc);

    // Core Operations
    if (auto *bin = dyn_cast<BinaryExpr>(e))
      return std::make_unique<BinaryExpr>(cloneExpr(bin->getLHS()),
                                          bin->getOp(),
                                          cloneExpr(bin->getRHS()), loc);
    if (auto *un = dyn_cast<UnaryExpr>(e))
      return std::make_unique<UnaryExpr>(
          un->getOp(), cloneExpr(un->getOperand()), un->isPostfixOp(), loc);
    if (auto *tern = dyn_cast<TernaryExpr>(e))
      return std::make_unique<TernaryExpr>(
          cloneExpr(tern->getCondition()), cloneExpr(tern->getTrueBranch()),
          cloneExpr(tern->getFalseBranch()), loc);
    if (auto *cast = dyn_cast<CastExpr>(e))
      return std::make_unique<CastExpr>(cloneType(cast->getTargetType()),
                                        cloneExpr(cast->getExpr()), loc);
    if (auto *bitcast = dyn_cast<BitcastExpr>(e))
      return std::make_unique<BitcastExpr>(cloneType(bitcast->getTargetType()),
                                           cloneExpr(bitcast->getExpr()), loc);

    // Access & Calls
    if (auto *call = dyn_cast<CallExpr>(e)) {
      std::vector<ExprPtr> newArgs;
      for (const auto &arg : call->getArgs())
        newArgs.push_back(cloneExpr(arg.get()));
      return std::make_unique<CallExpr>(cloneExpr(call->getCallee()),
                                        std::move(newArgs), loc);
    }
    if (auto *mem = dyn_cast<MemberExpr>(e)) {
      auto cloned = std::make_unique<MemberExpr>(cloneExpr(mem->getObject()),
                                                 mem->getName(),
                                                 mem->isOptionalAccess(), loc);
      cloned->setType(mem->getType());
      cloned->setLayoutInfo(mem->getMemberIndex(), mem->isBitfield(),
                            mem->getBitWidth(), mem->getBitOffset());
      cloned->setVirtualMethodInfo(mem->isVirtualMethod(),
                                   mem->getMemberIndex());
      cloned->setQualifiedParent(mem->getQualifiedParent());
      cloned->setParentUpcast(mem->isParentUpcast());
      return cloned;
    }
    if (auto *idx = dyn_cast<IndexExpr>(e)) {
      return std::make_unique<IndexExpr>(cloneExpr(idx->getArray()),
                                         cloneExpr(idx->getIndex()),
                                         idx->isOptionalAccess(), loc);
    }

    // Functions & Threads
    if (auto *lam = dyn_cast<LambdaExpr>(e)) {
      std::vector<LambdaParam> newParams;
      for (const auto &p : lam->getParams()) {
        newParams.push_back(LambdaParam(cloneType(p.getType()), p.getName(),
                                        cloneExpr(p.getDefaultValue())));
      }
      auto clonedLam = std::make_unique<LambdaExpr>(
          std::move(newParams), cloneStmt(lam->getBody()),
          lam->isExpressionBody(), lam->getCaptureMode(), loc);
      clonedLam->setAsync(lam->isAsyncLambda());
      return clonedLam;
    }
    if (auto *ne = dyn_cast<NewExpr>(e)) {
      std::vector<ExprPtr> newArgs;
      for (const auto &arg : ne->getArgs())
        newArgs.push_back(cloneExpr(arg.get()));
      return std::make_unique<NewExpr>(cloneType(ne->getType()),
                                       std::move(newArgs), loc);
    }
    if (auto *th = dyn_cast<ThreadExpr>(e)) {
      auto clonedBody = cloneExpr(th->getBody());
      std::unique_ptr<LambdaExpr> lambdaBody(
          static_cast<LambdaExpr *>(clonedBody.release()));
      return std::make_unique<ThreadExpr>(th->isWeakThread(),
                                          std::move(lambdaBody), loc);
    }
    if (auto *aw = dyn_cast<AwaitExpr>(e)) {
      return std::make_unique<AwaitExpr>(cloneExpr(aw->getExpr()), loc);
    }

    // Misc
    if (auto *ts = dyn_cast<TemplateStringExpr>(e)) {
      std::vector<ExprPtr> newParts;
      for (const auto &part : ts->getParts())
        newParts.push_back(cloneExpr(part.get()));
      return std::make_unique<TemplateStringExpr>(std::move(newParts), loc);
    }
    if (auto *sz = dyn_cast<SizeOfExpr>(e)) {
      return std::make_unique<SizeOfExpr>(cloneExpr(sz->getExpr()), loc);
    }
    if (auto *inp = dyn_cast<InputExpr>(e)) {
      return std::make_unique<InputExpr>(cloneExpr(inp->getPrompt()), loc);
    }
    if (auto *ae = dyn_cast<AsmExpr>(e)) {
      auto cloneOps = [&](const std::vector<AsmExpr::AsmOperand> &ops) {
        std::vector<AsmExpr::AsmOperand> clonedOps;
        for (const auto &op : ops) {
          clonedOps.emplace_back(op.constraint, cloneExpr(op.expr.get()));
        }
        return clonedOps;
      };

      auto cloned = std::make_unique<AsmExpr>(
          ae->getAssemblyStr(), cloneOps(ae->getOutputs()),
          cloneOps(ae->getInputs()), cloneOps(ae->getInouts()),
          ae->getClobbers(), ae->getIsVolatile(), nullptr, loc);
      cloned->setType(ae->getType());
      return cloned;
    }
    return nullptr;
  }

  std::unique_ptr<Decl> cloneDecl(const Decl *d) {
    if (!d)
      return nullptr;
    SourceLocation loc = d->getLoc();
    std::string name = d->getName();

    bool isParam = false;
    for (size_t i = 0; i < params.size(); ++i) {
      if (params[i].name == name) {
        if (i < args.size() && args[i]) {
          if (auto *idExpr = dyn_cast<IdentifierExpr>(args[i].get())) {
            name = idExpr->getName();
            isParam = true;
          }
        }
      }
    }

    if (isHygieneEnabled && !isParam && isa<VariableDecl>(d)) {
      std::string uniqueName =
          name + "_h" + std::to_string(reinterpret_cast<uintptr_t>(d));
      localRenameMap[d->getName()] = uniqueName;
      name = uniqueName;
    }

    if (auto *vd = dyn_cast<VariableDecl>(d)) {
      auto clonedVar = std::unique_ptr<VariableDecl>(
          static_cast<VariableDecl *>(vd->clone().release()));

      clonedVar->setName(name);
      clonedVar->setType(cloneType(vd->getType()));
      clonedVar->getInitializerMut() = cloneExpr(vd->getInitializer());

      return clonedVar;
    }
    if (auto *ud = dyn_cast<UsingDecl>(d)) {
      return std::make_unique<UsingDecl>(name, cloneType(ud->getTargetType()),
                                         loc);
    }
    return nullptr;
  }

  std::unique_ptr<Stmt> cloneStmt(const Stmt *s) {
    if (!s)
      return nullptr;
    SourceLocation loc = s->getLoc();

    if (auto *es = dyn_cast<ExpressionStmt>(s))
      return std::make_unique<ExpressionStmt>(cloneExpr(es->getExpr()), loc);
    if (auto *ds = dyn_cast<DeclStmt>(s))
      return std::make_unique<DeclStmt>(cloneDecl(ds->getDecl()), loc);
    if (auto *bs = dyn_cast<BlockStmt>(s)) {
      std::vector<StmtPtr> newStmts;
      for (const auto &sub : bs->getStatements())
        newStmts.push_back(cloneStmt(sub.get()));
      return std::make_unique<BlockStmt>(std::move(newStmts), loc);
    }
    if (auto *rs = dyn_cast<ReturnStmt>(s)) {
      return std::make_unique<ReturnStmt>(cloneExpr(rs->getReturnValue()), loc);
    }
    if (auto *is = dyn_cast<IfStmt>(s)) {
      return std::make_unique<IfStmt>(cloneExpr(is->getCondition()),
                                      cloneStmt(is->getThenStmt()),
                                      cloneStmt(is->getElseStmt()), loc);
    }
    if (auto *ws = dyn_cast<WhileStmt>(s)) {
      return std::make_unique<WhileStmt>(cloneExpr(ws->getCondition()),
                                         cloneStmt(ws->getBody()), loc);
    }
    if (auto *ds = dyn_cast<DoWhileStmt>(s)) {
      return std::make_unique<DoWhileStmt>(cloneStmt(ds->getBody()),
                                           cloneExpr(ds->getCondition()), loc);
    }
    if (auto *fs = dyn_cast<ForStmt>(s)) {
      return std::make_unique<ForStmt>(
          cloneStmt(fs->getInit()), cloneExpr(fs->getCondition()),
          cloneExpr(fs->getIncrement()), cloneStmt(fs->getBody()), loc);
    }
    if (auto *fis = dyn_cast<ForInStmt>(s)) {
      return std::make_unique<ForInStmt>(
          cloneDecl(fis->getVariable()), cloneDecl(fis->getIndexVariable()),
          cloneExpr(fis->getCollection()), cloneStmt(fis->getBody()), loc);
    }
    if (auto *sw = dyn_cast<SwitchStmt>(s)) {
      std::vector<SwitchCase> newCases;
      for (const auto &c : sw->getCases()) {
        std::vector<ExprPtr> newVals;
        for (const auto &v : c.getValues())
          newVals.push_back(cloneExpr(v.get()));
        auto clonedBody = cloneStmt(c.getBody());
        std::unique_ptr<BlockStmt> blockBody(
            static_cast<BlockStmt *>(clonedBody.release()));
        newCases.push_back(SwitchCase(std::move(newVals), std::move(blockBody),
                                      c.isDefaultCase()));
      }
      return std::make_unique<SwitchStmt>(cloneExpr(sw->getCondition()),
                                          std::move(newCases), loc);
    }
    if (auto *def = dyn_cast<DeferStmt>(s)) {
      return std::make_unique<DeferStmt>(cloneStmt(def->getDeferredStmt()),
                                         loc);
    }
    if (auto *ub = dyn_cast<UnsafeBlockStmt>(s)) {
      std::vector<StmtPtr> newStmts;
      for (const auto &sub : ub->getStatements())
        newStmts.push_back(cloneStmt(sub.get()));
      return std::make_unique<UnsafeBlockStmt>(std::move(newStmts), loc);
    }
    if (auto *tc = dyn_cast<TryCatchStmt>(s)) {
      std::vector<CatchClause> newCatches;
      for (const auto &c : tc->getCatches()) {
        newCatches.push_back(CatchClause(cloneDecl(c.var.get()),
                                         cloneStmt(c.body.get()), c.loc));
      }
      return std::make_unique<TryCatchStmt>(
          cloneStmt(tc->getTryBody()), std::move(newCatches),
          cloneStmt(tc->getFinallyBody()), loc);
    }
    if (auto *ts = dyn_cast<ThrowStmt>(s)) {
      return std::make_unique<ThrowStmt>(cloneExpr(ts->getExpr()), loc);
    }
    if (auto *ls = dyn_cast<LockStmt>(s)) {
      return std::make_unique<LockStmt>(cloneExpr(ls->getTarget()),
                                        cloneStmt(ls->getBody()),
                                        ls->isAsyncLock(), loc);
    }
    if (isa<BreakStmt>(s))
      return std::make_unique<BreakStmt>(loc);
    if (isa<ContinueStmt>(s))
      return std::make_unique<ContinueStmt>(loc);

    return nullptr;
  }
};

/** @brief Helper function to expand an expression using macros. */
namespace {
void expandExprHelper(std::unique_ptr<Expr> &exprPtr, MacroTable &macros,
                      ASTContext &ctx, ASTVisitor *visitor,
                      DiagnosticEngine &Diags, int depth = 0) {
  if (!exprPtr)
    return;

  // Throw a proper compiler error before bailing out
  if (depth > 256) {
    Diags.report(exprPtr->getLoc(), DiagID::err_type_mismatch)
        << "Macro recursion depth exceeded (limit 256)";
    return;
  }

  exprPtr->accept(*visitor);

  if (auto *call = dyn_cast<CallExpr>(exprPtr.get())) {
    if (auto *id = dyn_cast<IdentifierExpr>(call->getCallee())) {
      if (const Macro *m = macros.lookup(id->getName())) {
        auto exStmts = m->expand(call->getArgs(), ctx);

        if (exStmts.size() == 1) {
          if (auto *exprStmt = dyn_cast<ExpressionStmt>(exStmts[0].get())) {
            exprPtr = exprStmt->getExpr()->clone();
          } else if (auto *retStmt = dyn_cast<ReturnStmt>(exStmts[0].get())) {
            exprPtr = retStmt->getReturnValue()->clone();
          }
        } else if (exStmts.size() > 1) {
          SourceLocation loc = call->getLoc();

          if (!exStmts.empty()) {
            if (auto *exprStmt =
                    dyn_cast<ExpressionStmt>(exStmts.back().get())) {
              auto retStmt = std::make_unique<ReturnStmt>(
                  exprStmt->getExpr()->clone(), exprStmt->getLoc());
              exStmts.pop_back();
              exStmts.push_back(std::move(retStmt));
            }
          }

          auto block = std::make_unique<BlockStmt>(std::move(exStmts), loc);
          auto lambda = std::make_unique<LambdaExpr>(std::vector<LambdaParam>{},
                                                     std::move(block), false,
                                                     CaptureMode::Mut, loc);

          std::vector<std::unique_ptr<Expr>> emptyArgs;
          exprPtr = std::make_unique<CallExpr>(std::move(lambda),
                                               std::move(emptyArgs), loc);
        }

        expandExprHelper(exprPtr, macros, ctx, visitor, Diags, depth + 1);
        return;
      }
    }
  }
}
} // namespace

/** @brief Macro Expander for Object-like macros. */
std::vector<std::unique_ptr<Stmt>>
ObjectMacro::expand(const std::vector<std::unique_ptr<Expr>> &args,
                    ASTContext &ctx) const {
  std::vector<std::unique_ptr<Stmt>> result;
  std::vector<MacroParam> emptyParams;
  MacroSubstituter substituter(emptyParams, args, ctx, true);

  auto clonedExpr = substituter.cloneExpr(value.get());
  if (clonedExpr)
    result.push_back(
        std::make_unique<ExpressionStmt>(std::move(clonedExpr), loc));
  return result;
}

std::vector<std::unique_ptr<Stmt>>
FunctionMacro::expand(const std::vector<std::unique_ptr<Expr>> &args,
                      ASTContext &ctx) const {
  std::vector<std::unique_ptr<Stmt>> result;
  if (args.size() != params.size())
    return result;
  MacroSubstituter substituter(params, args, ctx, true);
  for (const auto &stmt : body) {
    if (auto expandedStmt = substituter.cloneStmt(stmt.get()))
      result.push_back(std::move(expandedStmt));
  }
  return result;
}

void MacroExpander::visitModuleDecl(const ModuleDecl *decl) {
  for (const auto &d : decl->getDecls()) {
    if (auto *md = dyn_cast<MacroDecl>(d.get())) {
      std::vector<MacroParam> mParams;
      for (const auto &pName : md->getParams())
        mParams.push_back({pName, SourceLocation()});
      std::vector<std::unique_ptr<Stmt>> clonedBody;

      std::vector<MacroParam> emptyParams;
      std::vector<std::unique_ptr<Expr>> emptyArgs;
      MacroSubstituter substituter(emptyParams, emptyArgs, ctx, false);

      for (const auto &s : md->getBody()) {
        if (auto cs = substituter.cloneStmt(s.get()))
          clonedBody.push_back(std::move(cs));
      }

      if (md->isFunctionMacro())
        macros.addMacro(std::make_unique<FunctionMacro>(
            md->getName(), std::move(mParams), std::move(clonedBody),
            md->getLoc()));
    }
  }
  for (const auto &d : decl->getDecls()) {
    if (!isa<MacroDecl>(d.get()))
      d->accept(*this);
  }
}

void MacroExpander::visitBlockStmt(const BlockStmt *stmt) {
  auto &stmts =
      const_cast<std::vector<std::unique_ptr<Stmt>> &>(stmt->getStatements());
  std::vector<std::unique_ptr<Stmt>> newStmts;

  std::function<void(std::unique_ptr<Stmt> &, int)> processStmt =
      [&](std::unique_ptr<Stmt> &s, int depth) {
        // Throw a proper compiler error before bailing out
        if (depth > 256) {
          Diags.report(s->getLoc(), DiagID::err_type_mismatch)
              << "Macro recursion depth exceeded (limit 256)";
          newStmts.push_back(std::move(s));
          return;
        }

        if (auto *exprStmt = dyn_cast<ExpressionStmt>(s.get())) {
          if (auto *call = dyn_cast<CallExpr>(exprStmt->getExpr())) {
            if (auto *id = dyn_cast<IdentifierExpr>(call->getCallee())) {
              if (const Macro *m = macros.lookup(id->getName())) {

                for (auto &arg : const_cast<CallExpr *>(call)->getArgsMut()) {
                  expandExprHelper(arg, macros, ctx, this, Diags);
                }
                auto ex = m->expand(call->getArgs(), ctx);
                for (auto &es : ex) {
                  processStmt(es, depth + 1);
                }
                return;
              }
            }
          }
        }

        s->accept(*this);
        newStmts.push_back(std::move(s));
      };

  for (auto &s : stmts) {
    processStmt(s, 0);
  }

  stmts = std::move(newStmts);
}

void MacroExpander::visitFunctionDecl(const FunctionDecl *decl) {
  if (decl->getBody())
    decl->getBody()->accept(*this);
}

void MacroExpander::visitIfStmt(const IfStmt *stmt) {
  if (stmt->getCondition())
    stmt->getCondition()->accept(*this);
  if (stmt->getThenStmt())
    stmt->getThenStmt()->accept(*this);
  if (stmt->getElseStmt())
    stmt->getElseStmt()->accept(*this);
}

void MacroExpander::visitWhileStmt(const WhileStmt *stmt) {
  if (stmt->getCondition())
    stmt->getCondition()->accept(*this);
  if (stmt->getBody())
    stmt->getBody()->accept(*this);
}

void MacroExpander::visitDoWhileStmt(const DoWhileStmt *stmt) {
  if (stmt->getBody())
    stmt->getBody()->accept(*this);
  if (stmt->getCondition())
    stmt->getCondition()->accept(*this);
}

void MacroExpander::visitForStmt(const ForStmt *stmt) {
  if (stmt->getInit())
    stmt->getInit()->accept(*this);
  if (stmt->getCondition())
    stmt->getCondition()->accept(*this);
  if (stmt->getIncrement())
    stmt->getIncrement()->accept(*this);
  if (stmt->getBody())
    stmt->getBody()->accept(*this);
}

void MacroExpander::visitForInStmt(const ForInStmt *stmt) {
  if (stmt->getVariable())
    stmt->getVariable()->accept(*this);
  if (stmt->getIndexVariable())
    stmt->getIndexVariable()->accept(*this);
  if (stmt->getCollection())
    stmt->getCollection()->accept(*this);
  if (stmt->getBody())
    stmt->getBody()->accept(*this);
}

void MacroExpander::visitSwitchStmt(const SwitchStmt *stmt) {
  if (stmt->getCondition())
    stmt->getCondition()->accept(*this);
  for (const auto &c : stmt->getCases()) {
    for (const auto &v : c.getValues()) {
      if (v)
        v->accept(*this);
    }
    if (c.getBody())
      c.getBody()->accept(*this);
  }
}

void MacroExpander::visitTryCatchStmt(const TryCatchStmt *stmt) {
  if (stmt->getTryBody())
    stmt->getTryBody()->accept(*this);

  for (const auto &clause : stmt->getCatches()) {
    if (clause.var)
      clause.var->accept(*this);
    if (clause.body)
      clause.body->accept(*this);
  }

  if (stmt->getFinallyBody())
    stmt->getFinallyBody()->accept(*this);
}

void MacroExpander::visitLockStmt(const LockStmt *stmt) {
  if (stmt->getTarget())
    stmt->getTarget()->accept(*this);
  if (stmt->getBody())
    stmt->getBody()->accept(*this);
}

void MacroExpander::visitClassDecl(const ClassDecl *decl) {
  for (const auto &member : decl->getMembers()) {
    member->accept(*this);
  }
}

void MacroExpander::visitGenericDecl(const GenericDecl *decl) {
  if (decl->getInnerDecl())
    decl->getInnerDecl()->accept(*this);
}

void MacroExpander::visitVariableDecl(const VariableDecl *decl) {
  auto *mutDecl = const_cast<VariableDecl *>(decl);
  if (mutDecl->getInitializer()) {
    expandExprHelper(mutDecl->getInitializerMut(), macros, ctx, this, Diags);
  }
}

void MacroExpander::visitReturnStmt(const ReturnStmt *stmt) {
  auto *mutStmt = const_cast<ReturnStmt *>(stmt);
  if (mutStmt->getReturnValue()) {
    expandExprHelper(mutStmt->getReturnValueMut(), macros, ctx, this, Diags);
  }
}

void MacroExpander::visitDeclStmt(const DeclStmt *stmt) {
  if (stmt->getDecl())
    stmt->getDecl()->accept(*this);
}

void MacroExpander::visitExpressionStmt(const ExpressionStmt *stmt) {
  if (stmt->getExpr())
    stmt->getExpr()->accept(*this);
}

void MacroExpander::visitDeferStmt(const DeferStmt *stmt) {
  if (stmt->getDeferredStmt())
    stmt->getDeferredStmt()->accept(*this);
}

void MacroExpander::visitUnsafeBlockStmt(const UnsafeBlockStmt *stmt) {
  visitBlockStmt(stmt);
}

void MacroExpander::visitThrowStmt(const ThrowStmt *stmt) {
  if (stmt->getExpr())
    stmt->getExpr()->accept(*this);
}

void MacroExpander::visitPointerType(const PointerType *type) {
  if (type->getPointee())
    type->getPointee()->accept(*this);
}
void MacroExpander::visitReferenceType(const ReferenceType *type) {
  if (type->getInner())
    type->getInner()->accept(*this);
}
void MacroExpander::visitArrayType(const ArrayType *type) {
  if (type->getElementType())
    type->getElementType()->accept(*this);
  if (type->getSizeExpr())
    type->getSizeExpr()->accept(*this);
}
void MacroExpander::visitSliceType(const SliceType *type) {
  if (type->getElementType())
    type->getElementType()->accept(*this);
}
void MacroExpander::visitMapType(const MapType *type) {
  if (type->getKeyType())
    type->getKeyType()->accept(*this);
  if (type->getValueType())
    type->getValueType()->accept(*this);
}
void MacroExpander::visitFunctionType(const FunctionType *type) {
  if (type->getReturnType())
    type->getReturnType()->accept(*this);
  for (const auto &p : type->getParamTypes()) {
    if (p)
      p->accept(*this);
  }
}
void MacroExpander::visitNamedType(const NamedType *type) {
  for (const auto &arg : type->getGenericArgs()) {
    if (arg.type)
      arg.type->accept(*this);
  }
}
void MacroExpander::visitNullableType(const NullableType *type) {
  if (type->getInner())
    type->getInner()->accept(*this);
}
void MacroExpander::visitLockType(const LockType *type) {
  if (type->getInner())
    type->getInner()->accept(*this);
}
void MacroExpander::visitViewType(const ViewType *type) {
  if (type->getInner())
    type->getInner()->accept(*this);
}
void MacroExpander::visitMutType(const MutType *type) {
  if (type->getInner())
    type->getInner()->accept(*this);
}
void MacroExpander::visitVolatileType(const VolatileType *type) {
  if (type->getInner())
    type->getInner()->accept(*this);
}
void MacroExpander::visitConstType(const ConstType *type) {
  if (type->getInner())
    type->getInner()->accept(*this);
}
void MacroExpander::visitClosureType(const ClosureType *type) {
  if (type->getReturnType())
    type->getReturnType()->accept(*this);
  for (const auto &param : type->getParamTypes()) {
    if (param)
      param->accept(*this);
  }
}
void MacroExpander::visitWeakType(const WeakType *type) {
  if (type->getInner())
    type->getInner()->accept(*this);
}
void MacroExpander::visitPromiseType(const PromiseType *type) {
  if (type->getInner())
    type->getInner()->accept(*this);
}

// Traversals for Expressions
void MacroExpander::visitArrayLiteral(const ArrayLiteral *expr) {
  auto &elements = const_cast<std::vector<ExprPtr> &>(expr->getElements());
  for (auto &el : elements)
    expandExprHelper(el, macros, ctx, this, Diags);
}
void MacroExpander::visitMapLiteral(const MapLiteral *expr) {
  auto &entries =
      const_cast<std::vector<MapLiteral::Entry> &>(expr->getEntries());
  for (auto &entry : entries) {
    expandExprHelper(entry.first, macros, ctx, this, Diags);
    expandExprHelper(entry.second, macros, ctx, this, Diags);
  }
}
void MacroExpander::visitBinaryExpr(const BinaryExpr *expr) {
  auto *mutExpr = const_cast<BinaryExpr *>(expr);
  expandExprHelper(mutExpr->getLHSMut(), macros, ctx, this, Diags);
  expandExprHelper(mutExpr->getRHSMut(), macros, ctx, this, Diags);
}
void MacroExpander::visitUnaryExpr(const UnaryExpr *expr) {
  if (expr->getOperand())
    expr->getOperand()->accept(*this);
}
void MacroExpander::visitTernaryExpr(const TernaryExpr *expr) {
  if (expr->getCondition())
    expr->getCondition()->accept(*this);
  if (expr->getTrueBranch())
    expr->getTrueBranch()->accept(*this);
  if (expr->getFalseBranch())
    expr->getFalseBranch()->accept(*this);
}
void MacroExpander::visitCastExpr(const CastExpr *expr) {
  if (expr->getTargetType())
    expr->getTargetType()->accept(*this);
  if (expr->getExpr())
    expr->getExpr()->accept(*this);
}
void MacroExpander::visitBitcastExpr(const BitcastExpr *expr) {
  if (expr->getTargetType())
    expr->getTargetType()->accept(*this);
  if (expr->getExpr())
    expr->getExpr()->accept(*this);
}
void MacroExpander::visitCallExpr(const CallExpr *expr) {
  if (expr->getCallee())
    expr->getCallee()->accept(*this);
  auto *mutExpr = const_cast<CallExpr *>(expr);
  for (auto &arg : mutExpr->getArgsMut()) {
    expandExprHelper(arg, macros, ctx, this, Diags);
  }
}
void MacroExpander::visitMemberExpr(const MemberExpr *expr) {
  if (expr->getObject())
    expr->getObject()->accept(*this);
}
void MacroExpander::visitIndexExpr(const IndexExpr *expr) {
  if (expr->getArray())
    expr->getArray()->accept(*this);
  if (expr->getIndex())
    expr->getIndex()->accept(*this);
}
void MacroExpander::visitLambdaExpr(const LambdaExpr *expr) {
  for (const auto &p : expr->getParams()) {
    if (p.getType())
      p.getType()->accept(*this);
    if (p.getDefaultValue())
      p.getDefaultValue()->accept(*this);
  }
  if (expr->getBody())
    expr->getBody()->accept(*this);
}
void MacroExpander::visitNewExpr(const NewExpr *expr) {
  if (expr->getType())
    expr->getType()->accept(*this);
  auto *mutExpr = const_cast<NewExpr *>(expr);
  for (auto &arg : mutExpr->getArgsMut()) {
    expandExprHelper(arg, macros, ctx, this, Diags);
  }
}
void MacroExpander::visitTemplateStringExpr(const TemplateStringExpr *expr) {
  auto &parts = const_cast<std::vector<ExprPtr> &>(expr->getParts());
  for (auto &part : parts)
    expandExprHelper(part, macros, ctx, this, Diags);
}
void MacroExpander::visitThreadExpr(const ThreadExpr *expr) {
  if (expr->getBody())
    expr->getBody()->accept(*this);
}
void MacroExpander::visitAwaitExpr(const AwaitExpr *expr) {
  if (expr->getExpr())
    expr->getExpr()->accept(*this);
}
void MacroExpander::visitSizeOfExpr(const SizeOfExpr *expr) {
  if (expr->getExpr())
    expr->getExpr()->accept(*this);
}
void MacroExpander::visitInputExpr(const InputExpr *expr) {
  if (expr->getPrompt())
    expr->getPrompt()->accept(*this);
}
void MacroExpander::visitAsmExpr(const AsmExpr *expr) {
  auto *mutExpr = const_cast<AsmExpr *>(expr);
  auto &outputs =
      const_cast<std::vector<AsmExpr::AsmOperand> &>(mutExpr->getOutputs());
  for (auto &o : outputs)
    expandExprHelper(o.expr, macros, ctx, this, Diags);
  auto &inputs =
      const_cast<std::vector<AsmExpr::AsmOperand> &>(mutExpr->getInputs());
  for (auto &i : inputs)
    expandExprHelper(i.expr, macros, ctx, this, Diags);
  auto &inouts =
      const_cast<std::vector<AsmExpr::AsmOperand> &>(mutExpr->getInouts());
  for (auto &io : inouts)
    expandExprHelper(io.expr, macros, ctx, this, Diags);
}

void MacroExpander::visitUsingDecl(const UsingDecl *decl) {
  if (decl->getTargetType())
    decl->getTargetType()->accept(*this);
}

} // namespace moksha
