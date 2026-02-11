#include "moksha/HIR/HIRType.h"
#include <sstream>
#include <cassert> // [FIX] Added for assertions

using namespace moksha::hir;

// ============================================================================
// [PrimitiveType]
// ============================================================================

std::string PrimitiveType::toString() const {
    switch (kind) {
        case TypeKind::Void:  return "void";
        case TypeKind::Int:   return "int";
        case TypeKind::Float: return "float";
        case TypeKind::Bool:  return "bool";
        default: return "unknown_primitive";
    }
}

void PrimitiveType::Profile(llvm::FoldingSetNodeID &ID) const {
    ID.AddInteger(static_cast<int>(kind));
    // Primitives are always leaf nodes with implied Ownership::None.
}

// ============================================================================
// [PointerType]
// ============================================================================

std::string PointerType::toString() const {
    // [FIX] Assert pointee is not null to prevent invalid states
    assert(pointee && "PointerType must have a valid pointee");

    std::string s;
    switch (ownership) {
        case Ownership::Borrowed: s = "&"; break;
        case Ownership::Owned:    s = "Box<"; break;
        case Ownership::Shared:   s = "Arc<"; break;
        case Ownership::None:     s = "*"; break; // Explicit Raw Pointer
        default:                  s = "*"; break;
    }

    s += pointee->toString();

    if (ownership == Ownership::Owned || ownership == Ownership::Shared) {
        s += ">";
    }
    return s;
}

void PointerType::Profile(llvm::FoldingSetNodeID &ID) const {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddPointer(pointee);
    ID.AddInteger(static_cast<int>(ownership));
}

// ============================================================================
// [StructType]
// ============================================================================

std::string StructType::toString() const {
    return name;
}

void StructType::Profile(llvm::FoldingSetNodeID &ID) const {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddString(name);
    // Ownership is always None for definitions

    for (const auto *field : fields) {
        ID.AddPointer(field);
    }
}

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

        if (i < paramTypes.size() - 1) ss << ", ";
    }
    ss << ") -> ";

    if (returnType) {
        ss << returnType->toString();
    } else {
        ss << "void";
    }

    return ss.str();
}

void FunctionType::Profile(llvm::FoldingSetNodeID &ID) const {
    ID.AddInteger(static_cast<int>(kind));
    ID.AddPointer(returnType);
    for (const auto *param : paramTypes) {
        ID.AddPointer(param);
    }
}
