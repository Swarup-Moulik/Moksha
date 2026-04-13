#include "moksha/Sema/TypeChecker.h"
#include "moksha/AST/ASTContext.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "moksha/Builtins/BuiltinRegistry.h"
#include "moksha/Sema/GenericResolver.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include <map>

namespace moksha {

namespace {

static std::string getMethodSignature(const FunctionDecl *fd) {
  std::string sig = fd->getName() + "(";
  for (size_t i = 0; i < fd->getParams().size(); ++i) {
    sig += fd->getParams()[i].type->toString();
    if (i < fd->getParams().size() - 1)
      sig += ",";
  }
  sig += ")";
  return sig;
}

static const Type *unwrapConcurrency(const Type *t) {
  while (t) {
    if (auto l = llvm::dyn_cast<const LockType>(t))
      t = l->getInner();
    else if (auto v = llvm::dyn_cast<const ViewType>(t))
      t = v->getInner();
    else if (auto m = llvm::dyn_cast<const MutType>(t))
      t = m->getInner();
    else if (auto vol = llvm::dyn_cast<const VolatileType>(t))
      t = vol->getInner();
    else if (auto c = llvm::dyn_cast<const ConstType>(t))
      t = c->getInner();
    else if (auto ref = llvm::dyn_cast<const ReferenceType>(t))
      t = ref->getInner();
    else if (auto w = llvm::dyn_cast<const WeakType>(t))
      t = w->getInner();
    else
      break;
  }
  return t;
}

static bool hasMutOrLock(const Type *t) {
  if (!t)
    return false;

  // Check if this specific node is a mutation or lock wrapper
  if (t->is<MutType>() || t->is<LockType>())
    return true;

  // Recursively peel other wrappers to look for mut/lock deeper in the type
  if (auto ptr = llvm::dyn_cast<const PointerType>(t))
    return hasMutOrLock(ptr->getPointee());
  if (auto ref = llvm::dyn_cast<const ReferenceType>(t))
    return hasMutOrLock(ref->getInner());
  if (auto v = llvm::dyn_cast<const ViewType>(t))
    return hasMutOrLock(v->getInner());
  if (auto arr = llvm::dyn_cast<const ArrayType>(t))
    return hasMutOrLock(arr->getElementType());

  return false;
}

static bool hasLock(const Type *t) {
  while (t) {
    if (t->is<LockType>())
      return true;
    if (auto ptr = llvm::dyn_cast<const PointerType>(t))
      t = ptr->getPointee();
    else if (auto ref = llvm::dyn_cast<const ReferenceType>(t))
      t = ref->getInner();
    else if (auto v = llvm::dyn_cast<const ViewType>(t))
      t = v->getInner();
    else if (auto m = llvm::dyn_cast<const MutType>(t))
      t = m->getInner();
    else
      break;
  }
  return false;
}

static bool hasView(const Type *t) {
  if (!t)
    return false;

  // 1. Explicit View Check
  if (t->is<ViewType>() || t->is<ConstType>())
    return true;

  // 2. Implicit View Check (Rule 1: Default pointers are immutable)
  if (auto ptr = llvm::dyn_cast<const PointerType>(t)) {
    return !hasMutOrLock(ptr->getPointee());
  }
  if (auto ref = llvm::dyn_cast<const ReferenceType>(t)) {
    return !hasMutOrLock(ref->getInner());
  }

  // 3. Peeling Phase for other wrappers
  if (auto l = llvm::dyn_cast<const LockType>(t))
    return hasView(l->getInner());
  if (auto m = llvm::dyn_cast<const MutType>(t))
    return hasView(m->getInner());
  if (auto arr = llvm::dyn_cast<const ArrayType>(t))
    return hasView(arr->getElementType());

  return false;
}

// Helpers to treat 'char' as a numeric 8-bit integer
bool isNumericOrChar(const Type *t) {
  t = unwrapConcurrency(t);
  if (t->isNumeric())
    return true;
  if (auto p = llvm::dyn_cast<const PrimitiveType>(t))
    return p->getScalar() == PrimitiveType::Scalar::Char;
  return false;
}

bool isIntegerOrChar(const Type *t) {
  t = unwrapConcurrency(t);
  if (t->isInteger())
    return true;
  if (auto p = llvm::dyn_cast<const PrimitiveType>(t))
    return p->getScalar() == PrimitiveType::Scalar::Char;
  return false;
}

bool isCharType(const Type *t) {
  t = unwrapConcurrency(t);
  if (auto p = llvm::dyn_cast<const PrimitiveType>(t)) {
    // Treat char, u8, and i8 interchangeably for C-strings
    return p->getScalar() == PrimitiveType::Scalar::Char ||
           p->getScalar() == PrimitiveType::Scalar::U8 ||
           p->getScalar() == PrimitiveType::Scalar::I8;
  }
  return false;
}

bool checkDefiniteReturn(const Stmt *stmt, DiagnosticEngine &Diags) {
  if (!stmt)
    return false;

  switch (stmt->getKind()) {
  case StmtKind::ReturnStmt:
  case StmtKind::ThrowStmt:
    return true;

  case StmtKind::BlockStmt: {
    auto block = static_cast<const BlockStmt *>(stmt);
    bool hasReturned = false;

    for (const auto &s : block->getStatements()) {
      if (hasReturned) {
        Diags.report(s->getLoc(), DiagID::err_unreachable_code)
            << "Unreachable code detected";
        break; // Only report the first unreachable statement per block
      }
      if (checkDefiniteReturn(s.get(), Diags)) {
        hasReturned = true;
      }
    }
    return hasReturned;
  }

  case StmtKind::IfStmt: {
    auto ifStmt = static_cast<const IfStmt *>(stmt);
    bool thenReturns = checkDefiniteReturn(ifStmt->getThenStmt(), Diags);
    bool elseReturns = checkDefiniteReturn(ifStmt->getElseStmt(), Diags);
    // An 'if' only guarantees a return if BOTH branches guarantee it.
    return thenReturns && elseReturns;
  }

  case StmtKind::SwitchStmt: {
    auto switchStmt = static_cast<const SwitchStmt *>(stmt);
    bool allCasesReturn = true;
    bool hasDefault = false;

    // Changed to index-based loop to detect fallthrough
    for (size_t i = 0; i < switchStmt->getCases().size(); ++i) {
      const auto &c = switchStmt->getCases()[i];
      if (c.isDefaultCase())
        hasDefault = true;

      // Empty Fallthrough Logic
      bool isEmpty = c.getBody()->getStatements().empty();
      bool isLastCase = (i == switchStmt->getCases().size() - 1);

      if (isEmpty && !isLastCase) {
        continue;
      }

      if (!checkDefiniteReturn(c.getBody(), Diags)) {
        allCasesReturn = false;
      }
    }
    return hasDefault && allCasesReturn;
  }

  case StmtKind::TryCatchStmt: {
    auto tcStmt = static_cast<const TryCatchStmt *>(stmt);
    bool finallyReturns = checkDefiniteReturn(tcStmt->getFinallyBody(), Diags);

    // If 'finally' returns/throws, the whole block guarantees it regardless of
    // try/catch.
    if (finallyReturns)
      return true;
    bool tryReturns = checkDefiniteReturn(tcStmt->getTryBody(), Diags);
    bool catchReturns = tcStmt->getCatchBody()
                            ? checkDefiniteReturn(tcStmt->getCatchBody(), Diags)
                            : true;
    return tryReturns && catchReturns;
  }

  default:
    return false;
  }
}
} // namespace

void TypeChecker::processPendingInstantiations() {
  int instantiationDepth = 0;
  const int MAX_INSTANTIATION_DEPTH = 64; // Standard compiler limit

  while (!pendingInstantiations.empty()) {
    // 1. Check for runaway recursive generics
    if (instantiationDepth++ > MAX_INSTANTIATION_DEPTH) {
      SourceLocation loc = pendingInstantiations.front()->getLoc();
      Diags.report(loc, DiagID::err_type_mismatch)
          << "Recursion Depth Exceeded";
      hasError = true;
      pendingInstantiations.clear();
      break;
    }

    // 2. Cascade Breaker: Stop instantiating if previous errors (like infinite
    // size) triggered
    if (hasError) {
      pendingInstantiations.clear();
      break;
    }

    auto queue = pendingInstantiations;
    pendingInstantiations.clear();

    for (const ClassDecl *concreteClass : queue) {
      // Type check the freshly generated class to infer types on its internal
      // expressions
      concreteClass->accept(*this);
    }
  }
}

TypeChecker::TypeChecker(ASTContext &ctx, SymbolTable &sym,
                         DiagnosticEngine &diags)
    : context(ctx), symbols(sym), Diags(diags), resolver(ctx) {
  lastComputedType = context.getVoidType();
  currentExpectedReturnType = nullptr;
  currentClassDecl = nullptr;
  hasError = false;
  inConstructorContext = false;
}

void TypeChecker::check(Decl *decl) {
  if (decl)
    decl->accept(*this);
  processPendingInstantiations();
}

void TypeChecker::check(Stmt *stmt) {
  if (stmt)
    stmt->accept(*this);
}

// --- Helper: Type Compatibility ---

bool TypeChecker::isCompatible(const Type *expected, const Type *actual) {
  if (!expected || !actual)
    return false;

  // Resolve type aliases before checking compatibility
  auto resolveAlias = [&](const Type *t) -> const Type * {
    while (auto named = llvm::dyn_cast_or_null<const NamedType>(t)) {
      if (context.lookupClass(named->getName()))
        break;
      Symbol *sym = symbols.lookup(named->getName());
      if (sym && sym->kind == SymbolKind::Type && sym->type) {
        if (auto inner = llvm::dyn_cast<const NamedType>(sym->type)) {
          if (inner->getName() == named->getName())
            break;
        }
        t = sym->type;
      } else
        break;
    }
    return t;
  };

  expected = resolveAlias(expected);
  actual = resolveAlias(actual);

  // Exact match
  if (expected->isEquivalent(*actual))
    return true;

  if (auto nullType = llvm::dyn_cast<const NullableType>(expected)) {
    if (actual->is<NullType>())
      return true;
    if (isCompatible(nullType->getInner(), actual))
      return true;
    return false;
  }

  const Type *rawExp = unwrapConcurrency(expected);
  const Type *rawAct = unwrapConcurrency(actual);

  // Ensures we can safely pass decimal<5,2> into a function expecting
  // decimal<9,2>
  if (auto expDec = llvm::dyn_cast<const DecimalType>(rawExp)) {
    if (auto actDec = llvm::dyn_cast<const DecimalType>(rawAct)) {
      unsigned int expWhole = expDec->getPrecision() - expDec->getScale();
      unsigned int actWhole = actDec->getPrecision() - actDec->getScale();

      // Target must safely hold both the whole numbers AND the fractional
      // parts!
      return (expDec->getScale() >= actDec->getScale() && expWhole >= actWhole);
    }
  }

  bool isPtrOrRefExp = rawExp->is<PointerType>() || rawExp->is<ReferenceType>();
  bool isPtrOrRefAct = rawAct->is<PointerType>() || rawAct->is<ReferenceType>();

  if (isPtrOrRefExp && isPtrOrRefAct) {
    // Rule 2: Cannot take mutable capability to immutable storage
    if (hasMutOrLock(expected) && hasView(actual)) {
      return false;
    }

    if (hasMutOrLock(expected) && !hasLock(expected) && hasLock(actual)) {
      return false;
    }
  }

  if (rawExp->isEquivalent(*rawAct))
    return true;

  // Handle Generic Parameters (NamedTypes without ClassDecls)
  if (auto namedExp = llvm::dyn_cast<const NamedType>(rawExp)) {
    if (auto namedAct = llvm::dyn_cast<const NamedType>(rawAct)) {
      if (namedExp->getName() != namedAct->getName())
        return false;
      const auto &expArgs = namedExp->getGenericArgs();
      const auto &actArgs = namedAct->getGenericArgs();
      if (expArgs.size() != actArgs.size())
        return false;

      for (size_t i = 0; i < expArgs.size(); ++i) {
        if (!expArgs[i].type->isEquivalent(*actArgs[i].type))
          return false;
      }
      return true;
    }
  }

  // --- Unified Array & Slice Compatibility ---
  bool expIsArray = rawExp->is<ArrayType>() || rawExp->is<SliceType>();
  bool actIsArray = rawAct->is<ArrayType>() || rawAct->is<SliceType>();

  if (expIsArray && actIsArray) {
    const Type *expElem = rawExp->is<SliceType>()
                              ? llvm::cast<SliceType>(rawExp)->getElementType()
                              : llvm::cast<ArrayType>(rawExp)->getElementType();
    const Type *actElem = rawAct->is<SliceType>()
                              ? llvm::cast<SliceType>(rawAct)->getElementType()
                              : llvm::cast<ArrayType>(rawAct)->getElementType();

    if (!isCompatible(expElem, actElem))
      return false;

    bool expIsDynamic = rawExp->is<SliceType>();
    bool actIsDynamic = rawAct->is<SliceType>();

    if (!expIsDynamic && !actIsDynamic) {
      // Both are fixed-size: safely compare their numeric values
      auto expSize = llvm::dyn_cast<const IntegerLiteral>(
          llvm::cast<ArrayType>(rawExp)->getSizeExpr());
      auto actSize = llvm::dyn_cast<const IntegerLiteral>(
          llvm::cast<ArrayType>(rawAct)->getSizeExpr());
      if (expSize && actSize) {
        return expSize->getValue() == actSize->getValue();
      }
      return rawExp->toString() == rawAct->toString();
    } else if (!expIsDynamic && actIsDynamic) {
      // Expected fixed, Actual dynamic: Cannot assign T[] to T[N] safely!
      return false;
    }
    return true;
  }

  // Recursive Function & Closure Type Compatibility
  const std::vector<TypePtr> *expParams = nullptr;
  const Type *expRet = nullptr;
  const std::vector<TypePtr> *actParams = nullptr;
  const Type *actRet = nullptr;

  if (auto fnExp = llvm::dyn_cast<const FunctionType>(rawExp)) {
    expParams = &fnExp->getParamTypes();
    expRet = fnExp->getReturnType();
  } else if (auto clExp = llvm::dyn_cast<const ClosureType>(rawExp)) {
    expParams = &clExp->getParamTypes();
    expRet = clExp->getReturnType();
  }

  if (auto fnAct = llvm::dyn_cast<const FunctionType>(rawAct)) {
    actParams = &fnAct->getParamTypes();
    actRet = fnAct->getReturnType();
  } else if (auto clAct = llvm::dyn_cast<const ClosureType>(rawAct)) {
    actParams = &clAct->getParamTypes();
    actRet = clAct->getReturnType();
  }

  if (expParams && actParams) {
    if (expParams->size() != actParams->size())
      return false;
    if (!isCompatible(expRet, actRet))
      return false;
    for (size_t i = 0; i < expParams->size(); ++i) {
      if (!isCompatible((*expParams)[i].get(), (*actParams)[i].get()))
        return false;
    }
    return true;
  }

  // Recursive Map/Table Compatibility
  if (auto mapExp = llvm::dyn_cast<const MapType>(rawExp)) {
    if (auto mapAct = llvm::dyn_cast<const MapType>(rawAct)) {
      return isCompatible(mapExp->getKeyType(), mapAct->getKeyType()) &&
             isCompatible(mapExp->getValueType(), mapAct->getValueType());
    }
  }

  // AnyType is the universal top/bottom type
  if (rawExp->is<AnyType>() || rawAct->is<AnyType>())
    return true;

  if (rawExp->is<PointerType>() && rawAct->is<NullType>())
    return true;

  // Nullable Logic: T? accepts T and Null
  if (auto nullType = llvm::dyn_cast<const NullableType>(rawExp)) {
    if (rawAct->is<NullType>())
      return true;
    if (auto actualNull = llvm::dyn_cast<const NullableType>(rawAct)) {
      return isCompatible(nullType->getInner(), actualNull->getInner());
    }
    return isCompatible(nullType->getInner(), rawAct);
  }

  // Expanded Pointer Logic
  if (auto ptrExp = llvm::dyn_cast<const PointerType>(rawExp)) {
    if (auto ptrAct = llvm::dyn_cast<const PointerType>(rawAct)) {
      return isCompatible(ptrExp->getPointee(), ptrAct->getPointee());
    }
    if (ptrExp->getPointee()->is<PrimitiveType>() &&
        ((const PrimitiveType *)ptrExp->getPointee())->getScalar() ==
            PrimitiveType::Scalar::Void) {
      return rawAct->is<PointerType>();
    }
  }

  if (auto refType = llvm::dyn_cast<const ReferenceType>(rawExp)) {
    return isCompatible(refType->getInner(), rawAct);
  }

  if (auto refAct = llvm::dyn_cast<const ReferenceType>(rawAct)) {
    return isCompatible(rawExp, refAct->getInner());
  }

  if (isIntegerOrChar(expected) && isIntegerOrChar(actual)) {
    return true;
  }

  // Allow Primitive = Function returning that Primitive
  if (auto fn = llvm::dyn_cast<const FunctionType>(actual)) {
    if (isCompatible(expected, fn->getReturnType()))
      return true;
  }

  // Float promotion
  if (expected->isFloat() && (actual->isFloat() || isIntegerOrChar(actual))) {
    return true;
  }

  // Class Inheritance Checking
  if (auto expectedClass = llvm::dyn_cast<const NamedType>(expected)) {
    const Type *actualToUse = actual;

    if (auto ptrAct = llvm::dyn_cast<const PointerType>(actual)) {
      if (const ClassDecl *cls =
              context.lookupClass(expectedClass->getName())) {
        if (cls->isReferenceType()) {
          actualToUse = ptrAct->getPointee();
        }
      }
    }

    if (auto actualClass = llvm::dyn_cast<const NamedType>(actualToUse)) {
      if (isSubclassOf(context.lookupClass(actualClass->getName()),
                       expectedClass->getName())) {
        return true;
      }
    }
  }

  // --- [NEW] C-String Interoperability ---
  // 1. Allow assigning a String to a Char Array/Slice (char[6] = "Hello" or
  // char[] = "Hello")
  if (expIsArray) {
    const Type *expElem = rawExp->is<SliceType>()
                              ? llvm::cast<SliceType>(rawExp)->getElementType()
                              : llvm::cast<ArrayType>(rawExp)->getElementType();
    if (isIntegerOrChar(expElem) && rawAct->isString()) {
      return true;
    }
  }

  // 2. Allow implicitly using a Char Array/Slice as a String
  if (rawExp->isString()) {
    if (actIsArray) {
      const Type *actElem =
          rawAct->is<SliceType>()
              ? llvm::cast<SliceType>(rawAct)->getElementType()
              : llvm::cast<ArrayType>(rawAct)->getElementType();
      if (isIntegerOrChar(actElem)) {
        return true;
      }
    }
    // 3. Allow Char Pointers to decay to Strings (string str = *char)
    if (auto ptrAct = llvm::dyn_cast<const PointerType>(rawAct)) {
      if (isIntegerOrChar(ptrAct->getPointee())) {
        return true;
      }
    }
  }

  return false;
}

bool TypeChecker::isCastAllowed(const Type *src, const Type *dst) {
  // Resolve type aliases before checking casts
  auto resolveAlias = [&](const Type *t) -> const Type * {
    while (auto named = llvm::dyn_cast_or_null<const NamedType>(t)) {
      if (context.lookupClass(named->getName()))
        break;
      Symbol *sym = symbols.lookup(named->getName());
      if (sym && sym->kind == SymbolKind::Type && sym->type) {
        if (auto inner = llvm::dyn_cast<const NamedType>(sym->type)) {
          if (inner->getName() == named->getName())
            break;
        }
        t = sym->type;
      } else
        break;
    }
    return t;
  };

  src = resolveAlias(unwrapConcurrency(src));
  dst = resolveAlias(unwrapConcurrency(dst));

  if (src->is<AnyType>() || dst->is<AnyType>()) {
    return true;
  }

  // 1. Allow casting between the same types
  if (src->isEquivalent(*dst))
    return true;

  if (dst->isString()) {
    return true;
  }

  // 2. Allow casting between any numeric types (int to float, etc.)
  if (isNumericOrChar(src) && isNumericOrChar(dst))
    return true;

  // 3. Pointer <-> Integer Casts (The Fix)
  // Allow pointers to be cast to/from isize and usize
  if (src->getKind() == TypeKind::Pointer && isIntegerOrChar(dst)) {
    return true;
  }
  if (dst->getKind() == TypeKind::Pointer && isIntegerOrChar(src)) {
    return true;
  }

  // 4. Pointer to Pointer casts (e.g., int* to void*)
  if (src->getKind() == TypeKind::Pointer &&
      dst->getKind() == TypeKind::Pointer) {
    return true;
  }

  return false;
}

const Type *TypeChecker::getCommonSuperType(const Type *t1, const Type *t2) {
  if (!t1 || !t2)
    return context.getAnyType();

  // Resolve type aliases
  auto resolveAlias = [&](const Type *t) -> const Type * {
    while (auto named = llvm::dyn_cast_or_null<const NamedType>(t)) {
      if (context.lookupClass(named->getName()))
        break;
      Symbol *sym = symbols.lookup(named->getName());
      if (sym && sym->kind == SymbolKind::Type && sym->type) {
        if (auto inner = llvm::dyn_cast<const NamedType>(sym->type)) {
          if (inner->getName() == named->getName())
            break;
        }
        t = sym->type;
      } else
        break;
    }
    return t;
  };

  t1 = resolveAlias(t1);
  t2 = resolveAlias(t2);

  if (isCompatible(t1, t2))
    return t1;
  if (isCompatible(t2, t1))
    return t2;

  // If both are classes, check if one is a subclass of the other
  if (t1->is<NamedType>() && t2->is<NamedType>()) {
    const ClassDecl *d1 =
        context.lookupClass(((const NamedType *)t1)->getName());
    const ClassDecl *d2 =
        context.lookupClass(((const NamedType *)t2)->getName());

    if (isSubclassOf(d1, d2->getName()))
      return context.createNamedType(d2->getName());
    if (isSubclassOf(d2, d1->getName()))
      return context.createNamedType(d1->getName());
  }

  return context.getAnyType();
}

bool TypeChecker::isSubclassOf(const ClassDecl *child,
                               const std::string &parentName) {
  if (!child)
    return false;
  if (child->getName() == parentName)
    return true;

  // Recursively check all inheritance branches
  for (const auto &pName : child->getParentNames()) {
    if (isSubclassOf(context.lookupClass(pName), parentName)) {
      return true;
    }
  }
  return false;
}

bool TypeChecker::checkVisibility(const Decl *memberDecl,
                                  const ClassDecl *ownerClass,
                                  SourceLocation loc) {
  Visibility vis = memberDecl->getVisibility();
  if (vis == Visibility::Public)
    return true;

  if (vis == Visibility::Private) {
    if (currentClassDecl &&
        currentClassDecl->getName() == ownerClass->getName())
      return true;
    // [FIX] Use getName() instead of getDeclName()
    Diags.report(loc, DiagID::err_invalid_access)
        << memberDecl->getName() << "private";
    return false;
  }

  if (vis == Visibility::Protected) {
    if (currentClassDecl &&
        isSubclassOf(currentClassDecl, ownerClass->getName()))
      return true;
    // [FIX] Use getName() instead of getDeclName()
    Diags.report(loc, DiagID::err_invalid_access)
        << memberDecl->getName() << "protected";
    return false;
  }
  return true;
}

bool TypeChecker::detectInfiniteSize(const Type *t,
                                     std::set<std::string> &visited) {
  if (!t)
    return false;

  if (auto named = llvm::dyn_cast<const NamedType>(t)) {
    // 1. CYCLE DETECTED!
    if (visited.count(named->getName()))
      return true;

    // 2. Value classes are inline; recursively explore their fields
    const ClassDecl *cls = context.lookupClass(named->getName());
    if (cls && !cls->isReferenceType()) {
      visited.insert(named->getName());
      for (const auto &member : cls->getMembers()) {
        if (auto varDecl = llvm::dyn_cast<const VariableDecl>(member.get())) {
          if (detectInfiniteSize(varDecl->getType(), visited)) {
            return true;
          }
        }
      }
      visited.erase(named->getName()); // Backtrack
    }

    // 3. Recurse into generic arguments (e.g., Node<Node<T>>)
    for (const auto &arg : named->getGenericArgs()) {
      if (detectInfiniteSize(arg.type.get(), visited))
        return true;
    }
  } else if (auto arr = llvm::dyn_cast<const ArrayType>(t)) {
    // 4. Fixed-size arrays (T[10]) are inline and must be checked.
    // Dynamic arrays (T[]) are heap pointers and break the cycle!
    if (arr->getSizeExpr() != nullptr) {
      return detectInfiniteSize(arr->getElementType(), visited);
    }
    return false;
  } else if (llvm::dyn_cast<const NullableType>(t)) {
    // 5. Nullables (T?) are boxed/heap-allocated pointers, breaking the cycle!
    return false;
  }

  // PointerType and ReferenceType return false implicitly here,
  // safely breaking the cycle because pointers have a known, fixed memory size!
  return false;
}

// --- ASTVisitor Overrides: Expressions ---

void TypeChecker::visitFloatLiteral(const FloatLiteral *expr) {
  switch (expr->getSuffix()) {
  case NumericSuffix::f8:
    lastComputedType = context.getF8Type();
    break;
  case NumericSuffix::f16:
    lastComputedType = context.getF16Type();
    break;
  case NumericSuffix::f32:
    lastComputedType = context.getF32Type();
    break;
  case NumericSuffix::f64:
  default:
    // Default to f64 (double) if no suffix is provided
    lastComputedType = context.getF64Type();
    break;
  }
  const_cast<FloatLiteral *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitDecimalLiteral(const DecimalLiteral *expr) {
  const std::string &raw = expr->getValue();
  unsigned int precision = 0;
  unsigned int scale = 0;
  bool inFraction = false;

  // Dynamically calculate precision and scale from the raw string
  for (char c : raw) {
    if (c == '.') {
      inFraction = true;
      continue;
    }
    if (c == 'd' || c == 'D' || c == '_') {
      continue; // Ignore suffix and visual separators
    }
    if (isdigit(c)) {
      precision++;
      if (inFraction) {
        scale++;
      }
    }
  }

  // Edge case: "0d"
  if (precision == 0)
    precision = 1;

  lastComputedType = context.createDecimalType(precision, scale);

  // Attach the inferred type to the AST node
  const_cast<DecimalLiteral *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitIntegerLiteral(const IntegerLiteral *expr) {
  switch (expr->getSuffix()) {
  case NumericSuffix::i8:
    lastComputedType = context.getI8Type();
    break;
  case NumericSuffix::i16:
    lastComputedType = context.getI16Type();
    break;
  case NumericSuffix::i64:
    lastComputedType = context.getI64Type();
    break;
  case NumericSuffix::u8:
    lastComputedType = context.getU8Type();
    break;
  case NumericSuffix::u16:
    lastComputedType = context.getU16Type();
    break;
  case NumericSuffix::u32:
    lastComputedType = context.getU32Type();
    break;
  case NumericSuffix::u64:
    lastComputedType = context.getU64Type();
    break;
  case NumericSuffix::isize:
    lastComputedType = context.getISizeType();
    break;
  case NumericSuffix::usize:
    lastComputedType = context.getUSizeType();
    break;
  default:
    lastComputedType = context.getI32Type();
    break;
  }
  // ✅ Attach type to AST node
  const_cast<IntegerLiteral *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitStringLiteral(const StringLiteral *expr) {
  lastComputedType = context.getStringType();
  // ✅ Attach type to AST node
  const_cast<StringLiteral *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitBoolLiteral(const BoolLiteral *expr) {
  lastComputedType = context.getBoolType();
  // ✅ Attach type to AST node
  const_cast<BoolLiteral *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitNullLiteral(const NullLiteral *expr) {
  lastComputedType = context.getNullType();
  // ✅ Attach type to AST node
  const_cast<NullLiteral *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitCharLiteral(const CharLiteral *expr) {
  lastComputedType = context.getCharType();
  // ✅ Attach type to AST node
  const_cast<CharLiteral *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitArrayLiteral(const ArrayLiteral *expr) {
  const Type *expectedElemType = nullptr;

  // Extract expected element type from the LHS declaration if it exists
  if (currentExpectedReturnType) {
    if (auto arrT =
            llvm::dyn_cast<const ArrayType>(currentExpectedReturnType)) {
      expectedElemType = arrT->getElementType();
    } else if (auto sliceT =
                   llvm::dyn_cast<const SliceType>(currentExpectedReturnType)) {
      // [FIX] Extract the expected type for dynamic arrays (int[])
      expectedElemType = sliceT->getElementType();
    }
  }

  // Push the expected element type down to the children so nested arrays
  // validate correctly
  const Type *previousExpected = currentExpectedReturnType;
  currentExpectedReturnType = expectedElemType;

  const Type *commonType = nullptr;
  bool hasSpread = false;
  for (const auto &elem : expr->getElements()) {
    elem->accept(*this);
    const Type *elemType = lastComputedType;

    // Check if the element is a UnaryExpr using the DotDotDot (...) operator
    if (auto unary = llvm::dyn_cast<const UnaryExpr>(elem.get())) {
      if (unary->getOp() == TokenKind::DotDotDot) {
        hasSpread = true;
        const Type *unwrapped = unwrapConcurrency(elemType);
        if (auto sliceType = llvm::dyn_cast<const SliceType>(unwrapped)) {
          elemType = sliceType->getElementType();
        } else if (auto arrType = llvm::dyn_cast<const ArrayType>(unwrapped)) {
          elemType = arrType->getElementType();
        }
      }
    }

    // Strictly enforce element types against the expected type
    if (expectedElemType && !isCompatible(expectedElemType, elemType)) {
      Diags.report(elem->getLoc(), DiagID::err_type_mismatch)
          << "Array element type mismatch. Expected "
          << expectedElemType->toString() << " but found "
          << elemType->toString();
      hasError = true;
    }

    // Recursive C-String Bounds Checking for 2D+ Arrays
    if (expectedElemType) {
      if (auto expectedArr =
              llvm::dyn_cast<const ArrayType>(expectedElemType)) {
        if (isCharType(expectedArr->getElementType()) && elemType->isString()) {
          if (auto strLit = llvm::dyn_cast<const StringLiteral>(elem.get())) {
            if (auto sizeLit = llvm::dyn_cast<const IntegerLiteral>(
                    expectedArr->getSizeExpr())) {
              uint64_t requiredSize =
                  strLit->getValue().length() + 1; // +1 for '\0'
              if (sizeLit->getValue() < requiredSize) {
                Diags.report(elem->getLoc(), DiagID::err_array_length)
                    << "Char array is too small. '\"" << strLit->getValue()
                    << "\"' requires " << requiredSize
                    << " bytes (including '\\0'), but array is size "
                    << sizeLit->getValue();
                hasError = true;
              }
            }
          }
        }
      }
    }

    if (!commonType) {
      commonType = elemType;
    } else {
      commonType = getCommonSuperType(commonType, elemType);
    }
  }

  // Restore the expected return type for the rest of the AST
  currentExpectedReturnType = previousExpected;

  if (!commonType) {
    commonType = expectedElemType ? expectedElemType : context.getAnyType();
  }

  // Type dynamic array literals as Slices natively
  if (hasSpread) {
    // Dynamic spread means the size is unknown at compile time -> emit a
    // SliceType!
    lastComputedType = context.getSliceType(commonType);
  } else {
    // Fixed number of elements -> emit a static ArrayType
    auto sizeExpr = std::make_unique<IntegerLiteral>(
        expr->getElements().size(), NumericSuffix::None, expr->getLoc());
    lastComputedType = context.createArrayType(commonType, std::move(sizeExpr));
  }

  const_cast<ArrayLiteral *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitMapLiteral(const MapLiteral *expr) {
  if (expr->getEntries().empty()) {
    // Empty map defaults to table<any, any> or context specific
    lastComputedType =
        context.createMapType(context.getAnyType(), context.getAnyType());
    return;
  }

  // Infer type from first element
  const auto &first = expr->getEntries()[0];

  first.first->accept(*this);
  const Type *keyType = lastComputedType;

  first.second->accept(*this);
  const Type *valType = lastComputedType;

  // Verify all other entries match
  for (size_t i = 1; i < expr->getEntries().size(); ++i) {
    const auto &entry = expr->getEntries()[i];

    entry.first->accept(*this);
    if (!isCompatible(keyType, lastComputedType)) {
      Diags.report(entry.first->getLoc(), DiagID::err_type_mismatch)
          << "Map key type mismatch";
    }

    entry.second->accept(*this);
    if (!isCompatible(valType, lastComputedType)) {
      Diags.report(entry.second->getLoc(), DiagID::err_type_mismatch)
          << "Map value type mismatch";
    }
  }

  lastComputedType = context.createMapType(keyType, valType);
  const_cast<MapLiteral *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitIdentifierExpr(const IdentifierExpr *expr) {
  //  Ambiguous Import Check
  if (ambiguousImports.count(expr->getName()) &&
      ambiguousImports[expr->getName()].size() > 1) {
    const auto &sources = ambiguousImports[expr->getName()];

    Diags.report(expr->getLoc(), DiagID::err_ambiguous_reference)
        << "Ambiguous reference to '" << expr->getName() << "'. "
        << "Did you mean '" << sources[0] << "." << expr->getName() << "' or '"
        << sources[1] << "." << expr->getName() << "'?";
    lastComputedType = context.getAnyType();
    hasError = true;
    return;
  }

  Symbol *sym = symbols.lookup(expr->getName());
  if (!sym) {
    Diags.report(expr->getLoc(), DiagID::err_undeclared_identifier)
        << expr->getName();
    lastComputedType = context.getAnyType();
    hasError = true;
  } else {
    if (!isLHSOfAssignment && sym->kind == SymbolKind::Variable && sym->decl) {
      if (auto varDecl = llvm::dyn_cast<const VariableDecl>(sym->decl)) {
        if (!initializedVars.count(varDecl)) {
          Diags.report(expr->getLoc(), DiagID::err_uninitialized_var)
              << ": '" << expr->getName()
              << "' used before being definitely assigned";
          hasError = true;
        }
      }
    }
    lastComputedType = sym->type;
  }
  const_cast<IdentifierExpr *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitBinaryExpr(const BinaryExpr *expr) {
  TokenKind op = expr->getOp();
  bool oldLhs = isLHSOfAssignment;
  if (op == TokenKind::Equal) {
    isLHSOfAssignment = true;
  }
  expr->getLHS()->accept(*this);
  const Type *lhsType = lastComputedType;
  isLHSOfAssignment = oldLhs;
  expr->getRHS()->accept(*this);
  const Type *rhsType = lastComputedType;

  // OPERATOR OVERLOADING DESUGARING
  if (auto *namedTy = llvm::dyn_cast_or_null<NamedType>(lhsType)) {
    if (const ClassDecl *classDecl = context.lookupClass(namedTy->getName())) {
      // Map the TokenKind to the operator method name
      std::string opFuncName = "operator";
      switch (expr->getOp()) {
      // Arithmetic
      case TokenKind::Plus:
        opFuncName += "+";
        break;
      case TokenKind::Minus:
        opFuncName += "-";
        break;
      case TokenKind::Star:
        opFuncName += "*";
        break;
      case TokenKind::Slash:
        opFuncName += "/";
        break;
      case TokenKind::Percent:
        opFuncName += "%";
        break;
      case TokenKind::Power:
        opFuncName += "**";
        break;

      // Relational / Equality
      case TokenKind::EqualEqual:
        opFuncName += "==";
        break;
      case TokenKind::NotEqual:
        opFuncName += "!=";
        break;
      case TokenKind::Less:
        opFuncName += "<";
        break;
      case TokenKind::LessEqual:
        opFuncName += "<=";
        break;
      case TokenKind::Greater:
        opFuncName += ">";
        break;
      case TokenKind::GreaterEqual:
        opFuncName += ">=";
        break;

      // Bitwise
      case TokenKind::Amp:
        opFuncName += "&";
        break;
      case TokenKind::Pipe:
        opFuncName += "|";
        break;
      case TokenKind::Caret:
        opFuncName += "^";
        break;
      case TokenKind::LessLess:
        opFuncName += "<<";
        break;
      case TokenKind::GreaterGreater:
        opFuncName += ">>";
        break;

      // Logical
      case TokenKind::AmpAmp:
        opFuncName += "&&";
        break;
      case TokenKind::PipePipe:
        opFuncName += "||";
        break;

      default:
        break;
      }

      // Search the class for this operator method
      for (const auto &member : classDecl->getMembers()) {
        if (auto *method = llvm::dyn_cast<FunctionDecl>(member.get())) {
          if (method->getName() == opFuncName) {
            // Found it! Check if the RHS type matches the parameter type
            if (method->getParams().size() == 1 &&
                method->getParams()[0].type->isEquivalent(*rhsType)) {

              // Annotate the AST using a safe const_cast
              const_cast<BinaryExpr *>(expr)->setResolvedOperator(method);

              // Set the type of this BinaryExpr to the method's return type
              const_cast<BinaryExpr *>(expr)->setType(method->getReturnType());
              lastComputedType = method->getReturnType();
              return;
            }
          }
        }
      }
    }
  }

  auto setAndReturn = [&]() {
    const_cast<BinaryExpr *>(expr)->setType(lastComputedType);
  };

  if (lhsType->is<AnyType>() || rhsType->is<AnyType>()) {
    lastComputedType = context.getAnyType();
    setAndReturn();
    return;
  }

  if (op == TokenKind::Equal) {
    bool isDirectVar = false;

    // 1. Direct Variable Assignment (e.g., p_view = &data)
    if (auto id = llvm::dyn_cast<const IdentifierExpr>(expr->getLHS())) {
      isDirectVar = true;
      Symbol *sym = symbols.lookup(id->getName());
      if (sym && sym->decl) {
        if (auto varDecl = llvm::dyn_cast<const VariableDecl>(sym->decl)) {
          if (varDecl->isConstVar()) {
            Diags.report(expr->getLoc(), DiagID::err_const_violation)
                << "'" << id->getName() << "'";
            hasError = true;
          }
        }
        initializedVars.insert(sym->decl); // Mark as definitely assigned
      }
    }

    // 2. Memory Access Mutations (e.g., *p = 10, arr[0] = 5, obj.x = 2)
    const Expr *target = expr->getLHS();
    while (true) {
      if (auto mem = llvm::dyn_cast<const MemberExpr>(target))
        target = mem->getObject();
      else if (auto idx = llvm::dyn_cast<const IndexExpr>(target))
        target = idx->getArray();
      else
        break;
    }

    if (!isDirectVar) {
      bool allowed = hasMutOrLock(lhsType);

      if (!allowed && inConstructorContext &&
          llvm::dyn_cast<const ThisExpr>(target)) {
        allowed = true;
      }

      // Deep check for pointers and base variables
      if (!allowed) {
        bool oldLhs = isLHSOfAssignment;
        isLHSOfAssignment = true;
        if (auto un = llvm::dyn_cast<const UnaryExpr>(target)) {
          if (un->getOp() == TokenKind::Star) {
            un->getOperand()->accept(*this);
            if (hasMutOrLock(lastComputedType))
              allowed = true;
          }
        } else if (auto baseId = llvm::dyn_cast<const IdentifierExpr>(target)) {
          baseId->accept(*this);
          if (lastComputedType->is<PointerType>() ||
              lastComputedType->is<ReferenceType>()) {
            if (hasMutOrLock(lastComputedType))
              allowed = true;
          } else {
            Symbol *sym = symbols.lookup(baseId->getName());
            if (sym && sym->decl) {
              if (auto varDecl =
                      llvm::dyn_cast<const VariableDecl>(sym->decl)) {
                if (!varDecl->isConstVar())
                  allowed = true; // Array or Struct declared as 'mut'
              }
            }
          }
        } else if (llvm::dyn_cast<const ThisExpr>(target)) {
          // 'this' is intrinsically mutable by default in member functions!
          allowed = true;
        }
        isLHSOfAssignment = oldLhs;
      }

      if (!allowed) {
        std::string typeStr = lhsType->toString();
        Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
            << "Cannot mutate immutable type '" << typeStr
            << "'. Did you forget 'mut' or 'lock'?";
        hasError = true;
      }

      // Mark base identifier as initialized (preserving old struct behavior)
      if (auto id = llvm::dyn_cast<const IdentifierExpr>(target)) {
        Symbol *sym = symbols.lookup(id->getName());
        if (sym && sym->decl) {
          initializedVars.insert(sym->decl);
        }
      }
    }
  }

  // Null Coalescing Operator (??)
  if (expr->getOp() == TokenKind::QuestionQuestion) {
    if (auto nullType = llvm::dyn_cast<const NullableType>(lhsType)) {
      if (!isCompatible(nullType->getInner(), rhsType)) {
        Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
            << "Right side of '?"
               "?' must match the inner type of the left side";
        hasError = true;
      }
      // Strips the Nullability! string? ?? string -> string
      lastComputedType = getCommonSuperType(nullType->getInner(), rhsType);
    } else {
      // LHS was never nullable, so ?? is redundant but safely evaluates to LHS
      lastComputedType = lhsType;
    }
    return;
  }

  // Operator Overloading Resolution
  if (auto namedLhs = llvm::dyn_cast<const NamedType>(lhsType)) {
    if (const ClassDecl *cls = context.lookupClass(namedLhs->getName())) {
      std::string opName = "operator";
      switch (expr->getOp()) {
      case TokenKind::Plus:
        opName += "+";
        break;
      case TokenKind::Minus:
        opName += "-";
        break;
      case TokenKind::Star:
        opName += "*";
        break;
      case TokenKind::Slash:
        opName += "/";
        break;
      case TokenKind::EqualEqual:
        opName += "==";
        break;
      case TokenKind::NotEqual:
        opName += "!=";
        break;
      case TokenKind::Less:
        opName += "<";
        break;
      case TokenKind::Greater:
        opName += ">";
        break;
      case TokenKind::LessEqual:
        opName += "<=";
        break;
      case TokenKind::GreaterEqual:
        opName += ">=";
        break;
      default:
        break;
      }

      // If it's a valid overloadable operator, search the class for it
      if (opName != "operator") {
        const FunctionDecl *bestMatch = nullptr;
        const ClassDecl *current = cls;

        while (current && !bestMatch) {
          for (const auto &member : current->getMembers()) {
            if (auto fn = llvm::dyn_cast<const FunctionDecl>(member.get())) {
              if (fn->getName() == opName && fn->getParams().size() == 1) {
                if (isCompatible(fn->getParams()[0].type.get(), rhsType)) {
                  bestMatch = fn;
                  break;
                }
              }
            }
          }
          current = current->getParentNames().empty()
                        ? nullptr
                        : context.lookupClass(current->getParentNames()[0]);
        }

        // If we found the operator method, resolve it and exit early!
        if (bestMatch) {
          lastComputedType = bestMatch->getReturnType();
          return;
        }
      }
    }
  }

  bool isMath = op == TokenKind::Plus || op == TokenKind::Minus ||
                op == TokenKind::Star || op == TokenKind::Slash ||
                op == TokenKind::Percent;

  bool isLogic = op == TokenKind::AmpAmp || op == TokenKind::PipePipe;

  bool isComp = op == TokenKind::EqualEqual || op == TokenKind::NotEqual ||
                op == TokenKind::Less || op == TokenKind::Greater ||
                op == TokenKind::LessEqual || op == TokenKind::GreaterEqual;

  bool isBitwise = op == TokenKind::Amp || op == TokenKind::Pipe ||
                   op == TokenKind::Caret || op == TokenKind::LessLess ||
                   op == TokenKind::GreaterGreater;

  const Type *rawLHS = unwrapConcurrency(lhsType);
  const Type *rawRHS = unwrapConcurrency(rhsType);

  if (isMath) {
    // Check string concatenation
    if (lhsType->isString() || rhsType->isString()) {
      if (op == TokenKind::Plus) {
        lastComputedType = context.getStringType();
        return;
      }
    }

    // --- [NEW] Compile-Time Decimal Math Scaling ---
    if (auto lhsDec =
            llvm::dyn_cast<const DecimalType>(unwrapConcurrency(lhsType))) {
      if (auto rhsDec =
              llvm::dyn_cast<const DecimalType>(unwrapConcurrency(rhsType))) {
        unsigned int p1 = lhsDec->getPrecision();
        unsigned int s1 = lhsDec->getScale();
        unsigned int p2 = rhsDec->getPrecision();
        unsigned int s2 = rhsDec->getScale();

        unsigned int newP = 0, newS = 0;

        switch (op) {
        case TokenKind::Plus:
        case TokenKind::Minus: {
          newS = std::max(s1, s2);
          unsigned int maxWhole = std::max(p1 - s1, p2 - s2);
          newP = maxWhole + newS + 1; // +1 for the carry bit!
          break;
        }
        case TokenKind::Star: {
          newP = p1 + p2;
          newS = s1 + s2;
          break;
        }
        case TokenKind::Slash: {
          newS = std::max(s1, s2);
          newS = std::max(newS, 6u); // Enforce min scale of 6 for fractions
          unsigned int maxWhole = (p1 - s1) + s2;
          newP = maxWhole + newS;
          break;
        }
        default:
          break;
        }

        if (newP > 0) {
          lastComputedType = context.createDecimalType(newP, newS);
          return; // Return the new perfectly scaled type!
        }
      }
    }

    if (op == TokenKind::Plus || op == TokenKind::Minus) {
      bool isPtrL = rawLHS->is<PointerType>();
      bool isPtrR = rawRHS->is<PointerType>();

      // ptr + int OR ptr - int => ptr
      if (isPtrL && isIntegerOrChar(rawRHS)) {
        lastComputedType =
            lhsType; // Preserve original wrappers (like volatile)
        return;
      }
      // int + ptr => ptr
      if (isPtrR && isIntegerOrChar(rawLHS) && op == TokenKind::Plus) {
        lastComputedType = rhsType;
        return;
      }
      // ptr - ptr => isize (Pointer Difference)
      if (isPtrL && isPtrR && op == TokenKind::Minus) {
        if (!isCompatible(rawLHS, rawRHS)) {
          Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
              << "Cannot subtract pointers of different types";
          hasError = true;
        }
        lastComputedType = context.getISizeType();
        return;
      }
    }

    if (!isNumericOrChar(rawLHS) || !isNumericOrChar(rawRHS)) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "arithmetic requires numeric types";
      hasError = true;
      lastComputedType = context.getAnyType();
    } else {
      lastComputedType = getCommonSuperType(lhsType, rhsType);
    }
  } else if (isComp) {
    if (!isCompatible(lhsType, rhsType) && !isCompatible(rhsType, lhsType)) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Comparison between incompatible types";
      hasError = true;
    }
    lastComputedType = context.getBoolType();
  } else if (isLogic) {
    if (!rawLHS->isBool() || !rawRHS->isBool()) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Logic ops require bool";
      hasError = true;
    }
    lastComputedType = context.getBoolType();
  } else if (op == TokenKind::Equal) {
    // --- [NEW] Detailed Decimal Assignment Safety Check ---
    if (auto targetDec =
            llvm::dyn_cast<const DecimalType>(unwrapConcurrency(lhsType))) {
      if (auto sourceDec =
              llvm::dyn_cast<const DecimalType>(unwrapConcurrency(rhsType))) {
        unsigned int targetWhole =
            targetDec->getPrecision() - targetDec->getScale();
        unsigned int sourceWhole =
            sourceDec->getPrecision() - sourceDec->getScale();

        // 1. Truncation Error
        if (sourceDec->getScale() > targetDec->getScale()) {
          Diags.report(expr->getLoc(), DiagID::err_decimal_precision_loss)
              << "Cannot assign decimal with scale " << sourceDec->getScale()
              << " to scale " << targetDec->getScale() << " (would truncate).";
          hasError = true;
        }

        // 2. Overflow Error
        if (sourceWhole > targetWhole) {
          Diags.report(expr->getLoc(), DiagID::err_decimal_overflow)
              << "Target supports " << targetWhole
              << " whole digits, but expression requires up to " << sourceWhole
              << ".";
          hasError = true;
        }

        if (!hasError) {
          lastComputedType = lhsType;
          return;
        }
      }
    }

    // Assignment
    if (!isCompatible(lhsType, rhsType)) {
      if (rhsType->is<NullType>() && !lhsType->is<NullableType>() &&
          !lhsType->is<AnyType>()) {
        Diags.report(expr->getLoc(), DiagID::err_null_assignment)
            << " '" << lhsType->toString() << "'";
      } else {
        Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
            << "Cannot assign type " << rhsType->toString() << " to "
            << lhsType->toString();
      }
      hasError = true;
    } else {
      const Type *targetLhs = unwrapConcurrency(lhsType);
      const Type *targetRhs = unwrapConcurrency(rhsType);

      if (!targetLhs->isEquivalent(*targetRhs)) {
        auto *mutExpr = const_cast<BinaryExpr *>(expr);
        auto cast = std::make_unique<CastExpr>(targetLhs->clone(),
                                               std::move(mutExpr->getRHSMut()),
                                               expr->getLoc());
        cast->setType(targetLhs);
        mutExpr->getRHSMut() = std::move(cast);
      }
    }

    // String Literal to Char Array Bounds Checking
    if (auto arrLHS = llvm::dyn_cast<const ArrayType>(lhsType)) {
      if (isCharType(arrLHS->getElementType()) && rhsType->isString()) {
        if (auto strLit = llvm::dyn_cast<const StringLiteral>(expr->getRHS())) {
          if (auto sizeLit =
                  llvm::dyn_cast<const IntegerLiteral>(arrLHS->getSizeExpr())) {
            uint64_t requiredSize =
                strLit->getValue().length() + 1; // +1 for '\0'
            if (sizeLit->getValue() < requiredSize) {
              Diags.report(expr->getLoc(), DiagID::err_array_length)
                  << "Char array is too small. '\"" << strLit->getValue()
                  << "\"' requires " << requiredSize
                  << " bytes (including '\\0'), but array is size "
                  << sizeLit->getValue();
              hasError = true;
            }
          }
        }
      }
    }
    lastComputedType = lhsType;
  } else if (isBitwise) {
    if (!isIntegerOrChar(rawLHS) || !isIntegerOrChar(rawRHS)) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Bitwise operations require integer types";
      hasError = true;
      lastComputedType = context.getAnyType();
    } else {
      if (op == TokenKind::LessLess || op == TokenKind::GreaterGreater) {
        if (auto literalRHS =
                llvm::dyn_cast<const IntegerLiteral>(expr->getRHS())) {
          uint64_t shiftVal = literalRHS->getValue();
          uint64_t maxBits = 32; // Default for standard int

          if (auto primLHS = llvm::dyn_cast<const PrimitiveType>(rawLHS)) {
            switch (primLHS->getScalar()) {
            case PrimitiveType::Scalar::I8:
            case PrimitiveType::Scalar::U8:
            case PrimitiveType::Scalar::Char:
              maxBits = 8;
              break;
            case PrimitiveType::Scalar::I16:
            case PrimitiveType::Scalar::U16:
              maxBits = 16;
              break;
            case PrimitiveType::Scalar::I32:
            case PrimitiveType::Scalar::U32:
            case PrimitiveType::Scalar::Int:
              maxBits = 32;
              break;
            case PrimitiveType::Scalar::I64:
            case PrimitiveType::Scalar::U64:
            case PrimitiveType::Scalar::ISize:
            case PrimitiveType::Scalar::USize:
              maxBits = 64;
              break;
            default:
              break;
            }
          }

          if (shiftVal >= maxBits) {
            Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
                << "Undefined Behavior: Shift count " << shiftVal
                << " is >= the bit width of the type (" << maxBits << " bits)";
            hasError = true;
          }
        }
      }
      lastComputedType = getCommonSuperType(rawLHS, rawRHS);
    }
  } else {
    // Fallback for other operators
    lastComputedType = lhsType;
  }
  setAndReturn();
}

void TypeChecker::visitUnaryExpr(const UnaryExpr *expr) {
  TokenKind op = expr->getOp();
  expr->getOperand()->accept(*this);
  const Type *operandType = lastComputedType;

  // UNARY OPERATOR OVERLOADING
  if (auto *namedTy = llvm::dyn_cast_or_null<NamedType>(operandType)) {
    if (const ClassDecl *classDecl = context.lookupClass(namedTy->getName())) {
      std::string opFuncName = "operator";
      switch (expr->getOp()) {
      case TokenKind::Minus:
        opFuncName += "-";
        break;
      case TokenKind::Plus:
        opFuncName += "+";
        break;
      case TokenKind::Bang:
        opFuncName += "!";
        break;
      case TokenKind::Tilde:
        opFuncName += "~";
        break;
      case TokenKind::PlusPlus:
        opFuncName += "++";
        break;
      case TokenKind::MinusMinus:
        opFuncName += "--";
        break;
      default:
        break;
      }

      for (const auto &member : classDecl->getMembers()) {
        if (auto *method = llvm::dyn_cast<FunctionDecl>(member.get())) {
          if (method->getName() == opFuncName && method->getParams().empty()) {
            const_cast<UnaryExpr *>(expr)->setResolvedOperator(method);
            const_cast<UnaryExpr *>(expr)->setType(method->getReturnType());
            lastComputedType = method->getReturnType();
            return;
          }
        }
      }
    }
  }

  auto setAndReturn = [&]() {
    const_cast<UnaryExpr *>(expr)->setType(lastComputedType);
  };

  // Intercept Spread Operator
  if (op == TokenKind::DotDotDot) {
    const Type *unwrapped = unwrapConcurrency(operandType);
    if (!unwrapped->is<SliceType>() && !unwrapped->is<ArrayType>()) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Spread operator (...) can only be used on arrays or slices; "
             "found '"
          << operandType->toString() << "'";
      hasError = true;
    }
    lastComputedType = operandType;
    return;
  }

  // 1. Unary Operator Overloading Resolution
  if (auto namedLhs = llvm::dyn_cast<const NamedType>(operandType)) {
    if (const ClassDecl *cls = context.lookupClass(namedLhs->getName())) {
      std::string opName = "operator";
      switch (op) {
      case TokenKind::Minus:
        opName += "-";
        break;
      case TokenKind::Bang:
        opName += "!";
        break;
      case TokenKind::Tilde:
        opName += "~";
        break;
      case TokenKind::PlusPlus:
        opName += "++";
        break;
      case TokenKind::MinusMinus:
        opName += "--";
        break;
      default:
        break;
      }

      if (opName != "operator") {
        const FunctionDecl *bestMatch = nullptr;
        std::vector<const ClassDecl *> queue = {cls};
        size_t head = 0;

        while (head < queue.size() && !bestMatch) {
          const ClassDecl *current = queue[head++];
          for (const auto &member : current->getMembers()) {
            if (auto fn = llvm::dyn_cast<const FunctionDecl>(member.get())) {
              // Unary operators take 0 parameters!
              if (fn->getName() == opName && fn->getParams().empty()) {
                bestMatch = fn;
                break;
              }
            }
          }
          if (!bestMatch) {
            for (const auto &pName : current->getParentNames()) {
              if (auto pCls = context.lookupClass(pName))
                queue.push_back(pCls);
            }
          }
        }

        if (bestMatch) {
          lastComputedType = bestMatch->getReturnType();
          return;
        }
      }
    }
  }

  // 2. Built-in Primitive Logic
  if (op == TokenKind::Bang) {
    if (!unwrapConcurrency(operandType)->isBool()) {
      Diags.report(expr->getLoc(), DiagID::warn_implicit_bool_conv);
    }
    lastComputedType = context.getBoolType();
  } else if (op == TokenKind::Minus) {
    if (!isNumericOrChar(operandType)) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Negation requires numeric";
      hasError = true;
    }
    lastComputedType = operandType;
  } else if (op == TokenKind::PlusPlus || op == TokenKind::MinusMinus) {
    if (!isIntegerOrChar(operandType) && !operandType->isFloat()) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Increment/Decrement requires numeric type";
      hasError = true;
    }
    lastComputedType = operandType;
  } else if (op == TokenKind::Tilde) {
    if (!isIntegerOrChar(operandType)) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Bitwise NOT requires integer type";
      hasError = true;
    }
    lastComputedType = operandType;
  } else if (op == TokenKind::Amp) {
    bool isImmutable = false;

    // 1. Check if the type itself is explicitly const or view
    if (operandType->is<ConstType>() || operandType->is<ViewType>()) {
      isImmutable = true;
    }

    // 2. Trace back to the base variable to see if it was declared as 'const'
    const Expr *target = expr->getOperand();
    while (true) {
      if (auto mem = llvm::dyn_cast<const MemberExpr>(target))
        target = mem->getObject();
      else if (auto idx = llvm::dyn_cast<const IndexExpr>(target))
        target = idx->getArray();
      else
        break;
    }

    if (auto id = llvm::dyn_cast<const IdentifierExpr>(target)) {
      Symbol *sym = symbols.lookup(id->getName());
      if (sym && sym->decl && sym->decl->getKind() == StmtKind::VariableDecl) {
        if (static_cast<const VariableDecl *>(sym->decl)->isConstVar()) {
          isImmutable = true;
        }
      }
    }

    const Type *inner = operandType;

    // 3. Since standard variables are mutable by default, taking their address
    // yields a mutable capability (*mut T) unless they are explicitly const.
    if (!isImmutable && !hasMutOrLock(inner)) {
      inner = context.createMutType(inner);
    }

    lastComputedType = context.createPointerType(inner);
  } else if (op == TokenKind::Star) {
    if (operandType->toString().find("lock") != std::string::npos) {
      std::string name = "";
      if (auto id = llvm::dyn_cast<const IdentifierExpr>(expr->getOperand()))
        name = id->getName();

      if (activeLocks.find(name) == activeLocks.end()) {
        Diags.report(expr->getLoc(), DiagID::err_invalid_access)
            << "Cannot dereference 'lock' pointer '" << name
            << "' outside a 'lock' block";
        hasError = true;
      }
    }
    // Dereference: T* -> T
    const Type *raw = unwrapConcurrency(operandType);
    if (auto ptr = llvm::dyn_cast<const PointerType>(raw)) {
      lastComputedType = ptr->getPointee();
    } else {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Cannot dereference non-pointer type '" << operandType->toString()
          << "'";
      hasError = true;
      lastComputedType = context.getAnyType();
    }
  } else {
    lastComputedType = operandType;
  }
  setAndReturn();
}

void TypeChecker::visitCallExpr(const CallExpr *expr) {
  auto setAndReturn = [&]() {
    const_cast<CallExpr *>(expr)->setType(lastComputedType);
  };

  // 1. Functional Casts (e.g., int(x))
  if (auto idExpr = llvm::dyn_cast<const IdentifierExpr>(expr->getCallee())) {
    // Ambiguous Import Check for Function Calls
    if (ambiguousImports.count(idExpr->getName()) &&
        ambiguousImports[idExpr->getName()].size() > 1) {
      const auto &sources = ambiguousImports[idExpr->getName()];
      Diags.report(expr->getLoc(), DiagID::err_ambiguous_reference)
          << "Ambiguous reference to '" << idExpr->getName() << "'. "
          << "Did you mean '" << sources[0] << "." << idExpr->getName()
          << "' or '" << sources[1] << "." << idExpr->getName() << "'?";
      lastComputedType = context.getAnyType();
      hasError = true;
      return;
    }

    Symbol *sym = symbols.lookup(idExpr->getName());
    if (sym && sym->kind == SymbolKind::Type) {

      // Declare targetType outside the if/else block so it stays in scope!
      const Type *targetType = sym->type;
      if (!targetType) {
        if (sym->name == "int" || sym->name == "i32")
          targetType = context.getI32Type();
        else if (sym->name == "short" || sym->name == "i16")
          targetType = context.getI16Type();
        else if (sym->name == "long" || sym->name == "i64")
          targetType = context.getI64Type();
        else if (sym->name == "isize")
          targetType = context.getISizeType();
        else if (sym->name == "usize")
          targetType = context.getUSizeType();
        else if (sym->name == "float" || sym->name == "f32")
          targetType = context.getF32Type();
        else if (sym->name == "double" || sym->name == "f64")
          targetType = context.getF64Type();
        else if (sym->name == "half" || sym->name == "f16")
          targetType = context.getF16Type();
        else if (sym->name == "quarter" || sym->name == "f8")
          targetType = context.getF8Type();
        else if (sym->name == "char")
          targetType = context.getCharType();
        else if (sym->name == "string")
          targetType = context.getStringType();
        else if (sym->name == "boolean" || sym->name == "bool")
          targetType = context.getBoolType();
        else
          targetType = context.getAnyType();
      }

      if (expr->getArgs().size() != 1) {
        Diags.report(expr->getLoc(), DiagID::err_argument_count_mismatch);
        hasError = true;
      } else {
        expr->getArgs()[0]->accept(*this);
        if (!isCastAllowed(lastComputedType, targetType)) {
          Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
              << "Invalid functional cast";
          hasError = true;
        }
      }

      lastComputedType = targetType;
      return;
    }
  }

  // 2. Super() calls
  if (llvm::dyn_cast<const SuperExpr>(expr->getCallee())) {
    for (auto &arg : expr->getArgs())
      arg->accept(*this);
    lastComputedType = context.getVoidType();
    return;
  }

  const Type *outerExpectedRet = currentExpectedReturnType;
  currentExpectedReturnType = nullptr;

  // PRE-EVALUATE ARGUMENTS (Crucial for overload resolution)
  std::vector<const Type *> argTypes;
  for (const auto &arg : expr->getArgs()) {
    arg->accept(*this);
    argTypes.push_back(lastComputedType);
  }

  currentExpectedReturnType = outerExpectedRet;

  // Helper for Argument Compatibility with Lock Elevation
  auto isArgCompatible = [&](const Type *expected, const Type *actual,
                             const Expr *argExpr) {
    if (isCompatible(expected, actual))
      return true;

    // Support Lock Elevation during function calls
    if (auto id = llvm::dyn_cast<const IdentifierExpr>(argExpr)) {
      if (hasLock(actual) && activeLocks.count(id->getName()) &&
          hasMutOrLock(expected)) {
        return true;
      }
    }
    return false;
  };

  // 3. Member Function Overload Resolution (e.g., p.printData(42))
  if (auto memExpr = llvm::dyn_cast<const MemberExpr>(expr->getCallee())) {
    memExpr->getObject()->accept(*this);
    const Type *objType = lastComputedType;

    if (auto namedType = llvm::dyn_cast<const NamedType>(objType)) {
      const ClassDecl *cls = context.lookupClass(namedType->getName());
      if (cls) {
        // Setup generic substitutions if the class is generic (e.g.
        // Box<string>)
        llvm::StringMap<const Type *> substitutions;
        if (!namedType->getGenericArgs().empty()) {
          Symbol *sym = symbols.lookup(namedType->getName());
          if (sym && sym->decl &&
              sym->decl->getKind() == StmtKind::GenericDecl) {
            auto gd = static_cast<const GenericDecl *>(sym->decl);
            const auto &typeParams = gd->getTypeParams();
            const auto &genArgs = namedType->getGenericArgs();
            size_t limit = std::min(typeParams.size(), genArgs.size());
            for (size_t i = 0; i < limit; ++i)
              substitutions[typeParams[i].name] = genArgs[i].type.get();
          }
        }

        const FunctionDecl *bestMatch = nullptr;
        GenericResolver::ConcreteSignature bestSig;
        std::vector<const ClassDecl *> queue = {cls};
        size_t head = 0;

        // Scan the class and its parents for the matching overload using BFS
        while (head < queue.size() && !bestMatch) {
          const ClassDecl *current = queue[head++];
          for (const auto &member : current->getMembers()) {
            if (auto fn = llvm::dyn_cast<const FunctionDecl>(member.get())) {
              if (fn->getName() == memExpr->getName()) {
                size_t minRequiredArgs = 0;
                for (const auto &p : fn->getParams()) {
                  if (!p.defaultValue)
                    minRequiredArgs++;
                }
                if (!fn->isVariadicFunc() &&
                    (argTypes.size() < minRequiredArgs ||
                     argTypes.size() > fn->getParams().size())) {
                  continue;
                }

                auto sig = resolver.resolveFunctionSignature(fn, substitutions);
                bool match = true;
                size_t limit = std::min(argTypes.size(), sig.paramTypes.size());
                for (size_t i = 0; i < limit; ++i) {
                  if (!isArgCompatible(sig.paramTypes[i].get(), argTypes[i],
                                       expr->getArgs()[i].get())) {
                    match = false;
                    break;
                  }
                }

                if (match) { // We found the exact overload!
                  bestMatch = fn;
                  bestSig = std::move(sig);
                  break;
                }
              }
            }
          }
          if (!bestMatch) {
            for (const auto &pName : current->getParentNames()) {
              if (auto pCls = context.lookupClass(pName))
                queue.push_back(pCls);
            }
          }
        }

        if (bestMatch) {
          std::vector<const Type *> pTypes;
          for (auto &p : bestSig.paramTypes)
            pTypes.push_back(p.get());
          if (bestMatch->isVirtualFunc()) {
            const_cast<MemberExpr *>(memExpr)->setVirtualMethodInfo(
                true, bestMatch->getVTableIndex());
          } else {
            const_cast<MemberExpr *>(memExpr)->setVirtualMethodInfo(false, 0);
          }
          auto *mutExpr = const_cast<CallExpr *>(expr);
          if (mutExpr->getArgs().size() < pTypes.size()) {
            for (size_t i = mutExpr->getArgs().size(); i < pTypes.size(); ++i) {
              if (bestMatch->getParams()[i].defaultValue) {
                mutExpr->getArgsMut().push_back(
                    bestMatch->getParams()[i].defaultValue->clone());
                argTypes.push_back(pTypes[i]); // Keep synchronized
              }
            }
          }
          const_cast<MemberExpr *>(memExpr)->setType(
              context.createFunctionType(pTypes, bestSig.returnType.get()));
          const_cast<CallExpr *>(expr)->setType(bestSig.returnType.get());
          if (bestSig.returnType) {
            lastComputedType = bestSig.returnType.get();
            parkedTypes.push_back(std::move(bestSig.returnType));
          } else {
            lastComputedType = context.getAnyType();
          }
          return;
        } else {
          // Check if the member is actually a variable of type 'any'
          memExpr->accept(*this);
          if (lastComputedType->is<AnyType>()) {
            // Do nothing here. Allow it to fall through to Part 5!
          } else {
            Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
                << "No matching overload for '" << memExpr->getName() << "'";
            hasError = true;
            lastComputedType = context.getAnyType();
            return;
          }
        }
      }
    }
  }

  // 4. Global Function Overload Resolution
  if (auto idExpr = llvm::dyn_cast<const IdentifierExpr>(expr->getCallee())) {
    std::string funcName = idExpr->getName();

    if (funcName.find("atomic_") == 0) {
      // 1. Validate Memory Ordering Logic
      if (funcName.find("atomic_cas") == 0) {
        if (expr->getArgs().size() >= 5) {
          auto succStr =
              llvm::dyn_cast<const StringLiteral>(expr->getArgs()[3].get());
          auto failStr =
              llvm::dyn_cast<const StringLiteral>(expr->getArgs()[4].get());
          if (succStr && failStr) {
            auto getOrder = [](const std::string &s) {
              if (s == "relaxed")
                return 0;
              if (s == "consume")
                return 1;
              if (s == "acquire")
                return 2;
              if (s == "release")
                return 3;
              if (s == "acq_rel")
                return 4;
              if (s == "seq_cst")
                return 5;
              return -1;
            };
            int sOrd = getOrder(succStr->getValue());
            int fOrd = getOrder(failStr->getValue());

            if (fOrd > sOrd) {
              Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
                  << "Invalid atomic ordering: Failure ordering ('"
                  << failStr->getValue()
                  << "') cannot be stronger than success ordering ('"
                  << succStr->getValue() << "')";
              hasError = true;
            }
          }
        }
      }

      // 2. Validate Hardware Memory Alignment
      if (!expr->getArgs().empty()) {
        if (auto unExpr =
                llvm::dyn_cast<const UnaryExpr>(expr->getArgs()[0].get())) {
          if (unExpr->getOp() == TokenKind::Amp) {
            if (auto id = llvm::dyn_cast<const IdentifierExpr>(
                    unExpr->getOperand())) {
              Symbol *sym = symbols.lookup(id->getName());
              if (sym && sym->decl &&
                  sym->decl->getKind() == StmtKind::VariableDecl) {
                auto varDecl = static_cast<const VariableDecl *>(sym->decl);
                if (varDecl->getAlignment() > 0 &&
                    varDecl->getAlignment() < 4) {
                  Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
                      << "Misaligned atomic access: Variable '"
                      << varDecl->getName() << "' has explicit alignment "
                      << varDecl->getAlignment()
                      << " (minimum 4 required for atomics)";
                  hasError = true;
                }
              }
            }
          }
        }
      }

      // 3. Resolve Intrinsic Return Type
      if (funcName == "atomic_load" && !argTypes.empty()) {
        if (auto ptrT = llvm::dyn_cast<const PointerType>(
                unwrapConcurrency(argTypes[0]))) {
          lastComputedType = ptrT->getPointee();
        } else {
          lastComputedType = context.getAnyType();
        }
      } else if (funcName.find("atomic_cas") == 0 || funcName == "atomic_add") {
        if (!argTypes.empty()) {
          if (auto ptrT = llvm::dyn_cast<const PointerType>(
                  unwrapConcurrency(argTypes[0]))) {
            lastComputedType = ptrT->getPointee();
          } else {
            lastComputedType = context.getAnyType();
          }
        } else {
          lastComputedType = context.getAnyType();
        }
      } else {
        lastComputedType = context.getVoidType();
      }
      return;
    }

    Symbol *sym = symbols.lookup(idExpr->getName());

    // Intercept variables to bypass overload resolution
    if (sym && sym->kind == SymbolKind::Variable) {
      // Do nothing. Allow it to fall through to Part 5!
    } else if (sym && sym->decl) {
      // Collect all candidate overloads (the main decl + any overloaded decls)
      std::vector<const Decl *> candidates;
      candidates.push_back(sym->decl);
      for (const auto &o : sym->overloads) {
        if (o.decl)
          candidates.push_back(o.decl);
      }

      // Structure to hold valid matches
      struct OverloadMatch {
        const Type *returnType;
        GenericResolver::ConcreteSignature sig;
      };
      std::vector<OverloadMatch> validMatches;

      for (const Decl *candidate : candidates) {
        // --- A. Check if candidate is a Generic Function ---
        if (candidate->getKind() == StmtKind::GenericDecl) {
          auto genericDecl = static_cast<const GenericDecl *>(candidate);
          auto innerFunc =
              llvm::dyn_cast<const FunctionDecl>(genericDecl->getInnerDecl());
          if (!innerFunc)
            continue;

          llvm::StringMap<const Type *> substitutions;
          const auto &typeParams = genericDecl->getTypeParams();

          // Infer types (T = string, T = i32, etc.)
          auto inferTypes = [&](const Type *expected, const Type *actual,
                                auto &self) -> void {
            if (!expected || !actual)
              return;
            if (auto named = llvm::dyn_cast<const NamedType>(expected)) {
              auto it = std::find_if(typeParams.begin(), typeParams.end(),
                                     [&](const GenericDecl::GenericParam &p) {
                                       return p.name == named->getName();
                                     });
              if (it != typeParams.end()) {
                if (!substitutions.count(named->getName()))
                  substitutions[named->getName()] = actual;
                return;
              }
            }
            if (auto expPtr = llvm::dyn_cast<const PointerType>(expected)) {
              if (auto actPtr = llvm::dyn_cast<const PointerType>(actual)) {
                self(expPtr->getPointee(), actPtr->getPointee(), self);
              }
            }
            if (auto expArr = llvm::dyn_cast<const ArrayType>(expected)) {
              if (auto actArr = llvm::dyn_cast<const ArrayType>(actual))
                self(expArr->getElementType(), actArr->getElementType(), self);
            } else if (auto expRef =
                           llvm::dyn_cast<const ReferenceType>(expected)) {
              if (auto actRef = llvm::dyn_cast<const ReferenceType>(actual))
                self(expRef->getInner(), actRef->getInner(), self);
              else
                self(expRef->getInner(), actual, self);
            }

            if (auto expFunc = llvm::dyn_cast<const FunctionType>(expected)) {
              if (auto actFunc = llvm::dyn_cast<const FunctionType>(actual)) {
                self(expFunc->getReturnType(), actFunc->getReturnType(), self);
                size_t pLimit = std::min(expFunc->getParamTypes().size(),
                                         actFunc->getParamTypes().size());
                for (size_t j = 0; j < pLimit; ++j) {
                  self(expFunc->getParamTypes()[j].get(),
                       actFunc->getParamTypes()[j].get(), self);
                }
              }
            }
          };

          size_t limit =
              std::min(argTypes.size(), innerFunc->getParams().size());
          for (size_t i = 0; i < limit; ++i)
            inferTypes(innerFunc->getParams()[i].type.get(), argTypes[i],
                       inferTypes);

          GenericResolver::ConcreteSignature sig =
              resolver.resolveFunctionSignature(innerFunc, substitutions);

          size_t minRequiredArgs = 0;
          for (const auto &p : innerFunc->getParams()) {
            if (!p.defaultValue)
              minRequiredArgs++;
          }
          if (argTypes.size() < minRequiredArgs ||
              argTypes.size() > sig.paramTypes.size()) {
            continue;
          }

          bool match = true;
          for (size_t i = 0; i < sig.paramTypes.size(); ++i) {
            if (!isArgCompatible(sig.paramTypes[i].get(), argTypes[i],
                                 expr->getArgs()[i].get())) {
              match = false; // Inference failed
              break;
            }
          }

          if (match) {
            // Store the match instead of breaking!
            validMatches.push_back({sig.returnType.get(), std::move(sig)});
          }
        }
        // --- B. Check if candidate is a Standard Function ---
        else if (candidate->getKind() == StmtKind::FunctionDecl) {
          auto fn = static_cast<const FunctionDecl *>(candidate);

          if (!fn->isVariadicFunc()) {
            size_t minRequiredArgs = 0;
            for (const auto &p : fn->getParams()) {
              if (!p.defaultValue)
                minRequiredArgs++;
            }
            if (argTypes.size() < minRequiredArgs ||
                argTypes.size() > fn->getParams().size()) {
              continue; // Arity truly doesn't match
            }
          }

          bool match = true;
          size_t limit = std::min(argTypes.size(), fn->getParams().size());
          for (size_t i = 0; i < limit; ++i) {
            if (!isArgCompatible(fn->getParams()[i].type.get(), argTypes[i],
                                 expr->getArgs()[i].get())) {
              match = false; // Argument type mismatch
              break;
            }
          }

          if (match) {
            auto sig = resolver.resolveFunctionSignature(fn, {});
            validMatches.push_back({fn->getReturnType(), std::move(sig)});
          }
        }
      }

      // Evaluate the collected matches
      if (validMatches.size() > 1) {
        Diags.report(expr->getLoc(), DiagID::err_ambiguous_reference)
            << "Ambiguous call to overloaded function '" << idExpr->getName()
            << "'. Multiple signatures are compatible with the provided "
               "arguments.";
        hasError = true;
        lastComputedType = context.getAnyType();
        return;
      } else if (validMatches.size() == 1) {
        auto *mutExpr = const_cast<CallExpr *>(expr);
        auto fnDecl =
            static_cast<const FunctionDecl *>(validMatches[0].sig.decl);

        std::vector<const Type *> pTypes;
        for (auto &p : validMatches[0].sig.paramTypes)
          pTypes.push_back(p.get());

        if (mutExpr->getArgs().size() < pTypes.size()) {
          for (size_t i = mutExpr->getArgs().size(); i < pTypes.size(); ++i) {
            if (fnDecl->getParams()[i].defaultValue) {
              mutExpr->getArgsMut().push_back(
                  fnDecl->getParams()[i].defaultValue->clone());
              argTypes.push_back(pTypes[i]); // Keep synchronized
            }
          }
        }

        for (size_t i = 0; i < mutExpr->getArgs().size(); ++i) {
          if (i < pTypes.size()) {
            const Type *targetPType = unwrapConcurrency(pTypes[i]);
            const Type *targetArgType = unwrapConcurrency(argTypes[i]);

            if (!targetArgType->isEquivalent(*targetPType)) {
              auto cast = std::make_unique<CastExpr>(
                  targetPType->clone(), std::move(mutExpr->getArgsMut()[i]),
                  expr->getLoc());
              cast->setType(targetPType);
              mutExpr->getArgsMut()[i] = std::move(cast);
            }
          }
        }

        lastComputedType = validMatches[0].returnType;
        const_cast<IdentifierExpr *>(idExpr)->setType(
            context.createFunctionType(pTypes, lastComputedType));
        const_cast<CallExpr *>(expr)->setType(lastComputedType);
        if (validMatches[0].sig.returnType) {
          parkedTypes.push_back(std::move(validMatches[0].sig.returnType));
        }
        return;
      } else {
        Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
            << "No matching overload for '" << idExpr->getName()
            << "' with provided arguments";
        hasError = true;
        lastComputedType = context.getAnyType();
        return;
      }
    }
  }

  // 5. Fallback for Closures / Function Pointers
  expr->getCallee()->accept(*this);
  const Type *calleeType = lastComputedType;

  const std::vector<TypePtr> *params = nullptr;
  const Type *retType = nullptr;
  bool isVariadic = false;

  if (auto *fnType = llvm::dyn_cast<const FunctionType>(calleeType)) {
    params = &fnType->getParamTypes();
    retType = fnType->getReturnType();
    isVariadic = fnType->isVariadicFunc();
  } else if (auto *closType = llvm::dyn_cast<const ClosureType>(calleeType)) {
    params = &closType->getParamTypes();
    retType = closType->getReturnType();
    isVariadic = false;
  }

  if (params) {
    size_t maxArgs = params->size();
    size_t minRequiredArgs = maxArgs;

    // [FIX 2] Trace closures back to their AST definitions to find default
    // arguments!
    const LambdaExpr *lambdaDef = nullptr;
    if (auto *ident = llvm::dyn_cast<const IdentifierExpr>(expr->getCallee())) {
      Symbol *sym = symbols.lookup(ident->getName());
      if (sym && sym->decl && sym->decl->getKind() == StmtKind::VariableDecl) {
        auto *varDecl = static_cast<const VariableDecl *>(sym->decl);
        if (varDecl->getInitializer() &&
            varDecl->getInitializer()->getKind() == ExprKind::LambdaExpr) {
          lambdaDef =
              static_cast<const LambdaExpr *>(varDecl->getInitializer());
        }
      }
    } else if (auto *inlineLambda =
                   llvm::dyn_cast<const LambdaExpr>(expr->getCallee())) {
      lambdaDef = inlineLambda;
    }

    // Calculate required arguments
    if (lambdaDef) {
      minRequiredArgs = 0;
      for (const auto &p : lambdaDef->getParams()) {
        if (!p.getDefaultValue())
          minRequiredArgs++;
      }
    }

    // Throw error ONLY if they didn't provide enough required args
    if (!isVariadic &&
        (argTypes.size() < minRequiredArgs || argTypes.size() > maxArgs)) {
      Diags.report(expr->getLoc(), DiagID::err_argument_count_mismatch);
      hasError = true;
    }

    auto *mutExpr = const_cast<CallExpr *>(expr);

    // Inject default arguments into the CallExpr if they were omitted
    if (lambdaDef && mutExpr->getArgs().size() < maxArgs) {
      for (size_t i = mutExpr->getArgs().size(); i < maxArgs; ++i) {
        if (lambdaDef->getParams()[i].getDefaultValue()) {
          mutExpr->getArgsMut().push_back(
              lambdaDef->getParams()[i].getDefaultValue()->clone());
          argTypes.push_back(
              (*params)[i].get()); // Keep synchronized for the loop below
        }
      }
    }

    size_t limit = std::min(argTypes.size(), params->size());
    for (size_t i = 0; i < limit; ++i) {
      if (!isArgCompatible((*params)[i].get(), argTypes[i],
                           expr->getArgs()[i].get())) {
        Diags.report(expr->getArgs()[i]->getLoc(), DiagID::err_type_mismatch)
            << "Argument type mismatch";
        hasError = true;
      } else {
        const Type *targetPType = unwrapConcurrency((*params)[i].get());
        const Type *targetArgType = unwrapConcurrency(argTypes[i]);

        if (!targetArgType->isEquivalent(*targetPType)) {
          auto cast = std::make_unique<CastExpr>(
              targetPType->clone(), std::move(mutExpr->getArgsMut()[i]),
              expr->getLoc());
          cast->setType(targetPType);
          mutExpr->getArgsMut()[i] = std::move(cast);
        }
      }
    }
    lastComputedType = retType;
  } else if (calleeType->is<AnyType>()) {
    lastComputedType = context.getAnyType();
  } else {
    Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
        << "Called object is not a function or closure";
    hasError = true;
    lastComputedType = context.getAnyType();
  }
  setAndReturn();
}

void TypeChecker::visitIndexExpr(const IndexExpr *expr) {
  expr->getArray()->accept(*this);
  const Type *arrType = lastComputedType;

  bool oldLhs = isLHSOfAssignment;
  isLHSOfAssignment = false;
  expr->getIndex()->accept(*this);
  isLHSOfAssignment = oldLhs;

  const Type *idxType = lastComputedType;

  if (auto *at = llvm::dyn_cast<const ArrayType>(arrType)) {
    if (!isIntegerOrChar(idxType)) {
      Diags.report(expr->getIndex()->getLoc(), DiagID::err_type_mismatch)
          << "Array index must be integer";
    }
    lastComputedType = at->getElementType();
  } else if (auto *st = llvm::dyn_cast<const SliceType>(arrType)) {
    if (!isIntegerOrChar(idxType)) {
      Diags.report(expr->getIndex()->getLoc(), DiagID::err_type_mismatch)
          << "Slice index must be integer";
    }
    lastComputedType = st->getElementType();
  } else if (auto *mt = llvm::dyn_cast<const MapType>(arrType)) {
    if (!isCompatible(mt->getKeyType(), idxType)) {
      Diags.report(expr->getIndex()->getLoc(), DiagID::err_type_mismatch)
          << "Map key type mismatch";
    }
    lastComputedType = mt->getValueType();
  } else if (arrType->isString()) {
    if (!isIntegerOrChar(idxType)) {
      Diags.report(expr->getIndex()->getLoc(), DiagID::err_type_mismatch)
          << "String index must be integer";
    }
    lastComputedType = context.getCharType();
  } else if (arrType->is<AnyType>()) {
    lastComputedType = context.getAnyType();
  } else {
    Diags.report(expr->getArray()->getLoc(), DiagID::err_type_mismatch)
        << "Type is not indexable";
    lastComputedType = context.getAnyType();
  }
  const_cast<IndexExpr *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitMemberExpr(const MemberExpr *expr) {
  // --- NEW: Handle Module Namespace Access ---
  if (auto *idObj = llvm::dyn_cast<const IdentifierExpr>(expr->getObject())) {
    Symbol *sym = symbols.lookup(idObj->getName());
    if (sym && sym->kind == SymbolKind::Module) {
      std::string fqName = idObj->getName() + "." + expr->getName();
      Symbol *targetSym = symbols.lookup(fqName);

      if (!targetSym) {
        // Fallback to the short name if aliased
        targetSym = symbols.lookup(expr->getName());
      }

      if (targetSym) {
        lastComputedType = targetSym->type;
      } else {
        // Opaque C-FFI / intrinsic module member fallback
        lastComputedType = context.getAnyType();
      }

      const_cast<MemberExpr *>(expr)->setType(lastComputedType);
      return; // Successfully resolved the namespace access!
    }
  }

  // 1. Typecheck the base object (e.g., deeplyNested.val)
  expr->getObject()->accept(*this);

  // Use lastComputedType directly. Do not rely on the AST node's type here,
  // as lastComputedType holds dynamically inferred generics correctly.
  const Type *objType = lastComputedType;
  if (!objType)
    return;

  // 2. Peel off pointers, references, and concurrency wrappers
  // so we can access fields on heap-allocated objects (e.g. `p->val` or
  // `(*p).val`)
  const Type *rawType = unwrapConcurrency(objType);

  if (auto *nullTy = llvm::dyn_cast<NullableType>(rawType)) {
    if (!expr->isOptionalAccess()) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Cannot access member on nullable type '" << objType->toString()
          << "'. Use '?.' instead.";
      hasError = true;
      lastComputedType = context.getAnyType();
      return;
    }
    // Unwrap the nullable layer and any potential inner concurrency wrappers
    rawType = nullTy->getInner();
    rawType = unwrapConcurrency(rawType);
  }

  if (auto *ptrTy = llvm::dyn_cast<PointerType>(rawType)) {
    rawType = ptrTy->getPointee();
  } else if (auto *refTy = llvm::dyn_cast<ReferenceType>(rawType)) {
    rawType = refTy->getInner();
  }

  // 3. Now verify the base object is actually a Struct/Class
  if (auto *namedObj = llvm::dyn_cast<NamedType>(rawType)) {

    // Resolve the mangled name if this is a generic instance
    std::string lookupName = namedObj->getName();
    if (!namedObj->getGenericArgs().empty()) {
      std::vector<const Type *> rawArgs;
      for (auto &arg : namedObj->getGenericArgs())
        rawArgs.push_back(arg.type.get());
      lookupName = resolver.getMangledName(lookupName, rawArgs);
    }

    // 3. Look up the instantiated CONCRETE class
    const ClassDecl *classDecl = context.lookupClass(lookupName);
    if (!classDecl) {
      Symbol *sym = symbols.lookup(lookupName);
      if (sym && sym->decl && sym->decl->getKind() == StmtKind::EnumDecl) {
        auto *enumDecl = static_cast<const EnumDecl *>(sym->decl);
        uint32_t index = 0;
        bool found = false;

        for (const auto &enumCase : enumDecl->getCases()) {
          if (enumCase.name == expr->getName()) {
            auto *mutExpr = const_cast<MemberExpr *>(expr);
            mutExpr->setType(objType); // The type is the Enum itself
            mutExpr->setLayoutInfo(index);
            lastComputedType = objType;
            found = true;
            break;
          }
          index++;
        }

        if (!found) {
          Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
              << "Enum member '" << expr->getName() << "' not found in '"
              << lookupName << "'";
          hasError = true;
          lastComputedType = context.getAnyType();
        }
        return;
      }

      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Cannot resolve member access on incomplete type: " << lookupName;
      hasError = true;
      return;
    }

    // Find the member's physical layout index, searching parent classes if
    // necessary
    bool found = false;
    std::vector<const ClassDecl *> queue = {classDecl};
    size_t head = 0;

    while (head < queue.size() && !found) {
      const ClassDecl *current = queue[head++];

      for (const auto &member : current->getMembers()) {
        if (auto *varDecl = llvm::dyn_cast<VariableDecl>(member.get())) {
          if (varDecl->getName() == expr->getName()) {

            // --- NEW: Enforce Access Modifiers ---
            if (!checkVisibility(varDecl, current, expr->getLoc())) {
              hasError = true;
              lastComputedType = context.getAnyType();
              return; // Stop processing, error already reported
            }

            // Apply the concrete type to the AST node
            auto *mutExpr = const_cast<MemberExpr *>(expr);
            mutExpr->setType(varDecl->getType());

            // Pull the actual physical layout data from the defining class
            mutExpr->setLayoutInfo(
                varDecl->getPhysicalIndex(), varDecl->isBitfield(),
                varDecl->getBitWidth(), varDecl->getBitOffset());

            // Update the global tracker!
            lastComputedType = varDecl->getType();

            found = true;
            break;
          }
        }
      }

      // If not found in this class, queue up its parents for the next iteration
      if (!found) {
        for (const auto &pName : current->getParentNames()) {
          if (auto pCls = context.lookupClass(pName)) {
            queue.push_back(pCls);
          }
        }
      }
    }

    if (!found) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Member '" << expr->getName() << "' not found in '" << lookupName
          << "'";
      hasError = true;
      lastComputedType = context.getAnyType();
    }
  } else {
    // It's not a class or struct (e.g. trying to do `5.key`)
    Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
        << "Cannot access member '" << expr->getName()
        << "' on non-object type '" << objType->toString() << "'";
    hasError = true;
    lastComputedType = context.getAnyType();
  }
}

void TypeChecker::visitCastExpr(const CastExpr *expr) {
  if (expr->getTargetType())
    expr->getTargetType()->accept(*this);

  expr->getExpr()->accept(*this);
  const Type *srcType = lastComputedType;

  if (!isCastAllowed(srcType, expr->getTargetType())) {
    Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
        << "Invalid cast from " << srcType->toString() << " to "
        << expr->getTargetType()->toString();
    hasError = true;
  }
  lastComputedType = expr->getTargetType();
  const_cast<CastExpr *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitTernaryExpr(const TernaryExpr *expr) {
  expr->getCondition()->accept(*this);
  if (!lastComputedType->isBool()) {
    if (lastComputedType->isNumeric() || lastComputedType->is<PointerType>()) {
      Diags.report(expr->getCondition()->getLoc(),
                   DiagID::warn_implicit_bool_conv);
    } else {
      Diags.report(expr->getCondition()->getLoc(), DiagID::err_type_mismatch)
          << "Ternary condition must be bool or numeric";
      hasError = true;
    }
  }
  expr->getTrueBranch()->accept(*this);
  const Type *trueType = lastComputedType;
  expr->getFalseBranch()->accept(*this);

  lastComputedType = getCommonSuperType(trueType, lastComputedType);
  const_cast<TernaryExpr *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitNewExpr(const NewExpr *expr) {
  // This call triggers visitNamedType, which mangles the type in-place!
  if (expr->getType())
    expr->getType()->accept(*this);

  const Type *type = expr->getType(); // Now holds the mangled NamedType

  auto setAndReturn = [&]() {
    const_cast<NewExpr *>(expr)->setType(lastComputedType);
  };

  if (auto named = llvm::dyn_cast<const NamedType>(type)) {
    // named->getName() is now the concrete mangled name (e.g., Map_i32_string)
    const ClassDecl *cls = context.lookupClass(named->getName());

    if (!cls) {
      Diags.report(expr->getLoc(), DiagID::err_unknown_type)
          << named->getName();
      lastComputedType = context.getAnyType();
      return;
    }

    // Check for constructor on the CONCRETE class
    const FunctionDecl *ctor = nullptr;
    const ClassDecl *current = cls;
    while (current && !ctor) {
      for (const auto &member : current->getMembers()) {
        if (auto fn = llvm::dyn_cast<const FunctionDecl>(member.get())) {
          if (fn->getName() == "constructor") {
            if (checkVisibility(fn, current, expr->getLoc())) {
              ctor = fn;
            }
            break;
          }
        }
      }
      current = current->getParentNames().empty()
                    ? nullptr
                    : context.lookupClass(current->getParentNames()[0]);
    }

    if (ctor) {
      if (expr->getArgs().size() != ctor->getParams().size()) {
        Diags.report(expr->getLoc(), DiagID::err_argument_count_mismatch);
      } else {
        // Constructor is already monomorphized, no substitutions needed!
        auto sig = resolver.resolveFunctionSignature(ctor, {});
        auto *mutExpr = const_cast<NewExpr *>(expr);

        for (size_t i = 0; i < expr->getArgs().size(); ++i) {
          expr->getArgs()[i]->accept(*this);
          if (!isCompatible(sig.paramTypes[i].get(), lastComputedType)) {
            Diags.report(expr->getArgs()[i]->getLoc(),
                         DiagID::err_type_mismatch)
                << "Constructor argument mismatch";
          } else {
            const Type *targetPType =
                unwrapConcurrency(sig.paramTypes[i].get());
            const Type *targetArgType = unwrapConcurrency(lastComputedType);

            if (!targetArgType->isEquivalent(*targetPType)) {
              auto cast = std::make_unique<CastExpr>(
                  targetPType->clone(), std::move(mutExpr->getArgsMut()[i]),
                  expr->getLoc());
              cast->setType(targetPType);
              mutExpr->getArgsMut()[i] = std::move(cast);
            }
          }
        }
      }
    }

    // 1. Read the Reference Semantics flag from the ClassDecl
    bool isRef = cls->isReferenceType();

    // 2. Clone the base NamedType and inject the isRef flag so the backend sees
    // it
    std::unique_ptr<Type> clonedType = named->clone();
    auto *clonedNamed = static_cast<NamedType *>(clonedType.get());
    clonedNamed->setRefClass(isRef);
    const Type *allocatedType = context.saveType(std::move(clonedType));

    // 3. Branch the resulting AST expression type
    if (isRef) {
      lastComputedType = context.createPointerType(allocatedType);
    } else {
      lastComputedType = allocatedType;
    }

    setAndReturn();
    return;
  }

  lastComputedType = expr->getType();
  setAndReturn();
}

void TypeChecker::visitLambdaExpr(const LambdaExpr *expr) {
  std::vector<const Type *> paramTypes;
  for (const auto &p : expr->getParams()) {
    paramTypes.push_back(p.getType());
  }

  symbols.enterScope(ScopeKind::Function);
  for (const auto &p : expr->getParams()) {
    symbols.addSymbol(p.getName(),
                      Symbol(SymbolKind::Variable, p.getName(), p.getType()),
                      expr->getLoc());
  }

  const Type *prevRet = currentExpectedReturnType;

  const Type *expectedForThisLambda = context.getAnyType();
  if (prevRet) {
    if (auto fnTy = llvm::dyn_cast<const FunctionType>(prevRet)) {
      expectedForThisLambda = fnTy->getReturnType();
    } else if (auto clTy = llvm::dyn_cast<const ClosureType>(prevRet)) {
      expectedForThisLambda = clTy->getReturnType();
    } else if (!prevRet->isVoid() && !prevRet->is<AnyType>()) {
      expectedForThisLambda = prevRet;
    }
  }
  currentExpectedReturnType = expectedForThisLambda;

  for (const auto &p : expr->getParams()) {
    if (p.getDefaultValue()) {
      p.getDefaultValue()->accept(*this);
      if (!isCompatible(p.getType(), lastComputedType)) {
        Diags.report(p.getDefaultValue()->getLoc(), DiagID::err_type_mismatch)
            << "Default value type does not match lambda parameter type";
        hasError = true;
      }
    }
  }

  const Type *inferredReturnType = context.getVoidType();
  if (expr->getBody()) {
    expr->getBody()->accept(*this);

    // Extract the actual return type from the LHS hint if it's a
    // function/closure
    if (prevRet && !prevRet->is<AnyType>()) {
      if (auto fnT = llvm::dyn_cast<const FunctionType>(prevRet)) {
        inferredReturnType = fnT->getReturnType();
      } else if (auto clT = llvm::dyn_cast<const ClosureType>(prevRet)) {
        inferredReturnType = clT->getReturnType();
      } else {
        inferredReturnType = prevRet;
      }
    } else if (lastComputedType && !lastComputedType->is<AnyType>()) {
      inferredReturnType = lastComputedType;
    }
  }

  symbols.exitScope();
  currentExpectedReturnType = prevRet;

  // 3. Bake the inferred return type into the lambda's FunctionType signature
  lastComputedType = context.createFunctionType(paramTypes, inferredReturnType);
  const_cast<LambdaExpr *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitTemplateStringExpr(const TemplateStringExpr *expr) {
  for (const auto &part : expr->getParts()) {
    part->accept(*this);
    if ((lastComputedType->is<PrimitiveType>() &&
         ((const PrimitiveType *)lastComputedType)->getScalar() ==
             PrimitiveType::Scalar::Void) ||
        lastComputedType->is<NullType>()) {
      // [FIX] Fixed parentheses
    }
  }
  lastComputedType = context.getStringType();
  const_cast<TemplateStringExpr *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitThreadExpr(const ThreadExpr *expr) {
  expr->getBody()->accept(*this);
  lastComputedType = context.createNamedType("Thread");
  const_cast<ThreadExpr *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitThisExpr(const ThisExpr *expr) {
  if (!currentClassDecl) {
    Diags.report(expr->getLoc(), DiagID::err_invalid_this)
        << "'this' used outside of class context";
    lastComputedType = context.getAnyType();
    return;
  }
  if (inStaticContext) {
    Diags.report(expr->getLoc(), DiagID::err_invalid_this)
        << "'this' cannot be used in static context";
  }
  lastComputedType = context.createNamedType(currentClassDecl->getName());
  const_cast<ThisExpr *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitSuperExpr(const SuperExpr *expr) {
  if (!currentClassDecl || currentClassDecl->getParentNames().empty()) {
    Diags.report(expr->getLoc(), DiagID::err_invalid_super)
        << "'super' used in class with no parent";
    lastComputedType = context.getAnyType();
    return;
  }
  // Default to the first parent for 'super'
  lastComputedType =
      context.createNamedType(currentClassDecl->getParentNames()[0]);
  const_cast<SuperExpr *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitAwaitExpr(const AwaitExpr *expr) {
  expr->getExpr()->accept(*this);
  const Type *type = lastComputedType;

  if (auto named = llvm::dyn_cast<const NamedType>(type)) {
    if (named->getName() == "Promise" && !named->getGenericArgs().empty()) {
      // Correctly access type pointer from GenericArg struct
      lastComputedType = named->getGenericArgs()[0].type.get();
      return;
    }
  }
  lastComputedType = type;
  const_cast<AwaitExpr *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitSizeOfExpr(const SizeOfExpr *expr) {
  // Ensure the expression inside sizeof() is valid
  expr->getExpr()->accept(*this);

  // Sizeof always evaluates to the platform's unsigned pointer size
  lastComputedType = context.getUSizeType();
  const_cast<SizeOfExpr *>(expr)->setType(lastComputedType);
}

void TypeChecker::visitInputExpr(const InputExpr *expr) {
  if (expr->getPrompt()) {
    // Dispatch expression evaluation correctly
    expr->getPrompt()->accept(*this);

    // Provide the required DiagID
    if (!lastComputedType->isString()) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "input() prompt must be a string";
      hasError = true;
    }
  }

  lastComputedType = context.getStringType();
  const_cast<InputExpr *>(expr)->setType(lastComputedType);
}

// --- ASTVisitor Overrides: Statements ---

void TypeChecker::visitVariableDecl(const VariableDecl *decl) {
  if (decl->getType())
    decl->getType()->accept(*this);

  const Type *varType = lastComputedType; // The resolved type of the variable

  if (!varType) {
    hasError = true;
    return;
  }

  if (auto namedTy = llvm::dyn_cast<const NamedType>(varType)) {
    const ClassDecl *classDecl = context.lookupClass(namedTy->getName());
    if (!classDecl) {
      if (const Symbol *sym = symbols.lookup(namedTy->getName())) {
        if (sym->kind == SymbolKind::Class && sym->decl) {
          classDecl = llvm::dyn_cast<const ClassDecl>(sym->decl);
        }
      }
    }

    if (classDecl) {
      if (classDecl->isReferenceType()) {
        const_cast<VariableDecl *>(decl)->setShared(true);
      }
      auto *mutDecl = const_cast<VariableDecl *>(decl);
      if (mutDecl->getAlignment() == 0 && classDecl->getAlignment() > 0) {
        mutDecl->setAlignment(classDecl->getAlignment());
      }
      if (mutDecl->getSection().empty() && !classDecl->getSection().empty()) {
        mutDecl->setSection(classDecl->getSection());
      }
    }
  }

  // --- 1. [NEW] OS-Level Bitfield Validation ---
  if (decl->getBitWidth() != -1) {
    if (!varType->isInteger()) {
      Diags.report(decl->getLoc(), DiagID::err_type_mismatch)
          << "Bitfields must have an integer type; found '"
          << varType->toString() << "'";
      hasError = true;
    } else {
      if (auto prim = llvm::dyn_cast<const PrimitiveType>(varType)) {
        int maxBits = 0;
        switch (prim->getScalar()) {
        case PrimitiveType::Scalar::I8:
        case PrimitiveType::Scalar::U8:
        case PrimitiveType::Scalar::Char:
          maxBits = 8;
          break;
        case PrimitiveType::Scalar::I16:
        case PrimitiveType::Scalar::U16:
          maxBits = 16;
          break;
        case PrimitiveType::Scalar::I32:
        case PrimitiveType::Scalar::U32:
        case PrimitiveType::Scalar::Int:
          maxBits = 32;
          break;
        case PrimitiveType::Scalar::I64:
        case PrimitiveType::Scalar::U64:
        case PrimitiveType::Scalar::ISize:
        case PrimitiveType::Scalar::USize:
          maxBits = 64; // Assuming 64-bit architecture
          break;
        default:
          break;
        }

        if (maxBits > 0 && decl->getBitWidth() > maxBits) {
          Diags.report(decl->getLoc(), DiagID::err_type_mismatch)
              << "Bitfield width " << decl->getBitWidth()
              << " exceeds the maximum width of type '" << varType->toString()
              << "' (" << maxBits << " bits)";
          hasError = true;
        }
      }
    }
  }

  // --- 2. Definite Assignment Tracking ---
  if (decl->getInitializer()) {
    initializedVars.insert(decl);
  } else if (decl->isExternVar()) {
    // Extern variables are defined elsewhere, so they are considered "assigned"
    initializedVars.insert(decl);
  }

  const Type *effectiveType = varType;

  // --- 3. Initializer Logic & Compatibility Checks ---
  if (decl->getInitializer()) {
    // Set return type hint for lambdas
    const Type *oldRet = currentExpectedReturnType;
    if (!varType->is<AnyType>()) {
      currentExpectedReturnType = varType;
    }

    decl->getInitializer()->accept(*this);
    currentExpectedReturnType = oldRet;

    // 1. C-String Bounds Checking (Runs independently of compatibility)
    if (auto declArr = llvm::dyn_cast<const ArrayType>(varType)) {
      if (isCharType(declArr->getElementType()) &&
          lastComputedType->isString()) {
        if (auto strLit =
                llvm::dyn_cast<const StringLiteral>(decl->getInitializer())) {
          if (auto sizeLit = llvm::dyn_cast<const IntegerLiteral>(
                  declArr->getSizeExpr())) {
            uint64_t requiredSize = strLit->getValue().length() + 1;
            if (sizeLit->getValue() < requiredSize) {
              Diags.report(decl->getLoc(), DiagID::err_array_length)
                  << "Char array is too small. '\"" << strLit->getValue()
                  << "\"' requires " << requiredSize
                  << " bytes (including '\\0'), but array is size "
                  << sizeLit->getValue();
              hasError = true;
            }
          }
        }
      }
    }

    // Compatibility check
    if (!isCompatible(varType, lastComputedType)) {
      bool customReported = false;

      // Support Lock Elevation: Allow *lock -> *mut if lock is held
      if (auto id =
              llvm::dyn_cast<const IdentifierExpr>(decl->getInitializer())) {
        // If the RHS type has a lock and that variable name is in activeLocks
        if (hasLock(lastComputedType) && activeLocks.count(id->getName())) {
          if (hasMutOrLock(varType)) {
            customReported = true; // Elevation allowed, skip further errors
          }
        }
      }

      // Check for Array Length Mismatch
      if (auto declArr = llvm::dyn_cast<const ArrayType>(varType)) {
        // String Literal to Char Array Bounds Checking
        if (isCharType(declArr->getElementType()) &&
            lastComputedType->isString()) {
          if (auto strLit =
                  llvm::dyn_cast<const StringLiteral>(decl->getInitializer())) {
            if (declArr->getSizeExpr()) {
              if (auto sizeLit = llvm::dyn_cast<const IntegerLiteral>(
                      declArr->getSizeExpr())) {
                uint64_t requiredSize = strLit->getValue().length() + 1;
                if (sizeLit->getValue() < requiredSize) {
                  Diags.report(decl->getLoc(), DiagID::err_array_length)
                      << "Char array is too small. '\"" << strLit->getValue()
                      << "\"' requires " << requiredSize
                      << " bytes (including '\\0'), but array is size "
                      << sizeLit->getValue();
                  customReported = true;
                  hasError = true;
                }
              }
            }
          }
        } else if (auto initArr =
                       llvm::dyn_cast<const ArrayType>(lastComputedType)) {
          if (declArr->getSizeExpr() && initArr->getSizeExpr()) {
            auto dSize =
                llvm::dyn_cast<const IntegerLiteral>(declArr->getSizeExpr());
            auto iSize =
                llvm::dyn_cast<const IntegerLiteral>(initArr->getSizeExpr());
            if (dSize && iSize && dSize->getValue() != iSize->getValue()) {
              Diags.report(decl->getLoc(), DiagID::err_array_length)
                  << "Array length mismatch. Expected " << dSize->getValue()
                  << " elements but found " << iSize->getValue();
              customReported = true;
              hasError = true;
            }
          }
        }
      }

      if (!customReported) {
        const Type *unwrappedVar = unwrapConcurrency(varType);
        bool isActuallyNullable = unwrappedVar->is<NullableType>();

        if (lastComputedType->is<NullType>() && !isActuallyNullable &&
            !varType->is<AnyType>()) {
          Diags.report(decl->getLoc(), DiagID::err_null_assignment)
              << " '" << varType->toString() << "'";
          hasError = true;
        } else {
          Diags.report(decl->getLoc(), DiagID::err_type_mismatch)
              << "Expected '" << varType->toString() << "' but found '"
              << lastComputedType->toString() << "'";
          hasError = true;
        }
      }
    }

    // If RHS is a function, store the full signature
    if (llvm::dyn_cast<const FunctionType>(lastComputedType)) {
      effectiveType = lastComputedType;
    }
  }

  // --- 4. Symbol Registration with OS Metadata ---
  if (!symbols.isDefinedInCurrentScope(decl->getName())) {
    Symbol sym(SymbolKind::Variable, decl->getName(), effectiveType, decl);

    // Transfer the bitWidth to the Symbol Table
    sym.bitWidth = decl->getBitWidth();

    if (!symbols.addSymbol(decl->getName(), sym, decl->getLoc())) {
      Diags.report(decl->getLoc(), DiagID::err_symbol_redefinition)
          << decl->getName();
      hasError = true;
    }
  } else {
    // Update the pre-registered global variable with the resolved type!
    if (Symbol *existing = symbols.lookup(decl->getName())) {
      if (existing->decl == decl) {
        existing->type = effectiveType;
      }
    }
  }
}

void TypeChecker::visitFunctionDecl(const FunctionDecl *decl) {
  const Type *retType = context.getVoidType();
  if (decl->getReturnType()) {
    decl->getReturnType()->accept(*this);
    retType = lastComputedType; // [FIX] Use the evaluated return type
  }

  std::vector<const Type *> pTypes;
  for (auto &p : decl->getParams()) {
    if (p.type) {
      p.type->accept(*this);
      pTypes.push_back(
          lastComputedType); // [FIX] Use the evaluated parameter type
    } else {
      pTypes.push_back(context.getAnyType());
    }
  }

  const Type *fnType =
      context.createFunctionType(pTypes, retType, decl->isVariadicFunc());

  if (!symbols.isDefinedInCurrentScope(decl->getName())) {
    Symbol sym(SymbolKind::Function, decl->getName(), fnType, decl);
    if (!symbols.addSymbol(decl->getName(), sym, decl->getLoc())) {
      Diags.report(decl->getLoc(), DiagID::err_symbol_redefinition)
          << decl->getName();
      hasError = true;
    }
  } else {
    // [FIX] Update pre-registered global function with resolved types
    if (Symbol *existing = symbols.lookup(decl->getName())) {
      if (existing->decl == decl) {
        existing->type = fnType;
      }
    }
  }

  // Enter the new scope for the function body
  symbols.enterScope(ScopeKind::Function);

  const Type *prevRet = currentExpectedReturnType;
  bool prevStatic = inStaticContext;
  bool prevInterrupt = inInterruptContext;
  bool prevConstructor = inConstructorContext;

  currentExpectedReturnType = retType;
  inStaticContext = decl->isStaticFunc();
  inInterruptContext = decl->isInterruptFunc();

  // We are in a constructor if the function is literally named 'constructor'
  inConstructorContext = (decl->getName() == "constructor");

  // Register all parameters as local variables inside the function scope
  for (size_t i = 0; i < decl->getParams().size(); ++i) {
    const auto &param = decl->getParams()[i];
    Symbol pSym(SymbolKind::Variable, param.name,
                pTypes[i]); // Use resolved pTypes
    symbols.addSymbol(param.name, pSym, decl->getLoc());
  }

  // Type-check default parameters
  for (const auto &param : decl->getParams()) {
    if (param.defaultValue) {
      param.defaultValue->accept(*this);
      if (!isCompatible(param.type.get(), lastComputedType)) {
        Diags.report(param.defaultValue->getLoc(), DiagID::err_type_mismatch)
            << "Default value type does not match parameter type '"
            << param.type->toString() << "'";
        hasError = true;
      }
    }
  }

  if (decl->getBody()) {
    decl->getBody()->accept(*this);
    bool guaranteesReturn = checkDefiniteReturn(decl->getBody(), Diags);

    // If the function promises to return something (not void, not any)
    if (retType && !retType->isVoid() && !retType->is<AnyType>()) {
      if (!guaranteesReturn) {
        Diags.report(decl->getLoc(), DiagID::err_missing_return)
            << "Function '" << decl->getName()
            << "' missing return statement on one or more paths";
        hasError = true;
      }
    }
  }

  inStaticContext = prevStatic;
  inInterruptContext = prevInterrupt;
  inConstructorContext = prevConstructor;
  currentExpectedReturnType = prevRet;
  symbols.exitScope();
}

void TypeChecker::visitUsingDecl(const UsingDecl *decl) {
  // Evaluate the target type (e.g., 'int' in 'using status_t = int')
  decl->getTargetType()->accept(*this);

  // Register the alias in the symbol table as a Type symbol
  Symbol sym(SymbolKind::Type, decl->getName(), lastComputedType, decl);
  symbols.addSymbol(decl->getName(), sym, decl->getLoc());
}

void TypeChecker::visitReturnStmt(const ReturnStmt *stmt) {
  if (stmt->getReturnValue()) {
    stmt->getReturnValue()->accept(*this);
    const Type *actualType = lastComputedType;

    if (currentExpectedReturnType) {
      if (currentExpectedReturnType->isEquivalent(*context.getVoidType())) {
        Diags.report(stmt->getLoc(), DiagID::err_type_mismatch)
            << "Void function cannot return a value";
        hasError = true;
      } else if (!isCompatible(currentExpectedReturnType, actualType)) {
        Diags.report(stmt->getLoc(), DiagID::err_type_incompatible_return)
            << "returning '" << actualType->toString()
            << "' instead of expected '"
            << currentExpectedReturnType->toString() << "'";
        hasError = true;
      }
    }
  } else {
    // Empty return statement (return;)
    if (currentExpectedReturnType &&
        !currentExpectedReturnType->isEquivalent(*context.getVoidType())) {
      Diags.report(stmt->getLoc(), DiagID::err_type_mismatch)
          << "Expected return value of type '"
          << currentExpectedReturnType->toString() << "'";
      hasError = true;
    }
  }
}

void TypeChecker::visitBlockStmt(const BlockStmt *stmt) {
  symbols.enterScope(ScopeKind::Block);
  for (const auto &s : stmt->getStatements()) {
    s->accept(*this);
  }
  symbols.exitScope();
}

void TypeChecker::visitIfStmt(const IfStmt *stmt) {
  stmt->getCondition()->accept(*this);
  if (!lastComputedType->isBool()) {
    Diags.report(stmt->getCondition()->getLoc(), DiagID::err_type_mismatch)
        << "If condition must be boolean";
  }

  // Save state before branching
  auto initBefore = initializedVars;

  stmt->getThenStmt()->accept(*this);
  auto initAfterThen = initializedVars;

  // Reset state for the Else branch
  initializedVars = initBefore;
  if (stmt->getElseStmt())
    stmt->getElseStmt()->accept(*this);

  auto initAfterElse = initializedVars;

  // A variable is only safe if it was assigned in BOTH branches
  std::set<const Decl *> intersection;
  for (auto d : initAfterThen) {
    if (initAfterElse.count(d))
      intersection.insert(d);
  }
  initializedVars = intersection;
}

void TypeChecker::visitWhileStmt(const WhileStmt *stmt) {
  // Save state before loop
  auto initBefore = initializedVars;

  stmt->getCondition()->accept(*this);
  if (!lastComputedType->isBool()) {
    Diags.report(stmt->getCondition()->getLoc(), DiagID::err_type_mismatch)
        << "While condition must be boolean";
  }
  loopDepth++;
  stmt->getBody()->accept(*this);
  loopDepth--;

  //  Variables initialized inside the loop are not guaranteed safe
  // afterwards
  initializedVars = initBefore;
}

void TypeChecker::visitDoWhileStmt(const DoWhileStmt *stmt) {
  loopDepth++;
  stmt->getBody()->accept(*this);
  loopDepth--;
  stmt->getCondition()->accept(*this);
  if (!lastComputedType->isBool()) {
    Diags.report(stmt->getCondition()->getLoc(), DiagID::err_type_mismatch)
        << "Do-While condition must be boolean";
  }
}

void TypeChecker::visitForStmt(const ForStmt *stmt) {
  symbols.enterScope(ScopeKind::Block);
  if (stmt->getInit())
    stmt->getInit()->accept(*this);

  auto initAfterInit = initializedVars;

  if (stmt->getCondition()) {
    stmt->getCondition()->accept(*this);
    if (!lastComputedType->isBool()) {
      Diags.report(stmt->getLoc(), DiagID::err_type_mismatch)
          << "For condition must be boolean";
    }
  }

  if (stmt->getIncrement())
    stmt->getIncrement()->accept(*this);

  loopDepth++;
  stmt->getBody()->accept(*this);
  loopDepth--;

  symbols.exitScope();

  initializedVars = initAfterInit;
}

void TypeChecker::visitForInStmt(const ForInStmt *stmt) {
  symbols.enterScope(ScopeKind::Block);

  stmt->getCollection()->accept(*this);
  const Type *colType = lastComputedType;

  auto initBeforeLoop = initializedVars;

  const Type *indexType = context.getI32Type();
  const Type *valType = context.getAnyType();

  if (auto arr = llvm::dyn_cast<const ArrayType>(colType)) {
    valType = arr->getElementType();
  } else if (auto slice = llvm::dyn_cast<const SliceType>(colType)) {
    valType = slice->getElementType();
  } else if (auto map = llvm::dyn_cast<const MapType>(colType)) {
    indexType = map->getKeyType();
    valType = map->getValueType();
  } else if (colType->isString()) {
    valType = context.getCharType();
  } else if (!colType->is<AnyType>()) {
    Diags.report(stmt->getCollection()->getLoc(), DiagID::err_type_mismatch)
        << "Type is not iterable";
  }

  if (stmt->getVariable()) {
    initializedVars.insert(stmt->getVariable());
    Symbol valSym(SymbolKind::Variable, stmt->getVariable()->getName(), valType,
                  stmt->getVariable());
    symbols.addSymbol(stmt->getVariable()->getName(), valSym, stmt->getLoc());
  }

  if (stmt->getIndexVariable()) {
    initializedVars.insert(stmt->getIndexVariable());
    Symbol idxSym(SymbolKind::Variable, stmt->getIndexVariable()->getName(),
                  indexType, stmt->getIndexVariable());
    symbols.addSymbol(stmt->getIndexVariable()->getName(), idxSym,
                      stmt->getLoc());
  }

  loopDepth++;
  stmt->getBody()->accept(*this);
  loopDepth--;

  symbols.exitScope();
  initializedVars = initBeforeLoop;
}

void TypeChecker::visitSwitchStmt(const SwitchStmt *stmt) {
  stmt->getCondition()->accept(*this);
  const Type *condType = lastComputedType;

  auto initBefore = initializedVars;
  std::vector<std::set<const Decl *>> caseStates;
  bool hasDefault = false;

  // If we are switching on an Enum, we want to track which cases are covered
  const EnumDecl *enumDecl = nullptr;
  std::vector<std::string> coveredCases;

  if (auto namedCond = llvm::dyn_cast<const NamedType>(condType)) {
    Symbol *sym = symbols.lookup(namedCond->getName());
    if (sym && sym->decl && sym->decl->getKind() == StmtKind::EnumDecl) {
      enumDecl = static_cast<const EnumDecl *>(sym->decl);
    }
  }

  loopDepth++;

  for (size_t i = 0; i < stmt->getCases().size(); ++i) {
    const auto &c = stmt->getCases()[i];

    initializedVars = initBefore;
    if (c.isDefaultCase()) {
      hasDefault = true;
    }

    for (const auto &val : c.getValues()) {
      val->accept(*this);
      if (!isCompatible(condType, lastComputedType)) {
        Diags.report(val->getLoc(), DiagID::err_type_mismatch)
            << "Case value type mismatch";
      }

      // Track enum coverage
      if (enumDecl) {
        if (auto memExpr = llvm::dyn_cast<const MemberExpr>(val.get())) {
          coveredCases.push_back(memExpr->getName());
        }
      }
    }

    c.getBody()->accept(*this);
    // A case "falls through" ONLY if its body is completely empty.
    bool isEmpty = c.getBody()->getStatements().empty();
    bool isLastCase = (i == stmt->getCases().size() - 1);

    if (isEmpty && !isLastCase) {
      // It falls through! Do NOT add its state to caseStates, because
      // execution doesn't stop here. It continues to the next case.
    } else {
      // It implicitly breaks (or is the last case). Record its terminal state.
      caseStates.push_back(initializedVars);
    }
  }

  loopDepth--;

  // Exhaustiveness Check!
  bool isExhaustive = hasDefault;

  if (enumDecl && !hasDefault) {
    bool missingCases = false;
    for (const auto &enumCase : enumDecl->getCases()) {
      if (std::find(coveredCases.begin(), coveredCases.end(), enumCase.name) ==
          coveredCases.end()) {
        Diags.report(stmt->getLoc(), DiagID::warn_switch_not_exhaustive)
            << "Switch is missing case: " << enumCase.name;
        missingCases = true;
      }
    }
    if (!missingCases) {
      isExhaustive = true;
    }
  }

  // Use `isExhaustive` instead of `hasDefault` for intersection logic
  if (!isExhaustive) {
    initializedVars = initBefore;
  } else if (!caseStates.empty()) {
    // Variable must be definitely assigned in EVERY branch
    std::set<const Decl *> intersection = caseStates[0];
    for (size_t i = 1; i < caseStates.size(); ++i) {
      std::set<const Decl *> currentIntersection;
      for (auto d : intersection) {
        if (caseStates[i].count(d)) {
          currentIntersection.insert(d);
        }
      }
      intersection = currentIntersection;
    }
    initializedVars = intersection;
  }
}

void TypeChecker::visitDeferStmt(const DeferStmt *stmt) {
  stmt->getDeferredStmt()->accept(*this);
}

void TypeChecker::visitUnsafeBlockStmt(const UnsafeBlockStmt *stmt) {
  symbols.enterScope(ScopeKind::Block);
  for (const auto &s : stmt->getStatements()) {
    s->accept(*this);
  }
  symbols.exitScope();
}

void TypeChecker::visitTryCatchStmt(const TryCatchStmt *stmt) {
  auto initBefore = initializedVars;

  stmt->getTryBody()->accept(*this);
  auto initAfterTry = initializedVars;

  initializedVars = initBefore;

  if (stmt->getCatchVar()) {
    symbols.enterScope(ScopeKind::Block);
    const auto *varDecl = llvm::dyn_cast<VariableDecl>(stmt->getCatchVar());
    if (!varDecl) {
      Diags.report(stmt->getLoc(), DiagID::err_internal)
          << "Catch variable is not a VariableDecl";
      hasError = true;
      return;
    }
    const Type *errType = varDecl->getType();
    if (errType->is<AnyType>()) {
      errType = context.getAnyType();
    }

    Symbol sym(SymbolKind::Variable, stmt->getCatchVar()->getName(), errType);
    symbols.addSymbol(stmt->getCatchVar()->getName(), sym, stmt->getLoc());

    // [NEW] The caught exception variable itself is safely initialized
    initializedVars.insert(stmt->getCatchVar());

    if (stmt->getCatchBody())
      stmt->getCatchBody()->accept(*this);
    symbols.exitScope();
  } else if (stmt->getCatchBody()) {
    stmt->getCatchBody()->accept(*this);
  }

  auto initAfterCatch = initializedVars;

  // [NEW] Intersection: Must be assigned in both Try AND Catch to be safe
  std::set<const Decl *> intersection;
  for (auto d : initAfterTry) {
    if (initAfterCatch.count(d))
      intersection.insert(d);
  }
  initializedVars = intersection;

  if (stmt->getFinallyBody())
    stmt->getFinallyBody()->accept(*this);
}

void TypeChecker::visitThrowStmt(const ThrowStmt *stmt) {
  if (inInterruptContext) {
    Diags.report(stmt->getLoc(),
                 DiagID::err_type_mismatch) // Or a custom DiagID
        << "Throwing exceptions inside an Interrupt Service Routine (ISR) is "
           "forbidden";
    hasError = true;
  }
  if (stmt->getExpr()) {
    stmt->getExpr()->accept(*this);
  }
}

void TypeChecker::visitExpressionStmt(const ExpressionStmt *stmt) {
  stmt->getExpr()->accept(*this);
}

void TypeChecker::visitDeclStmt(const DeclStmt *stmt) {
  stmt->getDecl()->accept(*this);
}

void TypeChecker::visitBreakStmt(const BreakStmt *stmt) {
  if (loopDepth <= 0) {
    Diags.report(stmt->getLoc(), DiagID::err_break_outside_loop);
    hasError = true;
  }
}

void TypeChecker::visitContinueStmt(const ContinueStmt *stmt) {
  if (loopDepth <= 0) {
    Diags.report(stmt->getLoc(), DiagID::err_continue_outside_loop);
    hasError = true;
  }
}

void TypeChecker::visitAsmStmt(const AsmStmt *stmt) {
  // Inline assembly is treated as an opaque block; no semantic check needed
}

void TypeChecker::visitLockStmt(const LockStmt *stmt) {
  if (stmt->getTarget()) {
    // 1. Evaluate the target expression
    stmt->getTarget()->accept(*this);

    // 2. Verify it's actually lockable
    if (!hasLock(lastComputedType)) {
      Diags.report(stmt->getTarget()->getLoc(), DiagID::err_type_mismatch)
          << "Target of 'lock' statement must be a lock type or a pointer to a "
             "lock type";
      hasError = true;
    }
  }

  // 3. Grant capability
  if (auto id = llvm::dyn_cast<const IdentifierExpr>(stmt->getTarget())) {
    activeLocks.insert(id->getName());
  }

  if (stmt->getBody())
    stmt->getBody()->accept(*this);

  // 4. Revoke capability
  if (auto id = llvm::dyn_cast<const IdentifierExpr>(stmt->getTarget())) {
    activeLocks.erase(id->getName());
  }
}

// --- Top-Level Declarations ---

void TypeChecker::visitModuleDecl(const ModuleDecl *decl) {
  // --- Pass 1: Global Symbol Registration ---
  // Registers all names so they are visible regardless of definition order.
  for (const auto &d : decl->getDecls()) {
    const Decl *currentDecl = d.get();

    // 1. Functions (Standard & Generic)
    if (auto fd = llvm::dyn_cast<const FunctionDecl>(currentDecl)) {
      std::vector<const Type *> pTypes;
      for (auto &p : fd->getParams())
        pTypes.push_back(p.type.get());
      const Type *fnType =
          context.createFunctionType(pTypes, fd->getReturnType());
      symbols.addSymbol(fd->getName(),
                        Symbol(SymbolKind::Function, fd->getName(), fnType, fd),
                        fd->getLoc());
    }
    // 2. Classes (Standard)
    else if (auto cd = llvm::dyn_cast<const ClassDecl>(currentDecl)) {
      symbols.addSymbol(cd->getName(),
                        Symbol(SymbolKind::Class, cd->getName(),
                               context.createNamedType(cd->getName()), cd),
                        cd->getLoc());
      context.registerClass(cd);
    }
    // 3. Generic Declarations (Classes, Functions, or Variables)
    else if (auto gd = llvm::dyn_cast<const GenericDecl>(currentDecl)) {
      if (auto innerClass =
              llvm::dyn_cast<const ClassDecl>(gd->getInnerDecl())) {
        symbols.addSymbol(innerClass->getName(),
                          Symbol(SymbolKind::Class, innerClass->getName(),
                                 context.createNamedType(innerClass->getName()),
                                 gd),
                          gd->getLoc());
        context.registerClass(innerClass);
      } else if (auto innerFunc =
                     llvm::dyn_cast<const FunctionDecl>(gd->getInnerDecl())) {
        symbols.addSymbol(innerFunc->getName(),
                          Symbol(SymbolKind::Function, innerFunc->getName(),
                                 context.getAnyType(), gd),
                          gd->getLoc());
      } else if (auto innerVar =
                     llvm::dyn_cast<const VariableDecl>(gd->getInnerDecl())) {
        symbols.addSymbol(innerVar->getName(),
                          Symbol(SymbolKind::Variable, innerVar->getName(),
                                 innerVar->getType(), gd),
                          gd->getLoc());
      }
    }
    // 4. Variables (Global / TLS / Static)
    else if (auto vd = llvm::dyn_cast<const VariableDecl>(currentDecl)) {
      symbols.addSymbol(
          vd->getName(),
          Symbol(SymbolKind::Variable, vd->getName(), vd->getType(), vd),
          vd->getLoc());
    }
    // 5. Imports (Brings symbols from other modules into current scope)
    else if (auto id = llvm::dyn_cast<const ImportDecl>(currentDecl)) {
      id->accept(*this); // Register imported symbols immediately
    }
    // 6. Enums
    else if (auto ed = llvm::dyn_cast<const EnumDecl>(currentDecl)) {
      symbols.addSymbol(ed->getName(),
                        Symbol(SymbolKind::Type, ed->getName(),
                               context.createNamedType(ed->getName()), ed),
                        ed->getLoc());
    }
  }

  // --- Pass 1.5: Alias Registration ---
  for (const auto &d : decl->getDecls()) {
    const Decl *currentDecl = d.get();
    if (auto ud = llvm::dyn_cast<const UsingDecl>(currentDecl)) {
      ud->accept(*this);
    }
  }

  // --- Pass 2: Full Type Checking ---
  for (const auto &d : decl->getDecls()) {
    const Decl *checkDecl = d.get();
    // Skip Decls already fully processed in Pass 1/1.5
    if (llvm::dyn_cast<const UsingDecl>(checkDecl) ||
        llvm::dyn_cast<const ImportDecl>(checkDecl) ||
        llvm::dyn_cast<const EnumDecl>(checkDecl)) {
      continue;
    }

    // Call accept on the original AST node to preserve AST traversal
    d->accept(*this);
  }

  // --- Pass 3: Process Generic Instantiations ---
  processPendingInstantiations();
}

void TypeChecker::visitClassDecl(const ClassDecl *decl) {
  if (!symbols.isDefinedInCurrentScope(decl->getName())) {
    symbols.addSymbol(decl->getName(),
                      Symbol(SymbolKind::Class, decl->getName(),
                             context.createNamedType(decl->getName()), decl),
                      decl->getLoc());
    context.registerClass(decl);
  }

  if (!decl->isReferenceType()) {
    std::set<std::string> visited;
    visited.insert(decl->getName());

    for (const auto &member : decl->getMembers()) {
      if (auto varDecl = llvm::dyn_cast<const VariableDecl>(member.get())) {
        if (detectInfiniteSize(varDecl->getType(), visited)) {
          Diags.report(varDecl->getLoc(), DiagID::err_infinite_size)
              << "due to recursive field '" << varDecl->getName() << "'";
          hasError = true;
        }
      }
    }
  }

  // Map signature -> VTable Index
  std::map<std::string, int> vtableMap;
  int currentVTableIndex = 0;

  // [NEW] Gather this class's explicit methods to check if it resolves
  // ambiguities
  std::set<std::string> myMethods;
  for (const auto &member : decl->getMembers()) {
    if (auto fd = llvm::dyn_cast<const FunctionDecl>(member.get())) {
      myMethods.insert(getMethodSignature(fd));
    }
  }
  std::map<std::string, std::string> inheritedMethods;

  for (const auto &pName : decl->getParentNames()) {
    if (const ClassDecl *parentDecl = context.lookupClass(pName)) {
      if (decl->isReferenceType() != parentDecl->isReferenceType()) {
        Diags.report(decl->getLoc(), DiagID::err_type_mismatch)
            << "Inheritance violation: Class '" << decl->getName()
            << "' and parent must both be 'ref' classes or value classes";
        hasError = true;
      }

      // [NEW] Scan this parent's entire hierarchy for methods
      std::map<std::string, std::string> parentMethods;
      std::vector<const ClassDecl *> queue = {parentDecl};
      size_t head = 0;
      while (head < queue.size()) {
        const ClassDecl *cur = queue[head++];
        for (const auto &member : cur->getMembers()) {
          if (auto fd = llvm::dyn_cast<const FunctionDecl>(member.get())) {
            std::string sig = getMethodSignature(fd);
            if (!parentMethods.count(sig)) {
              parentMethods[sig] = cur->getName();
            }
          }
        }
        for (const auto &p : cur->getParentNames()) {
          if (auto pc = context.lookupClass(p))
            queue.push_back(pc);
        }
      }

      // [NEW] Check for diamond collisions across parents
      for (const auto &[sig, origin] : parentMethods) {
        if (inheritedMethods.count(sig)) {
          if (!myMethods.count(sig)) {
            Diags.report(decl->getLoc(), DiagID::err_type_mismatch)
                << "Ambiguous implementation: Class '" << decl->getName()
                << "' inherits method '" << sig << "' from both '"
                << inheritedMethods[sig] << "' and '" << origin
                << "'. It must override this method to resolve the ambiguity.";
            hasError = true;
          }
        } else {
          inheritedMethods[sig] = origin;
        }
      }

      // 1. Inherit Parent's VTable layout
      for (const auto &member : parentDecl->getMembers()) {
        if (auto fd = llvm::dyn_cast<const FunctionDecl>(member.get())) {
          if (fd->isVirtualFunc()) {
            std::string sig = getMethodSignature(fd);
            vtableMap[sig] = fd->getVTableIndex();
            currentVTableIndex =
                std::max(currentVTableIndex, fd->getVTableIndex() + 1);
          }
        }
      }
    } else {
      Diags.report(decl->getLoc(), DiagID::err_unknown_type) << pName;
      hasError = true;
    }
  }

  bool classHasVTable = currentVTableIndex > 0;

  // 2. Assign VTable indices to this class's methods
  for (const auto &member : decl->getMembers()) {
    if (auto fd = llvm::dyn_cast<const FunctionDecl>(member.get())) {
      std::string sig = getMethodSignature(fd);

      if (vtableMap.count(sig)) {
        // Method matches a parent's virtual signature. It's an override!
        const_cast<FunctionDecl *>(fd)->setVirtual(true);
        const_cast<FunctionDecl *>(fd)->setVTableIndex(vtableMap[sig]);
      } else if (fd->isVirtualFunc()) {
        // Brand new virtual method introduced in this class
        const_cast<FunctionDecl *>(fd)->setVTableIndex(currentVTableIndex++);
        vtableMap[sig] = fd->getVTableIndex();
      }

      if (fd->isVirtualFunc()) {
        classHasVTable = true;
      }
    }
  }

  // Comprehensive Struct/Union Safety Checks
  std::set<std::string> seenFields;

  for (const auto &member : decl->getMembers()) {
    if (auto *field = llvm::dyn_cast<VariableDecl>(member.get())) {
      const Type *fieldType = field->getType();

      // 1. Check for duplicate field names
      if (!seenFields.insert(field->getName()).second) {
        Diags.report(field->getLoc(), DiagID::err_symbol_redefinition)
            << "Duplicate field name '" << field->getName() << "' in "
            << (decl->getAggregateKind() == AggregateKind::Union ? "union '"
                                                                 : "struct '")
            << decl->getName() << "'";
        hasError = true;
      }

      // 2. Prevent 'void' fields
      if (fieldType->is<PrimitiveType>() &&
          static_cast<const PrimitiveType *>(fieldType)->getScalar() ==
              PrimitiveType::Scalar::Void) {
        Diags.report(field->getLoc(), DiagID::err_type_mismatch)
            << "Field '" << field->getName() << "' cannot have type 'void'.";
        hasError = true;
      }

      // 3. Catch Cyclic Dependencies (Infinite Size Structs)
      if (decl->getAggregateKind() != AggregateKind::Class &&
          fieldType->is<NamedType>()) {
        auto *named = static_cast<const NamedType *>(fieldType);
        if (named->getName() == decl->getName()) {
          Diags.report(field->getLoc(), DiagID::err_type_mismatch)
              << "Struct/Union '" << decl->getName()
              << "' cannot contain itself by value. "
              << "Use a pointer ('" << decl->getName() << "*') instead.";
          hasError = true;
        }
      }
    }
  }

  // 4. Unions cannot contain ARC-managed types (ref class)
  if (decl->getAggregateKind() == AggregateKind::Union) {
    for (const auto &member : decl->getMembers()) {
      if (auto *field = llvm::dyn_cast<VariableDecl>(member.get())) {
        if (auto *named = llvm::dyn_cast<NamedType>(field->getType())) {
          const ClassDecl *targetClass = context.lookupClass(named->getName());
          // FIXED: Use getAggregateKind() == AggregateKind::Class
          if (targetClass &&
              targetClass->getAggregateKind() == AggregateKind::Class) {
            Diags.report(field->getLoc(), DiagID::err_type_mismatch)
                << "Union '" << decl->getName()
                << "' cannot contain ARC-managed field '" << field->getName()
                << "'. Unions must be trivially destructible under NLL.";
            hasError = true;
          }
        }
      }
    }
  }

  const_cast<ClassDecl *>(decl)->setHasVTable(classHasVTable);

  const ClassDecl *prevClass = currentClassDecl;
  currentClassDecl = decl;

  symbols.enterScope(ScopeKind::Class);

  // PRE-REGISTRATION PASS: Register all members so methods can see each other
  for (const auto &member : decl->getMembers()) {
    if (auto vd = llvm::dyn_cast<const VariableDecl>(member.get())) {
      Symbol sym(SymbolKind::Variable, vd->getName(), vd->getType(), vd);
      if (!symbols.addSymbol(vd->getName(), sym, vd->getLoc())) {
        Diags.report(vd->getLoc(), DiagID::err_symbol_redefinition)
            << vd->getName();
        hasError = true;
      }
    } else if (auto fd = llvm::dyn_cast<const FunctionDecl>(member.get())) {
      std::vector<const Type *> pTypes;
      for (auto &p : fd->getParams())
        pTypes.push_back(p.type.get());
      const Type *fnType = context.createFunctionType(
          pTypes, fd->getReturnType(), fd->isVariadicFunc());
      Symbol sym(SymbolKind::Function, fd->getName(), fnType, fd);

      if (!symbols.addSymbol(fd->getName(), sym, fd->getLoc())) {
        Diags.report(fd->getLoc(), DiagID::err_symbol_redefinition)
            << fd->getName();
        hasError = true;
      }
    }
  }

  // TYPE-CHECKING PASS: Visit members to check their bodies and initializers
  for (const auto &member : decl->getMembers()) {
    member->accept(*this);
  }

  // --- NEW: Physical Layout and Bitfield Packing Pass ---
  uint32_t currentPhysicalIndex = 0;
  uint32_t currentBitOffset = 0;

  for (const auto &member : decl->getMembers()) {
    if (auto *varDecl = llvm::dyn_cast<VariableDecl>(member.get())) {
      // Determine the storage size based on the primitive type
      uint32_t storageSize = 64; // Default safe fallback
      if (auto *prim = llvm::dyn_cast<PrimitiveType>(varDecl->getType())) {
        switch (prim->getScalar()) {
        case PrimitiveType::Scalar::U8:
        case PrimitiveType::Scalar::I8:
          storageSize = 8;
          break;
        case PrimitiveType::Scalar::U16:
        case PrimitiveType::Scalar::I16:
          storageSize = 16;
          break;
        case PrimitiveType::Scalar::U32:
        case PrimitiveType::Scalar::I32:
          storageSize = 32;
          break;
        case PrimitiveType::Scalar::U64:
        case PrimitiveType::Scalar::I64:
          storageSize = 64;
          break;
        default:
          break;
        }
      }

      // We use const_cast here because the AST visitor passes const pointers,
      // but Semantic Analysis is responsible for decorating the AST with layout
      // info.
      auto *mutVarDecl = const_cast<VariableDecl *>(varDecl);

      if (mutVarDecl->isBitfield()) {
        // If this bitfield doesn't fit in the current integer, move to the next
        // physical slot
        if (currentBitOffset > 0 &&
            (currentBitOffset + mutVarDecl->getBitWidth() > storageSize)) {
          currentPhysicalIndex++;
          currentBitOffset = 0;
        }

        mutVarDecl->setPhysicalIndex(currentPhysicalIndex);
        mutVarDecl->setBitOffset(currentBitOffset);

        // Advance the bit offset for the next member
        currentBitOffset += mutVarDecl->getBitWidth();
      } else {
        // If it's NOT a bitfield, we must flush any pending bitfield storage
        if (currentBitOffset > 0) {
          currentPhysicalIndex++;
          currentBitOffset = 0;
        }
        mutVarDecl->setPhysicalIndex(currentPhysicalIndex);
        currentPhysicalIndex++; // Standard variables take up a full physical
                                // slot
      }
    }
  }

  symbols.exitScope();
  currentClassDecl = prevClass;
}

void TypeChecker::visitGenericDecl(const GenericDecl *decl) {
  symbols.enterScope(ScopeKind::Class);
  for (const auto &param : decl->getTypeParams()) {
    symbols.addSymbol(param.name,
                      Symbol(SymbolKind::Type, param.name,
                             context.createNamedType(param.name)),
                      decl->getLoc());
  }
  decl->getInnerDecl()->accept(*this);
  symbols.exitScope();
}

void TypeChecker::visitImportDecl(const ImportDecl *decl) {
  llvm::StringRef modName = decl->getModuleName();
  size_t slash = modName.find_last_of('/');
  llvm::StringRef ns =
      (slash != llvm::StringRef::npos) ? modName.substr(slash + 1) : modName;

  if (decl->getSymbols().empty()) {
    if (!symbols.lookup(ns.str())) {
      symbols.addSymbol(
          ns, Symbol(SymbolKind::Module, ns.str(), context.getAnyType()),
          decl->getLoc());
    }
  } else {
    for (const auto &sym : decl->getSymbols()) {
      std::string fqName = ns.str() + "." + sym;

      // [FIX] 1. Look up the REAL symbol pre-loaded by the compiler driver
      Symbol *realSym = symbols.lookup(fqName);

      if (!realSym) {
        realSym = symbols.lookup(sym);
      }

      // 2. Ensure FQ name is registered (fallback for opaque C/C++ FFI)
      if (!realSym && !symbols.lookup(fqName)) {
        symbols.addSymbol(
            fqName,
            Symbol(SymbolKind::Variable, fqName, context.getAnyType(), decl),
            decl->getLoc());
      }

      // 3. Track the short name for ambiguity
      ambiguousImports[sym].push_back(ns.str());

      // 4. Register the short name ONLY if it's the first time we see it
      if (ambiguousImports[sym].size() == 1) {
        if (!symbols.lookup(sym)) {
          if (realSym) {
            // [CRITICAL FIX] Alias the short name directly to the REAL AST
            // symbol! This preserves the pointer to the GenericDecl/ClassDecl.
            Symbol aliasedSym = *realSym;
            aliasedSym.name =
                sym; // Update internal name to match the short alias
            symbols.addSymbol(sym, aliasedSym, decl->getLoc());
          } else {
            // Fallback for standard opaque imports
            symbols.addSymbol(
                sym,
                Symbol(SymbolKind::Variable, sym, context.getAnyType(), decl),
                decl->getLoc());
          }
        }
      }
    }
  }
}

void TypeChecker::visitEnumDecl(const EnumDecl *decl) {
  const Type *enumT = context.createNamedType(decl->getName());
  Symbol sym(SymbolKind::Type, decl->getName(), enumT, decl);
  symbols.addSymbol(decl->getName(), sym, decl->getLoc());
}

void TypeChecker::visitMacroDecl(const MacroDecl *decl) {}

// --- Structural Visitors (Types) ---

void TypeChecker::visitPrimitiveType(const PrimitiveType *t) {
  lastComputedType = t;
}
void TypeChecker::visitPointerType(const PointerType *t) {
  t->getPointee()->accept(*this);
  lastComputedType = t;
}
void TypeChecker::visitReferenceType(const ReferenceType *t) {
  t->getInner()->accept(*this);
  lastComputedType = t;
}
void TypeChecker::visitArrayType(const ArrayType *t) {
  t->getElementType()->accept(*this);
  if (t->getSizeExpr()) {
    if (!llvm::dyn_cast<const IntegerLiteral>(t->getSizeExpr())) {
      Diags.report(t->getSizeExpr()->getLoc(), DiagID::err_type_mismatch)
          << "Array size must be a constant integer literal";
      hasError = true;
    }
  }
  lastComputedType = t;
}
void TypeChecker::visitSliceType(const SliceType *type) {
  type->getElementType()->accept(*this);
  lastComputedType = type;
}
void TypeChecker::visitMapType(const MapType *t) {
  t->getKeyType()->accept(*this);
  t->getValueType()->accept(*this);
  lastComputedType = t;
}
void TypeChecker::visitFunctionType(const FunctionType *t) {
  for (auto &p : t->getParamTypes())
    p->accept(*this);
  t->getReturnType()->accept(*this);
  lastComputedType = t;
}
void TypeChecker::visitNullableType(const NullableType *t) {
  t->getInner()->accept(*this);
  lastComputedType = t;
}
void TypeChecker::visitAnyType(const AnyType *t) { lastComputedType = t; }
void TypeChecker::visitNamedType(const NamedType *type) {
  // 1. Handle Generic Instantiation FIRST
  if (!type->getGenericArgs().empty()) {
    Symbol *sym = symbols.lookup(type->getName());

    if (!sym || !sym->decl || sym->decl->getKind() != StmtKind::GenericDecl) {
      Diags.report(type->getLoc(), DiagID::err_type_mismatch)
          << "Type '" << type->getName() << "' is not a generic template";
      hasError = true;
      lastComputedType = type;
      return;
    }

    auto *genericDecl = static_cast<const GenericDecl *>(sym->decl);

    auto error = resolver.validateGenericArgs(genericDecl->getTypeParams(),
                                              type->getGenericArgs());
    if (error) {
      Diags.report(type->getLoc(), DiagID::err_type_mismatch)
          << "Generic argument mismatch for '" << type->getName() << "'";
      hasError = true;
    } else {
      std::vector<const Type *> concreteArgs;
      for (auto &arg : type->getGenericArgs()) {
        arg.type->accept(*this);
        concreteArgs.push_back(lastComputedType);
      }

      // --- [FIX] Check for existing instantiation to prevent duplicates! ---
      std::string mangledName =
          resolver.getMangledName(type->getName(), concreteArgs);
      const ClassDecl *concreteClass = context.lookupClass(mangledName);

      if (!concreteClass) {
        concreteClass = resolver.instantiateClass(genericDecl, concreteArgs);
        if (concreteClass) {
          pendingInstantiations.push_back(concreteClass);

          // Register it in the AST Context so subsequent lookups find it
          // instantly
          if (!context.lookupClass(concreteClass->getName())) {
            context.registerClass(concreteClass);
          }
        }
      }

      if (concreteClass) {
        auto *mutType = const_cast<NamedType *>(type);
        mutType->setName(concreteClass->getName());

        auto &mutableArgs = const_cast<std::vector<NamedType::GenericArg> &>(
            mutType->getGenericArgs());
        mutableArgs.clear();
      }
    }
  }

  // 2. Lookup the base class to ensure it actually exists
  const ClassDecl *classDecl = context.lookupClass(type->getName());

  if (!classDecl) {
    // [FIX] Check if this is a generic parameter (K, V, T) in the symbol table
    Symbol *sym = symbols.lookup(type->getName());
    if (sym && sym->kind == SymbolKind::Type) {
      lastComputedType = type;
      return; // It's perfectly valid, stop processing!
    }

    Diags.report(type->getLoc(), DiagID::err_unknown_type) << type->getName();
    hasError = true;
  }

  lastComputedType = type;
}
void TypeChecker::visitLockType(const LockType *t) {
  t->getInner()->accept(*this);
  lastComputedType = t;
}
void TypeChecker::visitViewType(const ViewType *t) {
  t->getInner()->accept(*this);
  lastComputedType = t;
}
void TypeChecker::visitMutType(const MutType *t) {
  t->getInner()->accept(*this);
  lastComputedType = t;
}
void TypeChecker::visitWeakType(const WeakType *t) {
  t->getInner()->accept(*this);
  const Type *inner = lastComputedType;

  // Validation: weak can only be applied to references/classes
  if (!inner->is<ReferenceType>() && !inner->is<NamedType>()) {
    Diags.report(t->getLoc(), DiagID::err_type_mismatch)
        << "weak can only be applied to reference types, found: "
        << inner->toString();
    hasError = true;
  }

  lastComputedType = t;
}
void TypeChecker::visitEnumType(const EnumType *t) { lastComputedType = t; }
void TypeChecker::visitNullType(const NullType *t) { lastComputedType = t; }
void TypeChecker::visitDecimalType(const DecimalType *t) {
  lastComputedType = t;
}
void TypeChecker::visitVolatileType(const VolatileType *t) {
  t->getInner()->accept(*this);
  lastComputedType = t;
}
void TypeChecker::visitConstType(const ConstType *t) {
  t->getInner()->accept(*this);
  lastComputedType = t;
}

void TypeChecker::visitClosureType(const ClosureType *t) {
  for (auto &p : t->getParamTypes()) {
    p->accept(*this);
  }
  t->getReturnType()->accept(*this);
  lastComputedType = t;
}
} // namespace moksha
