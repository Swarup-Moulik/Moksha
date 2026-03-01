#include "moksha/Sema/GenericResolver.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {

GenericResolver::GenericResolver(ASTContext &ctx) : context(ctx) {}

std::optional<GenericError> GenericResolver::validateGenericArgs(
    const std::vector<std::string> &typeParams,
    const std::vector<NamedType::GenericArg> &args) {

  if (typeParams.size() != args.size()) {
    return GenericError::ArityMismatch;
  }

  for (const auto &arg : args) {
    bool isAny = arg.type->is<AnyType>();
    if (!isAny) {
      if (auto named = dynamic_cast<const NamedType *>(arg.type.get())) {
        if (named->getName() == "any")
          isAny = true;
      }
    }

    if (isAny) {
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
    return std::make_unique<LockType>(
        substituteType(l->getInner(), substitutions), loc);
  }
  case TypeKind::View: {
    auto *v = llvm::cast<ViewType>(type);
    return std::make_unique<ViewType>(
        substituteType(v->getInner(), substitutions), loc);
  }
  case TypeKind::Mut: {
    auto *m = llvm::cast<MutType>(type);
    return std::make_unique<MutType>(
        substituteType(m->getInner(), substitutions), loc);
  }
  case TypeKind::Null:
    return std::make_unique<NullType>(loc);
  case TypeKind::Any:
    return std::make_unique<AnyType>(loc);

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
    return std::make_unique<ArrayType>(
        substituteType(arr->getElementType(), substitutions),
        arr->getSizeExpr() ? arr->getSizeExpr()->clone() : nullptr,
        arr->getLoc());
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

    // Pass func->isInterruptFunc() to the constructor
    return std::make_unique<FunctionType>(
        std::move(newRet), std::move(newParams), func->isVariadicFunc(),
        func->isInterruptFunc(), loc);
  }

  case TypeKind::Const: {
    auto *c = llvm::cast<ConstType>(type);
    return std::make_unique<ConstType>(
        substituteType(c->getInner(), substitutions), loc);
  }
  case TypeKind::Volatile: {
    auto *v = llvm::cast<VolatileType>(type);
    return std::make_unique<VolatileType>(
        substituteType(v->getInner(), substitutions), loc);
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
