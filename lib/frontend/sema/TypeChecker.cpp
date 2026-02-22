#include "moksha/Sema/TypeChecker.h"
#include "moksha/AST/ASTContext.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "moksha/Builtins/BuiltinRegistry.h"
#include "moksha/Sema/GenericResolver.h"
#include "llvm/Support/raw_ostream.h"
#include <map>

namespace moksha {

namespace {

static const Type *unwrapConcurrency(const Type *t) {
  while (t) {
    if (auto l = dynamic_cast<const LockType *>(t))
      t = l->getInner();
    else if (auto v = dynamic_cast<const ViewType *>(t))
      t = v->getInner();
    else if (auto m = dynamic_cast<const MutType *>(t))
      t = m->getInner();
    else if (auto vol = dynamic_cast<const VolatileType *>(t))
      t = vol->getInner();
    else if (auto c = dynamic_cast<const ConstType *>(t))
      t = c->getInner();
    else
      break;
  }
  return t;
}

// Helpers to treat 'char' as a numeric 8-bit integer
bool isNumericOrChar(const Type *t) {
  t = unwrapConcurrency(t);
  if (t->isNumeric())
    return true;
  if (auto p = dynamic_cast<const PrimitiveType *>(t))
    return p->getScalar() == PrimitiveType::Scalar::Char;
  return false;
}

bool isIntegerOrChar(const Type *t) {
  t = unwrapConcurrency(t);
  if (t->isInteger())
    return true;
  if (auto p = dynamic_cast<const PrimitiveType *>(t))
    return p->getScalar() == PrimitiveType::Scalar::Char;
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
    // Loops (While, For) conservatively evaluate to false because we
    // cannot statically guarantee the loop condition will be true even once.
    // Normal expressions also evaluate to false.
    return false;
  }
}

} // namespace

TypeChecker::TypeChecker(ASTContext &ctx, SymbolTable &sym,
                         DiagnosticEngine &diags)
    : context(ctx), symbols(sym), Diags(diags), resolver(ctx) {
  lastComputedType = context.getVoidType();
  currentExpectedReturnType = nullptr;
  currentClassDecl = nullptr;
  hasError = false;
}

void TypeChecker::check(Decl *decl) {
  if (decl)
    decl->accept(*this);
}

void TypeChecker::check(Stmt *stmt) {
  if (stmt)
    stmt->accept(*this);
}

// --- Helper: Type Compatibility ---

bool TypeChecker::isCompatible(const Type *expected, const Type *actual) {
  if (!expected || !actual)
    return false;

  // Exact match
  if (expected->isEquivalent(*actual))
    return true;

  const Type *rawExp = unwrapConcurrency(expected);
  const Type *rawAct = unwrapConcurrency(actual);
  if (rawExp->isEquivalent(*rawAct))
    return true;

  // Handle Generic Parameters (NamedTypes without ClassDecls)
  if (auto namedExp = dynamic_cast<const NamedType *>(rawExp)) {
    if (auto namedAct = dynamic_cast<const NamedType *>(rawAct)) {
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

  // Recursive Array Compatibility
  if (auto arrExp = dynamic_cast<const ArrayType *>(rawExp)) {
    if (auto arrAct = dynamic_cast<const ArrayType *>(rawAct)) {
      if (!isCompatible(arrExp->getElementType(), arrAct->getElementType()))
        return false;
      std::string expStr = arrExp->toString();
      std::string actStr = arrAct->toString();
      if (expStr.find("[]") == std::string::npos &&
          actStr.find("[]") == std::string::npos) {
        if (expStr != actStr)
          return false;
      }
      return true;
    }
  }

  // Recursive Function Type Compatibility
  if (auto fnExp = dynamic_cast<const FunctionType *>(rawExp)) {
    if (auto fnAct = dynamic_cast<const FunctionType *>(rawAct)) {
      if (fnExp->getParamTypes().size() != fnAct->getParamTypes().size())
        return false;
      if (!isCompatible(fnExp->getReturnType(), fnAct->getReturnType()))
        return false;
      for (size_t i = 0; i < fnExp->getParamTypes().size(); ++i) {
        if (!isCompatible(fnExp->getParamTypes()[i].get(),
                          fnAct->getParamTypes()[i].get()))
          return false;
      }
      return true;
    }
  }

  // Recursive Map/Table Compatibility
  if (auto mapExp = dynamic_cast<const MapType *>(rawExp)) {
    if (auto mapAct = dynamic_cast<const MapType *>(rawAct)) {
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
  if (auto nullType = dynamic_cast<const NullableType *>(rawExp)) {
    if (rawAct->is<NullType>())
      return true;
    if (auto actualNull = dynamic_cast<const NullableType *>(rawAct)) {
      return isCompatible(nullType->getInner(), actualNull->getInner());
    }
    return isCompatible(nullType->getInner(), rawAct);
  }

  // Expanded Pointer Logic
  if (auto ptrExp = dynamic_cast<const PointerType *>(rawExp)) {
    if (auto ptrAct = dynamic_cast<const PointerType *>(rawAct)) {
      return isCompatible(ptrExp->getPointee(), ptrAct->getPointee());
    }
    if (ptrExp->getPointee()->is<PrimitiveType>() &&
        ((const PrimitiveType *)ptrExp->getPointee())->getScalar() ==
            PrimitiveType::Scalar::Void) {
      return rawAct->is<PointerType>();
    }
  }

  if (auto refType = dynamic_cast<const ReferenceType *>(rawExp)) {
    return isCompatible(refType->getInner(), rawAct);
  }

  if (auto refAct = dynamic_cast<const ReferenceType *>(rawAct)) {
    return isCompatible(rawExp, refAct->getInner());
  }

  if (isIntegerOrChar(expected) && isIntegerOrChar(actual)) {
    return true;
  }

  // Allow Primitive = Function returning that Primitive
  if (auto fn = dynamic_cast<const FunctionType *>(actual)) {
    if (isCompatible(expected, fn->getReturnType()))
      return true;
  }

  // Float promotion
  if (expected->isFloat() && (actual->isFloat() || isIntegerOrChar(actual))) {
    return true;
  }

  // Class Inheritance Checking
  if (auto expectedClass = dynamic_cast<const NamedType *>(expected)) {
    if (auto actualClass = dynamic_cast<const NamedType *>(actual)) {
      if (isSubclassOf(context.lookupClass(actualClass->getName()),
                       expectedClass->getName())) {
        return true;
      }
    }
  }

  return false;
}

bool TypeChecker::isCastAllowed(const Type *src, const Type *dst) {
  src = unwrapConcurrency(src);
  dst = unwrapConcurrency(dst);

  // 1. Allow casting between the same types
  if (src->isEquivalent(*dst))
    return true;

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

  if (auto named = dynamic_cast<const NamedType *>(t)) {
    // 1. CYCLE DETECTED!
    if (visited.count(named->getName()))
      return true;

    // 2. Value classes are inline; recursively explore their fields
    const ClassDecl *cls = context.lookupClass(named->getName());
    if (cls && !cls->isReferenceType()) {
      visited.insert(named->getName());
      for (const auto &member : cls->getMembers()) {
        if (auto varDecl = dynamic_cast<const VariableDecl *>(member.get())) {
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
  } else if (auto arr = dynamic_cast<const ArrayType *>(t)) {
    // 4. Fixed-size arrays (T[10]) are inline and must be checked.
    // Dynamic arrays (T[]) are heap pointers and break the cycle!
    if (arr->getSizeExpr() != nullptr) {
      return detectInfiniteSize(arr->getElementType(), visited);
    }
    return false;
  } else if (dynamic_cast<const NullableType *>(t)) {
    // 5. Nullables (T?) are boxed/heap-allocated pointers, breaking the cycle!
    return false;
  }

  // PointerType and ReferenceType return false implicitly here,
  // safely breaking the cycle because pointers have a known, fixed memory size!
  return false;
}

// --- ASTVisitor Overrides: Expressions ---

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
}

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
}

void TypeChecker::visitStringLiteral(const StringLiteral *) {
  lastComputedType = context.getStringType();
}

void TypeChecker::visitBoolLiteral(const BoolLiteral *) {
  lastComputedType = context.getBoolType();
}

void TypeChecker::visitNullLiteral(const NullLiteral *) {
  lastComputedType = context.getNullType();
}

void TypeChecker::visitCharLiteral(const CharLiteral *) {
  lastComputedType = context.getCharType();
}

void TypeChecker::visitArrayLiteral(const ArrayLiteral *expr) {
  const Type *expectedElemType = nullptr;

  // Extract expected element type from the LHS declaration if it exists
  if (currentExpectedReturnType) {
    if (auto arrT =
            dynamic_cast<const ArrayType *>(currentExpectedReturnType)) {
      expectedElemType = arrT->getElementType();
    }
  }

  // Push the expected element type down to the children so nested arrays
  // validate correctly
  const Type *previousExpected = currentExpectedReturnType;
  currentExpectedReturnType = expectedElemType;

  const Type *commonType = nullptr;
  for (const auto &elem : expr->getElements()) {
    elem->accept(*this);
    const Type *elemType = lastComputedType;

    // Strictly enforce element types against the expected type
    if (expectedElemType && !isCompatible(expectedElemType, elemType)) {
      Diags.report(elem->getLoc(), DiagID::err_type_mismatch)
          << "Array element type mismatch. Expected "
          << expectedElemType->toString() << " but found "
          << elemType->toString();
      hasError = true;
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

  lastComputedType =
      context.createArrayType(commonType, expr->getElements().size());
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
      if (auto varDecl = dynamic_cast<const VariableDecl *>(sym->decl)) {
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

  if (lhsType->is<AnyType>() || rhsType->is<AnyType>()) {
    lastComputedType = context.getAnyType();
    return;
  }

  if (op == TokenKind::Equal) {
    if (lhsType->getKind() == TypeKind::Lock ||
        lhsType->getKind() == TypeKind::View) {
      std::string mod =
          (lhsType->getKind() == TypeKind::Lock) ? "lock" : "view";
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Cannot assign to read-only '" << mod << "' variable";
      hasError = true;
      return;
    }

    // [FIX] Unwrap MemberExpr (d.val) and IndexExpr (arr[0]) to find base
    // identifier
    const Expr *target = expr->getLHS();
    while (true) {
      if (auto mem = dynamic_cast<const MemberExpr *>(target))
        target = mem->getObject();
      else if (auto idx = dynamic_cast<const IndexExpr *>(target))
        target = idx->getArray();
      else
        break;
    }

    // Now safely register the base identifier as definitely assigned!
    if (auto id = dynamic_cast<const IdentifierExpr *>(target)) {
      Symbol *sym = symbols.lookup(id->getName());
      if (sym && sym->decl) {
        if (auto varDecl = dynamic_cast<const VariableDecl *>(sym->decl)) {
          if (varDecl->isConstVar()) {
            Diags.report(expr->getLoc(), DiagID::err_const_violation)
                << "'" << id->getName() << "'";
            hasError = true;
          }
        }
        initializedVars.insert(sym->decl); // Struct is now initialized!
      }
    }
  }

  // Null Coalescing Operator (??)
  if (expr->getOp() == TokenKind::QuestionQuestion) {
    if (auto nullType = dynamic_cast<const NullableType *>(lhsType)) {
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
  if (auto namedLhs = dynamic_cast<const NamedType *>(lhsType)) {
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
            if (auto fn = dynamic_cast<const FunctionDecl *>(member.get())) {
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
                dynamic_cast<const IntegerLiteral *>(expr->getRHS())) {
          uint64_t shiftVal = literalRHS->getValue();
          uint64_t maxBits = 32; // Default for standard int

          if (auto primLHS = dynamic_cast<const PrimitiveType *>(rawLHS)) {
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
}

void TypeChecker::visitUnaryExpr(const UnaryExpr *expr) {
  TokenKind op = expr->getOp(); // [FIX] Declare op here to resolve scope error
  expr->getOperand()->accept(*this);
  const Type *operandType = lastComputedType;

  // 1. Unary Operator Overloading Resolution
  if (auto namedLhs = dynamic_cast<const NamedType *>(operandType)) {
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
        const ClassDecl *current = cls;
        while (current && !bestMatch) {
          for (const auto &member : current->getMembers()) {
            if (auto fn = dynamic_cast<const FunctionDecl *>(member.get())) {
              if (fn->getName() == opName && fn->getParams().empty()) {
                bestMatch = fn;
                break;
              }
            }
          }
          current = current->getParentNames().empty()
                        ? nullptr
                        : context.lookupClass(current->getParentNames()[0]);
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
    lastComputedType = context.createPointerType(operandType);
  } else if (op == TokenKind::Star) {
    // [FIX] Dereference: T* -> T
    const Type *raw = unwrapConcurrency(operandType);
    if (auto ptr = dynamic_cast<const PointerType *>(raw)) {
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
}

void TypeChecker::visitCallExpr(const CallExpr *expr) {
  // 1. Functional Casts (e.g., int(x))
  if (auto idExpr = dynamic_cast<const IdentifierExpr *>(expr->getCallee())) {
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
  if (dynamic_cast<const SuperExpr *>(expr->getCallee())) {
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

  // 3. Member Function Overload Resolution (e.g., p.printData(42))
  if (auto memExpr = dynamic_cast<const MemberExpr *>(expr->getCallee())) {
    memExpr->getObject()->accept(*this);
    const Type *objType = lastComputedType;

    if (auto namedType = dynamic_cast<const NamedType *>(objType)) {
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
              substitutions[typeParams[i]] = genArgs[i].type.get();
          }
        }

        const FunctionDecl *bestMatch = nullptr;
        GenericResolver::ConcreteSignature bestSig;
        const ClassDecl *current = cls;

        // Scan the class and its parents for the matching overload
        while (current && !bestMatch) {
          for (const auto &member : current->getMembers()) {
            if (auto fn = dynamic_cast<const FunctionDecl *>(member.get())) {
              if (fn->getName() == memExpr->getName()) {
                if (!fn->isVariadicFunc() &&
                    fn->getParams().size() != argTypes.size())
                  continue;

                auto sig = resolver.resolveFunctionSignature(fn, substitutions);
                bool match = true;
                size_t limit = std::min(argTypes.size(), sig.paramTypes.size());
                for (size_t i = 0; i < limit; ++i) {
                  if (!isCompatible(sig.paramTypes[i].get(), argTypes[i])) {
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
          current = current->getParentNames().empty()
                        ? nullptr
                        : context.lookupClass(current->getParentNames()[0]);
        }

        if (bestMatch) {
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
  if (auto idExpr = dynamic_cast<const IdentifierExpr *>(expr->getCallee())) {
    std::string funcName = idExpr->getName();

    if (funcName.find("atomic_") == 0) {
      // 1. Validate Memory Ordering Logic
      if (funcName.find("atomic_cas") == 0) {
        if (expr->getArgs().size() >= 5) {
          auto succStr =
              dynamic_cast<const StringLiteral *>(expr->getArgs()[3].get());
          auto failStr =
              dynamic_cast<const StringLiteral *>(expr->getArgs()[4].get());
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
                dynamic_cast<const UnaryExpr *>(expr->getArgs()[0].get())) {
          if (unExpr->getOp() == TokenKind::Amp) {
            if (auto id = dynamic_cast<const IdentifierExpr *>(
                    unExpr->getOperand())) {
              Symbol *sym = symbols.lookup(id->getName());
              if (sym && sym->decl &&
                  sym->decl->getKind() == StmtKind::VariableDecl) {
                auto varDecl = static_cast<const VariableDecl *>(sym->decl);
                if (varDecl->alignment > 0 && varDecl->alignment < 4) {
                  Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
                      << "Misaligned atomic access: Variable '"
                      << varDecl->getName() << "' has explicit alignment "
                      << varDecl->alignment
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
        if (auto ptrT = dynamic_cast<const PointerType *>(
                unwrapConcurrency(argTypes[0]))) {
          lastComputedType = ptrT->getPointee();
        } else {
          lastComputedType = context.getAnyType();
        }
      } else if (funcName.find("atomic_cas") == 0 || funcName == "atomic_add") {
        if (!argTypes.empty()) {
          if (auto ptrT = dynamic_cast<const PointerType *>(
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
      return; // <-- BYPASS STANDARD OVERLOAD RESOLUTION!
    }

    Symbol *sym = symbols.lookup(idExpr->getName());

    // Intercept variables of type 'any' before doing overload resolution
    if (sym && sym->kind == SymbolKind::Variable && sym->type &&
        sym->type->is<AnyType>()) {
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
              dynamic_cast<const FunctionDecl *>(genericDecl->getInnerDecl());
          if (!innerFunc)
            continue;

          llvm::StringMap<const Type *> substitutions;
          const auto &typeParams = genericDecl->getTypeParams();

          // Infer types (T = string, T = i32, etc.)
          auto inferTypes = [&](const Type *expected, const Type *actual,
                                auto &self) -> void {
            if (!expected || !actual)
              return;
            if (auto named = dynamic_cast<const NamedType *>(expected)) {
              if (std::find(typeParams.begin(), typeParams.end(),
                            named->getName()) != typeParams.end()) {
                if (!substitutions.count(named->getName()))
                  substitutions[named->getName()] = actual;
                return;
              }
            }
            if (auto expPtr = dynamic_cast<const PointerType *>(expected)) {
              if (auto actPtr = dynamic_cast<const PointerType *>(actual)) {
                self(expPtr->getPointee(), actPtr->getPointee(), self);
              }
            }
            if (auto expArr = dynamic_cast<const ArrayType *>(expected)) {
              if (auto actArr = dynamic_cast<const ArrayType *>(actual))
                self(expArr->getElementType(), actArr->getElementType(), self);
            } else if (auto expRef =
                           dynamic_cast<const ReferenceType *>(expected)) {
              if (auto actRef = dynamic_cast<const ReferenceType *>(actual))
                self(expRef->getInner(), actRef->getInner(), self);
              else
                self(expRef->getInner(), actual, self);
            }

            if (auto expFunc = dynamic_cast<const FunctionType *>(expected)) {
              if (auto actFunc = dynamic_cast<const FunctionType *>(actual)) {
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

          if (expr->getArgs().size() != sig.paramTypes.size())
            continue;

          bool match = true;
          for (size_t i = 0; i < sig.paramTypes.size(); ++i) {
            if (!isCompatible(sig.paramTypes[i].get(), argTypes[i])) {
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

          if (!fn->isVariadicFunc() &&
              fn->getParams().size() != argTypes.size())
            continue;

          bool match = true;
          size_t limit = std::min(argTypes.size(), fn->getParams().size());
          for (size_t i = 0; i < limit; ++i) {
            if (!isCompatible(fn->getParams()[i].type.get(), argTypes[i])) {
              match = false; // Argument type mismatch
              break;
            }
          }

          if (match) {
            // Store the match instead of breaking!
            validMatches.push_back({fn->getReturnType(), {}});
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
        lastComputedType = validMatches[0].returnType;
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

  if (auto *fnType = dynamic_cast<const FunctionType *>(calleeType)) {
    const auto &params = fnType->getParamTypes();

    if (!fnType->isVariadicFunc() && argTypes.size() != params.size()) {
      Diags.report(expr->getLoc(), DiagID::err_argument_count_mismatch);
      hasError = true;
    }

    size_t limit = std::min(argTypes.size(), params.size());
    for (size_t i = 0; i < limit; ++i) {
      if (!isCompatible(params[i].get(), argTypes[i])) {
        Diags.report(expr->getArgs()[i]->getLoc(), DiagID::err_type_mismatch)
            << "Argument type mismatch";
        hasError = true;
      }
    }
    lastComputedType = fnType->getReturnType();
  } else if (calleeType->is<AnyType>()) {
    lastComputedType = context.getAnyType();
  } else {
    Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
        << "Called object is not a function";
    hasError = true;
    lastComputedType = context.getAnyType();
  }
}

void TypeChecker::visitIndexExpr(const IndexExpr *expr) {
  expr->getArray()->accept(*this);
  const Type *arrType = lastComputedType;

  expr->getIndex()->accept(*this);
  const Type *idxType = lastComputedType;

  if (auto *at = dynamic_cast<const ArrayType *>(arrType)) {
    if (!isIntegerOrChar(idxType)) {
      Diags.report(expr->getIndex()->getLoc(), DiagID::err_type_mismatch)
          << "Array index must be integer";
    }
    lastComputedType = at->getElementType();
  } else if (auto *mt = dynamic_cast<const MapType *>(arrType)) {
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
}

void TypeChecker::visitMemberExpr(const MemberExpr *expr) {
  expr->getObject()->accept(*this);
  const Type *objType = lastComputedType;

  // Handle Namespace/Module access (e.g., io.open)
  if (auto id = dynamic_cast<const IdentifierExpr *>(expr->getObject())) {
    if (Symbol *sym = symbols.lookup(id->getName())) {
      if (sym->kind == SymbolKind::Module) {
        lastComputedType = context.getAnyType(); // Allow any access on modules
        return;
      }
    }
  }

  // Unwrap Nullable Types if using '?.'
  bool isNullableAccess = false;
  if (auto nullType = dynamic_cast<const NullableType *>(objType)) {
    if (!expr->isOptionalAccess()) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Cannot access member of nullable type without '?.'";
      hasError = true;
    }
    objType = nullType->getInner();
    isNullableAccess = true;
  }

  if (objType->is<AnyType>()) {
    lastComputedType = context.getAnyType();
    return;
  }

  const Type *resultType = context.getAnyType();

  // Handle Class/Struct member lookup
  if (auto namedType = dynamic_cast<const NamedType *>(objType)) {
    const ClassDecl *cls = context.lookupClass(namedType->getName());
    if (cls) {
      llvm::StringMap<const Type *> substitutions;
      if (!namedType->getGenericArgs().empty()) {
        Symbol *sym = symbols.lookup(namedType->getName());
        if (sym && sym->decl && sym->decl->getKind() == StmtKind::GenericDecl) {
          auto gd = static_cast<const GenericDecl *>(sym->decl);
          const auto &typeParams = gd->getTypeParams();
          const auto &genArgs = namedType->getGenericArgs();
          size_t limit = std::min(typeParams.size(), genArgs.size());
          for (size_t i = 0; i < limit; ++i) {
            substitutions[typeParams[i]] = genArgs[i].type.get();
          }
        }
      }

      const ClassDecl *current = cls;
      bool found = false;
      while (current) {
        for (const auto &member : current->getMembers()) {
          if (auto varDecl = dynamic_cast<const VariableDecl *>(member.get())) {
            if (varDecl->getName() == expr->getName()) {
              if (!checkVisibility(varDecl, current, expr->getLoc())) {
                hasError = true;
              }
              auto subType =
                  resolver.substituteType(varDecl->getType(), substitutions);
              resultType = subType.get();
              parkedTypes.push_back(std::move(subType));
              found = true;
              break;
            }
          } else if (auto fnDecl =
                         dynamic_cast<const FunctionDecl *>(member.get())) {
            if (fnDecl->getName() == expr->getName()) {
              if (!checkVisibility(fnDecl, current, expr->getLoc())) {
                hasError = true;
              }
              std::vector<const Type *> pTypes;
              for (auto &p : fnDecl->getParams()) {
                auto subParam =
                    resolver.substituteType(p.type.get(), substitutions);
                pTypes.push_back(subParam.get());
                parkedTypes.push_back(std::move(subParam));
              }
              auto subRet = resolver.substituteType(fnDecl->getReturnType(),
                                                    substitutions);
              const Type *retType = subRet.get();
              parkedTypes.push_back(std::move(subRet));

              resultType = context.createFunctionType(pTypes, retType);
              found = true;
              break;
            }
          }
        }
        if (found)
          break;
        current = current->getParentNames().empty()
                      ? nullptr
                      : context.lookupClass(current->getParentNames()[0]);
      }

      if (!found) {
        Diags.report(expr->getLoc(), DiagID::err_no_member)
            << expr->getName() << namedType->getName();
        resultType = context.getAnyType();
      }
    }
  } else {
    // Primitive methods
    if (objType->isString() && expr->getName() == "length") {
      resultType = context.getI32Type();
    } else if (dynamic_cast<const ArrayType *>(objType)) {
      if (expr->getName() == "length" || expr->getName() == "size")
        resultType = context.getI32Type();
    } else {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Accessing member of non-aggregate type";
      resultType = context.getAnyType();
    }
  }

  // [NEW] If accessed via `?.`, the resulting type is now Nullable!
  if (isNullableAccess && resultType && !resultType->is<AnyType>() &&
      !resultType->is<NullableType>()) {
    lastComputedType = context.createNullableType(resultType);
  } else {
    lastComputedType = resultType;
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
}

void TypeChecker::visitNewExpr(const NewExpr *expr) {
  if (expr->getType())
    expr->getType()->accept(*this);

  const Type *type = expr->getType();
  if (auto named = dynamic_cast<const NamedType *>(type)) {
    const ClassDecl *cls = context.lookupClass(named->getName());
    if (!cls) {
      Symbol *sym = symbols.lookup(named->getName());

      // [FIX] Handle new Box<T>() where Box is a GenericDecl
      if (sym && sym->decl && sym->decl->getKind() == StmtKind::GenericDecl) {
        auto gd = static_cast<const GenericDecl *>(sym->decl);
        cls = dynamic_cast<const ClassDecl *>(gd->getInnerDecl());
      }
      // ADD THIS to handle standard imported classes!
      else if (sym && sym->decl &&
               sym->decl->getKind() == StmtKind::ClassDecl) {
        cls = static_cast<const ClassDecl *>(sym->decl);
      }

      // Existing fallback for standard opaque imports
      if (!cls && sym && sym->kind == SymbolKind::Type) {
        lastComputedType = expr->getType();
        return;
      }

      if (!cls) {
        Diags.report(expr->getLoc(), DiagID::err_unknown_type)
            << named->getName();
        lastComputedType = context.getAnyType();
        return;
      }
    }

    // Capture Generic Substitutions (e.g., mapping T = string)
    llvm::StringMap<const Type *> substitutions;
    if (!named->getGenericArgs().empty()) {
      Symbol *sym = symbols.lookup(named->getName());
      if (sym && sym->decl && sym->decl->getKind() == StmtKind::GenericDecl) {
        auto gd = static_cast<const GenericDecl *>(sym->decl);
        const auto &typeParams = gd->getTypeParams();
        const auto &genArgs = named->getGenericArgs();
        size_t limit = std::min(typeParams.size(), genArgs.size());
        for (size_t i = 0; i < limit; ++i) {
          substitutions[typeParams[i]] = genArgs[i].type.get();
        }
      }
    }

    // Check for constructor
    const FunctionDecl *ctor = nullptr;
    const ClassDecl *current = cls;
    while (current && !ctor) {
      for (const auto &member : current->getMembers()) {
        if (auto fn = dynamic_cast<const FunctionDecl *>(member.get())) {
          if (fn->getName() == "constructor") {
            // Check visibility of the constructor itself!
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
        auto sig = resolver.resolveFunctionSignature(ctor, substitutions);
        for (size_t i = 0; i < expr->getArgs().size(); ++i) {
          expr->getArgs()[i]->accept(*this);
          if (!isCompatible(sig.paramTypes[i].get(), lastComputedType)) {
            Diags.report(expr->getArgs()[i]->getLoc(),
                         DiagID::err_type_mismatch)
                << "Constructor argument mismatch";
          }
        }
      }
    }
  }

  lastComputedType = expr->getType();
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

  // Use the hint from the LHS (if any), otherwise default to Any for inference
  if (!currentExpectedReturnType || currentExpectedReturnType->isVoid() ||
      currentExpectedReturnType->is<AnyType>()) {
    currentExpectedReturnType = context.getAnyType();
  }

  const Type *inferredReturnType = context.getVoidType();
  if (expr->getBody()) {
    expr->getBody()->accept(*this);

    // If a hint was provided by the LHS (e.g. 'int'), use it as the
    // return
    if (prevRet && !prevRet->is<AnyType>()) {
      inferredReturnType = prevRet;
    } else if (lastComputedType && !lastComputedType->is<AnyType>()) {
      inferredReturnType = lastComputedType;
    }
  }

  symbols.exitScope();
  currentExpectedReturnType = prevRet;

  // 3. Bake the inferred return type into the lambda's FunctionType signature
  lastComputedType = context.createFunctionType(paramTypes, inferredReturnType);
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
}

void TypeChecker::visitThreadExpr(const ThreadExpr *expr) {
  expr->getBody()->accept(*this);
  lastComputedType = context.createNamedType("Thread");
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
}

void TypeChecker::visitAwaitExpr(const AwaitExpr *expr) {
  expr->getExpr()->accept(*this);
  const Type *type = lastComputedType;

  if (auto named = dynamic_cast<const NamedType *>(type)) {
    if (named->getName() == "Promise" && !named->getGenericArgs().empty()) {
      // [FIX] Correctly access type pointer from GenericArg struct
      lastComputedType = named->getGenericArgs()[0].type.get();
      return;
    }
  }
  lastComputedType = type;
}

void TypeChecker::visitSizeOfExpr(const SizeOfExpr *expr) {
  // Ensure the expression inside sizeof() is valid
  expr->getExpr()->accept(*this);

  // Sizeof always evaluates to the platform's unsigned pointer size
  lastComputedType = context.getUSizeType();
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

  // --- 1. [NEW] OS-Level Bitfield Validation ---
  if (decl->bitWidth != -1) {
    if (!varType->isInteger()) {
      Diags.report(decl->getLoc(), DiagID::err_type_mismatch)
          << "Bitfields must have an integer type; found '"
          << varType->toString() << "'";
      hasError = true;
    } else {
      // [FIX] Optional: Add a check if bitWidth exceeds the actual bit-size of
      // the primitive type
      if (auto prim = dynamic_cast<const PrimitiveType *>(varType)) {
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

        if (maxBits > 0 && decl->bitWidth > maxBits) {
          Diags.report(decl->getLoc(), DiagID::err_type_mismatch)
              << "Bitfield width " << decl->bitWidth
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
  } else if (decl->isExtern) {
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

    // Compatibility check
    if (!isCompatible(varType, lastComputedType)) {
      bool customReported = false;

      // Check for Array Length Mismatch
      if (auto declArr = dynamic_cast<const ArrayType *>(varType)) {
        if (auto initArr = dynamic_cast<const ArrayType *>(lastComputedType)) {
          if (declArr->getSizeExpr() && initArr->getSizeExpr()) {
            auto dSize =
                dynamic_cast<const IntegerLiteral *>(declArr->getSizeExpr());
            auto iSize =
                dynamic_cast<const IntegerLiteral *>(initArr->getSizeExpr());
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
        if (lastComputedType->is<NullType>() && !varType->is<NullableType>() &&
            !varType->is<AnyType>()) {
          Diags.report(decl->getLoc(), DiagID::err_null_assignment)
              << " '" << varType->toString() << "'";
          hasError = true;
        } else {
          Diags.report(decl->getLoc(), DiagID::err_type_mismatch)
              << "Initializer type mismatch. Expected '" << varType->toString()
              << "' but found '" << lastComputedType->toString() << "'";
          hasError = true;
        }
      }
    }

    // If RHS is a function, store the full signature
    if (dynamic_cast<const FunctionType *>(lastComputedType)) {
      effectiveType = lastComputedType;
    }
  }

  // --- 4. Symbol Registration with OS Metadata ---
  if (!symbols.isDefinedInCurrentScope(decl->getName())) {
    Symbol sym(SymbolKind::Variable, decl->getName(), effectiveType, decl);

    // [CRITICAL] Transfer the bitWidth to the Symbol Table
    sym.bitWidth = decl->bitWidth;

    if (!symbols.addSymbol(decl->getName(), sym, decl->getLoc())) {
      Diags.report(decl->getLoc(), DiagID::err_symbol_redefinition)
          << decl->getName();
      hasError = true;
    }
  } else {
    // [FIX] Update the pre-registered global variable with the resolved type!
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

  currentExpectedReturnType = retType; // Use the resolved retType
  inStaticContext = decl->isStaticFunc();

  bool prevInterrupt = inInterruptContext;
  inInterruptContext = decl->isInterrupt;

  // Register all parameters as local variables inside the function scope
  for (size_t i = 0; i < decl->getParams().size(); ++i) {
    const auto &param = decl->getParams()[i];
    Symbol pSym(SymbolKind::Variable, param.name,
                pTypes[i]); // Use resolved pTypes
    symbols.addSymbol(param.name, pSym, decl->getLoc());
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

  if (auto arr = dynamic_cast<const ArrayType *>(colType)) {
    valType = arr->getElementType();
  } else if (auto map = dynamic_cast<const MapType *>(colType)) {
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

  if (auto namedCond = dynamic_cast<const NamedType *>(condType)) {
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
        if (auto memExpr = dynamic_cast<const MemberExpr *>(val.get())) {
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
        hasError = true;
        missingCases = true;
      }
    }
    // If no cases are missing, the enum is exhaustive even without a
    // default!
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

// --- Top-Level Declarations ---

void TypeChecker::visitModuleDecl(const ModuleDecl *decl) {
  for (const auto &d : decl->getDecls()) {
    if (auto fd = dynamic_cast<const FunctionDecl *>(d.get())) {
      std::vector<const Type *> pTypes;
      for (auto &p : fd->getParams())
        pTypes.push_back(p.type.get());
      const Type *fnType =
          context.createFunctionType(pTypes, fd->getReturnType());
      symbols.addSymbol(fd->getName(),
                        Symbol(SymbolKind::Function, fd->getName(), fnType, fd),
                        fd->getLoc());
    } else if (auto cd = dynamic_cast<const ClassDecl *>(d.get())) {
      symbols.addSymbol(cd->getName(),
                        Symbol(SymbolKind::Class, cd->getName(),
                               context.createNamedType(cd->getName()), cd),
                        cd->getLoc());
      context.registerClass(cd);
    } else if (auto gd = dynamic_cast<const GenericDecl *>(d.get())) {
      if (auto innerClass =
              dynamic_cast<const ClassDecl *>(gd->getInnerDecl())) {
        symbols.addSymbol(innerClass->getName(),
                          Symbol(SymbolKind::Class, innerClass->getName(),
                                 context.createNamedType(innerClass->getName()),
                                 gd),
                          gd->getLoc());
        context.registerClass(innerClass);
      } else if (auto innerFunc =
                     dynamic_cast<const FunctionDecl *>(gd->getInnerDecl())) {
        symbols.addSymbol(innerFunc->getName(),
                          Symbol(SymbolKind::Function, innerFunc->getName(),
                                 context.getAnyType(), gd),
                          gd->getLoc());
      }
    } else if (auto vd = dynamic_cast<const VariableDecl *>(d.get())) {
      symbols.addSymbol(
          vd->getName(),
          Symbol(SymbolKind::Variable, vd->getName(), vd->getType(), vd),
          vd->getLoc());
    }
  }

  // [FIX] Pass 1.5: Evaluate aliases BEFORE type-checking variables
  for (const auto &d : decl->getDecls()) {
    if (auto ud = dynamic_cast<const UsingDecl *>(d.get())) {
      ud->accept(*this);
    }
  }

  // Pass 2: Type checking
  for (const auto &d : decl->getDecls()) {
    // [FIX] Skip UsingDecl as it's already processed
    if (!dynamic_cast<const UsingDecl *>(d.get())) {
      d->accept(*this);
    }
  }
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
      if (auto varDecl = dynamic_cast<const VariableDecl *>(member.get())) {
        if (detectInfiniteSize(varDecl->getType(), visited)) {
          Diags.report(varDecl->getLoc(), DiagID::err_infinite_size)
              << "due to recursive field '" << varDecl->getName() << "'";
          hasError = true;
        }
      }
    }
  }

  // Key is pair of <MethodName, TypeString> to distinguish overloads
  std::map<std::pair<std::string, std::string>, std::vector<std::string>>
      inheritedSignatures;

  for (const auto &pName : decl->getParentNames()) {
    // Look up the parent class in the context
    if (const ClassDecl *parentDecl = context.lookupClass(pName)) {

      // 1. Check Ref vs Value mismatch
      if (decl->isReferenceType() != parentDecl->isReferenceType()) {
        Diags.report(decl->getLoc(), DiagID::err_type_mismatch)
            << "Inheritance violation: Class '" << decl->getName()
            << "' and its parent '" << parentDecl->getName()
            << "' must both be 'ref' classes or both be value classes";
        hasError = true;
      }

      // 2. Gather methods using full signatures to support overloading
      for (const auto &member : parentDecl->getMembers()) {
        if (auto fd = dynamic_cast<const FunctionDecl *>(member.get())) {
          // Construct a unique signature string: "name(type1,type2,...)"
          std::string signature = fd->getName() + "(";
          for (size_t i = 0; i < fd->getParams().size(); ++i) {
            signature += fd->getParams()[i].type->toString();
            if (i < fd->getParams().size() - 1)
              signature += ",";
          }
          signature += ")";

          // Use inheritedSignatures instead of inheritedMethods
          inheritedSignatures[{fd->getName(), signature}].push_back(
              parentDecl->getName());
        }
      }
    } else {
      Diags.report(decl->getLoc(), DiagID::err_unknown_type) << pName;
      hasError = true;
    }
  }

  // 3. Enforce Diamond Conflict Resolution per unique signature
  for (const auto &[key, parents] : inheritedSignatures) {
    if (parents.size() > 1) { // Same signature inherited from multiple parents
      const std::string &fullSig = key.second;

      bool childOverrides = false;
      for (const auto &member : decl->getMembers()) {
        if (auto fd = dynamic_cast<const FunctionDecl *>(member.get())) {
          // Re-generate signature for child method to compare
          std::string childSig = fd->getName() + "(";
          for (size_t i = 0; i < fd->getParams().size(); ++i) {
            childSig += fd->getParams()[i].type->toString();
            if (i < fd->getParams().size() - 1)
              childSig += ",";
          }
          childSig += ")";

          if (childSig == fullSig) {
            childOverrides = true;
            break;
          }
        }
      }

      if (!childOverrides) {
        Diags.report(decl->getLoc(), DiagID::err_ambiguous_inheritance)
            << "Method '" << fullSig << "' is inherited from multiple parents ("
            << parents[0] << ", " << parents[1] << "). Class '"
            << decl->getName() << "' must override it.";
        hasError = true;
      }
    }
  }

  const ClassDecl *prevClass = currentClassDecl;
  currentClassDecl = decl;

  symbols.enterScope(ScopeKind::Class);

  // PRE-REGISTRATION PASS: Register all members so methods can see each other
  for (const auto &member : decl->getMembers()) {
    if (auto vd = dynamic_cast<const VariableDecl *>(member.get())) {
      Symbol sym(SymbolKind::Variable, vd->getName(), vd->getType(), vd);
      if (!symbols.addSymbol(vd->getName(), sym, vd->getLoc())) {
        Diags.report(vd->getLoc(), DiagID::err_symbol_redefinition)
            << vd->getName();
        hasError = true;
      }
    } else if (auto fd = dynamic_cast<const FunctionDecl *>(member.get())) {
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

  symbols.exitScope();
  currentClassDecl = prevClass;
}

void TypeChecker::visitGenericDecl(const GenericDecl *decl) {
  symbols.enterScope(ScopeKind::Class);
  for (const auto &param : decl->getTypeParams()) {
    symbols.addSymbol(
        param, Symbol(SymbolKind::Type, param, context.createNamedType(param)),
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
  lastComputedType = t;
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
void TypeChecker::visitNamedType(const NamedType *t) {
  // 1. Recurse into generic arguments to validate them
  for (const auto &arg : t->getGenericArgs()) {
    if (arg.type)
      arg.type->accept(*this);
  }

  // 2. Resolve the actual type and set lastComputedType
  Symbol *sym = symbols.lookup(t->getName());
  const ClassDecl *cls = context.lookupClass(t->getName());

  if (!sym && !cls) {
    Diags.report(t->getLoc(), DiagID::err_unknown_type) << t->getName();
    hasError = true;
    lastComputedType = context.getAnyType();
    return;
  }

  // [FIX] Set the type, but DO NOT early return. Fall through to validation!
  if (sym && sym->decl && sym->decl->getKind() == StmtKind::GenericDecl) {
    lastComputedType = t; // Preserve the generic arguments!
  } else if (sym && sym->type) {
    lastComputedType = sym->type;
  } else if (cls) {
    lastComputedType = context.createNamedType(cls->getName());
  }

  // 3. Validate Constraints (e.g., no 'any' allowed)
  if (!t->getGenericArgs().empty() && sym) {
    if (sym->decl && sym->decl->getKind() == StmtKind::GenericDecl) {
      auto genericDecl = static_cast<const GenericDecl *>(sym->decl);

      auto error = resolver.validateGenericArgs(genericDecl->getTypeParams(),
                                                t->getGenericArgs());

      if (error == GenericError::ConstraintViolation) {
        Diags.report(t->getLoc(), DiagID::err_generic_constraint)
            << "Cannot use 'any' as a generic argument in specialized type '"
            << t->getName() << "'";
        hasError = true;
      } else if (error == GenericError::ArityMismatch) {
        Diags.report(t->getLoc(), DiagID::err_generic_arity)
            << "Wrong number of generic arguments for '" << t->getName() << "'";
        hasError = true;
      }
    }
  }
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
void TypeChecker::visitEnumType(const EnumType *t) { lastComputedType = t; }
void TypeChecker::visitNullType(const NullType *t) { lastComputedType = t; }
void TypeChecker::visitVolatileType(const VolatileType *t) {
  t->getInner()->accept(*this);
  lastComputedType = t;
}
void TypeChecker::visitConstType(const ConstType *t) {
  t->getInner()->accept(*this);
  lastComputedType = t;
}
} // namespace moksha
