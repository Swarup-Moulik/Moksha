#include "moksha/HIR/HIRModule.h"
#include "moksha/HIR/HIRFunction.h"
#include "moksha/HIR/HIRStmt.h"
#include "llvm/Support/Casting.h"
#include <cassert>
#include <iostream>

using namespace moksha::hir;

HIRModule::HIRModule(std::string moduleName) : name(std::move(moduleName)) {}

HIRModule::~HIRModule() = default;

HIRFunction *HIRModule::getFunction(llvm::StringRef name) const {
  for (const auto &f : functions) {
    if (f->getName() == name)
      return f.get();
  }
  return nullptr;
}
// ============================================================================
// [Type Interning Logic]
// ============================================================================

PrimitiveType *HIRModule::getPrimitiveType(TypeKind kind) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(kind));
  // Primitives are leaf nodes; Ownership is implicit/None, so not added to ID.

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return llvm::cast<PrimitiveType>(existing);

  auto *newType = new (typeAllocator) PrimitiveType(kind);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

StructType *HIRModule::getStructType(std::string name,
                                     std::vector<const HIRType *> fields) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Struct));
  ID.AddString(name);

  // [CRITICAL] Do NOT add Ownership::None here.
  // This must match StructType::Profile in HIRType.cpp exactly.
  // Since StructType is just a layout definition, ownership is not part of its
  // identity.

  for (const auto *field : fields) {
    // Enforce non-null fields to prevent hard-to-debug crashes later
    assert(field && "Struct field type cannot be null");
    ID.AddPointer(field);
  }

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return llvm::cast<StructType>(existing);

  auto *newType =
      new (typeAllocator) StructType(std::move(name), std::move(fields));
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

PointerType *HIRModule::getPointerType(const HIRType *pointee, Ownership own) {
  // Enforce invariant
  assert(pointee && "Cannot create PointerType with null pointee");

  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Pointer));
  ID.AddPointer(pointee);
  ID.AddInteger(static_cast<int>(own));

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return llvm::cast<PointerType>(existing);

  auto *newType = new (typeAllocator) PointerType(pointee, own);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

FunctionType *HIRModule::getFunctionType(const HIRType *ret,
                                         std::vector<const HIRType *> params) {
  // Enforce invariant
  assert(ret && "Cannot create FunctionType with null return type");

  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Function));
  ID.AddPointer(ret);
  for (const auto *param : params) {
    assert(param && "Function parameter type cannot be null");
    ID.AddPointer(param);
  }

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return llvm::cast<FunctionType>(existing);

  auto *newType = new (typeAllocator) FunctionType(ret, std::move(params));
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

// ============================================================================
// [Debugging]
// ============================================================================

void HIRModule::dump(std::ostream &os) const {
  os << "Module: " << name << "\n";

  if (!globals.empty()) {
    os << "  Globals:\n"; // Indented for hierarchy
    for (const auto &g : globals) {
      if (g)
        g->dump(os, 2); // Pass indent level
    }
  }

  if (!functions.empty()) {
    os << "  Functions:\n"; // Indented for hierarchy
    for (const auto &f : functions) {
      if (f)
        f->dump(os, 2); // Pass indent level
    }
  }
}

// ============================================================================
// [Global Management]
// ============================================================================

void HIRModule::addGlobal(std::unique_ptr<HIRStmt> global) {
  globals.push_back(std::move(global));
}

llvm::ArrayRef<HIRStmt *> HIRModule::getGlobals() const {
  globalCache.clear();
  for (const auto &g : globals) {
    globalCache.push_back(g.get());
  }
  return globalCache;
}

// Ensure you also have the implementation for getFunctions()
llvm::ArrayRef<HIRFunction *> HIRModule::getFunctions() const {
  functionCache.clear();
  for (const auto &f : functions) {
    functionCache.push_back(f.get());
  }
  return functionCache;
}

void HIRModule::addFunction(std::unique_ptr<HIRFunction> func) {
  functions.push_back(std::move(func));
}
