#include "moksha/Sema/GenericResolver.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>

namespace moksha {

GenericResolver::GenericResolver(ASTContext &ctx) : context(ctx) {}

std::optional<GenericError> GenericResolver::validateGenericArgs(
    const std::vector<GenericDecl::GenericParam> &typeParams,
    const std::vector<NamedType::GenericArg> &args) {

  if (typeParams.size() != args.size()) {
    return GenericError::ArityMismatch;
  }

  for (size_t i = 0; i < typeParams.size(); ++i) {
    const Type *argType = args[i].type.get();

    // 1. Forbid 'any' as a generic argument
    const Type *currAnyCheck = argType;
    while (currAnyCheck) {
      if (currAnyCheck->getKind() == TypeKind::Any) {
        return GenericError::AnyConstraintViolation;
      }

      // Unwrap pointers/borrows to ensure 'any' isn't hiding inside
      if (currAnyCheck->getKind() == TypeKind::Pointer) {
        currAnyCheck =
            static_cast<const PointerType *>(currAnyCheck)->getPointee();
      } else if (currAnyCheck->getKind() == TypeKind::Reference) {
        currAnyCheck =
            static_cast<const ReferenceType *>(currAnyCheck)->getInner();
      } else if (currAnyCheck->getKind() == TypeKind::View) {
        currAnyCheck = static_cast<const ViewType *>(currAnyCheck)->getInner();
      } else if (currAnyCheck->getKind() == TypeKind::Mut) {
        currAnyCheck = static_cast<const MutType *>(currAnyCheck)->getInner();
      } else if (currAnyCheck->getKind() == TypeKind::Lock) {
        currAnyCheck = static_cast<const LockType *>(currAnyCheck)->getInner();
      } else if (currAnyCheck->getKind() == TypeKind::Promise) {
        currAnyCheck =
            static_cast<const PromiseType *>(currAnyCheck)->getInner();
      } else {
        break;
      }
    }

    // 2. Validate `shared` constraints
    if (typeParams[i].isShared) {
      bool hasBorrow = false;
      const Type *curr = argType;
      while (curr) {
        if (curr->getKind() == TypeKind::View ||
            curr->getKind() == TypeKind::Mut) {
          hasBorrow = true;
          break;
        }

        if (curr->getKind() == TypeKind::Pointer) {
          curr = static_cast<const PointerType *>(curr)->getPointee();
        } else if (curr->getKind() == TypeKind::Reference) {
          curr = static_cast<const ReferenceType *>(curr)->getInner();
        } else if (curr->getKind() == TypeKind::Promise) {
          curr = static_cast<const PromiseType *>(curr)->getInner();
        } else {
          break;
        }
      }

      if (hasBorrow) {
        return GenericError::AnyConstraintViolation;
      }
    }
  }

  return std::nullopt;
}

TypePtr GenericResolver::substituteType(
    const Type *type, const llvm::StringMap<const Type *> &substitutions) {

  if (!type)
    return nullptr;

  switch (type->getKind()) {
  case TypeKind::Named: {
    auto *named = static_cast<const NamedType *>(type);

    // 1. Base generic parameter (e.g., 'K' or 'V')
    if (substitutions.count(named->getName()) &&
        named->getGenericArgs().empty()) {
      return substitutions.lookup(named->getName())->clone();
    }

    // 2. Nested generic structure (e.g., 'List<K>')
    if (!named->getGenericArgs().empty()) {
      std::vector<NamedType::GenericArg> newArgs;
      for (const auto &arg : named->getGenericArgs()) {
        newArgs.push_back(
            {substituteType(arg.type.get(), substitutions), arg.variance});
      }
      return std::make_unique<NamedType>(named->getName(), std::move(newArgs),
                                         named->getLoc(), named->isRefClass());
    }
    return named->clone();
  }
  case TypeKind::Array: {
    auto *arr = static_cast<const ArrayType *>(type);
    return std::make_unique<ArrayType>(
        substituteType(arr->getElementType(), substitutions),
        arr->getSizeExpr() ? arr->getSizeExpr()->clone() : nullptr,
        arr->getLoc());
  }
  case TypeKind::Slice: {
    auto *slice = static_cast<const SliceType *>(type);
    return std::make_unique<SliceType>(
        substituteType(slice->getElementType(), substitutions),
        slice->getLoc());
  }
  case TypeKind::Pointer: {
    auto *ptr = static_cast<const PointerType *>(type);
    return std::make_unique<PointerType>(
        substituteType(ptr->getPointee(), substitutions), ptr->getLoc());
  }
  case TypeKind::Reference: {
    auto *ref = static_cast<const ReferenceType *>(type);
    return std::make_unique<ReferenceType>(
        substituteType(ref->getInner(), substitutions), ref->getLoc());
  }
  case TypeKind::Map: {
    auto *map = static_cast<const MapType *>(type);
    return std::make_unique<MapType>(
        substituteType(map->getKeyType(), substitutions),
        substituteType(map->getValueType(), substitutions), map->getLoc());
  }
  case TypeKind::Function: {
    auto *func = static_cast<const FunctionType *>(type);
    std::vector<TypePtr> newParams;
    for (const auto &p : func->getParamTypes()) {
      newParams.push_back(substituteType(p.get(), substitutions));
    }
    return std::make_unique<FunctionType>(
        substituteType(func->getReturnType(), substitutions),
        std::move(newParams), func->isVariadicFunc(), func->isInterruptFunc(),
        func->getLoc());
  }
  case TypeKind::Closure: {
    auto *cls = static_cast<const ClosureType *>(type);
    std::vector<TypePtr> newParams;
    for (const auto &p : cls->getParamTypes()) {
      newParams.push_back(substituteType(p.get(), substitutions));
    }
    return std::make_unique<ClosureType>(
        substituteType(cls->getReturnType(), substitutions),
        std::move(newParams), cls->getLoc());
  }
  case TypeKind::Nullable: {
    auto *nullb = static_cast<const NullableType *>(type);
    return std::make_unique<NullableType>(
        substituteType(nullb->getInner(), substitutions), nullb->getLoc());
  }
  case TypeKind::Mut: {
    auto *mt = static_cast<const MutType *>(type);
    return std::make_unique<MutType>(
        substituteType(mt->getInner(), substitutions), mt->getLoc());
  }
  case TypeKind::View: {
    auto *vw = static_cast<const ViewType *>(type);
    return std::make_unique<ViewType>(
        substituteType(vw->getInner(), substitutions), vw->getLoc());
  }
  case TypeKind::Lock: {
    auto *lck = static_cast<const LockType *>(type);
    return std::make_unique<LockType>(
        substituteType(lck->getInner(), substitutions), lck->getLoc());
  }
  case TypeKind::Const: {
    auto *cst = static_cast<const ConstType *>(type);
    return std::make_unique<ConstType>(
        substituteType(cst->getInner(), substitutions), cst->getLoc());
  }
  case TypeKind::Volatile: {
    auto *vol = static_cast<const VolatileType *>(type);
    return std::make_unique<VolatileType>(
        substituteType(vol->getInner(), substitutions), vol->getLoc());
  }
  case TypeKind::Weak: {
    auto *wk = static_cast<const WeakType *>(type);
    return std::make_unique<WeakType>(
        substituteType(wk->getInner(), substitutions), wk->getLoc());
  }
  case TypeKind::Promise: {
    auto *prom = static_cast<const PromiseType *>(type);
    return std::make_unique<PromiseType>(
        substituteType(prom->getInner(), substitutions), prom->getLoc());
  }
  default:
    // Primitives, Any, Null, Decimal, Enum etc.
    return type->clone();
  }
}

GenericResolver::ConcreteSignature GenericResolver::resolveFunctionSignature(
    const FunctionDecl *funcDecl,
    const llvm::StringMap<const Type *> &substitutions) {

  ConcreteSignature sig;
  sig.decl = funcDecl;

  // 1. Ensure we have a valid return type (default to void)
  const Type *retTy = funcDecl->getReturnType() ? funcDecl->getReturnType()
                                                : context.getVoidType();
  sig.returnType = substituteType(retTy, substitutions);

  // 2. Automatically wrap in Promise if the function is async
  if (funcDecl->isAsyncFunc() && !sig.returnType->is<PromiseType>()) {
    sig.returnType = std::make_unique<PromiseType>(std::move(sig.returnType),
                                                   funcDecl->getLoc());
  }

  for (const auto &param : funcDecl->getParams()) {
    sig.paramTypes.push_back(substituteType(param.type.get(), substitutions));
  }

  return sig;
}

std::string
GenericResolver::getMangledName(llvm::StringRef baseName,
                                const std::vector<const Type *> &typeArgs) {

  std::string mangled = baseName.str();

  for (const auto *arg : typeArgs) {
    mangled += "_";
    std::string argStr = arg->toString();
    for (char &c : argStr) {
      if (c == '<' || c == '>' || c == ',' || c == ' ' || c == '*' ||
          c == '&' || c == '[' || c == ']') {
        c = '_';
      }
    }

    auto new_end =
        std::unique(argStr.begin(), argStr.end(),
                    [](char a, char b) { return a == '_' && b == '_'; });
    argStr.erase(new_end, argStr.end());

    if (!argStr.empty() && argStr.back() == '_') {
      argStr.pop_back();
    }

    mangled += argStr;
  }

  return mangled;
}

const ClassDecl *
GenericResolver::instantiateClass(const GenericDecl *genericTemplate,
                                  const std::vector<const Type *> &typeArgs) {

  auto *innerClass = llvm::dyn_cast_or_null<ClassDecl>(genericTemplate->getInnerDecl());
  if (!innerClass)
    return nullptr;

  std::string mangledName = getMangledName(innerClass->getName(), typeArgs);

  if (instantiatedClasses.count(mangledName)) {
    return instantiatedClasses[mangledName];
  }

  llvm::StringMap<const Type *> substitutions;
  const auto &params = genericTemplate->getTypeParams();
  for (size_t i = 0; i < params.size() && i < typeArgs.size(); ++i) {
    substitutions[params[i].name] = typeArgs[i];
  }

  std::unique_ptr<Decl> clonedDecl = innerClass->clone();
  std::unique_ptr<ClassDecl> concreteClass(
      static_cast<ClassDecl *>(clonedDecl.release()));
  auto *mutClass = const_cast<ClassDecl *>(concreteClass.get());

  mutClass->setName(mangledName);

  auto &mutMembers = const_cast<std::vector<DeclPtr> &>(mutClass->getMembers());
  for (auto &member : mutMembers) {
    if (auto *varDecl = llvm::dyn_cast_or_null<VariableDecl>(member.get())) {
      TypePtr newType = substituteType(varDecl->getType(), substitutions);
      auto *mutVar = const_cast<VariableDecl *>(varDecl);
      mutVar->setType(std::move(newType));
    } else if (auto *funcDecl = llvm::dyn_cast_or_null<FunctionDecl>(member.get())) {
      auto *mutFunc = const_cast<FunctionDecl *>(funcDecl);
      mutFunc->setReturnType(
          substituteType(funcDecl->getReturnType(), substitutions));

      auto &mutParams =
          const_cast<std::vector<FunctionDecl::Param> &>(mutFunc->getParams());
      for (auto &p : mutParams) {
        p.type = substituteType(p.type.get(), substitutions);
      }
    }
  }

  const ClassDecl *registeredDecl = mutClass;
  context.registerInstantiatedClass(std::move(concreteClass));
  instantiatedClasses[mangledName] = registeredDecl;

  return registeredDecl;
}

const FunctionDecl *GenericResolver::instantiateFunction(
    const GenericDecl *genericTemplate,
    const std::vector<const Type *> &typeArgs) {

  auto *innerFunc =
      llvm::dyn_cast_or_null<FunctionDecl>(genericTemplate->getInnerDecl());
  if (!innerFunc)
    return nullptr;

  std::string mangledName = getMangledName(innerFunc->getName(), typeArgs);
  if (instantiatedFunctions.count(mangledName)) {
    return instantiatedFunctions[mangledName];
  }

  llvm::StringMap<const Type *> substitutions;
  const auto &params = genericTemplate->getTypeParams();
  for (size_t i = 0; i < params.size() && i < typeArgs.size(); ++i) {
    substitutions[params[i].name] = typeArgs[i];
  }

  std::unique_ptr<Decl> clonedDecl = innerFunc->clone();
  std::unique_ptr<FunctionDecl> concreteFunc(
      static_cast<FunctionDecl *>(clonedDecl.release()));

  concreteFunc->setName(mangledName);
  concreteFunc->setReturnType(
      substituteType(innerFunc->getReturnType(), substitutions));

  auto &mutParams =
      const_cast<std::vector<FunctionDecl::Param> &>(concreteFunc->getParams());
  for (auto &p : mutParams) {
    p.type = substituteType(p.type.get(), substitutions);
  }

  const FunctionDecl *registeredDecl = concreteFunc.get();
  context.registerInstantiatedFunction(std::move(concreteFunc));
  instantiatedFunctions[mangledName] = registeredDecl;

  return registeredDecl;
}

} // namespace moksha
