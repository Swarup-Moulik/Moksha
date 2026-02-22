#pragma once

#include "moksha/AST/Type.h"
#include "llvm/ADT/StringMap.h"
#include <memory>
#include <vector>

namespace moksha {

class Decl;
class ClassDecl;

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
  [[nodiscard]] const Type *getNullType() const { return NullTy.get(); }

  [[nodiscard]] const Type *getI8Type() const { return I8Ty.get(); }
  [[nodiscard]] const Type *getI16Type() const { return I16Ty.get(); }
  [[nodiscard]] const Type *getI32Type() const { return I32Ty.get(); }
  [[nodiscard]] const Type *getI64Type() const { return I64Ty.get(); }
  [[nodiscard]] const Type *getISizeType() const { return ISizeTy.get(); }

  [[nodiscard]] const Type *getU8Type() const { return U8Ty.get(); }
  [[nodiscard]] const Type *getU16Type() const { return U16Ty.get(); }
  [[nodiscard]] const Type *getU32Type() const { return U32Ty.get(); }
  [[nodiscard]] const Type *getU64Type() const { return U64Ty.get(); }
  [[nodiscard]] const Type *getUSizeType() const { return USizeTy.get(); }

  [[nodiscard]] const Type *getF8Type() const { return F8Ty.get(); }
  [[nodiscard]] const Type *getF16Type() const { return F16Ty.get(); }
  [[nodiscard]] const Type *getF32Type() const { return F32Ty.get(); }
  [[nodiscard]] const Type *getF64Type() const { return F64Ty.get(); }

  // Factory Methods
  const Type *createPointerType(const Type *pointee);
  const Type *createNullableType(const Type *inner);
  const Type *createArrayType(const Type *element, uint64_t size = 0);
  const Type *createMapType(const Type *key, const Type *value);
  const Type *createFunctionType(const std::vector<const Type *> &params,
                                 const Type *ret, bool isVariadic = false,
                                 bool isInterrupt = false);
  const Type *createNamedType(const std::string &name);
  void registerClass(const ClassDecl *decl);
  const ClassDecl *lookupClass(llvm::StringRef name) const;

  // Securely take ownership of synthetic builtin AST nodes
  void takeOwnership(std::unique_ptr<Decl> decl) {
    builtinDecls.push_back(std::move(decl));
  }

private:
  template <typename T> const Type *saveType(std::unique_ptr<T> t) {
    ownedTypes.push_back(std::move(t));
    return ownedTypes.back().get();
  }

  llvm::StringMap<const ClassDecl *> classMap;
  std::vector<TypePtr> ownedTypes;
  std::vector<std::unique_ptr<Decl>> builtinDecls;

  TypePtr VoidTy, BoolTy, CharTy, StringTy, AnyTy, NullTy;
  TypePtr I8Ty, I16Ty, I32Ty, I64Ty, ISizeTy;
  TypePtr U8Ty, U16Ty, U32Ty, U64Ty, USizeTy;
  TypePtr F8Ty, F16Ty, F32Ty, F64Ty;
};

} // namespace moksha
