#pragma once

#include <string>
#include <vector>
#include <memory>
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"

namespace moksha {
namespace hir {

enum class TypeKind {
    Void,
    Int,
    Float,
    Bool,
    Struct,
    Function,
    Pointer
};

// Explicit ownership semantics for HIR
enum class Ownership {
    None,    // Value types (primitives, structs) or non-owning references
    Owned,   // Strong reference (Box<T>, unique_ptr)
    Borrowed,// Weak/Unowned reference (&T)
    Shared   // Thread-safe shared (Arc<T>)
};

/// Base class for all HIR types.
/// INVARIANTS:
/// 1. HIRType instances are immutable once created.
/// 2. Pointer equality implies type equality (if uniqued by context).
/// 3. Ownership is encoded ONLY in PointerType or specific bindings, never in Struct/Function defs.
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

// Primitives are always value types (Ownership::None)
class PrimitiveType : public HIRType {
public:
    explicit PrimitiveType(TypeKind kind)
        : HIRType(kind, Ownership::None) {}

    std::string toString() const override;
    void Profile(llvm::FoldingSetNodeID &ID) const override;

    static bool classof(const HIRType *T) {
        switch (T->getKind()) {
            case TypeKind::Void:
            case TypeKind::Int:
            case TypeKind::Float:
            case TypeKind::Bool:
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

class StructType : public HIRType {
    std::string name;
    std::vector<const HIRType*> fields; // [Fixed] Enforce immutability

public:
    // Struct definition is neutral (Ownership::None)
    StructType(std::string name, std::vector<const HIRType*> fields)
        : HIRType(TypeKind::Struct, Ownership::None),
          name(std::move(name)), fields(std::move(fields)) {}

    llvm::ArrayRef<const HIRType*> getFields() const { return fields; }
    llvm::StringRef getName() const { return name; }

    std::string toString() const override;
    void Profile(llvm::FoldingSetNodeID &ID) const override;

    static bool classof(const HIRType *T) { return T->getKind() == TypeKind::Struct; }
};

class FunctionType : public HIRType {
    const HIRType* returnType; // [Fixed] Enforce immutability
    std::vector<const HIRType*> paramTypes;

public:
    // NOTE: FunctionType does not encode capture ownership.
    // Closure environment ownership is modeled at HIRExpr level (HIRLambdaExpr).
    FunctionType(const HIRType* ret, std::vector<const HIRType*> params)
        : HIRType(TypeKind::Function, Ownership::None), returnType(ret), paramTypes(std::move(params)) {}

    const HIRType* getReturnType() const { return returnType; }
    llvm::ArrayRef<const HIRType*> getParamTypes() const { return paramTypes; }

    std::string toString() const override;
    void Profile(llvm::FoldingSetNodeID &ID) const override;

    static bool classof(const HIRType *T) { return T->getKind() == TypeKind::Function; }
};

} // namespace hir
} // namespace moksha
