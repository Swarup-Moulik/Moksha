#include "moksha/AST/Type.h"
#include "moksha/AST/ASTVisitor.h"
#include "moksha/AST/Expr.h"
#include "llvm/Support/Casting.h"

namespace moksha {

// === Null Type ===
void NullType::accept(ASTVisitor &v) const { v.visitNullableType(nullptr); }

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

// === Any Type ===
void AnyType::accept(ASTVisitor &v) const { v.visitAnyType(this); }

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
  return std::make_unique<ArrayType>(elementType->clone(), nullptr, loc);
}

bool ArrayType::isEquivalent(const Type &other) const {
  if (const auto *otherArr = llvm::dyn_cast<ArrayType>(&other)) {
    if (!elementType->isEquivalent(*otherArr->elementType))
      return false;
    bool thisHasSize = (sizeExpr != nullptr);
    bool otherHasSize = (otherArr->sizeExpr != nullptr);
    return thisHasSize == otherHasSize;
  }
  return false;
}

std::string ArrayType::toString() const {
  std::string s = elementType->toString() + "[";
  if (sizeExpr)
    s += "...";
  s += "]";
  return s;
}

// === Map Type ===
void MapType::accept(ASTVisitor &v) const { v.visitMapType(this); }

// === Function Type ===
void FunctionType::accept(ASTVisitor &v) const { v.visitFunctionType(this); }

// === Named Type ===
void NamedType::accept(ASTVisitor &v) const { v.visitNamedType(this); }

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

} // namespace moksha
