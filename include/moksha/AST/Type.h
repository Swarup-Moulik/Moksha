#pragma once

#include "moksha/Support/SourceLocation.h"
#include "llvm/ADT/StringRef.h"
#include <memory>
#include <string>
#include <vector>

namespace moksha {

class Expr;
class ASTVisitor;

enum class TypeKind {
  Primitive,
  Pointer,
  Reference,
  Array,
  Slice,
  Map,
  Function,
  Named,
  Nullable,
  Enum,
  Any,
  Lock,
  View,
  Mut,
  Null,
  Volatile,
  Const,
  Decimal,
  Closure,
  Weak
};

enum class Variance { Invariant, Covariant, Contravariant };

/// Base class for all types
class Type {
public:
  virtual ~Type() = default;
  [[nodiscard]] TypeKind getKind() const { return kind; }
  [[nodiscard]] SourceLocation getLoc() const { return loc; }

  virtual void accept(ASTVisitor &v) const = 0;
  virtual bool isEquivalent(const Type &other) const = 0;
  virtual std::string toString() const = 0;
  virtual std::unique_ptr<Type> clone() const = 0;

  template <typename T> bool is() const { return kind == T::classKind; }

  virtual bool isInteger() const { return false; }
  virtual bool isFloat() const { return false; }
  virtual bool isNumeric() const { return isInteger() || isFloat(); }
  virtual bool isString() const { return false; }
  virtual bool isArray() const { return false; }
  virtual bool isSlice() const { return false; }
  virtual bool isBool() const { return false; }
  virtual bool isVoid() const { return false; }
  virtual bool isRefClass() const { return false; }

  virtual bool isImmutable() const { return false; }
  virtual const Type *stripModifiers() const { return this; }

protected:
  Type(TypeKind kind, SourceLocation loc) : kind(kind), loc(loc) {}
  TypeKind kind;
  SourceLocation loc;
};

using TypePtr = std::unique_ptr<Type>;

class NullType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::Null;
  explicit NullType(SourceLocation loc = SourceLocation())
      : Type(TypeKind::Null, loc) {}

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<NullType>(loc);
  }
  bool isEquivalent(const Type &other) const override;
  std::string toString() const override { return "null"; }
  static bool classof(const Type *T) { return T->getKind() == TypeKind::Null; }
};

class PrimitiveType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::Primitive;
  enum class Scalar {
    Void,
    Bool,
    Char,
    String,
    Int,
    I8,
    I16,
    I32,
    I64,
    ISize,
    U8,
    U16,
    U32,
    U64,
    USize,
    F8,
    F16,
    F32,
    F64
  };

  PrimitiveType(Scalar scalar, SourceLocation loc)
      : Type(TypeKind::Primitive, loc), scalar(scalar) {}

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<PrimitiveType>(scalar, loc);
  }

  bool isEquivalent(const Type &other) const override {
    if (!other.is<PrimitiveType>())
      return false;
    return static_cast<const PrimitiveType &>(other).scalar == scalar;
  }
  [[nodiscard]] Scalar getScalar() const { return scalar; }
  static bool classof(const Type *T) {
    return T->getKind() == TypeKind::Primitive;
  }

  bool isInteger() const override;
  bool isFloat() const override;
  std::string toString() const override;
  bool isString() const override { return scalar == Scalar::String; }
  bool isBool() const override { return scalar == Scalar::Bool; }
  bool isVoid() const override { return scalar == Scalar::Void; }

private:
  Scalar scalar;
};

class AnyType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::Any;
  explicit AnyType(SourceLocation loc) : Type(TypeKind::Any, loc) {}

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<AnyType>(loc);
  }
  bool isEquivalent(const Type &other) const override;
  std::string toString() const override;
  static bool classof(const Type *T) { return T->getKind() == TypeKind::Any; }
};

class PointerType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::Pointer;
  PointerType(TypePtr pointee, SourceLocation loc)
      : Type(TypeKind::Pointer, loc), pointee(std::move(pointee)) {}

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<PointerType>(pointee->clone(), loc);
  }
  bool isEquivalent(const Type &other) const override {
    if (!other.is<PointerType>())
      return false;
    return pointee->isEquivalent(
        *static_cast<const PointerType &>(other).getPointee());
  }
  std::string toString() const override;
  [[nodiscard]] const Type *getPointee() const { return pointee.get(); }
  static bool classof(const Type *T) {
    return T->getKind() == TypeKind::Pointer;
  }

private:
  TypePtr pointee;
};

class ReferenceType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::Reference;
  ReferenceType(TypePtr inner, SourceLocation loc)
      : Type(TypeKind::Reference, loc), inner(std::move(inner)) {}

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<ReferenceType>(inner->clone(), loc);
  }
  bool isEquivalent(const Type &other) const override {
    if (!other.is<ReferenceType>())
      return false;
    return inner->isEquivalent(
        *static_cast<const ReferenceType &>(other).getInner());
  }
  std::string toString() const override;
  [[nodiscard]] const Type *getInner() const { return inner.get(); }
  static bool classof(const Type *T) {
    return T->getKind() == TypeKind::Reference;
  }

private:
  TypePtr inner;
};

class ArrayType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::Array;

  // [FIX] Defer definitions to .cpp
  ~ArrayType() override;
  ArrayType(TypePtr element, std::unique_ptr<Expr> sizeExpr,
            SourceLocation loc);

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Type> clone() const override;

  bool isEquivalent(const Type &other) const override;
  std::string toString() const override;

  [[nodiscard]] const Type *getElementType() const { return elementType.get(); }
  [[nodiscard]] const Expr *getSizeExpr() const { return sizeExpr.get(); }
  static bool classof(const Type *T) { return T->getKind() == TypeKind::Array; }

  bool isArray() const override { return true; }

private:
  TypePtr elementType;
  std::unique_ptr<Expr> sizeExpr;
};

class SliceType : public Type {
  TypePtr elementType;

public:
  static constexpr TypeKind classKind = TypeKind::Slice;

  // Fix 1: Accept TypePtr and use std::move
  SliceType(TypePtr element, SourceLocation loc = SourceLocation())
      : Type(TypeKind::Slice, loc), elementType(std::move(element)) {}

  // Fix 2: Return the raw pointer from the unique_ptr
  const Type *getElementType() const { return elementType.get(); }

  void accept(ASTVisitor &v) const override;

  bool isEquivalent(const Type &other) const override {
    if (const auto *otherSlice = llvm::dyn_cast<SliceType>(&other)) {
      return elementType->isEquivalent(*otherSlice->elementType);
    }
    return false;
  }

  std::string toString() const override {
    return elementType->toString() + "[]";
  }

  // Fix 3: The constructor now matches the TypePtr returned by clone()
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<SliceType>(elementType->clone(), getLoc());
  }

  static bool classof(const Type *T) { return T->getKind() == TypeKind::Slice; }
};

class MapType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::Map;
  MapType(TypePtr key, TypePtr value, SourceLocation loc)
      : Type(TypeKind::Map, loc), keyType(std::move(key)),
        valueType(std::move(value)) {}

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<MapType>(keyType->clone(), valueType->clone(), loc);
  }
  bool isEquivalent(const Type &other) const override;
  std::string toString() const override;
  [[nodiscard]] const Type *getKeyType() const { return keyType.get(); }
  [[nodiscard]] const Type *getValueType() const { return valueType.get(); }
  static bool classof(const Type *T) { return T->getKind() == TypeKind::Map; }

private:
  TypePtr keyType;
  TypePtr valueType;
};

class ClosureType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::Closure;
  ClosureType(TypePtr returnType, std::vector<TypePtr> paramTypes,
              SourceLocation loc)
      : Type(TypeKind::Closure, loc), returnType(std::move(returnType)),
        paramTypes(std::move(paramTypes)) {}

  void accept(ASTVisitor &v) const override;

  std::unique_ptr<Type> clone() const override {
    std::vector<TypePtr> clonedParams;
    for (const auto &p : paramTypes)
      clonedParams.push_back(p->clone());
    return std::make_unique<ClosureType>(returnType->clone(),
                                         std::move(clonedParams), loc);
  }

  bool isEquivalent(const Type &other) const override {
    if (!other.is<ClosureType>())
      return false;
    auto &c = static_cast<const ClosureType &>(other);
    if (!returnType->isEquivalent(*c.getReturnType()))
      return false;
    if (paramTypes.size() != c.getParamTypes().size())
      return false;
    for (size_t i = 0; i < paramTypes.size(); ++i) {
      if (!paramTypes[i]->isEquivalent(*c.getParamTypes()[i]))
        return false;
    }
    return true;
  }

  std::string toString() const override { return "closure"; }
  [[nodiscard]] const Type *getReturnType() const { return returnType.get(); }
  [[nodiscard]] const std::vector<TypePtr> &getParamTypes() const {
    return paramTypes;
  }

  static bool classof(const Type *T) {
    return T->getKind() == TypeKind::Closure;
  }

private:
  TypePtr returnType;
  std::vector<TypePtr> paramTypes;
};

class FunctionType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::Function;
  FunctionType(TypePtr returnType, std::vector<TypePtr> paramTypes,
               bool isVariadic, bool isInterrupt, SourceLocation loc)
      : Type(TypeKind::Function, loc), returnType(std::move(returnType)),
        paramTypes(std::move(paramTypes)), isVariadic(isVariadic),
        isInterrupt(isInterrupt) {}

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Type> clone() const override {
    std::vector<TypePtr> clonedParams;
    for (const auto &p : paramTypes)
      clonedParams.push_back(p->clone());
    return std::make_unique<FunctionType>(returnType->clone(),
                                          std::move(clonedParams), isVariadic,
                                          isInterrupt, loc);
  }
  bool isEquivalent(const Type &other) const override {
    if (!other.is<FunctionType>())
      return false;
    auto &f = static_cast<const FunctionType &>(other);

    // Check flags
    if (isVariadic != f.isVariadicFunc() || isInterrupt != f.isInterruptFunc())
      return false;

    // Check return type
    if (!returnType->isEquivalent(*f.getReturnType()))
      return false;

    // Check parameters
    if (paramTypes.size() != f.getParamTypes().size())
      return false;
    for (size_t i = 0; i < paramTypes.size(); ++i) {
      if (!paramTypes[i]->isEquivalent(*f.getParamTypes()[i]))
        return false;
    }
    return true;
  }
  std::string toString() const override { return "func"; }
  [[nodiscard]] const Type *getReturnType() const { return returnType.get(); }
  [[nodiscard]] const std::vector<TypePtr> &getParamTypes() const {
    return paramTypes;
  }
  static bool classof(const Type *T) {
    return T->getKind() == TypeKind::Function;
  }
  bool isVariadicFunc() const { return isVariadic; }
  bool isInterruptFunc() const { return isInterrupt; }

private:
  TypePtr returnType;
  std::vector<TypePtr> paramTypes;
  bool isVariadic;
  bool isInterrupt;
};

class NamedType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::Named;
  struct GenericArg {
    TypePtr type;
    Variance variance;
  };

  NamedType(std::string name, std::vector<GenericArg> args, SourceLocation loc,
            bool isRefClass = false)
      : Type(TypeKind::Named, loc), name(std::move(name)),
        genericArgs(std::move(args)), isRefClassFlag(isRefClass) {}

  NamedType(std::string name, std::vector<TypePtr> &&simpleArgs,
            SourceLocation loc, bool isRefClass = false)
      : Type(TypeKind::Named, loc), name(std::move(name)),
        isRefClassFlag(isRefClass) {
    for (auto &t : simpleArgs)
      genericArgs.push_back({std::move(t), Variance::Invariant});
  }

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Type> clone() const override {
    std::vector<GenericArg> newArgs;
    for (auto &arg : genericArgs) {
      newArgs.push_back({arg.type->clone(), arg.variance});
    }
    return std::make_unique<NamedType>(name, std::move(newArgs), loc,
                                       isRefClassFlag);
  }
  bool isEquivalent(const Type &other) const override;
  std::string toString() const override;
  [[nodiscard]] const std::string &getName() const { return name; }
  [[nodiscard]] const std::vector<GenericArg> &getGenericArgs() const {
    return genericArgs;
  }
  static bool classof(const Type *T) { return T->getKind() == TypeKind::Named; }
  bool isRefClass() const override { return isRefClassFlag; }
  void setRefClass(bool isRef) { isRefClassFlag = isRef; }

private:
  std::string name;
  std::vector<GenericArg> genericArgs;
  bool isRefClassFlag = false;
};

class NullableType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::Nullable;
  NullableType(TypePtr inner, SourceLocation loc)
      : Type(TypeKind::Nullable, loc), innerType(std::move(inner)) {}

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<NullableType>(innerType->clone(), loc);
  }
  bool isEquivalent(const Type &other) const override {
    if (!other.is<NullableType>())
      return false;
    return innerType->isEquivalent(
        *static_cast<const NullableType &>(other).getInner());
  }
  std::string toString() const override;
  [[nodiscard]] const Type *getInner() const { return innerType.get(); }
  static bool classof(const Type *T) {
    return T->getKind() == TypeKind::Nullable;
  }

private:
  TypePtr innerType;
};

class EnumType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::Enum;
  EnumType(std::string name, std::vector<std::string> members,
           SourceLocation loc)
      : Type(TypeKind::Enum, loc), name(std::move(name)),
        members(std::move(members)) {}

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<EnumType>(name, members, loc);
  }
  [[nodiscard]] const std::string &getName() const { return name; }
  [[nodiscard]] const std::vector<std::string> &getMembers() const {
    return members;
  }
  [[nodiscard]] bool hasMember(const std::string &mem) const {
    for (const auto &m : members)
      if (m == mem)
        return true;
    return false;
  }
  bool isEquivalent(const Type &other) const override;
  std::string toString() const override;
  static bool classof(const Type *T) { return T->getKind() == TypeKind::Enum; }

private:
  std::string name;
  std::vector<std::string> members;
};

class LockType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::Lock;
  LockType(TypePtr inner, SourceLocation loc)
      : Type(TypeKind::Lock, loc), inner(std::move(inner)) {}

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<LockType>(inner->clone(), loc);
  }
  [[nodiscard]] const Type *getInner() const { return inner.get(); }
  const Type *stripModifiers() const override;
  bool isEquivalent(const Type &other) const override;
  std::string toString() const override;
  static bool classof(const Type *T) { return T->getKind() == TypeKind::Lock; }

private:
  TypePtr inner;
};

class ViewType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::View;
  ViewType(TypePtr inner, SourceLocation loc)
      : Type(TypeKind::View, loc), inner(std::move(inner)) {}

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<ViewType>(inner->clone(), loc);
  }
  [[nodiscard]] const Type *getInner() const { return inner.get(); }
  bool isImmutable() const override;
  const Type *stripModifiers() const override;
  bool isEquivalent(const Type &other) const override;
  std::string toString() const override;
  static bool classof(const Type *T) { return T->getKind() == TypeKind::View; }

private:
  TypePtr inner;
};

class MutType : public Type {
public:
  static constexpr TypeKind classKind = TypeKind::Mut;
  MutType(TypePtr inner, SourceLocation loc)
      : Type(TypeKind::Mut, loc), inner(std::move(inner)) {}

  void accept(ASTVisitor &v) const override;
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<MutType>(inner->clone(), loc);
  }
  [[nodiscard]] const Type *getInner() const { return inner.get(); }
  const Type *stripModifiers() const override;
  bool isEquivalent(const Type &other) const override;
  std::string toString() const override;
  static bool classof(const Type *T) { return T->getKind() == TypeKind::Mut; }

private:
  TypePtr inner;
};

class VolatileType : public Type {
  TypePtr inner;

public:
  static constexpr TypeKind classKind = TypeKind::Volatile;
  VolatileType(TypePtr inner, SourceLocation loc)
      : Type(TypeKind::Volatile, loc), inner(std::move(inner)) {}
  const Type *getInner() const { return inner.get(); }

  const Type *stripModifiers() const override;

  // [FIX] Removed inline body
  void accept(ASTVisitor &v) const override;

  bool isEquivalent(const Type &other) const override {
    if (!other.is<VolatileType>())
      return false;
    return inner->isEquivalent(
        *static_cast<const VolatileType &>(other).getInner());
  }
  std::string toString() const override {
    return "volatile " + inner->toString();
  }

  // [FIX] Added missing clone
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<VolatileType>(inner->clone(), loc);
  }

  // [FIX] Added missing LLVM classof
  static bool classof(const Type *T) {
    return T->getKind() == TypeKind::Volatile;
  }
};

class ConstType : public Type {
  TypePtr inner;

public:
  static constexpr TypeKind classKind = TypeKind::Const;
  ConstType(TypePtr inner, SourceLocation loc)
      : Type(TypeKind::Const, loc), inner(std::move(inner)) {}
  const Type *getInner() const { return inner.get(); }

  bool isImmutable() const override;
  const Type *stripModifiers() const override;

  // [FIX] Removed inline body
  void accept(ASTVisitor &v) const override;

  bool isEquivalent(const Type &other) const override {
    if (!other.is<ConstType>())
      return false;
    return inner->isEquivalent(
        *static_cast<const ConstType &>(other).getInner());
  }
  std::string toString() const override { return "const " + inner->toString(); }
  std::unique_ptr<Type> clone() const override {
    return std::make_unique<ConstType>(inner->clone(), loc);
  }

  // Added missing LLVM classof
  static bool classof(const Type *T) { return T->getKind() == TypeKind::Const; }
};

class WeakType : public Type {
  TypePtr inner;

public:
  static constexpr TypeKind classKind = TypeKind::Weak;
  WeakType(TypePtr inner, SourceLocation loc)
      : Type(TypeKind::Weak, loc), inner(std::move(inner)) {}

  const Type *getInner() const { return inner.get(); }

  void accept(ASTVisitor &v) const override;

  bool isEquivalent(const Type &other) const override {
    if (!other.is<WeakType>())
      return false;
    return inner->isEquivalent(
        *static_cast<const WeakType &>(other).getInner());
  }

  std::string toString() const override { return "weak " + inner->toString(); }

  std::unique_ptr<Type> clone() const override {
    return std::make_unique<WeakType>(inner->clone(), loc);
  }

  static bool classof(const Type *T) { return T->getKind() == TypeKind::Weak; }
};

// --- [NEW] Decimal / Fixed-Point Type ---
class DecimalType : public Type {
  unsigned int precision;
  unsigned int scale;

public:
  static constexpr TypeKind classKind = TypeKind::Decimal;

  DecimalType(unsigned int p, unsigned int s,
              SourceLocation loc = SourceLocation())
      : Type(TypeKind::Decimal, loc), precision(p), scale(s) {}

  unsigned int getPrecision() const { return precision; }
  unsigned int getScale() const { return scale; }

  void accept(ASTVisitor &v) const override;

  bool isEquivalent(const Type &other) const override {
    if (!other.is<DecimalType>())
      return false;
    const auto &otherDec = static_cast<const DecimalType &>(other);
    return precision == otherDec.precision && scale == otherDec.scale;
  }

  std::string toString() const override {
    return "decimal<" + std::to_string(precision) + ", " +
           std::to_string(scale) + ">";
  }

  std::unique_ptr<Type> clone() const override {
    return std::make_unique<DecimalType>(precision, scale, loc);
  }

  bool isNumeric() const override { return true; }

  static bool classof(const Type *T) {
    return T->getKind() == TypeKind::Decimal;
  }
};

} // namespace moksha
