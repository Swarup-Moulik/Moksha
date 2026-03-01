#include "moksha/HIR/HIRGen.h"
#include "moksha/AST/ASTContext.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "moksha/HIR/HIRExpr.h"
#include "moksha/HIR/HIRFunction.h"
#include "moksha/HIR/HIRModule.h"
#include "moksha/HIR/HIRStmt.h"
#include "llvm/Support/Casting.h"
#include <memory>
#include <vector>

using namespace moksha;

// Helper to cast generic HIRStmt to specific HIR subclass safely
template <typename T>
std::unique_ptr<T> castToHIR(std::unique_ptr<hir::HIRStmt> stmt) {
  if (!stmt)
    return nullptr;
  return std::unique_ptr<T>(static_cast<T *>(stmt.release()));
}

static bool isTypeMutable(const Type *t) {
  while (t) {
    if (llvm::isa<MutType>(t) || llvm::isa<LockType>(t))
      return true;
    if (llvm::isa<ViewType>(t))
      return false;
    if (auto *pt = llvm::dyn_cast<PointerType>(t))
      t = pt->getPointee();
    else if (auto *rt = llvm::dyn_cast<ReferenceType>(t))
      t = rt->getInner();
    else
      break;
  }
  return false;
}

HIRGen::HIRGen(ASTContext &ctx, hir::HIRModule &hirModule)
    : ctx(ctx), hirModule(hirModule) {}

const hir::HIRType *HIRGen::lowerType(const Type *astType) {
  if (!astType)
    return hirModule.getVoidType();

  hir::Ownership accumulatedOwn = hir::Ownership::None;
  const Type *current = astType;

  while (current) {
    if (auto viewT = llvm::dyn_cast<ViewType>(current)) {
      accumulatedOwn = hir::Ownership::Borrowed; // view -> Borrowed
      current = viewT->getInner();
    } else if (auto lockT = llvm::dyn_cast<LockType>(current)) {
      accumulatedOwn = hir::Ownership::Shared; // lock -> Shared
      current = lockT->getInner();
    } else if (auto mutT = llvm::dyn_cast<MutType>(current)) {
      if (accumulatedOwn == hir::Ownership::None)
        accumulatedOwn = hir::Ownership::Owned;
      current = mutT->getInner();
    } else if (auto volT = llvm::dyn_cast<VolatileType>(current)) {
      current = volT->getInner();
    } else if (auto constT = llvm::dyn_cast<ConstType>(current)) {
      current = constT->getInner();
    } else {
      break; // No more wrappers to peel
    }
  }

  // Primitives
  if (auto prim = llvm::dyn_cast<PrimitiveType>(current)) {
    switch (prim->getScalar()) {
    case PrimitiveType::Scalar::Void:
      return hirModule.getVoidType();
    case PrimitiveType::Scalar::Bool:
      return hirModule.getBoolType();
    case PrimitiveType::Scalar::String:
      return hirModule.getStringType();
    case PrimitiveType::Scalar::I8:
      return hirModule.getIntType(8, true);
    case PrimitiveType::Scalar::U8:
    case PrimitiveType::Scalar::Char:
      return hirModule.getIntType(8, false);
    case PrimitiveType::Scalar::I16:
      return hirModule.getIntType(16, true);
    case PrimitiveType::Scalar::U16:
      return hirModule.getIntType(16, false);
    case PrimitiveType::Scalar::I32:
    case PrimitiveType::Scalar::Int:
      return hirModule.getIntType(32, true);
    case PrimitiveType::Scalar::U32:
      return hirModule.getIntType(32, false);
    case PrimitiveType::Scalar::I64:
      return hirModule.getIntType(64, true);
    case PrimitiveType::Scalar::U64:
      return hirModule.getIntType(64, false);
    case PrimitiveType::Scalar::ISize:
      return hirModule.getIntType(64, true, true);
    case PrimitiveType::Scalar::USize:
      return hirModule.getIntType(64, false, true);
    case PrimitiveType::Scalar::F8:
      return hirModule.getFloatType(8);
    case PrimitiveType::Scalar::F16:
      return hirModule.getFloatType(16);
    case PrimitiveType::Scalar::F32:
      return hirModule.getFloatType(32);
    case PrimitiveType::Scalar::F64:
      return hirModule.getFloatType(64);
    default:
      return hirModule.getIntType(32, true);
    }
  }

  // Handle 'ref T' (References are always Borrowed in HIR)
  if (auto *refT = llvm::dyn_cast<ReferenceType>(current)) {
    return hirModule.getPointerType(lowerType(refT->getInner()),
                                    hir::Ownership::Borrowed);
  }

  // Handle Pointers (Use the ownership we accumulated from the wrappers!)
  if (auto ptrT = llvm::dyn_cast<PointerType>(current)) {
    hir::Ownership ptrOwn = accumulatedOwn;
    const Type *inner = ptrT->getPointee();

    while (inner) {
      if (auto viewT = llvm::dyn_cast<ViewType>(inner)) {
        if (ptrOwn == hir::Ownership::None)
          ptrOwn = hir::Ownership::Borrowed;
        inner = viewT->getInner();
      } else if (auto lockT = llvm::dyn_cast<LockType>(inner)) {
        if (ptrOwn == hir::Ownership::None)
          ptrOwn = hir::Ownership::Shared;
        inner = lockT->getInner();
      } else if (auto mutT = llvm::dyn_cast<MutType>(inner)) {
        if (ptrOwn == hir::Ownership::None)
          ptrOwn = hir::Ownership::Owned;
        inner = mutT->getInner();
      } else if (auto volT = llvm::dyn_cast<VolatileType>(inner)) {
        inner = volT->getInner();
      } else if (auto constT = llvm::dyn_cast<ConstType>(inner)) {
        inner = constT->getInner();
      } else {
        break;
      }
    }

    return hirModule.getPointerType(lowerType(inner), ptrOwn);
  }

  if (auto *nullT = llvm::dyn_cast<NullableType>(current)) {
    return hirModule.getNullableType(lowerType(nullT->getInner()));
  }

  // Arrays
  if (auto arrT = llvm::dyn_cast<ArrayType>(current)) {
    uint64_t size = 0;
    if (arrT->getSizeExpr()) {
      if (auto lit = llvm::dyn_cast<IntegerLiteral>(arrT->getSizeExpr())) {
        size = lit->getValue();
      }
    }
    return hirModule.getArrayType(lowerType(arrT->getElementType()), size);
  }

  // Functions
  if (auto fnT = llvm::dyn_cast<FunctionType>(current)) {
    std::vector<const hir::HIRType *> paramTypes;
    for (const auto &p : fnT->getParamTypes()) {
      paramTypes.push_back(lowerType(p.get()));
    }
    return hirModule.getFunctionType(lowerType(fnT->getReturnType()),
                                     paramTypes);
  }

  // Named Types (Structs / Classes)
  if (auto namedT = llvm::dyn_cast<NamedType>(current)) {
    return hirModule.getStructType(namedT->getName(), {});
  }

  // Built-ins
  if (llvm::dyn_cast<MapType>(current))
    return hirModule.getStructType("table", {});
  if (llvm::dyn_cast<AnyType>(current))
    return hirModule.getStructType("any", {});

  return hirModule.getVoidType();
}

std::unique_ptr<hir::HIRStmt> HIRGen::takeStmt() { return std::move(lastStmt); }
std::unique_ptr<hir::HIRExpr> HIRGen::takeExpr() { return std::move(lastExpr); }

// [FIX] Implement the dispatchers
void HIRGen::visit(const Decl *d) {
  if (d)
    d->accept(*this);
}
void HIRGen::visit(const Stmt *s) {
  if (s)
    s->accept(*this);
}
void HIRGen::visit(const Expr *e) {
  if (e)
    e->accept(*this);
}

void HIRGen::lowerModule(const ModuleDecl *mod) {
  functions.clear();
  globals.clear();

  for (const auto &decl : mod->getDecls()) {
    visit(decl.get());
    if (lastStmt) {
      globals.push_back(takeStmt());
    }
  }
}

void HIRGen::visitModuleDecl(const ModuleDecl *decl) {
  for (const auto &d : decl->getDecls()) {
    visit(d.get());
  }
}

void HIRGen::visitFunctionDecl(const FunctionDecl *decl) {
  lastStmt = nullptr;

  // 1. Lower the return type
  const hir::HIRType *retType = lowerType(decl->getReturnType());

  // 2. Lower parameters
  std::vector<hir::HIRParam> hirParams;
  for (const auto &p : decl->getParams()) {
    hirParams.emplace_back(p.name, lowerType(p.type.get()), decl->getLoc());
  }

  // 3. Lower the function body (if it exists)
  std::unique_ptr<hir::HIRStmt> body = nullptr;
  if (decl->getBody()) {
    visit(decl->getBody());
    body = takeStmt();
  }

  // 4. Setup metadata
  std::vector<std::string> typeParams;

  // 5. Create the HIRFunction object (Now correctly passing the AST flags!)
  auto func = std::make_unique<hir::HIRFunction>(
      decl->getName(), typeParams, std::move(hirParams), retType,
      std::move(body), decl->isAsyncFunc(), decl->isVariadicFunc(),
      decl->isInterruptFunc(), decl->isNakedFunc(), decl->isNoReturnFunc(),
      decl->getSection(), decl->getLoc());

  // 6. Store the generated HIR function
  functions.push_back(std::move(func));
}

void HIRGen::visitClassDecl(const ClassDecl *decl) {
  // 1. Get the struct type representing the memory layout
  const hir::HIRType *classType =
      lowerType(ctx.createNamedType(decl->getName()));

  std::vector<std::unique_ptr<hir::HIRFunction>> methods;

  // 2. Lower all member functions
  for (const auto &member : decl->getMembers()) {
    if (auto fnDecl = llvm::dyn_cast<FunctionDecl>(member.get())) {
      fnDecl->accept(*this);

      // visitFunctionDecl automatically pushes to the global `functions`
      // vector. We pop the last one off so it belongs to the class instead!
      if (!functions.empty()) {
        methods.push_back(std::move(functions.back()));
        functions.pop_back();
      }
    }
  }

  // 3. Create and store the class
  classes.push_back(std::make_unique<hir::HIRClass>(decl->getName(), classType,
                                                    std::move(methods)));
}

void HIRGen::visitVariableDecl(const VariableDecl *decl) {
  std::unique_ptr<hir::HIRExpr> hirInit = nullptr;
  bool isMut = isTypeMutable(decl->getType());

  if (decl->getInitializer()) {
    visit(decl->getInitializer());
    hirInit = takeExpr();

    if (auto *addrOf =
            llvm::dyn_cast_or_null<hir::HIRAddressOfExpr>(hirInit.get())) {
      addrOf->setMutableBorrow(isMut);
    }
  }

  const hir::HIRType *hirType = lowerType(decl->getType());
  auto sharedType =
      std::shared_ptr<const hir::HIRType>(hirType, [](const hir::HIRType *) {});

  lastStmt = std::make_unique<hir::HIRVarDeclStmt>(
      decl->getName(), sharedType.get(), std::move(hirInit), isMut, false,
      false, 0, decl->getLoc());
}

void HIRGen::visitBlockStmt(const BlockStmt *stmt) {
  std::vector<std::unique_ptr<hir::HIRStmt>> hirStmts;
  for (const auto &s : stmt->getStatements()) {
    visit(s.get());
    if (lastStmt) {
      hirStmts.push_back(takeStmt());
    }
  }
  lastStmt =
      std::make_unique<hir::BlockStmt>(std::move(hirStmts), stmt->getLoc());
}

void HIRGen::visitReturnStmt(const ReturnStmt *stmt) {
  std::unique_ptr<hir::HIRExpr> retVal = nullptr;
  if (stmt->getReturnValue()) {
    visit(stmt->getReturnValue());
    retVal = takeExpr();
  }
  lastStmt =
      std::make_unique<hir::ReturnStmt>(std::move(retVal), stmt->getLoc());
}

void HIRGen::visitIfStmt(const IfStmt *stmt) {
  visit(stmt->getCondition());
  auto cond = takeExpr();

  visit(stmt->getThenStmt());
  auto thenBlock = takeStmt();

  std::unique_ptr<hir::HIRStmt> elseBlock = nullptr;
  if (stmt->getElseStmt()) {
    visit(stmt->getElseStmt());
    elseBlock = takeStmt();
  }

  lastStmt =
      std::make_unique<hir::IfStmt>(std::move(cond), std::move(thenBlock),
                                    std::move(elseBlock), stmt->getLoc());
}

void HIRGen::visitWhileStmt(const WhileStmt *stmt) {
  // 1. Lower Condition
  visit(stmt->getCondition());
  auto cond = takeExpr();

  // 2. Lower Body
  visit(stmt->getBody());
  auto body = takeStmt();

  // Assuming your HIR node is named hir::WhileStmt
  lastStmt = std::make_unique<hir::WhileStmt>(std::move(cond), std::move(body),
                                              stmt->getLoc());
}

void HIRGen::visitForInStmt(const ForInStmt *stmt) {
  visit(stmt->getVariable());
  auto varDecl = castToHIR<hir::HIRVarDeclStmt>(takeStmt());

  std::unique_ptr<hir::HIRVarDeclStmt> indexDecl = nullptr;
  if (stmt->getIndexVariable()) {
    visit(stmt->getIndexVariable());
    indexDecl = castToHIR<hir::HIRVarDeclStmt>(takeStmt());
  }

  visit(stmt->getCollection());
  auto collection = takeExpr();

  visit(stmt->getBody());
  auto body = takeStmt();

  lastStmt = std::make_unique<hir::ForInStmt>(
      std::move(varDecl), std::move(indexDecl), std::move(collection),
      std::move(body), stmt->getLoc());
}

void HIRGen::visitExpressionStmt(const ExpressionStmt *stmt) {
  visit(stmt->getExpr());
  lastStmt = std::make_unique<hir::ExprStmt>(takeExpr(), stmt->getLoc());
}

void HIRGen::visitUnsafeBlockStmt(const UnsafeBlockStmt *stmt) {
  std::vector<std::unique_ptr<hir::HIRStmt>> hirStmts;
  for (const auto &s : stmt->getStatements()) {
    visit(s.get());
    if (lastStmt) {
      hirStmts.push_back(takeStmt());
    }
  }
  lastStmt = std::make_unique<hir::UnsafeBlockStmt>(std::move(hirStmts),
                                                    stmt->getLoc());
}

void HIRGen::visitDoWhileStmt(const DoWhileStmt *stmt) {
  visit(stmt->getBody());
  auto body = takeStmt();

  visit(stmt->getCondition());
  auto cond = takeExpr();

  lastStmt = std::make_unique<hir::DoWhileStmt>(
      std::move(body), std::move(cond), stmt->getLoc());
}

void HIRGen::visitForStmt(const ForStmt *stmt) {
  std::unique_ptr<hir::HIRStmt> init = nullptr;
  if (stmt->getInit()) {
    visit(stmt->getInit());
    init = takeStmt();
  }

  std::unique_ptr<hir::HIRExpr> cond = nullptr;
  if (stmt->getCondition()) {
    visit(stmt->getCondition());
    cond = takeExpr();
  }

  std::unique_ptr<hir::HIRExpr> inc = nullptr;
  if (stmt->getIncrement()) {
    visit(stmt->getIncrement());
    inc = takeExpr();
  }

  visit(stmt->getBody());
  auto body = takeStmt();

  lastStmt = std::make_unique<hir::ForStmt>(std::move(init), std::move(cond),
                                            std::move(inc), std::move(body),
                                            stmt->getLoc());
}

void HIRGen::visitSwitchStmt(const SwitchStmt *stmt) {
  visit(stmt->getCondition());
  auto cond = takeExpr();

  std::vector<hir::SwitchCase> hirCases;
  for (const auto &c : stmt->getCases()) {
    std::vector<std::unique_ptr<hir::HIRExpr>> vals;
    for (const auto &v : c.getValues()) {
      visit(v.get());
      vals.push_back(takeExpr());
    }

    visit(c.getBody());
    auto caseBody = castToHIR<hir::BlockStmt>(takeStmt());

    hirCases.emplace_back(std::move(vals), std::move(caseBody),
                          c.isDefaultCase());
  }

  lastStmt = std::make_unique<hir::SwitchStmt>(
      std::move(cond), std::move(hirCases), stmt->getLoc());
}

void HIRGen::visitBreakStmt(const BreakStmt *stmt) {
  lastStmt = std::make_unique<hir::BreakStmt>(stmt->getLoc());
}

void HIRGen::visitContinueStmt(const ContinueStmt *stmt) {
  lastStmt = std::make_unique<hir::ContinueStmt>(stmt->getLoc());
}

void HIRGen::visitDeferStmt(const DeferStmt *stmt) {
  visit(stmt->getDeferredStmt());
  lastStmt = std::make_unique<hir::DeferStmt>(takeStmt(), stmt->getLoc());
}

void HIRGen::visitTryCatchStmt(const TryCatchStmt *stmt) {
  visit(stmt->getTryBody());
  auto tryBody = castToHIR<hir::BlockStmt>(takeStmt());

  std::unique_ptr<hir::HIRStmt> catchBody = nullptr;
  std::unique_ptr<hir::HIRExpr> catchVar = nullptr;

  if (stmt->getCatchBody()) {
    visit(stmt->getCatchBody());
    catchBody = takeStmt();
  }

  if (stmt->getCatchVar()) {
    lastExpr = std::make_unique<hir::HIRIdentifierExpr>(
        stmt->getCatchVar()->getName(), nullptr, stmt->getCatchVar()->getLoc());
    catchVar = takeExpr();
  }

  std::unique_ptr<hir::HIRStmt> finallyBody = nullptr;
  if (stmt->getFinallyBody()) {
    visit(stmt->getFinallyBody());
    finallyBody = takeStmt();
  }

  lastStmt = std::make_unique<hir::TryCatchStmt>(
      std::move(tryBody), std::move(catchVar), std::move(catchBody),
      std::move(finallyBody), stmt->getLoc());
}

void HIRGen::visitDeclStmt(const DeclStmt *stmt) {
  // Unwrap the statement to get the underlying declaration (e.g., VariableDecl)
  if (stmt->getDecl()) {
    visit(stmt->getDecl());
    // visitVariableDecl will run and set `lastStmt` automatically!
  }
}

// --- Expressions ---

void HIRGen::visitIntegerLiteral(const IntegerLiteral *expr) {
  lastExpr = std::make_unique<hir::HIRIntegerLiteral>(expr->getValue(), nullptr,
                                                      expr->getLoc());
}

void HIRGen::visitFloatLiteral(const FloatLiteral *expr) {
  lastExpr = std::make_unique<hir::HIRFloatLiteral>(expr->getValue(), nullptr,
                                                    expr->getLoc());
}

void HIRGen::visitBoolLiteral(const BoolLiteral *expr) {
  lastExpr = std::make_unique<hir::HIRBoolLiteral>(expr->getValue(), nullptr,
                                                   expr->getLoc());
}

void HIRGen::visitStringLiteral(const StringLiteral *expr) {
  lastExpr = std::make_unique<hir::HIRStringLiteral>(expr->getValue(), nullptr,
                                                     expr->getLoc());
}

void HIRGen::visitBinaryExpr(const BinaryExpr *expr) {
  visit(expr->getLHS());
  auto lhs = takeExpr();
  visit(expr->getRHS());
  auto rhs = takeExpr();

  hir::BinaryOp hirOp;
  bool isCompound = false;
  hir::BinaryOp compoundMathOp = hir::BinaryOp::Add; // Dummy init

  // Switch on AST TokenKind, map to HIR BinaryOp
  switch (expr->getOp()) {
  // 1. Standard Arithmetic
  case TokenKind::Plus:
    hirOp = hir::BinaryOp::Add;
    break;
  case TokenKind::Minus:
    hirOp = hir::BinaryOp::Sub;
    break;
  case TokenKind::Star:
    hirOp = hir::BinaryOp::Mul;
    break;
  case TokenKind::Slash:
    hirOp = hir::BinaryOp::Div;
    break;
  case TokenKind::Percent:
    hirOp = hir::BinaryOp::Mod;
    break;
  case TokenKind::Power:
    hirOp = hir::BinaryOp::Pow;
    break;

  // 2. Compound Arithmetic (Mark for Desugaring)
  case TokenKind::PlusEqual:
    isCompound = true;
    compoundMathOp = hir::BinaryOp::Add;
    break;
  case TokenKind::MinusEqual:
    isCompound = true;
    compoundMathOp = hir::BinaryOp::Sub;
    break;
  case TokenKind::StarEqual:
    isCompound = true;
    compoundMathOp = hir::BinaryOp::Mul;
    break;
  case TokenKind::SlashEqual:
    isCompound = true;
    compoundMathOp = hir::BinaryOp::Div;
    break;
  case TokenKind::PercentEqual:
    isCompound = true;
    compoundMathOp = hir::BinaryOp::Mod;
    break;

  // 3. Comparisons
  case TokenKind::EqualEqual:
    hirOp = hir::BinaryOp::Equal;
    break;
  case TokenKind::NotEqual:
    hirOp = hir::BinaryOp::NotEqual;
    break;
  case TokenKind::Less:
    hirOp = hir::BinaryOp::Less;
    break;
  case TokenKind::LessEqual:
    hirOp = hir::BinaryOp::LessEqual;
    break;
  case TokenKind::Greater:
    hirOp = hir::BinaryOp::Greater;
    break;
  case TokenKind::GreaterEqual:
    hirOp = hir::BinaryOp::GreaterEqual;
    break;

  // 4. Bitwise
  case TokenKind::Amp:
    hirOp = hir::BinaryOp::BitAnd;
    break;
  case TokenKind::Pipe:
    hirOp = hir::BinaryOp::BitOr;
    break;
  case TokenKind::Caret:
    hirOp = hir::BinaryOp::BitXor;
    break;
  case TokenKind::LessLess:
    hirOp = hir::BinaryOp::Shl;
    break;
  case TokenKind::GreaterGreater:
    hirOp = hir::BinaryOp::Shr;
    break;

  // 5. Compound Bitwise (Mark for Desugaring)
  case TokenKind::AmpEqual:
    isCompound = true;
    compoundMathOp = hir::BinaryOp::BitAnd;
    break;
  case TokenKind::PipeEqual:
    isCompound = true;
    compoundMathOp = hir::BinaryOp::BitOr;
    break;
  case TokenKind::CaretEqual:
    isCompound = true;
    compoundMathOp = hir::BinaryOp::BitXor;
    break;
  case TokenKind::LessLessEqual:
    isCompound = true;
    compoundMathOp = hir::BinaryOp::Shl;
    break;
  case TokenKind::GreaterGreaterEqual:
    isCompound = true;
    compoundMathOp = hir::BinaryOp::Shr;
    break;

  // 6. Logical Operators
  case TokenKind::AmpAmp:
    hirOp = hir::BinaryOp::And;
    break;
  case TokenKind::PipePipe:
    hirOp = hir::BinaryOp::Or;
    break;
  case TokenKind::QuestionQuestion:
    hirOp = hir::BinaryOp::NullCoalesce;
    break;

  // 7. Standard Assignment
  case TokenKind::Equal:
    hirOp = hir::BinaryOp::Assign;
    // Inject mutability for re-assignments (e.g. m1 = &p)
    if (auto *addrOf =
            llvm::dyn_cast_or_null<hir::HIRAddressOfExpr>(rhs.get())) {
      addrOf->setMutableBorrow(isTypeMutable(expr->getLHS()->getType()));
    }
    break;

    // 8. Ranges
  case TokenKind::Colon:
    hirOp = hir::BinaryOp::Range;
    break;

  default:
    llvm_unreachable("Unhandled binary operator in HIRGen");
  }

  const hir::HIRType *type = lowerType(expr->getType());

  if (isCompound) {
    // DESUGAR: a += 5  =>  a = a + 5
    visit(expr->getLHS());
    auto lhsCopy = takeExpr();

    // Step 1: Create the math expression (a + 5)
    auto mathExpr = std::make_unique<hir::HIRBinaryExpr>(
        compoundMathOp, std::move(lhsCopy), std::move(rhs), type,
        expr->getLoc());

    // Step 2: Create the assignment (a = mathExpr)
    lastExpr = std::make_unique<hir::HIRBinaryExpr>(
        hir::BinaryOp::Assign, std::move(lhs), std::move(mathExpr), type,
        expr->getLoc());
  } else {
    // Normal binary operation
    lastExpr = std::make_unique<hir::HIRBinaryExpr>(
        hirOp, std::move(lhs), std::move(rhs), type, expr->getLoc());
  }
}

void HIRGen::visitUnaryExpr(const UnaryExpr *expr) {
  visit(expr->getOperand());
  auto operand = takeExpr();

  // [FIX] Map AST TokenKind to HIR Deref/AddressOf logic
  if (expr->getOp() == TokenKind::Star) {
    const hir::HIRType *derefType = lowerType(expr->getType());
    lastExpr = std::make_unique<hir::HIRDerefExpr>(std::move(operand),
                                                   derefType, expr->getLoc());
    return;
  }

  if (expr->getOp() == TokenKind::Amp) {
    const hir::HIRType *ptrType = lowerType(expr->getType());
    lastExpr = std::make_unique<hir::HIRAddressOfExpr>(
        std::move(operand), ptrType, false, expr->getLoc());
    return;
  }

  hir::UnaryOp hirOp;
  // [FIX] Switch on TokenKind and use correct hir::UnaryOp names
  switch (expr->getOp()) {
  case TokenKind::Minus:
    hirOp = hir::UnaryOp::Neg;
    break;
  case TokenKind::Bang:
    hirOp = hir::UnaryOp::Not;
    break;
  case TokenKind::Tilde:
    hirOp = hir::UnaryOp::BitNot;
    break;
  case TokenKind::PlusPlus:
    hirOp = expr->isPostfixOp() ? hir::UnaryOp::PostInc : hir::UnaryOp::PreInc;
    break;
  case TokenKind::MinusMinus:
    hirOp = expr->isPostfixOp() ? hir::UnaryOp::PostDec : hir::UnaryOp::PreDec;
    break;
  default:
    hirOp = hir::UnaryOp::Neg;
    break;
  }

  const hir::HIRType *type = lowerType(expr->getType());
  lastExpr = std::make_unique<hir::HIRUnaryExpr>(hirOp, std::move(operand),
                                                 type, expr->getLoc());
}

void HIRGen::visitIdentifierExpr(const IdentifierExpr *expr) {
  auto hirId = std::make_unique<hir::HIRIdentifierExpr>(
      expr->getName(), lowerType(expr->getType()), expr->getLoc());

  // [FIX] If the variable is a 'ref', every access is an implicit dereference
  // (*)
  if (auto *refT = llvm::dyn_cast_or_null<ReferenceType>(expr->getType())) {
    lastExpr = std::make_unique<hir::HIRDerefExpr>(
        std::move(hirId),
        lowerType(refT->getInner()), // The actual i32 value
        expr->getLoc());
  } else {
    lastExpr = std::move(hirId);
  }
}

void HIRGen::visitCallExpr(const CallExpr *expr) {
  visit(expr->getCallee());
  auto callee = takeExpr();

  // 1. Extract the AST Function Signature
  const FunctionType *astFuncType = nullptr;
  if (expr->getCallee()->getType()) {
    const Type *t = expr->getCallee()->getType();
    while (t) {
      if (auto m = dynamic_cast<const MutType *>(t))
        t = m->getInner();
      else if (auto v = dynamic_cast<const ViewType *>(t))
        t = v->getInner();
      else if (auto l = dynamic_cast<const LockType *>(t))
        t = l->getInner();
      else
        break;
    }
    astFuncType = llvm::dyn_cast_or_null<FunctionType>(t);
  }

  // 2. Process Arguments and inject mutability into '&' expressions
  std::vector<std::unique_ptr<hir::HIRExpr>> args;
  const auto &astArgs = expr->getArgs();

  for (size_t i = 0; i < astArgs.size(); ++i) {
    visit(astArgs[i].get());
    auto argExpr = takeExpr();

    if (astFuncType && i < astFuncType->getParamTypes().size()) {
      const Type *paramT = astFuncType->getParamTypes()[i].get();

      // Inject the parameter's mutability into the &ptr argument!
      if (auto *addrOf =
              llvm::dyn_cast_or_null<hir::HIRAddressOfExpr>(argExpr.get())) {
        addrOf->setMutableBorrow(isTypeMutable(paramT));
      }

      if (paramT && llvm::isa<ReferenceType>(paramT)) {
        argExpr = std::make_unique<hir::HIRAddressOfExpr>(
            std::move(argExpr), lowerType(paramT), true, astArgs[i]->getLoc());
      }
    }
    args.push_back(std::move(argExpr));
  }

  lastExpr = std::make_unique<hir::HIRCallExpr>(
      std::move(callee), std::move(args), lowerType(expr->getType()),
      expr->getLoc());
}

void HIRGen::visitMemberExpr(const MemberExpr *expr) {
  visit(expr->getObject());
  auto object = takeExpr();

  // Map AST getName() to HIR constructor
  lastExpr = std::make_unique<hir::HIRMemberExpr>(
      std::move(object), expr->getName(), nullptr, expr->getLoc());
}

void HIRGen::visitThreadExpr(const ThreadExpr *expr) {
  visit(expr->getBody());
  auto bodyLambda = llvm::dyn_cast<hir::HIRLambdaExpr>(lastExpr.release());
  std::unique_ptr<hir::HIRLambdaExpr> lambdaTask(bodyLambda);

  hir::ThreadKind kind =
      expr->isWeakThread() ? hir::ThreadKind::Weak : hir::ThreadKind::Strong;

  lastExpr = std::make_unique<hir::HIRThreadExpr>(std::move(lambdaTask), kind,
                                                  nullptr, expr->getLoc());
}

void HIRGen::visitNullLiteral(const NullLiteral *expr) {
  lastExpr = std::make_unique<hir::HIRNullLiteral>(nullptr, expr->getLoc());
}

void HIRGen::visitTernaryExpr(const TernaryExpr *expr) {
  visit(expr->getCondition());
  auto cond = takeExpr();
  visit(expr->getTrueBranch());
  auto t = takeExpr();
  visit(expr->getFalseBranch());
  auto f = takeExpr();

  lastExpr = std::make_unique<hir::HIRTernaryExpr>(
      std::move(cond), std::move(t), std::move(f), nullptr, expr->getLoc());
}

void HIRGen::visitCastExpr(const CastExpr *expr) {
  visit(expr->getExpr());
  auto e = takeExpr();

  hir::CastOp castOp = static_cast<hir::CastOp>(0);

  // Stubbing type conversion
  const hir::HIRType *targetType = nullptr;

  lastExpr = std::make_unique<hir::HIRCastExpr>(castOp, std::move(e),
                                                targetType, expr->getLoc());
}

void HIRGen::visitCharLiteral(const CharLiteral *expr) {
  lastExpr = std::make_unique<hir::HIRIntegerLiteral>(
      static_cast<uint64_t>(expr->getValue()), nullptr, expr->getLoc());
}

void HIRGen::visitTemplateStringExpr(const TemplateStringExpr *expr) {
  std::vector<std::unique_ptr<hir::HIRExpr>> hirParts;

  for (const auto &part : expr->getParts()) {
    visit(part.get());
    hirParts.push_back(takeExpr());
  }

  lastExpr = std::make_unique<hir::HIRTemplateStringExpr>(
      std::move(hirParts), hirModule.getStringType(), expr->getLoc());
}

void HIRGen::visitArrayLiteral(const ArrayLiteral *expr) {
  std::vector<std::unique_ptr<hir::HIRExpr>> elements;

  for (const auto &elem : expr->getElements()) {
    // Look ahead for the Spread Operator
    if (auto *unary = llvm::dyn_cast<UnaryExpr>(elem.get())) {
      if (unary->getOp() == TokenKind::DotDotDot) {

        visit(unary->getOperand());
        auto loweredOperand = takeExpr();

        elements.push_back(std::make_unique<hir::HIRSpreadExpr>(
            std::move(loweredOperand), loweredOperand->getType(),
            unary->getLoc()));
        continue;
      }
    }

    // Standard elements
    visit(elem.get());
    elements.push_back(takeExpr());
  }

  lastExpr = std::make_unique<hir::HIRArrayLiteral>(
      std::move(elements), lowerType(expr->getType()), expr->getLoc());
}

void HIRGen::visitMapLiteral(const MapLiteral *expr) {
  std::vector<hir::HIRMapLiteral::Entry> hirEntries;

  for (const auto &entry : expr->getEntries()) {
    visit(entry.first.get());
    auto key = takeExpr();

    visit(entry.second.get());
    auto val = takeExpr();

    hirEntries.emplace_back(std::move(key), std::move(val));
  }

  // Maps are lowered to an opaque "table" type in the backend
  const hir::HIRType *type = hirModule.getStructType("table", {});
  lastExpr = std::make_unique<hir::HIRMapLiteral>(std::move(hirEntries), type,
                                                  expr->getLoc());
}

void HIRGen::visitIndexExpr(const IndexExpr *expr) {
  visit(expr->getArray());
  auto base = takeExpr();
  visit(expr->getIndex());
  auto index = takeExpr();
  lastExpr = std::make_unique<hir::HIRIndexExpr>(
      std::move(base), std::move(index), nullptr, expr->getLoc());
}

void HIRGen::visitNewExpr(const NewExpr *expr) {
  std::vector<std::unique_ptr<hir::HIRExpr>> args;
  for (const auto &arg : expr->getArgs()) {
    visit(arg.get());
    args.push_back(takeExpr());
  }

  // Stubbing type conversion
  const hir::HIRType *type = nullptr;

  lastExpr = std::make_unique<hir::HIRNewExpr>(type, std::move(args), type,
                                               expr->getLoc());
}

void HIRGen::visitLambdaExpr(const LambdaExpr *expr) {
  std::vector<hir::HIRLambdaParam> params;
  for (const auto &p : expr->getParams()) {
    params.emplace_back(p.getName(), lowerType(p.getType()));
  }

  visit(expr->getBody());
  auto body = castToHIR<hir::HIRStmt>(takeStmt());

  lastExpr = std::make_unique<hir::HIRLambdaExpr>(
      std::move(params), std::move(body), lowerType(expr->getType()),
      expr->getLoc());
}

void HIRGen::visitThisExpr(const ThisExpr *expr) {
  lastExpr = std::make_unique<hir::HIRThisExpr>(lowerType(expr->getType()),
                                                expr->getLoc());
}

void HIRGen::visitSuperExpr(const SuperExpr *expr) {
  lastExpr = std::make_unique<hir::HIRIdentifierExpr>(
      "super", lowerType(expr->getType()), expr->getLoc());
}

void HIRGen::visitAwaitExpr(const AwaitExpr *expr) {
  visit(expr->getExpr());
  auto inner = takeExpr();
  lastExpr = std::make_unique<hir::HIRAwaitExpr>(
      std::move(inner), lowerType(expr->getType()), expr->getLoc());
}

// ============================================================================
// [Systems & OS Level Statements]
// ============================================================================

void HIRGen::visitAsmStmt(const AsmStmt *stmt) {
  // Passes the raw assembly and register constraints directly to the backend
  lastStmt = std::make_unique<hir::HIRAsmStmt>(
      stmt->getAssemblyStr(), stmt->getConstraints(), stmt->getLoc());
}

void HIRGen::visitThrowStmt(const ThrowStmt *stmt) {
  // Evaluate the exception payload
  visit(stmt->getExpr());
  auto payload = takeExpr();
  lastStmt =
      std::make_unique<hir::HIRThrowStmt>(std::move(payload), stmt->getLoc());
}

void HIRGen::visitLockStmt(const LockStmt *stmt) {
  std::unique_ptr<hir::HIRExpr> hirTarget = nullptr;

  // 1. Lower the target (e.g. `p1`)
  if (stmt->getTarget()) {
    visit(stmt->getTarget());
    hirTarget = takeExpr();
  }

  // 2. Lower the inner block
  visit(stmt->getBody());
  auto hirBody = castToHIR<hir::HIRStmt>(takeStmt());

  // 3. Construct the High-Level IR node
  lastStmt = std::make_unique<hir::LockStmt>(
      std::move(hirTarget), std::move(hirBody), stmt->getLoc());
}
