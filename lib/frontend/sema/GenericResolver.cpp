#include "moksha/Sema/GenericResolver.h"
#include "llvm/Support/raw_ostream.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "llvm/Support/Casting.h"

namespace moksha {

static std::unique_ptr<Expr> cloneExpr(const Expr *expr) {
  if (!expr)
    return nullptr;

  SourceLocation loc = expr->getLoc();

  // --- Literals ---

  if (const auto *i = llvm::dyn_cast<IntegerLiteral>(expr)) {
    return std::make_unique<IntegerLiteral>(i->getValue(), NumericSuffix::None,
                                            loc);
  }

  if (const auto *f = llvm::dyn_cast<FloatLiteral>(expr)) {
    return std::make_unique<FloatLiteral>(f->getValue(), NumericSuffix::None,
                                          loc);
  }

  if (const auto *b = llvm::dyn_cast<BoolLiteral>(expr)) {
    return std::make_unique<BoolLiteral>(b->getValue(), loc);
  }

  if (const auto *c = llvm::dyn_cast<CharLiteral>(expr)) {
    return std::make_unique<CharLiteral>(c->getValue(), loc);
  }

  if (const auto *s = llvm::dyn_cast<StringLiteral>(expr)) {
    // Note: Assuming isTemplate=false as getter is unavailable; strictly safe
    // for cloning simple strings
    return std::make_unique<StringLiteral>(s->getValue(), false, loc);
  }

  if (llvm::isa<NullLiteral>(expr)) {
    return std::make_unique<NullLiteral>(loc);
  }

  // --- Structural Expressions (Common in Array Sizes) ---

  if (const auto *id = llvm::dyn_cast<IdentifierExpr>(expr)) {
    return std::make_unique<IdentifierExpr>(id->getName(), loc);
  }

  if (const auto *bin = llvm::dyn_cast<BinaryExpr>(expr)) {
    return std::make_unique<BinaryExpr>(cloneExpr(bin->getLHS()), bin->getOp(),
                                        cloneExpr(bin->getRHS()), loc);
  }

  if (const auto *un = llvm::dyn_cast<UnaryExpr>(expr)) {
    return std::make_unique<UnaryExpr>(un->getOp(), cloneExpr(un->getOperand()),
                                       un->isPostfixOp(), loc);
  }

#ifndef NDEBUG
  llvm::errs() << "Unsupported expression kind in cloneExpr\n";
  llvm_unreachable("Unsupported expression in cloneExpr");
#endif
  return nullptr;
}

GenericResolver::GenericResolver(ASTContext &ctx) : context(ctx) {}

std::optional<GenericError> GenericResolver::validateGenericArgs(
    const std::vector<llvm::StringRef> &typeParams,
    const std::vector<NamedType::GenericArg> &args) {

  if (typeParams.size() != args.size()) {
    return GenericError::ArityMismatch;
  }

  for (const auto &arg : args) {
    if (arg.type->is<AnyType>()) {
      return GenericError::ConstraintViolation;
    }
  }

  return std::nullopt;
}

TypePtr GenericResolver::substituteType(
    const Type *type, const llvm::StringMap<const Type *> &substitutions) {
  if (!type)
    return nullptr;

  SourceLocation loc = type->getLoc();

  switch (type->getKind()) {
  case TypeKind::Primitive: {
    auto *prim = llvm::cast<PrimitiveType>(type);
    return std::make_unique<PrimitiveType>(prim->getScalar(), loc);
  }

  case TypeKind::Any:
    return std::make_unique<AnyType>(loc);

  case TypeKind::Named: {
    auto *named = llvm::cast<NamedType>(type);
    llvm::StringRef name = named->getName();

    // 1. Substitute Generic Parameter (T -> int)
    if (substitutions.count(name)) {
      const Type *replacement = substitutions.lookup(name);
      return substituteType(replacement, substitutions);
    }

    // 2. Recurse into Generic Arguments (Box<T> -> Box<int>)
    std::vector<NamedType::GenericArg> newArgs;
    for (const auto &arg : named->getGenericArgs()) {
      TypePtr newArgType = substituteType(arg.type.get(), substitutions);
      newArgs.push_back({std::move(newArgType), arg.variance});
    }

    return std::make_unique<NamedType>(named->getName(), std::move(newArgs),
                                       loc);
  }

  case TypeKind::Lock: {
      auto *l = llvm::cast<LockType>(type);
      return std::make_unique<LockType>(substituteType(l->getInner(), substitutions), loc);
    }

    case TypeKind::View: {
      auto *v = llvm::cast<ViewType>(type);
      return std::make_unique<ViewType>(substituteType(v->getInner(), substitutions), loc);
    }

    case TypeKind::Mut: {
      auto *m = llvm::cast<MutType>(type);
      return std::make_unique<MutType>(substituteType(m->getInner(), substitutions), loc);
    }

  case TypeKind::Pointer: {
    auto *ptr = llvm::cast<PointerType>(type);
    return std::make_unique<PointerType>(
        substituteType(ptr->getPointee(), substitutions), loc);
  }

  case TypeKind::Reference: {
    auto *ref = llvm::cast<ReferenceType>(type);
    return std::make_unique<ReferenceType>(
        substituteType(ref->getInner(), substitutions), loc);
  }

  case TypeKind::Nullable: {
    auto *null = llvm::cast<NullableType>(type);
    return std::make_unique<NullableType>(
        substituteType(null->getInner(), substitutions), loc);
  }

  case TypeKind::Array: {
    auto *arr = llvm::cast<ArrayType>(type);
    TypePtr newElem = substituteType(arr->getElementType(), substitutions);
    std::unique_ptr<Expr> newSize = cloneExpr(arr->getSizeExpr());
    return std::make_unique<ArrayType>(std::move(newElem), std::move(newSize),
                                       loc);
  }

  case TypeKind::Map: {
    auto *map = llvm::cast<MapType>(type);
    TypePtr newKey = substituteType(map->getKeyType(), substitutions);
    TypePtr newVal = substituteType(map->getValueType(), substitutions);
    return std::make_unique<MapType>(std::move(newKey), std::move(newVal), loc);
  }

  case TypeKind::Function: {
    auto *func = llvm::cast<FunctionType>(type);
    TypePtr newRet = substituteType(func->getReturnType(), substitutions);

    std::vector<TypePtr> newParams;
    for (const auto &p : func->getParamTypes()) {
      newParams.push_back(substituteType(p.get(), substitutions));
    }

    return std::make_unique<FunctionType>(std::move(newRet),
                                          std::move(newParams), loc);
  }

  default:
    llvm_unreachable("Unhandled TypeKind in substituteType");
  }
}

GenericResolver::ConcreteSignature GenericResolver::resolveFunctionSignature(
    const FunctionDecl *funcDecl,
    const llvm::StringMap<const Type *> &substitutions) {

  ConcreteSignature sig;
  sig.returnType = substituteType(funcDecl->getReturnType(), substitutions);

  for (const auto &param : funcDecl->getParams()) {
    sig.paramTypes.push_back(substituteType(param.type.get(), substitutions));
  }

  return sig;
}

} // namespace moksha
