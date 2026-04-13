#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "moksha/Macros/Macro.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {

using llvm::dyn_cast;
using llvm::isa;

// ===========================================================================
// Helper: MacroSubstituter
// ===========================================================================

class MacroSubstituter {
  const std::vector<MacroParam> &params;
  const std::vector<std::unique_ptr<Expr>> &args;
  ASTContext &ctx;
  bool isHygieneEnabled; // [NEW] Toggle for hygiene
  std::unordered_map<std::string, std::string> localRenameMap;

public:
  MacroSubstituter(const std::vector<MacroParam> &params,
                   const std::vector<std::unique_ptr<Expr>> &args,
                   ASTContext &ctx, bool enableHygiene = false) // [MODIFIED]
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

    if (auto *id = dyn_cast<IdentifierExpr>(e)) {
      std::string name = id->getName();
      // Only apply hygiene map if enabled
      if (isHygieneEnabled && localRenameMap.count(name)) {
        return std::make_unique<IdentifierExpr>(localRenameMap[name], loc);
      }
      for (size_t i = 0; i < params.size(); ++i) {
        if (params[i].name == name) {
          if (i < args.size() && args[i])
            return cloneExpr(args[i].get());
        }
      }
      return std::make_unique<IdentifierExpr>(name, loc);
    }

    if (isa<ThisExpr>(e))
      return std::make_unique<ThisExpr>(loc);
    if (auto *bin = dyn_cast<BinaryExpr>(e))
      return std::make_unique<BinaryExpr>(cloneExpr(bin->getLHS()),
                                          bin->getOp(),
                                          cloneExpr(bin->getRHS()), loc);
    if (auto *un = dyn_cast<UnaryExpr>(e))
      return std::make_unique<UnaryExpr>(
          un->getOp(), cloneExpr(un->getOperand()), un->isPostfixOp(), loc);
    if (auto *call = dyn_cast<CallExpr>(e)) {
      std::vector<ExprPtr> newArgs;
      for (const auto &arg : call->getArgs())
        newArgs.push_back(cloneExpr(arg.get()));
      return std::make_unique<CallExpr>(cloneExpr(call->getCallee()),
                                        std::move(newArgs), loc);
    }
    if (auto *sz = dyn_cast<SizeOfExpr>(e)) {
      return std::make_unique<SizeOfExpr>(cloneExpr(sz->getExpr()), loc);
    }
    if (auto *inp = dyn_cast<InputExpr>(e)) {
      return std::make_unique<InputExpr>(cloneExpr(inp->getPrompt()), loc);
    }
    return nullptr; // Simplified for brevity; keep your other expr clones!
  }

  std::unique_ptr<Decl> cloneDecl(const Decl *d) {
    if (!d)
      return nullptr;
    SourceLocation loc = d->getLoc();
    std::string name = d->getName();
    Visibility vis = d->getVisibility();

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

    // [FIX] Only rename if hygiene is enabled and it's not a parameter
    if (isHygieneEnabled && !isParam && isa<VariableDecl>(d)) {
      std::string uniqueName =
          name + "_h" + std::to_string(reinterpret_cast<uintptr_t>(d));
      localRenameMap[d->getName()] = uniqueName;
      name = uniqueName;
    }

    if (auto *vd = dyn_cast<VariableDecl>(d)) {
      return std::make_unique<VariableDecl>(
          cloneType(vd->getType()), name, cloneExpr(vd->getInitializer()),
          vd->isConstVar(), vd->isStaticVar(), vd->isSharedVar(), vis, loc);
    }
    if (auto *ud = dyn_cast<UsingDecl>(d)) {
      return std::make_unique<UsingDecl>(name, cloneType(ud->getTargetType()),
                                         loc);
    }
    return nullptr; // Simplified; keep your function/class clones!
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
    if (auto *as = dyn_cast<AsmStmt>(s)) {
      return std::make_unique<AsmStmt>(as->getAssemblyStr(),
                                       as->getConstraints(), loc);
    }
    return nullptr; // Simplified; keep your other stmt clones!
  }
};

// ===========================================================================
// Macro Implementation
// ===========================================================================

std::vector<std::unique_ptr<Stmt>>
ObjectMacro::expand(const std::vector<std::unique_ptr<Expr>> &args,
                    ASTContext &ctx) const {
  std::vector<std::unique_ptr<Stmt>> result;

  // Create a local variable so it survives the scope
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
  MacroSubstituter substituter(params, args, ctx, true); // Hygiene ENABLED
  for (const auto &stmt : body) {
    if (auto expandedStmt = substituter.cloneStmt(stmt.get()))
      result.push_back(std::move(expandedStmt));
  }
  return result;
}

// ===========================================================================
// MacroExpander Pass
// ===========================================================================

void MacroExpander::visitModuleDecl(const ModuleDecl *decl) {
  for (const auto &d : decl->getDecls()) {
    if (auto *md = dyn_cast<MacroDecl>(d.get())) {
      std::vector<MacroParam> mParams;
      for (const auto &pName : md->getParams())
        mParams.push_back({pName, SourceLocation()});
      std::vector<std::unique_ptr<Stmt>> clonedBody;

      // Create local variables to prevent dangling references
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

  for (auto &s : stmts) {
    bool expanded = false;
    if (auto *exprStmt = dyn_cast<ExpressionStmt>(s.get())) {
      if (auto *call = dyn_cast<CallExpr>(exprStmt->getExpr())) {
        if (auto *id = dyn_cast<IdentifierExpr>(call->getCallee())) {
          if (const Macro *m = macros.lookup(id->getName())) {
            auto ex = m->expand(call->getArgs(), ctx);
            for (auto &es : ex) {
              es->accept(*this);
              newStmts.push_back(std::move(es));
            }
            expanded = true;
          }
        }
      }
    }
    if (!expanded) {
      s->accept(*this);
      newStmts.push_back(std::move(s));
    }
  }
  // Always commit the new list to prevent nullptr pointers in the AST
  stmts = std::move(newStmts);
}

void MacroExpander::visitFunctionDecl(const FunctionDecl *decl) {
  if (decl->getBody())
    decl->getBody()->accept(*this);
}

void MacroExpander::visitIfStmt(const IfStmt *stmt) {
  if (stmt->getThenStmt())
    stmt->getThenStmt()->accept(*this);
  if (stmt->getElseStmt())
    stmt->getElseStmt()->accept(*this);
}

void MacroExpander::visitWhileStmt(const WhileStmt *stmt) {
  if (stmt->getBody())
    stmt->getBody()->accept(*this);
}

void MacroExpander::visitDoWhileStmt(const DoWhileStmt *stmt) {
  if (stmt->getBody())
    stmt->getBody()->accept(*this);
}

void MacroExpander::visitForStmt(const ForStmt *stmt) {
  if (stmt->getBody())
    stmt->getBody()->accept(*this);
}

void MacroExpander::visitForInStmt(const ForInStmt *stmt) {
  if (stmt->getBody())
    stmt->getBody()->accept(*this);
}

void MacroExpander::visitSwitchStmt(const SwitchStmt *stmt) {
  for (const auto &c : stmt->getCases()) {
    if (c.getBody())
      c.getBody()->accept(*this);
  }
}

void MacroExpander::visitTryCatchStmt(const TryCatchStmt *stmt) {
  if (stmt->getTryBody())
    stmt->getTryBody()->accept(*this);
  if (stmt->getCatchBody())
    stmt->getCatchBody()->accept(*this);
  if (stmt->getFinallyBody())
    stmt->getFinallyBody()->accept(*this);
}

void MacroExpander::visitLockStmt(const LockStmt *stmt) {
  // 1. Expand any macros inside the lock target (e.g. lock (my_macro()))
  if (stmt->getTarget()) {
    stmt->getTarget()->accept(*this);
  }

  // 2. Expand any macros inside the lock block
  if (stmt->getBody()) {
    stmt->getBody()->accept(*this);
  }
}

void MacroExpander::visitClassDecl(const ClassDecl *decl) {
  for (const auto &member : decl->getMembers()) {
    member->accept(*this);
  }
}

void MacroExpander::visitGenericDecl(const GenericDecl *decl) {
  decl->getInnerDecl()->accept(*this);
}

void MacroExpander::visitVolatileType(const VolatileType *type) {
  // Macros typically don't affect modifiers, just traverse the inner type
  type->getInner()->accept(*this);
}

void MacroExpander::visitConstType(const ConstType *type) {
  type->getInner()->accept(*this);
}

void MacroExpander::visitClosureType(const ClosureType *type) {
  // Traverse return type
  if (type->getReturnType()) {
    type->getReturnType()->accept(*this);
  }
  // Traverse parameter types
  for (const auto &param : type->getParamTypes()) {
    if (param) {
      param->accept(*this);
    }
  }
}

void MacroExpander::visitInputExpr(const InputExpr *expr) {
  if (expr->getPrompt()) {
    expr->getPrompt()->accept(*this);
  }
}

void MacroExpander::visitSliceType(const SliceType *type) {
  type->getElementType()->accept(*this);
}
} // namespace moksha
