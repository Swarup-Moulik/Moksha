#pragma once

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/StringRef.h"
#include <memory>
#include <string>
#include <vector>

namespace moksha {
namespace hir {

enum class TypeKind {
  Void,
  Int,
  Float,
  Bool,
  String,
  Struct,
  Union,
  Array,
  Function,
  Pointer,
  Reference,
  Nullable
};

// Explicit ownership semantics for HIR
enum class Ownership {
  None,     // Value types (primitives, structs) or non-owning references
  Owned,    // Strong reference (Box<T>, unique_ptr)
  Borrowed, // Weak/Unowned reference (&T)
  Shared    // Thread-safe shared (Arc<T>)
};

/// Base class for all HIR types.
/// INVARIANTS:
/// 1. HIRType instances are immutable once created.
/// 2. Pointer equality implies type equality (if uniqued by context).
/// 3. Ownership is encoded ONLY in PointerType or specific bindings, never in
/// Struct/Function defs.
class HIRType : public llvm::FoldingSetNode {
public:
  virtual ~HIRType() = default;

  TypeKind getKind() const { return kind; }
  Ownership getOwnership() const { return ownership; }

  virtual std::string toString() const = 0;
  virtual void Profile(llvm::FoldingSetNodeID &ID) const = 0;

protected:
  HIRType(TypeKind kind, Ownership ownership)
      : kind(kind), ownership(ownership) {}

  TypeKind kind;
  Ownership ownership;
};

class HIRIntType : public HIRType {
  uint16_t bitWidth;
  bool isSigned;
  bool isPointerWidth; // true for usize/isize

public:
  HIRIntType(uint16_t width, bool isSigned, bool isPtrWidth = false)
      : HIRType(TypeKind::Int, Ownership::None), bitWidth(width),
        isSigned(isSigned), isPointerWidth(isPtrWidth) {}

  uint16_t getBitWidth() const { return bitWidth; }
  bool getIsSigned() const { return isSigned; }
  bool isSystemSize() const { return isPointerWidth; }

  std::string toString() const override {
    if (isPointerWidth)
      return isSigned ? "isize" : "usize";
    return (isSigned ? "i" : "u") + std::to_string(bitWidth);
  }

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(getKind()));
    ID.AddInteger(bitWidth);
    ID.AddBoolean(isSigned);
    ID.AddBoolean(isPointerWidth);
  }

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Int;
  }
};

class HIRFloatType : public HIRType {
  uint16_t bitWidth; // 8 (quarter), 16 (half), 32 (float), 64 (double)

public:
  explicit HIRFloatType(uint16_t width)
      : HIRType(TypeKind::Float, Ownership::None), bitWidth(width) {}

  uint16_t getBitWidth() const { return bitWidth; }

  std::string toString() const override {
    return "f" + std::to_string(bitWidth);
  }

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(getKind()));
    ID.AddInteger(bitWidth);
  }

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Float;
  }
};

class HIRStringType : public HIRType {
public:
  HIRStringType() : HIRType(TypeKind::String, Ownership::None) {}

  std::string toString() const override { return "string"; }

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(getKind()));
  }

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::String;
  }
};

// Primitives are always value types (Ownership::None)
class PrimitiveType : public HIRType {
public:
  explicit PrimitiveType(TypeKind kind) : HIRType(kind, Ownership::None) {}

  std::string toString() const override;
  void Profile(llvm::FoldingSetNodeID &ID) const override;

  static bool classof(const HIRType *T) {
    switch (T->getKind()) {
    case TypeKind::Void:
    case TypeKind::Bool: // [FIX] Int and Float removed from here!
      return true;
    default:
      return false;
    }
  }
};

class PointerType : public HIRType {
  const HIRType *pointee; // [Fixed] Enforce immutability
public:
  PointerType(const HIRType *pointee, Ownership own)
      : HIRType(TypeKind::Pointer, own), pointee(pointee) {}

  const HIRType *getPointee() const { return pointee; }

  std::string toString() const override;
  void Profile(llvm::FoldingSetNodeID &ID) const override;

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Pointer;
  }
};

class ArrayType : public HIRType {
  const HIRType *elementType;
  uint64_t size;

public:
  ArrayType(const HIRType *element, uint64_t size)
      : HIRType(TypeKind::Array, Ownership::None), elementType(element),
        size(size) {}

  const HIRType *getElementType() const { return elementType; }
  uint64_t getSize() const { return size; }

  std::string toString() const override;
  void Profile(llvm::FoldingSetNodeID &ID) const override;

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Array;
  }
};

class StructType : public HIRType {
  std::string name;
  std::vector<const HIRType *>
      fields; // [FIX] Non-const so we can populate later!

public:
  StructType(std::string name, std::vector<const HIRType *> fields)
      : HIRType(TypeKind::Struct, Ownership::None), name(std::move(name)),
        fields(std::move(fields)) {}

  // [NEW] Allows HIRGen::visitClassDecl to populate opaque structs!
  void setFields(std::vector<const HIRType *> newFields) {
    fields = std::move(newFields);
  }

  llvm::ArrayRef<const HIRType *> getFields() const { return fields; }
  llvm::StringRef getName() const { return name; }

  std::string toString() const override;

  // [FIX] Hash ONLY the name, never the fields, to prevent infinite recursion
  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddString(name);
  }

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Struct;
  }
};

class UnionType : public HIRType {
  std::string name;
  std::vector<const HIRType *>
      fields; // [FIX] Non-const so we can populate later!

public:
  UnionType(std::string name, std::vector<const HIRType *> fields)
      : HIRType(TypeKind::Union, Ownership::None), name(std::move(name)),
        fields(std::move(fields)) {}

  // [NEW] Allows mutating opaque unions!
  void setFields(std::vector<const HIRType *> newFields) {
    fields = std::move(newFields);
  }

  llvm::ArrayRef<const HIRType *> getFields() const { return fields; }
  llvm::StringRef getName() const { return name; }

  std::string toString() const override;

  // [FIX] Hash ONLY the name, never the fields, to prevent infinite recursion
  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddString(name);
  }

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Union;
  }
};

class FunctionType : public HIRType {
  const HIRType *returnType; // [Fixed] Enforce immutability
  std::vector<const HIRType *> paramTypes;

public:
  // NOTE: FunctionType does not encode capture ownership.
  // Closure environment ownership is modeled at HIRExpr level (HIRLambdaExpr).
  FunctionType(const HIRType *ret, std::vector<const HIRType *> params)
      : HIRType(TypeKind::Function, Ownership::None), returnType(ret),
        paramTypes(std::move(params)) {}

  const HIRType *getReturnType() const { return returnType; }
  llvm::ArrayRef<const HIRType *> getParamTypes() const { return paramTypes; }

  std::string toString() const override;
  void Profile(llvm::FoldingSetNodeID &ID) const override;

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Function;
  }
};

class HIRNullableType : public HIRType {
  const HIRType *inner;

public:
  HIRNullableType(const HIRType *inner)
      : HIRType(TypeKind::Nullable, Ownership::None), inner(inner) {}

  const HIRType *getInner() const { return inner; }
  std::string toString() const override { return inner->toString() + "?"; }

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddPointer(inner);
  }
  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Nullable;
  }
};

class ReferenceType : public HIRType {
  const HIRType *inner;

public:
  ReferenceType(const HIRType *inner, Ownership own)
      : HIRType(TypeKind::Reference, own), inner(inner) {}

  const HIRType *getInner() const { return inner; }
  std::string toString() const override { return "&" + inner->toString(); }

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(getKind()));
    ID.AddPointer(inner);
  }

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Reference;
  }
};

} // namespace hir
} // namespace moksha
