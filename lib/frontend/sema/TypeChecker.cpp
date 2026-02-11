#include "moksha/Sema/TypeChecker.h"
#include "moksha/AST/ASTContext.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "moksha/Sema/GenericResolver.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {

TypeChecker::TypeChecker(ASTContext &ctx, SymbolTable &sym,
                         DiagnosticEngine &diags)
    : context(ctx), symbols(sym), Diags(diags), resolver(ctx),
      lastComputedType(nullptr), currentExpectedReturnType(nullptr),
      hasError(false), loopDepth(0) {}

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
  if (expected->isEquivalent(*actual))
    return true;

  if (expected->is<AnyType>() || actual->is<AnyType>())
    return true;

  // Nullable Logic (Covariant)
  if (auto nullType = dynamic_cast<const NullableType *>(expected)) {
    // 1. A nullable type accepts the literal 'null' (NullType)
    if (actual->is<NullType>()) {
      return true;
    }
    // 2. Nullable<T> accepts Nullable<U> if T accepts U
    if (auto actualNull = dynamic_cast<const NullableType *>(actual)) {
      return isCompatible(nullType->getInner(), actualNull->getInner());
    }
    // 3. A nullable type accepts its inner type (T? accepts T)
    return isCompatible(nullType->getInner(), actual);
  }

  // Arrays: T[] accepts T[N]
  if (auto a1 = dynamic_cast<const ArrayType *>(expected)) {
    if (auto a2 = dynamic_cast<const ArrayType *>(actual)) {
      if (!a1->getElementType()->isEquivalent(*a2->getElementType()))
        return false;

      // Size check (Slice accepts Fixed, but sizes must match if both present)
      if (!a1->getSizeExpr())
        return true;
      if (!a2->getSizeExpr())
        return false;

      auto i1 = dynamic_cast<const IntegerLiteral *>(a1->getSizeExpr());
      auto i2 = dynamic_cast<const IntegerLiteral *>(a2->getSizeExpr());
      if (i1 && i2)
        return i1->getValue() == i2->getValue();
      return false;
    }
  }

  // View<T> Logic
  if (auto v1 = dynamic_cast<const ViewType *>(expected)) {
    if (auto v2 = dynamic_cast<const ViewType *>(actual)) {
      return isCompatible(v1->getInner(), v2->getInner());
    }
  }

  return false;
}

static bool allPathsReturn(const Stmt *stmt) {
  if (!stmt)
    return false;

  if (dynamic_cast<const ReturnStmt *>(stmt))
    return true;

  if (auto block = dynamic_cast<const BlockStmt *>(stmt)) {
    for (const auto &s : block->getStatements()) {
      if (allPathsReturn(s.get()))
        return true;
    }
    return false;
  }

  if (auto ifStmt = dynamic_cast<const IfStmt *>(stmt)) {
    // Both branches must return
    return ifStmt->getElseStmt() && allPathsReturn(ifStmt->getThenStmt()) &&
           allPathsReturn(ifStmt->getElseStmt());
  }

  if (auto switchStmt = dynamic_cast<const SwitchStmt *>(stmt)) {
    bool hasDefault = false;
    for (const auto &c : switchStmt->getCases()) {
      if (c.isDefaultCase())
        hasDefault = true;
      if (!allPathsReturn(c.getBody()))
        return false;
    }
    return hasDefault;
  }

  return false;
}

bool TypeChecker::isCastAllowed(const Type *src, const Type *dst) {
  if (src->isEquivalent(*dst))
    return true;
  if (src->is<AnyType>() || dst->is<AnyType>())
    return true;

  // Numeric casts
  if (src->is<PrimitiveType>() && dst->is<PrimitiveType>())
    return true;

  // Pointer <-> Integer casts (unsafe but standard)
  bool srcPtr = src->is<PointerType>();
  bool dstPtr = dst->is<PointerType>();
  bool srcInt = src->is<PrimitiveType>() &&
                static_cast<const PrimitiveType *>(src)->isInteger();
  bool dstInt = dst->is<PrimitiveType>() &&
                static_cast<const PrimitiveType *>(dst)->isInteger();

  if ((srcPtr && dstInt) || (srcInt && dstPtr))
    return true;
  if (srcPtr && dstPtr)
    return true; // Ptr to Ptr

  return false;
}

const Type *TypeChecker::getCommonNumericType(const Type *t1, const Type *t2) {
  auto p1 = dynamic_cast<const PrimitiveType *>(t1);
  auto p2 = dynamic_cast<const PrimitiveType *>(t2);

  if (!p1 || !p2)
    return nullptr;

  auto s1 = p1->getScalar();
  auto s2 = p2->getScalar();

  // Floating point dominance
  if (s1 == PrimitiveType::Scalar::F64 || s2 == PrimitiveType::Scalar::F64)
    return context.getF64Type();
  if (s1 == PrimitiveType::Scalar::F32 || s2 == PrimitiveType::Scalar::F32)
    return context.getF32Type();

  // Helper: Get Rank
  auto getRank = [](PrimitiveType::Scalar s) {
    using S = PrimitiveType::Scalar;
    switch (s) {
    case S::I8:
      return 1;
    case S::U8:
      return 1;
    case S::I16:
      return 2;
    case S::U16:
      return 2;
    case S::I32:
    case S::Int:
      return 3;
    case S::U32:
      return 3;
    case S::I64:
      return 4;
    case S::U64:
      return 4;
    default:
      return 0;
    }
  };

  // Helper: Is Signed?
  auto isSigned = [](PrimitiveType::Scalar s) {
    using S = PrimitiveType::Scalar;
    return (s == S::I8 || s == S::I16 || s == S::I32 || s == S::Int ||
            s == S::I64);
  };

  int r1 = getRank(s1);
  int r2 = getRank(s2);
  if (r1 == 0 || r2 == 0)
    return nullptr;

  bool sign1 = isSigned(s1);
  bool sign2 = isSigned(s2);

  // If signs match, pick the larger rank
  if (sign1 == sign2) {
    return (r1 >= r2) ? t1 : t2;
  }

  int signedRank = sign1 ? r1 : r2;
  int unsignedRank = sign1 ? r2 : r1;
  const Type *signedType = sign1 ? t1 : t2;

  if (signedRank > unsignedRank) {
    return signedType;
  }

  return nullptr;
}

const Type *TypeChecker::getCommonSuperType(const Type *t1, const Type *t2) {
  if (!t1)
    return t2;
  if (!t2)
    return t1;

  if (t1->is<AnyType>() && !t2->is<AnyType>())
    return t2;
  if (t2->is<AnyType>() && !t1->is<AnyType>())
    return t1;

  // Nullable Promotion
  bool t1Null = t1->is<NullType>();
  bool t2Null = t2->is<NullType>();

  if (t1Null && t2Null)
    return t1; // Both null

  if (t1Null) {
    if (t2->is<NullableType>())
      return t2;
    if (t2->is<PrimitiveType>() &&
        static_cast<const PrimitiveType *>(t2)->getScalar() ==
            PrimitiveType::Scalar::Void)
      return nullptr;
    return context.createNullableType(const_cast<Type *>(t2));
  }

  if (t2Null) {
    if (t1->is<NullableType>())
      return t1;
    if (t1->is<PrimitiveType>() &&
        static_cast<const PrimitiveType *>(t1)->getScalar() ==
            PrimitiveType::Scalar::Void)
      return nullptr;
    return context.createNullableType(const_cast<Type *>(t1));
  }

  if (isCompatible(t1, t2))
    return t1;
  if (isCompatible(t2, t1))
    return t2;

  if (auto common = getCommonNumericType(t1, t2))
    return common;

  return nullptr;
}

// --- Expressions ---

void TypeChecker::visitIntegerLiteral(const IntegerLiteral *expr) {
  lastComputedType = context.getI32Type();
}
void TypeChecker::visitFloatLiteral(const FloatLiteral *expr) {
  lastComputedType = context.getF32Type();
}
void TypeChecker::visitStringLiteral(const StringLiteral *expr) {
  lastComputedType = context.getStringType();
}
void TypeChecker::visitBoolLiteral(const BoolLiteral *expr) {
  lastComputedType = context.getBoolType();
}
void TypeChecker::visitNullLiteral(const NullLiteral *expr) {
  lastComputedType = context.getNullType();
}
void TypeChecker::visitCharLiteral(const CharLiteral *expr) {
  lastComputedType = context.getCharType();
}

void TypeChecker::visitArrayLiteral(const ArrayLiteral *expr) {
  const auto &elements = expr->getElements();
  if (elements.empty()) {
    lastComputedType = context.getAnyType();
    return;
  }

  elements[0]->accept(*this);
  const Type *elemType = lastComputedType;

  for (size_t i = 1; i < elements.size(); ++i) {
    elements[i]->accept(*this);
    if (!isCompatible(elemType, lastComputedType)) {
      const Type *common = getCommonSuperType(elemType, lastComputedType);
      if (common) {
        elemType = common;
      } else {
        Diags.report(elements[i]->getLoc(), DiagID::err_type_mismatch)
            << "Inconsistent array element types";
        hasError = true;
        lastComputedType = context.getAnyType();
        return;
      }
    }
  }

  lastComputedType = context.createArrayType(elemType, elements.size());
}

void TypeChecker::visitIdentifierExpr(const IdentifierExpr *expr) {
  Symbol *sym = symbols.lookup(expr->getName());
  if (!sym) {
    Diags.report(expr->getLoc(), DiagID::err_undeclared_identifier)
        << expr->getName();
    lastComputedType = context.getAnyType();
    return;
  }
  if (sym->type) {
    lastComputedType = sym->type;
  } else {
    lastComputedType = context.getAnyType();
  }
}

void TypeChecker::visitBinaryExpr(const BinaryExpr *expr) {
  expr->getLHS()->accept(*this);
  const Type *lhsType = lastComputedType;
  expr->getRHS()->accept(*this);
  const Type *rhsType = lastComputedType;

  if (!lhsType || !rhsType) {
    lastComputedType = context.getAnyType();
    return;
  }

  TokenKind op = expr->getOp();

  // 1. Arithmetic
  if (op == TokenKind::Plus || op == TokenKind::Minus ||
      op == TokenKind::Star || op == TokenKind::Slash ||
      op == TokenKind::Percent) {

    // String concatenation
    if (op == TokenKind::Plus) {
      if (lhsType->isEquivalent(*context.getStringType()) ||
          rhsType->isEquivalent(*context.getStringType())) {
        lastComputedType = context.getStringType();
        return;
      }
    }

    const Type *common = getCommonNumericType(lhsType, rhsType);
    if (!common) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Operands must be numeric for arithmetic operators";
      lastComputedType = context.getI32Type();
    } else {
      if (op == TokenKind::Percent) {
        auto prim = static_cast<const PrimitiveType *>(common);
        if (prim->getScalar() == PrimitiveType::Scalar::F32 ||
            prim->getScalar() == PrimitiveType::Scalar::F64) {
          Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
              << "Modulo operator requires integer operands";
          lastComputedType = context.getAnyType();
          return;
        }
      }
      lastComputedType = common;
    }
    return;
  }

  // 2. Comparison & Logic
  switch (op) {
  case TokenKind::EqualEqual:
  case TokenKind::NotEqual:
    if (lhsType->is<NullType>() || rhsType->is<NullType>()) {
      lastComputedType = context.getBoolType();
      return;
    }
    // Fallthrough
  case TokenKind::Less:
  case TokenKind::Greater:
  case TokenKind::LessEqual:
  case TokenKind::GreaterEqual:
    if (lhsType->is<NullType>() || rhsType->is<NullType>()) {
      Diags.report(expr->getLoc(), DiagID::err_invalid_bin_op)
          << "Ordered comparison with 'null' is not allowed";
      lastComputedType = context.getBoolType();
      return;
    }
    if (!isCompatible(lhsType, rhsType) && !isCompatible(rhsType, lhsType)) {
      if (!(lhsType->is<PrimitiveType>() && rhsType->is<PrimitiveType>())) {
        Diags.report(expr->getLoc(), DiagID::err_invalid_bin_op)
            << "Invalid comparison between " << lhsType->toString() << " and "
            << rhsType->toString();
      }
    }
    lastComputedType = context.getBoolType();
    return;

  case TokenKind::AmpAmp:
  case TokenKind::PipePipe:
    if (lhsType != context.getBoolType() || rhsType != context.getBoolType()) {
      Diags.report(expr->getLoc(), DiagID::err_invalid_bin_op)
          << "Logical ops require bool";
    }
    lastComputedType = context.getBoolType();
    return;
  default:
    break;
  }

  if (lhsType->is<AnyType>() || rhsType->is<AnyType>()) {
    lastComputedType = context.getAnyType();
    return;
  }

  Diags.report(expr->getLoc(), DiagID::err_invalid_bin_op)
      << "Invalid binary operands";
  lastComputedType = context.getAnyType();
}

void TypeChecker::visitUnaryExpr(const UnaryExpr *expr) {
  expr->getOperand()->accept(*this);

  if (!lastComputedType) {
    lastComputedType = context.getAnyType();
    return;
  }

  TokenKind op = expr->getOp();

  if (op == TokenKind::Bang) {
    if (lastComputedType != context.getBoolType()) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Operator '!' requires boolean";
      hasError = true;
    }
    lastComputedType = context.getBoolType();
  } else if (op == TokenKind::Minus || op == TokenKind::Plus) {
    if (!lastComputedType->is<PrimitiveType>()) {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Unary operator requires numeric type";
      hasError = true;
      lastComputedType = context.getAnyType();
    }
  } else {
    Diags.report(expr->getLoc(), DiagID::err_invalid_unary_op);
    hasError = true;
    lastComputedType = context.getAnyType();
  }
}

void TypeChecker::visitCallExpr(const CallExpr *expr) {
  expr->getCallee()->accept(*this);

  if (!lastComputedType || lastComputedType->is<AnyType>()) {
    for (auto &arg : expr->getArgs())
      arg->accept(*this);
    lastComputedType = context.getAnyType();
    return;
  }

  const FunctionType *funcType =
      dynamic_cast<const FunctionType *>(lastComputedType);
  if (!funcType) {
    Diags.report(expr->getLoc(), DiagID::err_type_mismatch) << "Not a function";
    lastComputedType = context.getAnyType();
    return;
  }

  const auto &params = funcType->getParamTypes();
  const auto &args = expr->getArgs();

  if (params.size() != args.size()) {
    Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
        << "Arg count mismatch: expected " << params.size() << ", got "
        << args.size();
  }

  size_t limit = std::min(params.size(), args.size());
  for (size_t i = 0; i < limit; ++i) {
    args[i]->accept(*this);
    if (!isCompatible(params[i].get(), lastComputedType)) {
      Diags.report(args[i]->getLoc(), DiagID::err_type_mismatch)
          << "Arg mismatch at index " << i;
    }
  }

  lastComputedType = funcType->getReturnType();
}

void TypeChecker::visitIndexExpr(const IndexExpr *expr) {
  expr->getArray()->accept(*this);
  const Type *base = lastComputedType;

  if (!base) {
    lastComputedType = context.getAnyType();
    return;
  }

  expr->getIndex()->accept(*this);
  const Type *idx = lastComputedType;

  if (!idx) {
    lastComputedType = context.getAnyType();
    return;
  }

  if (auto arr = dynamic_cast<const ArrayType *>(base)) {
    bool isValidIndex = false;
    if (auto prim = dynamic_cast<const PrimitiveType *>(idx)) {
      if (prim->isInteger())
        isValidIndex = true;
    }

    if (!isValidIndex) {
      Diags.report(expr->getIndex()->getLoc(), DiagID::err_type_mismatch)
          << "Array index must be an integer";
    }
    lastComputedType = arr->getElementType();
  } else if (auto map = dynamic_cast<const MapType *>(base)) {
    if (!isCompatible(map->getKeyType(), idx)) {
      Diags.report(expr->getIndex()->getLoc(), DiagID::err_type_mismatch)
          << "Map key type mismatch";
    }
    lastComputedType = map->getValueType();
  } else if (base->is<AnyType>()) {
    lastComputedType = context.getAnyType();
  } else {
    Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
        << "Type is not indexable";
    lastComputedType = context.getAnyType();
  }
}

void TypeChecker::visitMemberExpr(const MemberExpr *expr) {
  expr->getObject()->accept(*this);
  const Type *objType = lastComputedType;

  if (!objType || objType->is<AnyType>()) {
    lastComputedType = context.getAnyType();
    return;
  }

  std::string name = expr->getMemberName();

  if (objType->isArray() || objType->isString()) {
    if (name == "size" || name == "length") {
      lastComputedType = context.getI32Type();
      return;
    }
  }

  if (auto enumType = dynamic_cast<const EnumType *>(objType)) {
    if (enumType->hasMember(name)) {
      lastComputedType = enumType;
      return;
    } else {
      Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
          << "Enum '" << enumType->getName() << "' has no member '" << name
          << "'";
      lastComputedType = context.getAnyType();
      return;
    }
  }

  if (auto named = dynamic_cast<const NamedType *>(objType)) {
    // Requires class member lookup implementation
    Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
        << "Member lookup not yet implemented for class types";
    lastComputedType = context.getAnyType();
    return;
  }

  Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
      << "Type '" << objType->toString() << "' has no member '" << name << "'";
  lastComputedType = context.getAnyType();
}

void TypeChecker::visitCastExpr(const CastExpr *expr) {
  expr->getExpr()->accept(*this);
  const Type *src = lastComputedType;
  const Type *dst = expr->getTargetType();

  if (!src) {
    lastComputedType = context.getAnyType();
    return;
  }

  if (!isCastAllowed(src, dst)) {
    Diags.report(expr->getLoc(), DiagID::err_invalid_cast)
        << "Cannot cast '" << src->toString() << "' to '" << dst->toString()
        << "'";
    lastComputedType = context.getAnyType();
    return;
  }
  lastComputedType = dst;
}

void TypeChecker::visitTernaryExpr(const TernaryExpr *expr) {
  expr->getCondition()->accept(*this);
  if (lastComputedType != context.getBoolType()) {
    Diags.report(expr->getCondition()->getLoc(), DiagID::err_if_condition_bool);
  }
  expr->getTrueBranch()->accept(*this);
  const Type *t = lastComputedType;
  expr->getFalseBranch()->accept(*this);
  const Type *f = lastComputedType;

  const Type *common = getCommonSuperType(t, f);

  if (common) {
    lastComputedType = common;
  } else {
    Diags.report(expr->getLoc(), DiagID::err_type_mismatch)
        << "Ternary branches have incompatible types";
    lastComputedType = context.getAnyType();
  }
}

void TypeChecker::visitNewExpr(const NewExpr *expr) {
  for (const auto &arg : expr->getArgs())
    arg->accept(*this);
  lastComputedType = expr->getType();
}

void TypeChecker::visitLambdaExpr(const LambdaExpr *expr) {
  symbols.enterScope(ScopeKind::Function);

  const Type *savedLastComputed = lastComputedType;
  const Type *outerExpectedReturn = currentExpectedReturnType;
  currentExpectedReturnType = nullptr;

  lambdaStack.push_back(nullptr);

  for (const auto &p : expr->getParams()) {
    const Type *t = p.getType() ? p.getType() : context.getAnyType();
    symbols.addSymbol(p.getName(), Symbol(SymbolKind::Variable, p.getName(),
                                          const_cast<Type *>(t)));
  }

  if (expr->getBody()) {
    expr->getBody()->accept(*this);
  }

  const Type *inferredRet = lambdaStack.back();
  lambdaStack.pop_back();

  if (!inferredRet) {
    inferredRet = context.getVoidType();
  }

  if (currentExpectedReturnType &&
      !isCompatible(currentExpectedReturnType, inferredRet)) {
    Diags.report(expr->getLoc(), DiagID::err_type_incompatible_return)
        << "Lambda expected to return '"
        << currentExpectedReturnType->toString() << "'";
  }

  currentExpectedReturnType = outerExpectedReturn;
  symbols.exitScope();

  std::vector<const Type *> paramTypes;
  for (const auto &p : expr->getParams()) {
    paramTypes.push_back(
        const_cast<Type *>(p.getType() ? p.getType() : context.getAnyType()));
  }

  lastComputedType = context.createFunctionType(paramTypes, inferredRet);
}

void TypeChecker::visitTemplateStringExpr(const TemplateStringExpr *expr) {
  for (const auto &part : expr->getParts())
    part->accept(*this);
  lastComputedType = context.getStringType();
}

void TypeChecker::visitThreadExpr(const ThreadExpr *expr) {
  expr->getBody()->accept(*this);
  Symbol *threadSym = symbols.lookup("Thread");
  if (threadSym && threadSym->type) {
    lastComputedType = threadSym->type;
  } else {
    lastComputedType = context.getAnyType();
  }
}

// --- Statements ---

void TypeChecker::visitVariableDecl(const VariableDecl *decl) {
  const Type *varType = decl->getType();

  if (auto namedTy = dynamic_cast<const NamedType *>(varType)) {
    visitNamedType(namedTy);
  }

  if (decl->getInitializer()) {
    const Type *saved = currentExpectedReturnType;
    if (auto fnTy = dynamic_cast<const FunctionType *>(varType)) {
      currentExpectedReturnType = fnTy->getReturnType();
    }
    decl->getInitializer()->accept(*this);
    currentExpectedReturnType = saved;

    if (!isCompatible(varType, lastComputedType)) {
      Diags.report(decl->getLoc(), DiagID::err_type_incompatible_assignment)
          << "Cannot assign '" << lastComputedType->toString() << "' to '"
          << varType->toString() << "'";
    }
  }

  Symbol sym(SymbolKind::Variable, decl->getName(),
             const_cast<Type *>(varType));
  symbols.addSymbol(decl->getName(), sym, decl->getLoc());
}

void TypeChecker::visitFunctionDecl(const FunctionDecl *decl) {
  symbols.enterScope(ScopeKind::Function);
  for (const auto &param : decl->getParams()) {
    Symbol sym(SymbolKind::Variable, param.name, param.type.get());
    symbols.addSymbol(param.name, sym, decl->getLoc());
  }

  const Type *prevRet = currentExpectedReturnType;
  currentExpectedReturnType = decl->getReturnType();

  if (decl->getBody())
    decl->getBody()->accept(*this);

  bool isVoid = currentExpectedReturnType->isEquivalent(*context.getVoidType());
  if (!isVoid && decl->getBody() && !allPathsReturn(decl->getBody())) {
    Diags.report(decl->getLoc(), DiagID::err_missing_return)
        << "Function '" << decl->getName()
        << "' may end without returning a value";
    hasError = true;
  }

  currentExpectedReturnType = prevRet;
  symbols.exitScope();
}

void TypeChecker::visitReturnStmt(const ReturnStmt *stmt) {
  const Type *exprType = context.getVoidType();

  if (stmt->getReturnValue()) {
    stmt->getReturnValue()->accept(*this);
    exprType = lastComputedType;
  }

  if (!lambdaStack.empty()) {
    const Type *currentInferred = lambdaStack.back();

    if (!currentInferred) {
      lambdaStack.back() = exprType;
    } else {
      bool isVoidA = currentInferred->isEquivalent(*context.getVoidType());
      bool isVoidB = exprType->isEquivalent(*context.getVoidType());

      if (isVoidA != isVoidB) {
        Diags.report(stmt->getLoc(), DiagID::err_type_mismatch)
            << "Inconsistent lambda return";
        lambdaStack.back() = context.getAnyType();
        return;
      }

      const Type *lub = getCommonSuperType(currentInferred, exprType);
      if (!lub) {
        Diags.report(stmt->getLoc(), DiagID::err_type_mismatch)
            << "Incompatible return types in lambda";
      } else {
        lambdaStack.back() = lub;
      }
    }
    return;
  }

  if (!currentExpectedReturnType) {
    Diags.report(stmt->getLoc(), DiagID::err_type_incompatible_return)
        << "Return outside function";
    return;
  }

  if (!isCompatible(currentExpectedReturnType, exprType)) {
    Diags.report(stmt->getLoc(), DiagID::err_type_incompatible_return)
        << "Expected '" << currentExpectedReturnType->toString() << "', got '"
        << exprType->toString() << "'";
  }
}

void TypeChecker::visitBlockStmt(const BlockStmt *stmt) {
  symbols.enterScope(ScopeKind::Block);
  for (const auto &s : stmt->getStatements())
    s->accept(*this);
  symbols.exitScope();
}

void TypeChecker::visitIfStmt(const IfStmt *stmt) {
  stmt->getCondition()->accept(*this);
  if (lastComputedType != context.getBoolType()) {
    Diags.report(stmt->getCondition()->getLoc(), DiagID::err_if_condition_bool);
  }
  stmt->getThenStmt()->accept(*this);
  if (stmt->getElseStmt())
    stmt->getElseStmt()->accept(*this);
}

void TypeChecker::visitWhileStmt(const WhileStmt *stmt) {
  stmt->getCondition()->accept(*this);
  if (lastComputedType != context.getBoolType()) {
    Diags.report(stmt->getCondition()->getLoc(), DiagID::err_if_condition_bool);
  }
  loopDepth++;
  stmt->getBody()->accept(*this);
  loopDepth--;
}

void TypeChecker::visitDoWhileStmt(const DoWhileStmt *stmt) {
  loopDepth++;
  stmt->getBody()->accept(*this);
  loopDepth--;
  stmt->getCondition()->accept(*this);
  if (lastComputedType != context.getBoolType()) {
    Diags.report(stmt->getCondition()->getLoc(), DiagID::err_if_condition_bool);
  }
}

void TypeChecker::visitForStmt(const ForStmt *stmt) {
  symbols.enterScope(ScopeKind::Block);
  if (stmt->getInit())
    stmt->getInit()->accept(*this);
  if (stmt->getCondition()) {
    stmt->getCondition()->accept(*this);
    if (lastComputedType != context.getBoolType()) {
      Diags.report(stmt->getCondition()->getLoc(),
                   DiagID::err_if_condition_bool);
    }
  }
  if (stmt->getIncrement())
    stmt->getIncrement()->accept(*this);
  loopDepth++;
  stmt->getBody()->accept(*this);
  loopDepth--;
  symbols.exitScope();
}

void TypeChecker::visitForInStmt(const ForInStmt *stmt) {
  symbols.enterScope(ScopeKind::Block);

  // 1. Analyze Collection
  stmt->getCollection()->accept(*this);
  const Type *colType = lastComputedType;

  // 2. Determine Loop Variable Types
  const Type *type1 = nullptr; // val / key / ch
  const Type *type2 = nullptr; // idx / val

  if (auto arr = dynamic_cast<const ArrayType *>(colType)) {
    type1 = arr->getElementType(); // val
    type2 = context.getI32Type();  // idx
  } else if (auto map = dynamic_cast<const MapType *>(colType)) {
    type1 = map->getKeyType();   // key
    type2 = map->getValueType(); // val
  } else if (colType->isString()) {
    type1 = context.getCharType(); // ch
    type2 = context.getI32Type();  // idx
  } else if (colType->is<AnyType>()) {
    type1 = context.getAnyType();
    type2 = context.getAnyType();
  } else {
    Diags.report(stmt->getCollection()->getLoc(), DiagID::err_type_mismatch)
        << "For-in requires array, table, or string";
    type1 = context.getAnyType();
    type2 = context.getAnyType();
  }

  // 3. Register Variables
  if (const auto *decl =
          dynamic_cast<const VariableDecl *>(stmt->getVariable())) {
    if (!decl->getType()->is<AnyType>() &&
        !isCompatible(decl->getType(), type1)) {
      Diags.report(decl->getLoc(), DiagID::err_type_mismatch)
          << "Loop variable type mismatch";
    }
    Symbol sym(SymbolKind::Variable, decl->getName(),
               const_cast<Type *>(type1));
    symbols.addSymbol(decl->getName(), sym, decl->getLoc());
  }

  if (const auto *decl =
          dynamic_cast<const VariableDecl *>(stmt->getIndexVariable())) {
    if (!decl->getType()->is<AnyType>() &&
        !isCompatible(decl->getType(), type2)) {
      Diags.report(decl->getLoc(), DiagID::err_type_mismatch)
          << "Loop index/value variable type mismatch";
    }
    Symbol sym(SymbolKind::Variable, decl->getName(),
               const_cast<Type *>(type2));
    symbols.addSymbol(decl->getName(), sym, decl->getLoc());
  }

  loopDepth++;
  stmt->getBody()->accept(*this);
  loopDepth--;
  symbols.exitScope();
}

void TypeChecker::visitSwitchStmt(const SwitchStmt *stmt) {
  stmt->getCondition()->accept(*this);
  const Type *condType = lastComputedType;
  bool hasDefault = false;

  for (const auto &c : stmt->getCases()) {
    if (c.isDefaultCase())
      hasDefault = true;

    symbols.enterScope(ScopeKind::Block);
    for (const auto &val : c.getValues()) {
      val->accept(*this);
      if (!isCompatible(condType, lastComputedType)) {
        Diags.report(val->getLoc(), DiagID::err_type_mismatch)
            << "Switch case type mismatch";
      }
    }
    if (c.getBody())
      c.getBody()->accept(*this);
    symbols.exitScope();
  }

  if (!hasDefault) {
    Diags.report(stmt->getLoc(), DiagID::warn_switch_not_exhaustive);
  }
}

void TypeChecker::visitDeferStmt(const DeferStmt *stmt) {
  stmt->getDeferredStmt()->accept(*this);
}
void TypeChecker::visitUnsafeBlockStmt(const UnsafeBlockStmt *stmt) {
  visitBlockStmt(stmt);
}
void TypeChecker::visitExpressionStmt(const ExpressionStmt *stmt) {
  stmt->getExpr()->accept(*this);
}
void TypeChecker::visitDeclStmt(const DeclStmt *stmt) {
  stmt->getDecl()->accept(*this);
}

void TypeChecker::visitBreakStmt(const BreakStmt *stmt) {
  if (loopDepth == 0) {
    Diags.report(stmt->getLoc(), DiagID::err_unexpected_token)
        << "'break' usage outside of loop";
  }
}

void TypeChecker::visitContinueStmt(const ContinueStmt *stmt) {
  if (loopDepth == 0) {
    Diags.report(stmt->getLoc(), DiagID::err_unexpected_token)
        << "'continue' usage outside of loop";
  }
}

void TypeChecker::visitTryCatchStmt(const TryCatchStmt *stmt) {
  stmt->getTryBody()->accept(*this);
  if (stmt->getCatchBody()) {
    symbols.enterScope(ScopeKind::Block);
    if (stmt->getCatchVar())
      stmt->getCatchVar()->accept(*this);
    stmt->getCatchBody()->accept(*this);
    symbols.exitScope();
  }
  if (stmt->getFinallyBody())
    stmt->getFinallyBody()->accept(*this);
}

void TypeChecker::visitModuleDecl(const ModuleDecl *decl) {
  for (const auto &d : decl->getDecls())
    d->accept(*this);
}

void TypeChecker::visitClassDecl(const ClassDecl *decl) {
  // [FIX] Register class name in current scope BEFORE entering class scope
  Symbol sym(SymbolKind::Class, decl->getName(), nullptr);
  symbols.addSymbol(decl->getName(), sym, decl->getLoc());

  symbols.enterScope(ScopeKind::Class);
  for (const auto &m : decl->getMembers())
    m->accept(*this);
  symbols.exitScope();
}

void TypeChecker::visitGenericDecl(const GenericDecl *decl) {
  symbols.enterScope(ScopeKind::Block);
  for (const auto &param : decl->getTypeParams()) {
    Symbol sym(SymbolKind::Type, param);
    symbols.addSymbol(param, sym);
  }
  decl->getInnerDecl()->accept(*this);
  symbols.exitScope();
}

void TypeChecker::visitImportDecl(const ImportDecl *decl) {
  // [FIX] Adjusted logic to match ImportDecl structure (no getAlias)
  if (!decl->getSymbols().empty()) {
    // from "mod" import { A, B }
    for (const auto &symName : decl->getSymbols()) {
      Symbol sym(SymbolKind::Variable, symName,
                 const_cast<Type *>(context.getAnyType()));
      if (!symbols.addSymbol(symName, sym, decl->getLoc())) {
        Diags.report(decl->getLoc(), DiagID::err_symbol_redefinition)
            << "Import '" << symName << "' conflicts";
        hasError = true;
      }
    }
  } else {
    // import "mod"
    std::string name = decl->getModuleName();
    Symbol sym(SymbolKind::Module, name,
               const_cast<Type *>(context.getAnyType()));
    if (!symbols.addSymbol(name, sym, decl->getLoc())) {
      Diags.report(decl->getLoc(), DiagID::err_symbol_redefinition)
          << "Import '" << name << "' conflicts";
      hasError = true;
    }
  }
}

void TypeChecker::visitEnumDecl(const EnumDecl *decl) {
  std::vector<std::string> members;
  for (const auto &c : decl->getCases()) {
    members.push_back(c.name);
    if (c.value) {
      c.value->accept(*this);
      if (!lastComputedType->is<PrimitiveType>() ||
          static_cast<const PrimitiveType *>(lastComputedType)->getScalar() !=
              PrimitiveType::Scalar::I32) {
        Diags.report(c.value->getLoc(), DiagID::err_type_mismatch)
            << "Enum value must be an integer";
      }
    }
  }

  // Note: Creates a raw pointer, requires cleanup or ASTContext ownership in
  // future
  auto enumType =
      std::make_unique<EnumType>(decl->getName(), members, decl->getLoc());
  Type *rawType = enumType.release();

  Symbol sym(SymbolKind::Type, decl->getName(), rawType);
  if (!symbols.addSymbol(decl->getName(), sym, decl->getLoc())) {
    Diags.report(decl->getLoc(), DiagID::err_symbol_redefinition)
        << decl->getName();
    hasError = true;
  }
}

void TypeChecker::visitNamedType(const NamedType *type) {
  for (const auto &arg : type->getGenericArgs()) {
    if (arg.type->is<AnyType>()) {
      Diags.report(type->getLoc(), DiagID::err_type_mismatch)
          << "Any not allowed in generic";
    }
    arg.type->accept(*this);
  }
}

void TypeChecker::visitLockType(const LockType *type) {
  type->getInner()->accept(*this);
}

void TypeChecker::visitViewType(const ViewType *type) {
  type->getInner()->accept(*this);
}

void TypeChecker::visitMutType(const MutType *type) {
  type->getInner()->accept(*this);
}

void TypeChecker::visitPrimitiveType(const PrimitiveType *) {}
void TypeChecker::visitPointerType(const PointerType *) {}
void TypeChecker::visitReferenceType(const ReferenceType *) {}
void TypeChecker::visitArrayType(const ArrayType *) {}
void TypeChecker::visitMapType(const MapType *) {}
void TypeChecker::visitFunctionType(const FunctionType *) {}
void TypeChecker::visitNullableType(const NullableType *) {}
void TypeChecker::visitAnyType(const AnyType *) {}

} // namespace moksha
