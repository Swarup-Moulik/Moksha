#pragma once

#include "moksha/AST/Type.h"
#include <memory>
#include <vector>

namespace moksha {

/// Holds long-lived AST nodes (types, decls) and global compilation state.
class ASTContext {
public:
  ASTContext();
  ~ASTContext();

  ASTContext(const ASTContext &) = delete;
  ASTContext &operator=(const ASTContext &) = delete;

  // --- Built-in Type Accessors ---

  [[nodiscard]] const Type *getVoidType() const { return VoidTy.get(); }
  [[nodiscard]] const Type *getBoolType() const { return BoolTy.get(); }
  [[nodiscard]] const Type *getCharType() const { return CharTy.get(); }
  [[nodiscard]] const Type *getStringType() const { return StringTy.get(); }
  [[nodiscard]] const Type *getAnyType() const { return AnyTy.get(); }

  // [FIX] Added getNullType
  [[nodiscard]] const Type *getNullType() const { return NullTy.get(); }

  // Signed Integers
  [[nodiscard]] const Type *getI8Type() const { return I8Ty.get(); }
  [[nodiscard]] const Type *getI16Type() const { return I16Ty.get(); }
  [[nodiscard]] const Type *getI32Type() const { return I32Ty.get(); }
  [[nodiscard]] const Type *getI64Type() const { return I64Ty.get(); }
  [[nodiscard]] const Type *getISizeType() const { return ISizeTy.get(); }

  // Unsigned Integers
  [[nodiscard]] const Type *getU8Type() const { return U8Ty.get(); }
  [[nodiscard]] const Type *getU16Type() const { return U16Ty.get(); }
  [[nodiscard]] const Type *getU32Type() const { return U32Ty.get(); }
  [[nodiscard]] const Type *getU64Type() const { return U64Ty.get(); }
  [[nodiscard]] const Type *getUSizeType() const { return USizeTy.get(); }

  // Floating Point
  [[nodiscard]] const Type *getF8Type() const { return F8Ty.get(); }
  [[nodiscard]] const Type *getF16Type() const { return F16Ty.get(); }
  [[nodiscard]] const Type *getF32Type() const { return F32Ty.get(); }
  [[nodiscard]] const Type *getF64Type() const { return F64Ty.get(); }

  // [FIX] Factory Methods required by TypeChecker
  const Type *createNullableType(const Type *inner);
  const Type *createArrayType(const Type *element,
                              uint64_t size = 0); // Simplified size for checker
  const Type *createFunctionType(const std::vector<const Type *> &params,
                                 const Type *ret);

private:
  // Helper to store complex types
  template <typename T> const Type *saveType(std::unique_ptr<T> t) {
    ownedTypes.push_back(std::move(t));
    return ownedTypes.back().get();
  }

  std::vector<TypePtr> ownedTypes;

  TypePtr VoidTy;
  TypePtr BoolTy;
  TypePtr CharTy;
  TypePtr StringTy;
  TypePtr AnyTy;
  TypePtr NullTy; // [FIX]

  TypePtr I8Ty;
  TypePtr I16Ty;
  TypePtr I32Ty;
  TypePtr I64Ty;
  TypePtr ISizeTy;

  TypePtr U8Ty;
  TypePtr U16Ty;
  TypePtr U32Ty;
  TypePtr U64Ty;
  TypePtr USizeTy;

  TypePtr F8Ty;
  TypePtr F16Ty;
  TypePtr F32Ty;
  TypePtr F64Ty;
};

} // namespace moksha
