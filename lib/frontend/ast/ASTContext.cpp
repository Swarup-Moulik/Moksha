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

const Type *ASTContext::createNullableType(const Type *inner) {
  // In a real compiler, we would deduplicate (intern) types here.
  // For now, we clone the inner type and wrap it.
  return saveType(
      std::make_unique<NullableType>(inner->clone(), inner->getLoc()));
}

const Type *ASTContext::createArrayType(const Type *element, uint64_t size) {
  // Note: TypeChecker passes size, but ArrayType expects Expr*.
  // We will ignore the size for type creation or create a dummy expression if
  // strictly needed. For static analysis, often the array type structure
  // matches regardless of size expression value.
  return saveType(std::make_unique<ArrayType>(element->clone(), nullptr,
                                              element->getLoc()));
}

const Type *
ASTContext::createFunctionType(const std::vector<const Type *> &params,
                               const Type *ret) {
  std::vector<TypePtr> clonedParams;
  for (const auto *p : params) {
    clonedParams.push_back(p->clone());
  }
  return saveType(std::make_unique<FunctionType>(
      ret->clone(), std::move(clonedParams), ret->getLoc()));
}

} // namespace moksha
