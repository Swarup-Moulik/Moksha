#include "moksha/Macros/Macro.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {

using llvm::dyn_cast;
using llvm::isa;

// ===========================================================================
// Helper: MacroSubstituter
// Handles recursive cloning of AST nodes with parameter substitution.
// ===========================================================================

class MacroSubstituter {
  const std::vector<MacroParam> &params;
  const std::vector<std::unique_ptr<Expr>> &args;
  ASTContext &ctx;

public:
  MacroSubstituter(const std::vector<MacroParam> &params,
                   const std::vector<std::unique_ptr<Expr>> &args,
                   ASTContext &ctx)
      : params(params), args(args), ctx(ctx) {}

  // --- Type Cloning (Deep Copy) ---

  TypePtr cloneType(const Type *t) {
    if (!t)
      return nullptr;
    SourceLocation loc = t->getLoc();

    if (auto *p = dyn_cast<PrimitiveType>(t))
      return std::make_unique<PrimitiveType>(p->getScalar(), loc);

    if (isa<AnyType>(t))
      return std::make_unique<AnyType>(loc);

    if (auto *p = dyn_cast<PointerType>(t))
      return std::make_unique<PointerType>(cloneType(p->getPointee()), loc);

    if (auto *r = dyn_cast<ReferenceType>(t))
      return std::make_unique<ReferenceType>(cloneType(r->getInner()), loc);

    if (auto *a = dyn_cast<ArrayType>(t))
      return std::make_unique<ArrayType>(cloneType(a->getElementType()),
                                         cloneExpr(a->getSizeExpr()), loc);

    if (auto *m = dyn_cast<MapType>(t))
      return std::make_unique<MapType>(cloneType(m->getKeyType()),
                                       cloneType(m->getValueType()), loc);

    if (auto *n = dyn_cast<NullableType>(t))
      return std::make_unique<NullableType>(cloneType(n->getInner()), loc);

    if (auto *f = dyn_cast<FunctionType>(t)) {
      std::vector<TypePtr> newParams;
      for (const auto &pt : f->getParamTypes())
        newParams.push_back(cloneType(pt.get()));
      return std::make_unique<FunctionType>(cloneType(f->getReturnType()),
                                            std::move(newParams), loc);
    }

    if (auto *nt = dyn_cast<NamedType>(t)) {
      std::vector<NamedType::GenericArg> newArgs;
      for (const auto &arg : nt->getGenericArgs()) {
        newArgs.push_back({cloneType(arg.type.get()), arg.variance});
      }
      return std::make_unique<NamedType>(nt->getName(), std::move(newArgs),
                                         loc);
    }

    if (auto *et = dyn_cast<EnumType>(t)) {
      return std::make_unique<EnumType>(et->getName(), et->getMembers(), loc);
    }

    return std::make_unique<AnyType>(loc); // Fallback
  }

  // --- Expression Cloning (Substitution Logic) ---

  std::unique_ptr<Expr> cloneExpr(const Expr *e) {
    if (!e)
      return nullptr;
    SourceLocation loc = e->getLoc();

    // 1. Literals
    if (auto *l = dyn_cast<IntegerLiteral>(e))
      return std::make_unique<IntegerLiteral>(l->getValue(),
                                              NumericSuffix::None, loc);
    if (auto *l = dyn_cast<FloatLiteral>(e))
      return std::make_unique<FloatLiteral>(l->getValue(), NumericSuffix::None,
                                            loc);
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

    // 2. Identifier (SUBSTITUTION POINT)
    if (auto *id = dyn_cast<IdentifierExpr>(e)) {
      for (size_t i = 0; i < params.size(); ++i) {
        if (params[i].name == id->getName()) {
          // Found parameter match: Substitute with cloned argument
          if (i < args.size() && args[i]) {
            return cloneExpr(args[i].get());
          }
        }
      }
      return std::make_unique<IdentifierExpr>(id->getName(), loc);
    }

    // 3. Operations
    if (auto *bin = dyn_cast<BinaryExpr>(e)) {
      return std::make_unique<BinaryExpr>(cloneExpr(bin->getLHS()),
                                          bin->getOp(),
                                          cloneExpr(bin->getRHS()), loc);
    }
    if (auto *un = dyn_cast<UnaryExpr>(e)) {
      return std::make_unique<UnaryExpr>(
          un->getOp(), cloneExpr(un->getOperand()), un->isPostfixOp(), loc);
    }
    if (auto *tern = dyn_cast<TernaryExpr>(e)) {
      return std::make_unique<TernaryExpr>(
          cloneExpr(tern->getCondition()), cloneExpr(tern->getTrueBranch()),
          cloneExpr(tern->getFalseBranch()), loc);
    }
    if (auto *cast = dyn_cast<CastExpr>(e)) {
      return std::make_unique<CastExpr>(cloneType(cast->getTargetType()),
                                        cloneExpr(cast->getExpr()), loc);
    }

    // 4. Calls & Access
    if (auto *call = dyn_cast<CallExpr>(e)) {
      std::vector<ExprPtr> newArgs;
      for (const auto &arg : call->getArgs())
        newArgs.push_back(cloneExpr(arg.get()));
      return std::make_unique<CallExpr>(cloneExpr(call->getCallee()),
                                        std::move(newArgs), loc);
    }
    if (auto *mem = dyn_cast<MemberExpr>(e)) {
      return std::make_unique<MemberExpr>(cloneExpr(mem->getObject()),
                                          mem->getMemberName(),
                                          mem->isOptionalAccess(), loc);
    }
    if (auto *idx = dyn_cast<IndexExpr>(e)) {
      return std::make_unique<IndexExpr>(cloneExpr(idx->getArray()),
                                         cloneExpr(idx->getIndex()), loc);
    }

    // 5. Complex Expressions
    if (auto *lam = dyn_cast<LambdaExpr>(e)) {
      std::vector<LambdaParam> newParams;
      for (const auto &p : lam->getParams()) {
        newParams.emplace_back(cloneType(p.getType()), p.getName());
      }
      auto newBody = cloneStmt(lam->getBody());
      return std::make_unique<LambdaExpr>(std::move(newParams),
                                          std::move(newBody),
                                          lam->isExpressionBody(), loc);
    }
    if (auto *newE = dyn_cast<NewExpr>(e)) {
      std::vector<ExprPtr> newArgs;
      for (const auto &arg : newE->getArgs())
        newArgs.push_back(cloneExpr(arg.get()));
      return std::make_unique<NewExpr>(cloneType(newE->getType()),
                                       std::move(newArgs), loc);
    }
    if (auto *tmpl = dyn_cast<TemplateStringExpr>(e)) {
      std::vector<ExprPtr> newParts;
      for (const auto &p : tmpl->getParts())
        newParts.push_back(cloneExpr(p.get()));
      return std::make_unique<TemplateStringExpr>(std::move(newParts), loc);
    }

    llvm::errs() << "MacroSubstituter: Unsupported Expr kind at "
                 << loc.getPointer() << "\n";
    return nullptr;
  }

  // --- Declaration Cloning ---

  std::unique_ptr<Decl> cloneDecl(const Decl *d) {
    if (!d)
      return nullptr;
    SourceLocation loc = d->getLoc();
    std::string name = d->getName();

    if (auto *vd = dyn_cast<VariableDecl>(d)) {
      return std::make_unique<VariableDecl>(
          cloneType(vd->getType()), name, cloneExpr(vd->getInitializer()), loc);
    }

    if (auto *fd = dyn_cast<FunctionDecl>(d)) {
      std::vector<FunctionDecl::Param> newParams;
      for (const auto &p : fd->getParams()) {
        newParams.push_back({cloneType(p.type.get()), p.name});
      }
      return std::make_unique<FunctionDecl>(
          cloneType(fd->getReturnType()), name, std::move(newParams),
          cloneStmt(fd->getBody()), fd->isAsyncFunc(), loc);
    }

    if (auto *cd = dyn_cast<ClassDecl>(d)) {
      std::vector<DeclPtr> newMembers;
      for (const auto &m : cd->getMembers())
        newMembers.push_back(cloneDecl(m.get()));
      return std::make_unique<ClassDecl>(name, std::move(newMembers),
                                         cd->isReferenceType(), loc);
    }

    if (auto *ed = dyn_cast<EnumDecl>(d)) {
      std::vector<EnumDecl::Case> newCases;
      for (const auto &c : ed->getCases()) {
        newCases.push_back({c.name, cloneExpr(c.value.get())});
      }
      return std::make_unique<EnumDecl>(name, std::move(newCases), loc);
    }

    return nullptr;
  }

  // --- Statement Cloning ---

  std::unique_ptr<Stmt> cloneStmt(const Stmt *s) {
    if (!s)
      return nullptr;
    SourceLocation loc = s->getLoc();

    if (auto *bs = dyn_cast<BlockStmt>(s)) {
      std::vector<StmtPtr> newStmts;
      for (const auto &sub : bs->getStatements()) {
        if (auto cloned = cloneStmt(sub.get()))
          newStmts.push_back(std::move(cloned));
      }

      if (isa<UnsafeBlockStmt>(s))
        return std::make_unique<UnsafeBlockStmt>(std::move(newStmts), loc);

      return std::make_unique<BlockStmt>(std::move(newStmts), loc);
    }

    if (auto *es = dyn_cast<ExpressionStmt>(s)) {
      return std::make_unique<ExpressionStmt>(cloneExpr(es->getExpr()), loc);
    }

    if (auto *ds = dyn_cast<DeclStmt>(s)) {
      auto decl = cloneDecl(ds->getDecl());
      if (!decl)
        return nullptr;
      return std::make_unique<DeclStmt>(std::move(decl), loc);
    }

    if (auto *rs = dyn_cast<ReturnStmt>(s)) {
      return std::make_unique<ReturnStmt>(cloneExpr(rs->getReturnValue()), loc);
    }

    if (isa<BreakStmt>(s))
      return std::make_unique<BreakStmt>(loc);
    if (isa<ContinueStmt>(s))
      return std::make_unique<ContinueStmt>(loc);

    if (auto *is = dyn_cast<IfStmt>(s)) {
      return std::make_unique<IfStmt>(cloneExpr(is->getCondition()),
                                      cloneStmt(is->getThenStmt()),
                                      cloneStmt(is->getElseStmt()), loc);
    }

    if (auto *ws = dyn_cast<WhileStmt>(s)) {
      return std::make_unique<WhileStmt>(cloneExpr(ws->getCondition()),
                                         cloneStmt(ws->getBody()), loc);
    }

    if (auto *dws = dyn_cast<DoWhileStmt>(s)) {
      return std::make_unique<DoWhileStmt>(cloneStmt(dws->getBody()),
                                           cloneExpr(dws->getCondition()), loc);
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

    if (auto *ss = dyn_cast<SwitchStmt>(s)) {
      std::vector<SwitchCase> newCases;
      for (const auto &c : ss->getCases()) {
        std::vector<ExprPtr> newVals;
        for (const auto &v : c.getValues())
          newVals.push_back(cloneExpr(v.get()));

        auto newBodyStmt = cloneStmt(c.getBody());
        auto newBodyBlock = std::unique_ptr<BlockStmt>(
            static_cast<BlockStmt *>(newBodyStmt.release()));

        newCases.emplace_back(std::move(newVals), std::move(newBodyBlock),
                              c.isDefaultCase());
      }
      return std::make_unique<SwitchStmt>(cloneExpr(ss->getCondition()),
                                          std::move(newCases), loc);
    }

    if (auto *ds = dyn_cast<DeferStmt>(s)) {
      return std::make_unique<DeferStmt>(cloneStmt(ds->getDeferredStmt()), loc);
    }

    if (auto *tcs = dyn_cast<TryCatchStmt>(s)) {
      return std::make_unique<TryCatchStmt>(
          cloneStmt(tcs->getTryBody()), cloneDecl(tcs->getCatchVar()),
          cloneStmt(tcs->getCatchBody()), cloneStmt(tcs->getFinallyBody()),
          loc);
    }

    llvm::errs() << "MacroSubstituter: Unsupported Stmt kind at "
                 << loc.getPointer() << "\n";
    return nullptr;
  }
};

// ===========================================================================
// ObjectMacro
// ===========================================================================

std::vector<std::unique_ptr<Stmt>>
ObjectMacro::expand(const std::vector<std::unique_ptr<Expr>> &args,
                    ASTContext &ctx) const {
  std::vector<std::unique_ptr<Stmt>> result;

  MacroSubstituter substituter({}, args, ctx);
  auto clonedExpr = substituter.cloneExpr(value.get());

  if (clonedExpr) {
    result.push_back(
        std::make_unique<ExpressionStmt>(std::move(clonedExpr), loc));
  }

  return result;
}

// ===========================================================================
// FunctionMacro
// ===========================================================================

std::vector<std::unique_ptr<Stmt>>
FunctionMacro::expand(const std::vector<std::unique_ptr<Expr>> &args,
                      ASTContext &ctx) const {
  std::vector<std::unique_ptr<Stmt>> result;

  if (args.size() != params.size()) {
    llvm::errs() << "Error: Macro '" << name << "' expects " << params.size()
                 << " arguments, but got " << args.size() << "\n";
    return result;
  }

  MacroSubstituter substituter(params, args, ctx);

  for (const auto &stmt : body) {
    if (auto expandedStmt = substituter.cloneStmt(stmt.get())) {
      result.push_back(std::move(expandedStmt));
    }
  }

  return result;
}

// ===========================================================================
// MacroTable
// ===========================================================================

void MacroTable::addMacro(std::unique_ptr<Macro> macro) {
  table[macro->getName().str()] = std::move(macro);
}

const Macro *MacroTable::lookup(llvm::StringRef name) const {
  auto it = table.find(name.str());
  if (it != table.end()) {
    return it->second.get();
  }
  return nullptr;
}

bool MacroTable::contains(llvm::StringRef name) const {
  return table.find(name.str()) != table.end();
}

} // namespace moksha
