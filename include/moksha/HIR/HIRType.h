#pragma once

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/StringRef.h"
#include <memory>
#include <string>
#include <vector>

namespace moksha {
namespace hir {

struct FieldInfo {
  std::string name;
  uint32_t index;
  bool isBitfield;
  uint32_t bitWidth;
  uint32_t bitOffset;

  FieldInfo() : index(0), isBitfield(false), bitWidth(0), bitOffset(0) {}
  FieldInfo(std::string name, uint32_t index, bool isBitfield = false,
            uint32_t bitWidth = 0, uint32_t bitOffset = 0)
      : name(std::move(name)), index(index), isBitfield(isBitfield),
        bitWidth(bitWidth), bitOffset(bitOffset) {}
};

enum class TypeKind {
  Void,
  Int,
  Float,
  Bool,
  String,
  Struct,
  Union,
  Array,
  Slice,
  Function,
  Pointer,
  Reference,
  Nullable,
  Decimal,
  Promise,
  Closure,
  Weak,
  View,
  Mut,
  Lock,
  Const,
  Volatile
};

// Explicit ownership semantics for HIR
enum class Ownership {
  None,     // Value types (primitives, structs) or non-owning references
  Owned,    // Strong reference (Box<T>, unique_ptr)
  Borrowed, // Weak/Unowned reference (&T)
  Shared    // Thread-safe shared (Arc<T>)
};

enum class BorrowState { None, View, Mut, Lock };

/// Base class for all HIR types.
/// INVARIANTS:
/// 1. HIRType instances are immutable once created.
/// 2. Pointer equality implies type equality (if uniqued by context).
/// 3. Ownership is encoded ONLY in PointerType or specific bindings, never in
/// Struct/Function defs.
class HIRType : public llvm::FoldingSetNode {
public:
  virtual ~HIRType() = default;

  void operator delete(void *, llvm::BumpPtrAllocator &, size_t) {}
  void operator delete(void *, size_t) {}

  TypeKind getKind() const { return kind; }
  Ownership getOwnership() const { return ownership; }

  virtual std::string toString() const = 0;
  virtual void Profile(llvm::FoldingSetNodeID &ID) const = 0;

  virtual bool isImmutable() const { return false; }
  virtual const HIRType *stripModifiers() const { return this; }

protected:
  HIRType(TypeKind kind, Ownership ownership)
      : kind(kind), ownership(ownership) {}

  TypeKind kind;
  Ownership ownership;
};

// --- [FIX] Update HIRIntType: Remove bodies ---
class HIRIntType : public HIRType {
  uint16_t width;
  bool isSignedFlag;
  bool isPtrWidth;

public:
  HIRIntType(uint16_t width, bool isSigned, bool isPtrWidth = false)
      : HIRType(TypeKind::Int, Ownership::None), width(width),
        isSignedFlag(isSigned), isPtrWidth(isPtrWidth) {}

  uint16_t getWidth() const { return width; }
  bool isSigned() const { return isSignedFlag; }

  std::string toString() const override; // Logic moved to .cpp
  void
  Profile(llvm::FoldingSetNodeID &ID) const override; // Logic moved to .cpp

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Int;
  }
};

// --- [FIX] Update HIRFloatType: Add width member and remove bodies ---
class HIRFloatType : public HIRType {
  uint16_t width; // Added this member

public:
  // Update constructor to take width
  explicit HIRFloatType(uint16_t width)
      : HIRType(TypeKind::Float, Ownership::None), width(width) {}

  uint16_t getWidth() const { return width; }

  std::string toString() const override; // Logic moved to .cpp
  void Profile(llvm::FoldingSetNodeID &ID) const override;

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Float;
  }
};

// --- [FIX] Update HIRStringType: Remove bodies ---
class HIRStringType : public HIRType {
public:
  HIRStringType() : HIRType(TypeKind::String, Ownership::None) {}
  std::string toString() const override;
  void Profile(llvm::FoldingSetNodeID &ID) const override;
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
    case TypeKind::Bool:
      return true;
    default:
      return false;
    }
  }
};

class PointerType : public HIRType {
  const HIRType *pointee;
  BorrowState borrowState; // [NEW] Store the state!

public:
  PointerType(const HIRType *pointee, Ownership own,
              BorrowState state = BorrowState::None)
      : HIRType(TypeKind::Pointer, own), pointee(pointee), borrowState(state) {}

  const HIRType *getPointee() const { return pointee; }
  BorrowState getBorrowState() const { return borrowState; }

  // [NEW] Fast O(1) Accessors
  bool isMut() const { return borrowState == BorrowState::Mut; }
  bool isView() const { return borrowState == BorrowState::View; }
  bool isLock() const { return borrowState == BorrowState::Lock; }

  std::string toString() const override {
    std::string prefix;
    if (borrowState == BorrowState::Mut)
      prefix = "*mut ";
    else if (borrowState == BorrowState::View)
      prefix = "*view ";
    else if (borrowState == BorrowState::Lock)
      prefix = "*lock ";
    else
      prefix = "*";
    return prefix + pointee->toString();
  }

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddPointer(pointee);
    ID.AddInteger(static_cast<int>(
        borrowState)); // [NEW] Ensure Mut and View are hashed uniquely!
  }

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

class SliceType : public HIRType {
  const HIRType *elementType;

public:
  explicit SliceType(const HIRType *element)
      : HIRType(TypeKind::Slice, Ownership::None), elementType(element) {}

  const HIRType *getElementType() const { return elementType; }

  std::string toString() const override;
  void Profile(llvm::FoldingSetNodeID &ID) const override;

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Slice;
  }
};

class StructType : public HIRType {
  std::string name;
  std::vector<const HIRType *> fields;
  std::vector<std::string> fieldNames;
  bool isPackedFlag;
  bool hasVTableFlag = false;

public:
  StructType(std::string name, std::vector<const HIRType *> fields,
             std::vector<std::string> fieldNames = {}, bool isPacked = false)
      : HIRType(TypeKind::Struct, Ownership::None), name(std::move(name)),
        fields(std::move(fields)), fieldNames(std::move(fieldNames)),
        isPackedFlag(isPacked) {}

  bool isPacked() const { return isPackedFlag; }
  void setPacked(bool packed) { isPackedFlag = packed; }
  bool hasVTable() const { return hasVTableFlag; }
  void setHasVTable(bool v) { hasVTableFlag = v; }
  void setFields(std::vector<const HIRType *> newFields,
                 std::vector<std::string> newNames) {
    fields = std::move(newFields);
    fieldNames = std::move(newNames);
  }

  llvm::ArrayRef<const HIRType *> getFields() const { return fields; }
  llvm::StringRef getName() const { return name; }

  int getFieldIndex(const std::string &searchName) const {
    for (size_t i = 0; i < fieldNames.size(); ++i) {
      if (fieldNames[i] == searchName) {
        return static_cast<int>(i);
      }
    }
    return -1; // Field not found
  }

  std::string toString() const override;

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
  std::vector<const HIRType *> fields;
  std::vector<std::string> fieldNames; // [NEW] Store the names!

public:
  // [FIX] Added fieldNames parameter to the constructor to fix the self-move
  // warning!
  UnionType(std::string name, std::vector<const HIRType *> fields,
            std::vector<std::string> fieldNames = {})
      : HIRType(TypeKind::Union, Ownership::None), name(std::move(name)),
        fields(std::move(fields)), fieldNames(std::move(fieldNames)) {}

  // [FIX] Added newNames parameter to match HIRGen.cpp expectations!
  void setFields(std::vector<const HIRType *> newFields,
                 std::vector<std::string> newNames) {
    fields = std::move(newFields);
    fieldNames = std::move(newNames);
  }

  llvm::ArrayRef<const HIRType *> getFields() const { return fields; }
  llvm::StringRef getName() const { return name; }

  // [NEW] The getFieldIndex Helper for Unions
  int getFieldIndex(const std::string &searchName) const {
    for (size_t i = 0; i < fieldNames.size(); ++i) {
      if (fieldNames[i] == searchName) {
        return static_cast<int>(i);
      }
    }
    return -1; // Field not found
  }

  std::string toString() const override;

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddString(name);
  }

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Union;
  }
};

class FunctionType : public HIRType {
  const HIRType *returnType;
  std::vector<const HIRType *> paramTypes;
  bool isVariadic;
  bool isInterrupt;

public:
  FunctionType(const HIRType *ret, std::vector<const HIRType *> params,
               bool isVariadic = false, bool isInterrupt = false)
      : HIRType(TypeKind::Function, Ownership::None), returnType(ret),
        paramTypes(std::move(params)), isVariadic(isVariadic),
        isInterrupt(isInterrupt) {}

  const HIRType *getReturnType() const { return returnType; }
  llvm::ArrayRef<const HIRType *> getParamTypes() const { return paramTypes; }

  bool isVariadicFunc() const { return isVariadic; }
  bool isInterruptFunc() const { return isInterrupt; }

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
  BorrowState borrowState; // [NEW] Store the state!

public:
  // [NEW] Accept BorrowState, defaulting to None
  ReferenceType(const HIRType *inner, Ownership own,
                BorrowState state = BorrowState::None)
      : HIRType(TypeKind::Reference, own), inner(inner), borrowState(state) {}

  const HIRType *getInner() const { return inner; }
  BorrowState getBorrowState() const { return borrowState; }

  // [NEW] Fast O(1) Accessors
  bool isMut() const { return borrowState == BorrowState::Mut; }
  bool isView() const { return borrowState == BorrowState::View; }
  bool isLock() const { return borrowState == BorrowState::Lock; }

  // [FIX] Format the string based on the active borrow state
  std::string toString() const override {
    std::string prefix = "&";
    if (borrowState == BorrowState::Mut)
      prefix += "mut ";
    else if (borrowState == BorrowState::View)
      prefix += "view ";
    else if (borrowState == BorrowState::Lock)
      prefix += "lock ";

    return prefix + inner->toString();
  }

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(getKind()));
    ID.AddPointer(inner);
    ID.AddInteger(static_cast<int>(borrowState));
  }

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Reference;
  }
};

// --- [NEW] Decimal / Fixed-Point Type ---
class HIRDecimalType : public HIRType {
  unsigned int precision;
  unsigned int scale;

public:
  HIRDecimalType(unsigned int p, unsigned int s)
      : HIRType(TypeKind::Decimal, Ownership::None), precision(p), scale(s) {}

  unsigned int getPrecision() const { return precision; }
  unsigned int getScale() const { return scale; }

  std::string toString() const override {
    return "decimal<" + std::to_string(precision) + ", " +
           std::to_string(scale) + ">";
  }

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(getKind()));
    ID.AddInteger(precision);
    ID.AddInteger(scale);
  }

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Decimal;
  }
};

// Promise / Task Type for Async
class HIRPromiseType : public HIRType {
  const HIRType *inner;

public:
  // Promises "own" their resolved value
  explicit HIRPromiseType(const HIRType *inner)
      : HIRType(TypeKind::Promise, Ownership::Owned), inner(inner) {}

  const HIRType *getInner() const { return inner; }

  std::string toString() const override {
    return "Promise<" + (inner ? inner->toString() : "void") + ">";
  }

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(getKind()));
    ID.AddPointer(inner);
  }

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Promise;
  }
};

// --- [NEW] Closure Type ---
class HIRClosureType : public HIRType {
  const HIRType *returnType;
  std::vector<const HIRType *> paramTypes;

public:
  HIRClosureType(const HIRType *ret, std::vector<const HIRType *> params)
      : HIRType(TypeKind::Closure, Ownership::None), returnType(ret),
        paramTypes(std::move(params)) {}

  const HIRType *getReturnType() const { return returnType; }
  llvm::ArrayRef<const HIRType *> getParamTypes() const { return paramTypes; }

  std::string toString() const override { return "closure"; }

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddPointer(returnType);
    for (const auto *p : paramTypes) {
      ID.AddPointer(p);
    }
  }

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Closure;
  }
};

class HIRWeakType : public HIRType {
  const HIRType *inner;

public:
  // Weak types don't "own" the memory, they borrow it safely via ARC tracking
  explicit HIRWeakType(const HIRType *inner)
      : HIRType(TypeKind::Weak, Ownership::Borrowed), inner(inner) {}

  const HIRType *getInner() const { return inner; }

  std::string toString() const override { return "weak " + inner->toString(); }

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddPointer(inner);
  }

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Weak;
  }
};

class HIRViewType : public HIRType {
  const HIRType *inner;

public:
  explicit HIRViewType(const HIRType *inner)
      : HIRType(TypeKind::View, Ownership::Borrowed), inner(inner) {}
  const HIRType *getInner() const { return inner; }

  // Enforce Immutability at the HIR level
  bool isImmutable() const override { return true; }

  const HIRType *stripModifiers() const override {
    return inner->stripModifiers();
  }

  std::string toString() const override { return "view " + inner->toString(); }

  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddPointer(inner);
  }

  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::View;
  }
};

class HIRMutType : public HIRType {
  const HIRType *inner;

public:
  explicit HIRMutType(const HIRType *inner)
      : HIRType(TypeKind::Mut, Ownership::Borrowed), inner(inner) {}
  const HIRType *getInner() const { return inner; }
  const HIRType *stripModifiers() const override {
    return inner->stripModifiers();
  }
  std::string toString() const override { return "mut " + inner->toString(); }
  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddPointer(inner);
  }
  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Mut;
  }
};

class HIRLockType : public HIRType {
  const HIRType *inner;

public:
  explicit HIRLockType(const HIRType *inner)
      : HIRType(TypeKind::Lock, Ownership::Borrowed), inner(inner) {}
  const HIRType *getInner() const { return inner; }
  const HIRType *stripModifiers() const override {
    return inner->stripModifiers();
  }
  std::string toString() const override { return "lock " + inner->toString(); }
  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddPointer(inner);
  }
  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Lock;
  }
};

class HIRConstType : public HIRType {
  const HIRType *inner;

public:
  explicit HIRConstType(const HIRType *inner)
      : HIRType(TypeKind::Const, Ownership::None), inner(inner) {}
  const HIRType *getInner() const { return inner; }
  bool isImmutable() const override { return true; } // Enforce Immutability
  const HIRType *stripModifiers() const override {
    return inner->stripModifiers();
  }
  std::string toString() const override { return "const " + inner->toString(); }
  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddPointer(inner);
  }
  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Const;
  }
};

class HIRVolatileType : public HIRType {
  const HIRType *inner;

public:
  explicit HIRVolatileType(const HIRType *inner)
      : HIRType(TypeKind::Volatile, Ownership::None), inner(inner) {}
  const HIRType *getInner() const { return inner; }
  const HIRType *stripModifiers() const override {
    return inner->stripModifiers();
  }
  std::string toString() const override {
    return "volatile " + inner->toString();
  }
  void Profile(llvm::FoldingSetNodeID &ID) const override {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddPointer(inner);
  }
  static bool classof(const HIRType *T) {
    return T->getKind() == TypeKind::Volatile;
  }
};

} // namespace hir
} // namespace moksha
