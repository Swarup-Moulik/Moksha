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

static hir::CastOp determineCastOp(const hir::HIRType *srcTy,
                                   const hir::HIRType *dstTy) {
  if (!srcTy || !dstTy)
    return hir::CastOp::BitCast;

  auto srcKind = srcTy->getKind();
  auto dstKind = dstTy->getKind();

  // Float <-> Int Conversions
  if (srcKind == hir::TypeKind::Float && dstKind == hir::TypeKind::Int)
    return hir::CastOp::FloatToInt;
  if (srcKind == hir::TypeKind::Int && dstKind == hir::TypeKind::Float)
    return hir::CastOp::IntToFloat;

  // Int <-> Int Conversions (Truncation & Extension)
  if (srcKind == hir::TypeKind::Int && dstKind == hir::TypeKind::Int) {
    if (auto *srcInt = llvm::dyn_cast<hir::HIRIntType>(srcTy)) {
      if (auto *dstInt = llvm::dyn_cast<hir::HIRIntType>(dstTy)) {
        if (dstInt->getWidth() < srcInt->getWidth()) {
          return hir::CastOp::Truncate;
        }
        if (dstInt->getWidth() > srcInt->getWidth()) {
          return srcInt->isSigned() ? hir::CastOp::SignExtend
                                    : hir::CastOp::ZeroExtend;
        }
      }
    }
    return hir::CastOp::BitCast; // Same width
  }

  // Pointer Conversions
  if (srcKind == hir::TypeKind::Pointer && dstKind == hir::TypeKind::Pointer) {
    return hir::CastOp::PointerCast;
  }

  // Fallback for slices, structs, or any
  return hir::CastOp::BitCast;
}

HIRGen::HIRGen(ASTContext &ctx, hir::HIRModule &hirModule)
    : ctx(ctx), hirModule(hirModule) {}

const hir::HIRType *HIRGen::lowerType(const Type *astType) {
  if (!astType)
    return nullptr;

  bool hasView = false, hasMut = false, hasLock = false, hasConst = false,
       hasVolatile = false;

  hir::Ownership accumulatedOwn = hir::Ownership::None;
  hir::BorrowState accumulatedState = hir::BorrowState::None;
  const Type *current = astType;
  bool isVolatilePtr = false;

  while (current) {
    if (auto viewT = llvm::dyn_cast<ViewType>(current)) {
      accumulatedOwn = hir::Ownership::Borrowed; // view -> Borrowed
      accumulatedState = hir::BorrowState::View;
      hasView = true;
      current = viewT->getInner();
    } else if (auto lockT = llvm::dyn_cast<LockType>(current)) {
      accumulatedOwn = hir::Ownership::Shared; // lock -> Shared
      accumulatedState = hir::BorrowState::Lock;
      hasLock = true;
      current = lockT->getInner();
    } else if (auto mutT = llvm::dyn_cast<MutType>(current)) {
      if (accumulatedOwn == hir::Ownership::None)
        accumulatedOwn = hir::Ownership::Owned;
      accumulatedState = hir::BorrowState::Mut;
      hasMut = true;
      current = mutT->getInner();
    } else if (auto volT = llvm::dyn_cast<VolatileType>(current)) {
      hasVolatile = true;
      current = volT->getInner();
    } else if (auto constT = llvm::dyn_cast<ConstType>(current)) {
      hasConst = true;
      current = constT->getInner();
    } else {
      break; // No more wrappers to peel
    }
  }

  // Lambda to compute the underlying base type without wrappers
  const hir::HIRType *baseType = [&]() -> const hir::HIRType * {
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
      const hir::HIRType *innerTy = lowerType(refT->getInner());
      if (!innerTy)
        innerTy = hirModule.getVoidType();

      // [FIX] Prevent double-pointer wrapping for ref classes!
      if (auto *alreadyPtr = llvm::dyn_cast<hir::PointerType>(innerTy)) {
        return hirModule.getPointerType(alreadyPtr->getPointee(),
                                        hir::Ownership::Borrowed,
                                        accumulatedState);
      }

      return hirModule.getPointerType(innerTy, hir::Ownership::Borrowed,
                                      accumulatedState);
    }

    // Handle Pointers
    if (auto ptrT = llvm::dyn_cast<PointerType>(current)) {
      const Type *inner = ptrT->getPointee();

      while (inner) {
        if (auto viewT = llvm::dyn_cast<ViewType>(inner)) {
          accumulatedState = hir::BorrowState::View;
          inner = viewT->getInner();
        } else if (auto mutT = llvm::dyn_cast<MutType>(inner)) {
          accumulatedState = hir::BorrowState::Mut;
          inner = mutT->getInner();
        } else if (auto lockT = llvm::dyn_cast<LockType>(inner)) {
          accumulatedState = hir::BorrowState::Lock;
          inner = lockT->getInner();
        } else if (auto volT = llvm::dyn_cast<VolatileType>(inner)) {
          isVolatilePtr = true;
          inner = volT->getInner();
        } else {
          break;
        }
      }

      const hir::HIRType *pointeeTy = lowerType(inner);
      if (!pointeeTy)
        pointeeTy = hirModule.getVoidType();

      // [FIX] Prevent double-pointer wrapping!
      if (auto *alreadyPtr = llvm::dyn_cast<hir::PointerType>(pointeeTy)) {
        hir::Ownership mergedOwn = (accumulatedOwn != hir::Ownership::None)
                                       ? accumulatedOwn
                                       : alreadyPtr->getOwnership();
        return hirModule.getPointerType(alreadyPtr->getPointee(), mergedOwn,
                                        accumulatedState);
      }

      hir::Ownership own = (accumulatedOwn == hir::Ownership::None)
                               ? hir::Ownership::Borrowed
                               : accumulatedOwn;

      // The PointerType is now successfully created with the rescued
      // BorrowState!
      return hirModule.getPointerType(pointeeTy, own, accumulatedState);
    }
    if (auto *nullT = llvm::dyn_cast<NullableType>(current)) {
      const hir::HIRType *innerTy = lowerType(nullT->getInner());
      if (!innerTy)
        innerTy = hirModule.getVoidType();
      return hirModule.getNullableType(innerTy);
    }

    if (auto *weakT = llvm::dyn_cast<WeakType>(current)) {
      const hir::HIRType *innerTy = lowerType(weakT->getInner());
      if (!innerTy)
        innerTy = hirModule.getVoidType();
      return hirModule.getWeakType(innerTy);
    }

    if (auto *decT = llvm::dyn_cast<DecimalType>(current)) {
      return hirModule.getDecimalType(decT->getPrecision(), decT->getScale());
    }

    // Arrays
    if (auto arrT = llvm::dyn_cast<ArrayType>(current)) {
      uint64_t size = 0;
      if (arrT->getSizeExpr()) {
        if (auto lit = llvm::dyn_cast<IntegerLiteral>(arrT->getSizeExpr())) {
          size = lit->getValue();
        }
      }
      const hir::HIRType *elemTy = lowerType(arrT->getElementType());
      if (!elemTy)
        elemTy = hirModule.getVoidType(); // [FIX] Guard null
      return hirModule.getArrayType(elemTy, size);
    }

    if (auto sliceT = llvm::dyn_cast<SliceType>(current)) {
      const hir::HIRType *elemTy = lowerType(sliceT->getElementType());
      if (!elemTy)
        elemTy = hirModule.getVoidType();
      return hirModule.getSliceType(elemTy);
    }

    // Functions
    if (auto fnT = llvm::dyn_cast<FunctionType>(current)) {
      std::vector<const hir::HIRType *> paramTypes;
      for (const auto &p : fnT->getParamTypes()) {
        const hir::HIRType *pTy = lowerType(p.get());
        if (!pTy)
          pTy = hirModule.getVoidType(); // Guard null
        paramTypes.push_back(pTy);
      }
      const hir::HIRType *retTy = lowerType(fnT->getReturnType());
      if (!retTy)
        retTy = hirModule.getVoidType(); // Guard null

      // Forward the FFI flags to the HIR Module!
      return hirModule.getFunctionType(retTy, paramTypes, fnT->isVariadicFunc(),
                                       fnT->isInterruptFunc());
    }

    // Closures
    if (auto *closureTy = llvm::dyn_cast<ClosureType>(astType)) {
      const hir::HIRType *ret = lowerType(closureTy->getReturnType());
      std::vector<const hir::HIRType *> params;
      for (const auto &p : closureTy->getParamTypes()) {
        params.push_back(lowerType(p.get()));
      }
      return hirModule.getClosureType(ret, std::move(params));
    }

    if (llvm::isa<EnumType>(current)) {
      return hirModule.getIntType(32, true);
    }

    // Named Types (Structs / Classes / Unions)
    if (auto namedT = llvm::dyn_cast<NamedType>(current)) {
      if (const ClassDecl *cls = ctx.lookupClass(namedT->getName())) {
        if (cls->getAggregateKind() == AggregateKind::Union) {
          auto *unionTy = hirModule.getUnionType(namedT->getName(), {});
          if (accumulatedOwn == hir::Ownership::Borrowed) {
            return hirModule.getPointerType(unionTy, hir::Ownership::Borrowed,
                                            accumulatedState);
          }
          return unionTy;
        }

        bool isRefClass = cls->isReferenceType();

        // ONLY wrap 'ref class' in a PointerType (Shared/ARC).
        if (isRefClass) {
          hir::Ownership own = hir::Ownership::Shared;

          if (accumulatedOwn == hir::Ownership::Borrowed) {
            own = hir::Ownership::Borrowed;
          }

          auto *structTy =
              hirModule.getStructType(namedT->getName(), {}, {}, false, true);
          return hirModule.getPointerType(structTy, own, accumulatedState);
        }
      } else {
        return hirModule.getIntType(32, true);
      }

      // Fallback: Standard struct/class by value (Stack Semantics)
      auto *structTy =
          hirModule.getStructType(namedT->getName(), {}, {}, false, false);

      if (accumulatedOwn == hir::Ownership::Borrowed) {
        return hirModule.getPointerType(structTy, hir::Ownership::Borrowed,
                                        accumulatedState);
      }
      return structTy;
    }

    // Built-ins
    if (auto *mapT = llvm::dyn_cast<MapType>(current)) {
      const hir::HIRType *kTy = lowerType(mapT->getKeyType());
      const hir::HIRType *vTy = lowerType(mapT->getValueType());
      if (!kTy)
        kTy = hirModule.getVoidType();
      if (!vTy)
        vTy = hirModule.getVoidType();
      return hirModule.getMapType(kTy, vTy);
    }
    if (llvm::dyn_cast<AnyType>(current))
      return hirModule.getAnyType();
    if (llvm::dyn_cast<NullType>(current))
      return hirModule.getNullType();

    return hirModule.getVoidType();
  }(); // Execute lambda

  if (!baseType)
    return nullptr;

  if (!llvm::isa<hir::PointerType>(baseType) &&
      !llvm::isa<hir::ReferenceType>(baseType)) {
    if (hasView)
      baseType = hirModule.getViewType(baseType);
    if (hasMut)
      baseType = hirModule.getMutType(baseType);
    if (hasLock)
      baseType = hirModule.getLockType(baseType);
    if (hasConst)
      baseType = hirModule.getConstType(baseType);
    if (hasVolatile)
      baseType = hirModule.getVolatileType(baseType);
  }

  return baseType;
}

std::unique_ptr<hir::HIRStmt> HIRGen::takeStmt() { return std::move(lastStmt); }
std::unique_ptr<hir::HIRExpr> HIRGen::takeExpr() { return std::move(lastExpr); }

// Implement the dispatchers
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

  // 1. Lower all dynamically generated monomorphized classes
  for (const ClassDecl *concreteClass : ctx.getInstantiatedClasses()) {
    visit(concreteClass);
  }

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

  // [FIX] Guard against null types from incomplete ASTs
  if (!retType) {
    retType = hirModule.getVoidType();
  }

  // --- [NEW] Wrap async return types in a Promise ---
  if (decl->isAsyncFunc()) {
    retType = hirModule.getPromiseType(retType);
  }
  // --------------------------------------------------

  // 2. Lower parameters
  std::vector<hir::HIRParam> hirParams;
  for (const auto &p : decl->getParams()) {
    const hir::HIRType *paramType = lowerType(p.type.get());

    // Lower the default value expression if it exists
    std::unique_ptr<hir::HIRExpr> defVal = nullptr;
    if (p.defaultValue) {
      visit(p.defaultValue.get());
      defVal = takeExpr();
    }

    hirParams.emplace_back(p.name, paramType, p.loc, std::move(defVal));
  }

  // 3. Lower the function body (if it exists)
  std::unique_ptr<hir::HIRStmt> body = nullptr;
  if (decl->getBody()) {
    visit(decl->getBody());
    body = takeStmt();
  }

  // 4. Setup metadata
  std::vector<hir::HIRGenericParam> typeParams;

  // 5. Create the HIRFunction object
  auto func = std::make_unique<hir::HIRFunction>(
      decl->getName(), typeParams, std::move(hirParams), retType,
      std::move(body), decl->isAsyncFunc(), decl->isVariadicFunc(),
      decl->isInterruptFunc(), decl->isNakedFunc(), decl->isNoReturnFunc(),
      decl->getSection(), decl->getLoc());

  func->setWeak(decl->isWeakFunc());
  func->setABI(decl->getABI());
  func->setVirtual(decl->isVirtualFunc());
  func->setOverride(decl->isOverrideFunc());
  func->setVTableIndex(decl->getVTableIndex());
  func->setStatic(decl->isStaticFunc());
  func->setNoInline(decl->isNoInlineFunc());
  func->setInline(decl->isInlineFunc());
  func->setPure(decl->isPureFunc());
  func->setCold(decl->isColdFunc());
  func->setUsed(decl->isUsedFunc());

  // 6. Store the generated HIR function
  functions.push_back(std::move(func));
}

void HIRGen::visitClassDecl(const ClassDecl *decl) {
  // 1. Get the struct type representing the memory layout (starts empty)
  const hir::HIRType *classType =
      lowerType(ctx.createNamedType(decl->getName()));

  std::vector<std::unique_ptr<hir::HIRFunction>> methods;
  std::vector<const hir::HIRType *> fieldTypes;
  std::vector<std::string> fieldNames;

  // 2. Lower all member functions AND extract fields!
  for (const auto &member : decl->getMembers()) {
    if (auto varDecl = llvm::dyn_cast<VariableDecl>(member.get())) {
      if (varDecl->getPhysicalIndex() == fieldTypes.size()) {
        fieldTypes.push_back(lowerType(varDecl->getType()));

        if (varDecl->isBitfield()) {
          // Name the collapsed storage generically
          fieldNames.push_back("_bitfield_" +
                               std::to_string(varDecl->getPhysicalIndex()));
        } else {
          fieldNames.push_back(varDecl->getName());
        }
      }

    } else if (auto fnDecl = llvm::dyn_cast<FunctionDecl>(member.get())) {
      fnDecl->accept(*this);

      if (!functions.empty()) {
        methods.push_back(std::move(functions.back()));
        functions.pop_back();
      }
    }
  }

  // Unwrap the pointer type before populating the fields!
  const hir::HIRType *innerType = classType;
  if (auto *ptrTy = llvm::dyn_cast<hir::PointerType>(innerType)) {
    innerType = ptrTy->getPointee();
  }

  // 3. Populate the opaque Struct/Union with the extracted layout
  if (auto *structTy = const_cast<hir::StructType *>(
          llvm::dyn_cast<hir::StructType>(innerType))) {
    structTy->setFields(std::move(fieldTypes), std::move(fieldNames));
    structTy->setPacked(decl->isPackedClass());
    structTy->setHasVTable(decl->hasVTable()); // Bind VTable presence
  } else if (auto *unionTy = const_cast<hir::UnionType *>(
                 llvm::dyn_cast<hir::UnionType>(innerType))) {
    unionTy->setFields(std::move(fieldTypes), std::move(fieldNames));
  }

  // 4. Create and store the class
  auto hirClass = std::make_unique<hir::HIRClass>(
      decl->getName(), classType, std::move(methods), decl->isPackedClass(),
      decl->getAlignment(), decl->getSection(), decl->isReferenceType());

  // Bind Class VTable & Parents
  hirClass->setHasVTable(decl->hasVTable());

  std::vector<const hir::HIRType *> parentTypes;
  for (const auto &parentName : decl->getParentNames()) {
    // Create a temporary NamedType to feed into lowerType
    NamedType tempNamed(parentName, std::vector<TypePtr>(), decl->getLoc());
    parentTypes.push_back(lowerType(&tempNamed));
  }
  hirClass->setParentTypes(std::move(parentTypes));

  classes.push_back(std::move(hirClass));
}

void HIRGen::visitVariableDecl(const VariableDecl *decl) {
  std::unique_ptr<hir::HIRExpr> hirInit = nullptr;
  bool isMut = isTypeMutable(decl->getType());

  if (decl->getInitializer()) {
    visit(decl->getInitializer());
    hirInit = takeExpr();

    if (llvm::isa<ReferenceType>(decl->getType()) &&
        !llvm::isa<hir::HIRAddressOfExpr>(hirInit.get())) {

      const hir::HIRType *ptrType = lowerType(decl->getType());

      hirInit = std::make_unique<hir::HIRAddressOfExpr>(
          std::move(hirInit), ptrType, isMut, decl->getLoc());
    } else if (auto *addrOf = llvm::dyn_cast_or_null<hir::HIRAddressOfExpr>(
                   hirInit.get())) {
      addrOf->setMutableBorrow(isMut);
    }
  }

  const hir::HIRType *hirType = lowerType(decl->getType());

  if (hirInit && hirInit->getType()) {
    if (auto *initPtrTy =
            llvm::dyn_cast<hir::PointerType>(hirInit->getType())) {
      if (hirType && (hirType->getKind() == hir::TypeKind::Struct ||
                      hirType->getKind() == hir::TypeKind::Union)) {
        hirType = initPtrTy;
      }
    }
  }

  if (decl->isSharedVar()) {
    if (auto *ptrTy = llvm::dyn_cast<hir::PointerType>(hirType)) {
      if (ptrTy->getOwnership() != hir::Ownership::Shared) {
        hirType = hirModule.getPointerType(ptrTy->getPointee(),
                                           hir::Ownership::Shared,
                                           ptrTy->getBorrowState());
      }
    } else {
      hirType = hirModule.getPointerType(hirType, hir::Ownership::Shared,
                                         hir::BorrowState::None);
    }
  }

  auto varStmt = std::make_unique<hir::HIRVarDeclStmt>(
      decl->getName(), hirType, std::move(hirInit), isMut,
      decl->isThreadLocalVar(), decl->isVolatileVar(), decl->getAlignment(),
      decl->isStaticVar(), decl->isUsedVar(), decl->getSection(),
      decl->getLoc());

  varStmt->setExtern(decl->isExternVar());

  lastStmt = std::move(varStmt);
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
  const hir::HIRType *ty = lowerType(expr->getType());
  if (!ty)
    ty = hirModule.getIntType(32, true, false);
  lastExpr = std::make_unique<hir::HIRIntegerLiteral>(expr->getValue(), ty,
                                                      expr->getLoc());
}

void HIRGen::visitFloatLiteral(const FloatLiteral *expr) {
  const hir::HIRType *ty = lowerType(expr->getType());
  if (!ty)
    ty = hirModule.getFloatType(32);
  lastExpr = std::make_unique<hir::HIRFloatLiteral>(expr->getValue(), ty,
                                                    expr->getLoc());
}

void HIRGen::visitDecimalLiteral(const DecimalLiteral *expr) {
  // 1. Lower the AST type to the HIR type
  const hir::HIRType *hirType = lowerType(expr->getType());

  // 2. Create the HIR literal preserving the exact string
  lastExpr = std::make_unique<hir::HIRDecimalLiteral>(expr->getValue(), hirType,
                                                      expr->getLoc());
}

void HIRGen::visitBoolLiteral(const BoolLiteral *expr) {
  lastExpr = std::make_unique<hir::HIRBoolLiteral>(expr->getValue(), nullptr,
                                                   expr->getLoc());
}

void HIRGen::visitStringLiteral(const StringLiteral *expr) {
  const hir::HIRType *ty = lowerType(expr->getType());
  if (!ty)
    ty = hirModule.getStringType();
  lastExpr = std::make_unique<hir::HIRStringLiteral>(expr->getValue(), ty,
                                                     expr->getLoc());
}

void HIRGen::visitBinaryExpr(const BinaryExpr *expr) {
  visit(expr->getLHS());
  auto lhs = takeExpr();
  visit(expr->getRHS());
  auto rhs = takeExpr();

  // OVERLOADED OPERATOR DESUGARING
  if (const FunctionDecl *opFunc = expr->getResolvedOperator()) {
    std::vector<std::unique_ptr<hir::HIRExpr>> args;
    args.push_back(std::move(rhs));

    auto memberAccess = std::make_unique<hir::HIRMemberExpr>(
        std::move(lhs), opFunc->getName(), nullptr, expr->getLoc());

    // Create a CallExpr representing `lhs.operator+(rhs)`
    const hir::HIRType *retTy = lowerType(expr->getType());
    lastExpr = std::make_unique<hir::HIRCallExpr>(
        std::move(memberAccess), std::move(args), retTy, expr->getLoc());
    return;
  }

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

  // OVERLOADED UNARY OPERATOR DESUGARING
  if (const FunctionDecl *opFunc = expr->getResolvedOperator()) {

    // Create a MemberExpr representing `operand.operator-`
    auto memberAccess = std::make_unique<hir::HIRMemberExpr>(
        std::move(operand), opFunc->getName(), nullptr, expr->getLoc());

    // Create a CallExpr representing `operand.operator-()` with no args
    std::vector<std::unique_ptr<hir::HIRExpr>> emptyArgs;
    const hir::HIRType *retTy = lowerType(expr->getType());
    lastExpr = std::make_unique<hir::HIRCallExpr>(
        std::move(memberAccess), std::move(emptyArgs), retTy, expr->getLoc());
    return;
  }

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

  // If the variable is a 'ref', every access is an implicit dereference (*)
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
      if (auto m = llvm::dyn_cast<const MutType>(t))
        t = m->getInner();
      else if (auto v = llvm::dyn_cast<const ViewType>(t))
        t = v->getInner();
      else if (auto l = llvm::dyn_cast<const LockType>(t))
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
  const Type *baseType = expr->getObject()->getType();
  if (baseType) {
    bool isEnum = llvm::isa<EnumType>(baseType);
    if (auto *namedT = llvm::dyn_cast<NamedType>(baseType)) {
      // If it is a NamedType but not a class, it's an Enum reference
      if (!ctx.lookupClass(namedT->getName())) {
        isEnum = true;
      }
    }

    if (isEnum) {
      // getMemberIndex() contains the layout value evaluated by TypeChecker
      lastExpr = std::make_unique<hir::HIRIntegerLiteral>(
          expr->getMemberIndex(), hirModule.getIntType(32, true),
          expr->getLoc());
      return;
    }
  }

  visit(expr->getObject());
  auto object = takeExpr();

  hir::FieldInfo fieldInfo(expr->getName(), expr->getMemberIndex(),
                           expr->isBitfield(), expr->getBitWidth(),
                           expr->getBitOffset());

  // Unwrap the base type to check if it's a Union
  while (baseType) {
    if (auto *ptr = llvm::dyn_cast<PointerType>(baseType))
      baseType = ptr->getPointee();
    else if (auto *ref = llvm::dyn_cast<ReferenceType>(baseType))
      baseType = ref->getInner();
    else if (auto *mut = llvm::dyn_cast<MutType>(baseType))
      baseType = mut->getInner();
    else if (auto *view = llvm::dyn_cast<ViewType>(baseType))
      baseType = view->getInner();
    else
      break;
  }

  // Force index 0 for Unions
  if (auto *namedType = llvm::dyn_cast<NamedType>(baseType)) {
    if (const ClassDecl *cls = ctx.lookupClass(namedType->getName())) {
      if (cls->getAggregateKind() == AggregateKind::Union) {
        fieldInfo.index = 0;
      }
    }
  }

  // Pass the layout metadata to the HIR node
  lastExpr = std::make_unique<hir::HIRMemberExpr>(
      std::move(object), expr->getName(), lowerType(expr->getType()),
      expr->getLoc(), std::move(fieldInfo));
}

void HIRGen::visitThreadExpr(const ThreadExpr *expr) {
  // 1. Lower the closure
  expr->getBody()->accept(*this);
  auto task = takeExpr();

  // 2. Extract the closure's return type
  const hir::HIRType *closureRetType = hirModule.getVoidType();

  if (const auto *closureTy =
          llvm::dyn_cast<hir::HIRClosureType>(task->getType())) {
    closureRetType = closureTy->getReturnType();
  } else if (const auto *funcTy =
                 llvm::dyn_cast<hir::FunctionType>(task->getType())) {
    closureRetType = funcTy->getReturnType();
  }

  // 3. Wrap it in a PromiseType
  const hir::HIRType *promiseType = hirModule.getPromiseType(closureRetType);

  // 4. Map the ThreadKind using the exact AST method
  hir::ThreadKind hirKind = hir::ThreadKind::Strong;
  if (expr->isWeakThread()) {
    hirKind = hir::ThreadKind::Weak;
  }

  // 5. Construct the HIR node with the PromiseType!
  lastExpr = std::make_unique<hir::HIRThreadExpr>(std::move(task), hirKind,
                                                  promiseType, expr->getLoc());
}

void HIRGen::visitSizeOfExpr(const SizeOfExpr *expr) {
  // 1. The type we want to measure (e.g., from the sub-expression's type)
  const hir::HIRType *targetTy = lowerType(expr->getExpr()->getType());

  // 2. The result type of the sizeof operation (u64 / usize)
  const hir::HIRType *usizeTy = hirModule.getIntType(64, false, true);

  // 3. This call will now match the new constructor in HIRExpr.h
  lastExpr =
      std::make_unique<hir::HIRSizeOfExpr>(targetTy, usizeTy, expr->getLoc());
}

void HIRGen::visitNullLiteral(const NullLiteral *expr) {
  lastExpr = std::make_unique<hir::HIRNullLiteral>(hirModule.getNullType(),
                                                   expr->getLoc());
}

void HIRGen::visitTernaryExpr(const TernaryExpr *expr) {
  visit(expr->getCondition());
  auto cond = takeExpr();
  visit(expr->getTrueBranch());
  auto t = takeExpr();
  visit(expr->getFalseBranch());
  auto f = takeExpr();

  // Pass the lowered type instead of nullptr
  lastExpr = std::make_unique<hir::HIRTernaryExpr>(
      std::move(cond), std::move(t), std::move(f), lowerType(expr->getType()),
      expr->getLoc());
}

void HIRGen::visitCastExpr(const CastExpr *expr) {
  visit(expr->getExpr());
  auto e = takeExpr();

  const hir::HIRType *sourceType = e->getType();
  const hir::HIRType *targetType = lowerType(expr->getTargetType());

  // Dynamically resolve the correct cast operation
  hir::CastOp castOp = determineCastOp(sourceType, targetType);

  lastExpr = std::make_unique<hir::HIRCastExpr>(castOp, std::move(e),
                                                targetType, expr->getLoc());
}

void HIRGen::visitInputExpr(const InputExpr *expr) {
  std::unique_ptr<hir::HIRExpr> prompt = nullptr;
  if (expr->getPrompt()) {
    visit(expr->getPrompt());
    prompt = takeExpr();
  }

  // input() always returns a string
  const hir::HIRType *strType = hirModule.getStringType();

  lastExpr = std::make_unique<hir::HIRInputExpr>(std::move(prompt), strType,
                                                 expr->getLoc());
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

  const hir::HIRType *type = lowerType(expr->getType());

  lastExpr = std::make_unique<hir::HIRMapLiteral>(std::move(hirEntries), type,
                                                  expr->getLoc());
}

void HIRGen::visitIndexExpr(const IndexExpr *expr) {
  visit(expr->getArray());
  auto base = takeExpr();
  visit(expr->getIndex());
  auto index = takeExpr();
  lastExpr = std::make_unique<hir::HIRIndexExpr>(
      std::move(base), std::move(index), lowerType(expr->getType()),
      expr->getLoc());
}

void HIRGen::visitNewExpr(const NewExpr *expr) {
  std::vector<std::unique_ptr<hir::HIRExpr>> args;
  for (const auto &arg : expr->getArgs()) {
    visit(arg.get());
    args.push_back(takeExpr());
  }

  const hir::HIRType *type = lowerType(expr->getType());
  lastExpr = std::make_unique<hir::HIRNewExpr>(type, std::move(args), type,
                                               expr->getLoc());
}

void HIRGen::visitLambdaExpr(const LambdaExpr *expr) {
  // 1. Lower Parameters
  std::vector<hir::HIRLambdaParam> hirParams;
  for (const auto &p : expr->getParams()) {
    const hir::HIRType *paramType =
        p.getType() ? lowerType(p.getType()) : nullptr;

    // [NEW] Lower the default value expression if it exists
    std::unique_ptr<hir::HIRExpr> defVal = nullptr;
    if (p.getDefaultValue()) {
      visit(p.getDefaultValue());
      defVal = takeExpr();
    }

    hirParams.emplace_back(p.getName(), paramType, std::move(defVal));
  }

  // 2. Lower Captures (The Promise Machine Bridge)
  std::vector<hir::HIRCapture> captures;
  for (const auto &cap : expr->getCaptures()) {
    // Determine how the MIR generator should handle memory ownership
    hir::CaptureKind kind =
        (cap.mode == CaptureMode::View || cap.mode == CaptureMode::Mut)
            ? hir::CaptureKind::ByReference
            : hir::CaptureKind::ByValue;

    captures.emplace_back(cap.name, lowerType(cap.type), kind);
  }

  // 3. Lower Body
  visit(expr->getBody());
  auto body = castToHIR<hir::HIRStmt>(takeStmt());

  // [FIX] Map the AST CaptureMode to the hir::CaptureMode
  hir::CaptureMode hirMode = hir::CaptureMode::Snapshot;
  switch (expr->getCaptureMode()) {
  case CaptureMode::View:
    hirMode = hir::CaptureMode::View;
    break;
  case CaptureMode::Mut:
    hirMode = hir::CaptureMode::Mut;
    break;
  case CaptureMode::Move:
    hirMode = hir::CaptureMode::Move;
    break;
  default:
    break;
  }

  // 4. Construct HIR Lambda with hirMode
  lastExpr = std::make_unique<hir::HIRLambdaExpr>(
      std::move(hirParams), std::move(captures), std::move(body),
      lowerType(expr->getType()), hirMode, expr->getLoc());
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

// ============================================================================
// [Type Visitors]
// ============================================================================

void HIRGen::visitDecimalType(const DecimalType *type) {
  // In HIRGen, types are lowered dynamically via lowerType.
  // No statement/expression is generated here.
}

void HIRGen::visitClosureType(const ClosureType *type) {
  // Types are lowered via lowerType(), so this visitor stub is intentionally
  // empty.
}

void HIRGen::visitWeakType(const WeakType *type) {
  // Handled dynamically via lowerType()
}

void HIRGen::visitSliceType(const SliceType *type) {
  // In HIRGen, types are lowered dynamically via lowerType.
  // No statement/expression is generated here.
}

void HIRGen::visitLockType(const LockType *type) {}
void HIRGen::visitViewType(const ViewType *type) {}
void HIRGen::visitMutType(const MutType *type) {}
void HIRGen::visitConstType(const ConstType *type) {}
void HIRGen::visitVolatileType(const VolatileType *type) {}
void HIRGen::visitNullType(const NullType *type) {}
void HIRGen::visitAnyType(const AnyType *type) {}
