#include "moksha/HIR/HIRType.h"
#include <cassert> // [FIX] Added for assertions
#include <sstream>

using namespace moksha::hir;

// ============================================================================
// [HIRIntType]
// ============================================================================
std::string HIRIntType::toString() const {
  return (isSignedFlag ? "i" : "u") + std::to_string(width);
}

void HIRIntType::Profile(llvm::FoldingSetNodeID &ID) const {
  ID.AddInteger(static_cast<int>(kind));
  ID.AddInteger(width);
  ID.AddBoolean(isSignedFlag);
  ID.AddBoolean(isPtrWidth);
}

// ============================================================================
// [HIRFloatType] - Now successfully uses 'width'
// ============================================================================
std::string HIRFloatType::toString() const {
  // Returns f8, f16, f32, f64 etc.
  return "f" + std::to_string(width);
}

void HIRFloatType::Profile(llvm::FoldingSetNodeID &ID) const {
  ID.AddInteger(static_cast<int>(kind));
  ID.AddInteger(width); // Used for interning unique float types
}

// ============================================================================
// [HIRStringType]
// ============================================================================
std::string HIRStringType::toString() const { return "string"; }

void HIRStringType::Profile(llvm::FoldingSetNodeID &ID) const {
  ID.AddInteger(static_cast<int>(kind));
}

// ============================================================================
// [PrimitiveType]
// ============================================================================

std::string PrimitiveType::toString() const {
  switch (kind) {
  case TypeKind::Void:
    return "void";
  case TypeKind::Bool:
    return "bool";
  case TypeKind::String:
    return "string";
  case TypeKind::Int:
    return "int_generic";
  case TypeKind::Float:
    return "float_generic";
  default:
    return "unknown_primitive";
  }
}

void PrimitiveType::Profile(llvm::FoldingSetNodeID &ID) const {
  ID.AddInteger(static_cast<int>(kind));
  // Primitives are always leaf nodes with implied Ownership::None.
}

// ============================================================================
// [StructType]
// ============================================================================

std::string StructType::toString() const { return name; }

// ============================================================================
// [FunctionType]
// ============================================================================

std::string FunctionType::toString() const {
  std::stringstream ss;
  ss << "fn(";
  for (size_t i = 0; i < paramTypes.size(); ++i) {
    if (paramTypes[i]) {
      ss << paramTypes[i]->toString();
    } else {
      ss << "<?> ";
    }

    if (i < paramTypes.size() - 1)
      ss << ", ";
  }

  if (isVariadic) {
    if (!paramTypes.empty())
      ss << ", ";
    ss << "...";
  }

  ss << ") -> ";

  if (returnType) {
    ss << returnType->toString();
  } else {
    ss << "void";
  }

  if (isInterrupt) {
    ss << " [interrupt]";
  }

  return ss.str();
}

void FunctionType::Profile(llvm::FoldingSetNodeID &ID) const {
  ID.AddInteger(static_cast<int>(kind));
  ID.AddPointer(returnType);
  for (const auto *param : paramTypes) {
    ID.AddPointer(param);
  }

  ID.AddBoolean(isVariadic);
  ID.AddBoolean(isInterrupt);
}

// ============================================================================
// [Arrays & Unions]
// ============================================================================
std::string ArrayType::toString() const {
  return elementType->toString() + "[" + std::to_string(size) + "]";
}
void ArrayType::Profile(llvm::FoldingSetNodeID &ID) const {
  ID.AddInteger(static_cast<int>(kind));
  ID.AddPointer(elementType);
  ID.AddInteger(size);
}

std::string UnionType::toString() const { return name; }

// ============================================================================
// [SliceType]
// ============================================================================
std::string SliceType::toString() const {
  return elementType->toString() + "[]";
}

void SliceType::Profile(llvm::FoldingSetNodeID &ID) const {
  ID.AddInteger(static_cast<int>(kind));
  ID.AddPointer(elementType);
}

// ============================================================================
// [PromiseType]
// ============================================================================
void HIRPromiseType::Profile(llvm::FoldingSetNodeID &ID) const {
  ID.AddInteger(static_cast<int>(kind));
  ID.AddPointer(innerType);
}

std::string HIRPromiseType::toString() const {
  return "promise<" + innerType->toString() + ">";
}
