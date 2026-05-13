#pragma once

#include "moksha/AST/Stmt.h"
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
  const Type *createDecimalType(unsigned int precision, unsigned int scale);
  const Type *createPointerType(const Type *pointee);
  const Type *createReferenceType(const Type *inner);
  const Type *createNullableType(const Type *inner);
  const Type *createMutType(const Type *inner);
  const Type *createViewType(const Type *inner);
  const Type *createLockType(const Type *inner);
  const Type *createWeakType(const Type *inner);
  const Type *createPromiseType(const Type *inner);
  const Type *createArrayType(const Type *element,
                              std::unique_ptr<Expr> sizeExpr);
  const Type *getSliceType(const Type *elementType);
  const Type *createMapType(const Type *key, const Type *value);
  const Type *createClosureType(const std::vector<const Type *> &params,
                                const Type *ret);
  const Type *createFunctionType(const std::vector<const Type *> &params,
                                 const Type *ret, bool isVariadic = false,
                                 bool isInterrupt = false);
  const Type *createNamedType(const std::string &name);
  const Type *createConstType(const Type *inner);
  const Type *createVolatileType(const Type *inner);
  const Type *createEnumType(const std::string &name);
  void registerClass(const ClassDecl *decl);
  const ClassDecl *lookupClass(llvm::StringRef name) const;

  // Securely take ownership of synthetic builtin AST nodes
  void takeOwnership(std::unique_ptr<Decl> decl) {
    builtinDecls.push_back(std::move(decl));
  }

  template <typename T> const Type *saveType(std::unique_ptr<T> t) {
    ownedTypes.push_back(std::move(t));
    return ownedTypes.back().get();
  }

  // Store concrete classes generated from generic templates
  void registerInstantiatedClass(std::unique_ptr<ClassDecl> decl);
  void registerInstantiatedFunction(std::unique_ptr<FunctionDecl> decl) {
    instantiatedFuncDecls.push_back(decl.get());
    ownedFuncInstantiations.push_back(std::move(decl));
  }

  // Retrieve a list of all dynamically generated classes
  const std::vector<const ClassDecl *> &getInstantiatedClasses() const {
    return instantiatedClassDecls;
  }
  const std::vector<const FunctionDecl *> &getInstantiatedFunctions() const {
    return instantiatedFuncDecls;
  }

private:
  llvm::StringMap<const ClassDecl *> classMap;
  std::vector<TypePtr> ownedTypes;
  std::vector<std::unique_ptr<Decl>> builtinDecls;
  std::vector<std::unique_ptr<ClassDecl>> ownedInstantiations;
  std::vector<const ClassDecl *> instantiatedClassDecls;
  std::vector<std::unique_ptr<FunctionDecl>> ownedFuncInstantiations;
  std::vector<const FunctionDecl *> instantiatedFuncDecls;
  TypePtr VoidTy, BoolTy, CharTy, StringTy, AnyTy, NullTy;
  TypePtr I8Ty, I16Ty, I32Ty, I64Ty, ISizeTy;
  TypePtr U8Ty, U16Ty, U32Ty, U64Ty, USizeTy;
  TypePtr F8Ty, F16Ty, F32Ty, F64Ty;
};

} // namespace moksha
