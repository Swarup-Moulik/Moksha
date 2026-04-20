#include "moksha/AST/Type.h"
#include "moksha/AST/ASTVisitor.h"
#include "moksha/AST/Expr.h"
#include "llvm/Support/Casting.h"

namespace moksha {

// === Null Type ===
void NullType::accept(ASTVisitor &v) const { v.visitNullType(this); }

bool NullType::isEquivalent(const Type &other) const {
  return llvm::isa<NullType>(other);
}

// === Primitive Type ===
void PrimitiveType::accept(ASTVisitor &v) const { v.visitPrimitiveType(this); }

// isEquivalent is INLINE in header, so we DO NOT implement it here.

bool PrimitiveType::isInteger() const {
  switch (scalar) {
  case Scalar::I8:
  case Scalar::I16:
  case Scalar::I32:
  case Scalar::I64:
  case Scalar::ISize:
  case Scalar::U8:
  case Scalar::U16:
  case Scalar::U32:
  case Scalar::U64:
  case Scalar::USize:
  case Scalar::Int:
    return true;
  default:
    return false;
  }
}

bool PrimitiveType::isFloat() const {
  switch (scalar) {
  case Scalar::F8:
  case Scalar::F16:
  case Scalar::F32:
  case Scalar::F64:
    return true;
  default:
    return false;
  }
}

std::string PrimitiveType::toString() const {
  switch (scalar) {
  case Scalar::I8:
    return "char";
  case Scalar::I16:
    return "short";
  case Scalar::I32:
    return "int";
  case Scalar::I64:
    return "long";
  case Scalar::U8:
    return "unsigned char";
  case Scalar::U16:
    return "unsigned short";
  case Scalar::U32:
    return "unsigned int";
  case Scalar::U64:
    return "unsigned long";
  case Scalar::F8:
    return "quarter";
  case Scalar::F16:
    return "half";
  case Scalar::F32:
    return "float";
  case Scalar::F64:
    return "double";
  case Scalar::ISize:
    return "isize";
  case Scalar::USize:
    return "usize";
  case Scalar::Bool:
    return "bool";
  case Scalar::Char:
    return "char";
  case Scalar::String:
    return "string";
  case Scalar::Void:
    return "void";
  default:
    return "unknown_primitive";
  }
}

// === Any Type ===
void AnyType::accept(ASTVisitor &v) const { v.visitAnyType(this); }

bool AnyType::isEquivalent(const Type &other) const {
  return llvm::isa<AnyType>(other);
}

std::string AnyType::toString() const { return "any"; }

// === Pointer Type ===
void PointerType::accept(ASTVisitor &v) const { v.visitPointerType(this); }

// === Reference Type ===
void ReferenceType::accept(ASTVisitor &v) const { v.visitReferenceType(this); }

// === Array Type ===
// ArrayType methods are NOT inline in Type.h
ArrayType::ArrayType(TypePtr element, std::unique_ptr<Expr> sizeExpr,
                     SourceLocation loc)
    : Type(TypeKind::Array, loc), elementType(std::move(element)),
      sizeExpr(std::move(sizeExpr)) {}

ArrayType::~ArrayType() = default;

void ArrayType::accept(ASTVisitor &v) const { v.visitArrayType(this); }

std::unique_ptr<Type> ArrayType::clone() const {
  std::unique_ptr<Expr> clonedSize = nullptr;
  if (sizeExpr) {
    // Now safely clones ANY expression (BinaryExpr, SizeOfExpr, etc.)
    clonedSize = sizeExpr->clone();
  }
  return std::make_unique<ArrayType>(elementType->clone(),
                                     std::move(clonedSize), loc);
}

bool ArrayType::isEquivalent(const Type &other) const {
  if (const auto *otherArr = llvm::dyn_cast<ArrayType>(&other)) {
    // 1. Check element types
    if (!elementType->isEquivalent(*otherArr->elementType))
      return false;

    bool thisHasSize = (sizeExpr != nullptr);
    bool otherHasSize = (otherArr->sizeExpr != nullptr);

    // 2. If one has a size and the other doesn't, they are not equivalent
    if (thisHasSize != otherHasSize)
      return false;

    // 3. If both have sizes, the numbers MUST match exactly
    if (thisHasSize && otherHasSize) {
      auto thisInt = llvm::dyn_cast<IntegerLiteral>(sizeExpr.get());
      auto otherInt = llvm::dyn_cast<IntegerLiteral>(otherArr->sizeExpr.get());
      if (thisInt && otherInt) {
        if (thisInt->getValue() != otherInt->getValue()) {
          return false; // The literal sizes are different
        }
      } else {
        return false;
      }
    }
    return true;
  }
  return false;
}

std::string ArrayType::toString() const {
  std::string s = elementType->toString() + "[";

  if (sizeExpr) {
    // Dynamically cast to an IntegerLiteral to print the exact number (e.g.,
    // [2])
    if (auto intLit = llvm::dyn_cast<IntegerLiteral>(sizeExpr.get())) {
      s += std::to_string(intLit->getValue());
    } else {
      s += "...";
    }
  }

  s += "]";
  return s;
}

// === Slice Type ===
void SliceType::accept(ASTVisitor &v) const { v.visitSliceType(this); }

// === Map Type ===
void MapType::accept(ASTVisitor &v) const { v.visitMapType(this); }

bool MapType::isEquivalent(const Type &other) const {
  if (const auto *o = llvm::dyn_cast<MapType>(&other)) {
    return keyType->isEquivalent(*o->keyType) &&
           valueType->isEquivalent(*o->valueType);
  }
  return false;
}

std::string MapType::toString() const {
  return "table<" + keyType->toString() + ", " + valueType->toString() + ">";
}

// === Function Type ===
void FunctionType::accept(ASTVisitor &v) const { v.visitFunctionType(this); }

// === Named Type ===
void NamedType::accept(ASTVisitor &v) const { v.visitNamedType(this); }

bool NamedType::isEquivalent(const Type &other) const {
  if (const auto *otherNamed = llvm::dyn_cast<NamedType>(&other)) {
    // 1. Names must match
    if (name != otherNamed->name)
      return false;

    // 2. Argument counts must match
    if (genericArgs.size() != otherNamed->genericArgs.size())
      return false;

    // 3. Arguments must be recursively equivalent AND match variance
    for (size_t i = 0; i < genericArgs.size(); ++i) {
      if (genericArgs[i].variance != otherNamed->genericArgs[i].variance)
        return false;
      if (!genericArgs[i].type->isEquivalent(*otherNamed->genericArgs[i].type))
        return false;
    }
    return true;
  }
  return false;
}

std::string NamedType::toString() const {
  std::string s = name;
  if (!genericArgs.empty()) {
    s += "<";
    for (size_t i = 0; i < genericArgs.size(); ++i) {
      if (i > 0)
        s += ", ";
      s += genericArgs[i].type->toString();
    }
    s += ">";
  }
  return s;
}

// === Nullable Type ===
void NullableType::accept(ASTVisitor &v) const { v.visitNullableType(this); }

// === Enum Type ===
void EnumType::accept(ASTVisitor &v) const { v.visitEnumType(this); }

bool EnumType::isEquivalent(const Type &other) const {
  if (const auto *otherEnum = llvm::dyn_cast<EnumType>(&other)) {
    return name == otherEnum->name;
  }
  return false;
}

std::string EnumType::toString() const { return "enum " + name; }

// === Lock Type ===
void LockType::accept(ASTVisitor &v) const { v.visitLockType(this); }

bool LockType::isEquivalent(const Type &other) const {
  if (const auto *otherLock = llvm::dyn_cast<LockType>(&other)) {
    return inner->isEquivalent(*otherLock->inner);
  }
  return false;
}

std::string LockType::toString() const { return "lock " + inner->toString(); }

// === View Type ===
void ViewType::accept(ASTVisitor &v) const { v.visitViewType(this); }

bool ViewType::isEquivalent(const Type &other) const {
  if (const auto *otherView = llvm::dyn_cast<ViewType>(&other)) {
    return inner->isEquivalent(*otherView->inner);
  }
  return false;
}

std::string ViewType::toString() const { return "view " + inner->toString(); }

// === Mut Type ===
void MutType::accept(ASTVisitor &v) const { v.visitMutType(this); }

bool MutType::isEquivalent(const Type &other) const {
  if (const auto *otherMut = llvm::dyn_cast<MutType>(&other)) {
    return inner->isEquivalent(*otherMut->inner);
  }
  return false;
}

std::string MutType::toString() const { return "mut " + inner->toString(); }

// === Weak Type ===
void WeakType::accept(ASTVisitor &v) const { v.visitWeakType(this); }

// === Decimal Type ===
void DecimalType::accept(ASTVisitor &v) const { v.visitDecimalType(this); }

void VolatileType::accept(ASTVisitor &v) const { v.visitVolatileType(this); }

void ConstType::accept(ASTVisitor &v) const { v.visitConstType(this); }

std::string PointerType::toString() const { return "*" + pointee->toString(); }

std::string ReferenceType::toString() const { return "&" + inner->toString(); }

std::string NullableType::toString() const {
  if (llvm::isa<WeakType>(innerType.get())) {
    return innerType->toString();
  }
  return innerType->toString() + "?";
}

void ClosureType::accept(ASTVisitor &v) const { v.visitClosureType(this); }

void PromiseType::accept(ASTVisitor &v) const { v.visitPromiseType(this); }

// === View Type ===
bool ViewType::isImmutable() const { return true; }
const Type *ViewType::stripModifiers() const { return inner->stripModifiers(); }

// === Const Type ===
bool ConstType::isImmutable() const { return true; }
const Type *ConstType::stripModifiers() const {
  return inner->stripModifiers();
}

// === Mut Type ===
const Type *MutType::stripModifiers() const { return inner->stripModifiers(); }

// === Lock Type ===
const Type *LockType::stripModifiers() const { return inner->stripModifiers(); }

// === Volatile Type ===
const Type *VolatileType::stripModifiers() const {
  return inner->stripModifiers();
}

} // namespace moksha
