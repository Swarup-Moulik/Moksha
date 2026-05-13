#include "moksha/AST/ASTContext.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
namespace moksha {

ASTContext::ASTContext() {
  SourceLocation loc; // Empty loc for builtins

  VoidTy = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc);
  BoolTy = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Bool, loc);
  CharTy = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Char, loc);
  StringTy =
      std::make_unique<PrimitiveType>(PrimitiveType::Scalar::String, loc);
  AnyTy = std::make_unique<AnyType>(loc);

  // [FIX] Initialize NullTy
  NullTy = std::make_unique<NullType>(loc);

  I8Ty = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I8, loc);
  I16Ty = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I16, loc);
  I32Ty = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc);
  I64Ty = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I64, loc);
  ISizeTy = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::ISize, loc);

  U8Ty = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::U8, loc);
  U16Ty = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::U16, loc);
  U32Ty = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::U32, loc);
  U64Ty = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::U64, loc);
  USizeTy = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::USize, loc);

  F8Ty = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::F8, loc);
  F16Ty = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::F16, loc);
  F32Ty = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::F32, loc);
  F64Ty = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::F64, loc);
}

ASTContext::~ASTContext() = default;

const Type *ASTContext::createDecimalType(unsigned int precision,
                                          unsigned int scale) {
  return saveType(
      std::make_unique<DecimalType>(precision, scale, SourceLocation()));
}

const Type *ASTContext::createPointerType(const Type *pointee) {
  // Use saveType to store the unique_ptr in the context's ownedTypes vector
  return saveType(
      std::make_unique<PointerType>(pointee->clone(), pointee->getLoc()));
}

const Type *ASTContext::createReferenceType(const Type *inner) {
  return saveType(
      std::make_unique<ReferenceType>(inner->clone(), inner->getLoc()));
}

const Type *ASTContext::createNullableType(const Type *inner) {
  // In a real compiler, we would deduplicate (intern) types here.
  // For now, we clone the inner type and wrap it.
  return saveType(
      std::make_unique<NullableType>(inner->clone(), inner->getLoc()));
}

const Type *ASTContext::createMutType(const Type *inner) {
  return saveType(std::make_unique<MutType>(inner->clone(), inner->getLoc()));
}

const Type *ASTContext::createViewType(const Type *inner) {
  return saveType(std::make_unique<ViewType>(inner->clone(), inner->getLoc()));
}

const Type *ASTContext::createLockType(const Type *inner) {
  return saveType(std::make_unique<LockType>(inner->clone(), inner->getLoc()));
}

const Type *ASTContext::createArrayType(const Type *element,
                                        std::unique_ptr<Expr> sizeExpr) {
  return saveType(std::make_unique<ArrayType>(
      element->clone(), std::move(sizeExpr), element->getLoc()));
}

const Type *ASTContext::createMapType(const Type *key, const Type *value) {
  return saveType(
      std::make_unique<MapType>(key->clone(), value->clone(), key->getLoc()));
}

const Type *
ASTContext::createFunctionType(const std::vector<const Type *> &params,
                               const Type *ret, bool isVariadic,
                               bool isInterrupt) { // [FIX] Added parameter
  std::vector<TypePtr> clonedParams;
  for (const auto *p : params) {
    clonedParams.push_back(p->clone());
  }
  return saveType(
      std::make_unique<FunctionType>(ret->clone(), std::move(clonedParams),
                                     isVariadic, isInterrupt, ret->getLoc()));
}

const Type *ASTContext::createNamedType(const std::string &name) {
  // Assuming invariant for simple creation
  std::vector<TypePtr> args;
  return saveType(
      std::make_unique<NamedType>(name, std::move(args), SourceLocation()));
}

const Type *
ASTContext::createClosureType(const std::vector<const Type *> &params,
                              const Type *ret) {
  std::vector<TypePtr> clonedParams;
  for (const auto *p : params) {
    clonedParams.push_back(p->clone());
  }
  return saveType(std::make_unique<ClosureType>(
      ret->clone(), std::move(clonedParams), ret->getLoc()));
}

const Type *ASTContext::createPromiseType(const Type *inner) {
  return saveType(
      std::make_unique<PromiseType>(inner->clone(), inner->getLoc()));
}

const Type *ASTContext::createWeakType(const Type *inner) {
  auto weakTy = std::make_unique<WeakType>(inner->clone(), inner->getLoc());
  return saveType(
      std::make_unique<NullableType>(std::move(weakTy), inner->getLoc()));
}

const Type *ASTContext::createConstType(const Type *inner) {
  return saveType(std::make_unique<ConstType>(inner->clone(), inner->getLoc()));
}

const Type *ASTContext::createVolatileType(const Type *inner) {
  return saveType(
      std::make_unique<VolatileType>(inner->clone(), inner->getLoc()));
}

const Type *ASTContext::createEnumType(const std::string &name) {
  return saveType(std::make_unique<EnumType>(name, std::vector<std::string>{},
                                             SourceLocation()));
}

void ASTContext::registerClass(const ClassDecl *decl) {
  classMap[decl->getName()] = decl;
}

const ClassDecl *ASTContext::lookupClass(llvm::StringRef name) const {
  auto it = classMap.find(name);
  return it != classMap.end() ? it->second : nullptr;
}

const Type *ASTContext::getSliceType(const Type *elementType) {
  // If you aren't interning slices yet, just return a new owned instance:
  return saveType(
      std::make_unique<SliceType>(elementType->clone(), elementType->getLoc()));
}

void ASTContext::registerInstantiatedClass(std::unique_ptr<ClassDecl> decl) {
  instantiatedClassDecls.push_back(decl.get());
  ownedInstantiations.push_back(std::move(decl));
}

} // namespace moksha
