#include "moksha/HIR/HIRModule.h"
#include "moksha/HIR/HIRFunction.h"
#include "moksha/HIR/HIRStmt.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
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

HIRType *HIRModule::getVoidType() {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Void));
  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return existing;
  auto *newType = new (typeAllocator) PrimitiveType(TypeKind::Void);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

HIRType *HIRModule::getBoolType() {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Bool));
  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return existing;
  auto *newType = new (typeAllocator) PrimitiveType(TypeKind::Bool);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

HIRStringType *HIRModule::getStringType() {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::String));
  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return llvm::cast<HIRStringType>(existing);
  auto *newType = new (typeAllocator) HIRStringType();
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

// Full Type Interning Implementations:
HIRIntType *HIRModule::getIntType(uint16_t width, bool isSigned,
                                  bool isPtrWidth) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Int));
  ID.AddInteger(width);
  ID.AddBoolean(isSigned);
  ID.AddBoolean(isPtrWidth);

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return llvm::cast<HIRIntType>(existing);

  auto *newType = new (typeAllocator) HIRIntType(width, isSigned, isPtrWidth);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

HIRFloatType *HIRModule::getFloatType(uint16_t width) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Float));
  ID.AddInteger(width);

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return static_cast<HIRFloatType *>(existing);

  auto *newType = new (typeAllocator) HIRFloatType(width);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

HIRDecimalType *HIRModule::getDecimalType(unsigned int precision,
                                          unsigned int scale) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Decimal));
  ID.AddInteger(precision);
  ID.AddInteger(scale);

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return static_cast<HIRDecimalType *>(existing);

  auto *newType = new (typeAllocator) HIRDecimalType(precision, scale);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

ArrayType *HIRModule::getArrayType(const HIRType *element, uint64_t size) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Array));
  ID.AddPointer(element);
  ID.AddInteger(size);

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return llvm::cast<ArrayType>(existing);

  auto *newType = new (typeAllocator) ArrayType(element, size);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

SliceType *HIRModule::getSliceType(const HIRType *element) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Slice));
  ID.AddPointer(element);

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return static_cast<SliceType *>(existing);

  auto *newType = new (typeAllocator) SliceType(element);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

HIRPromiseType *HIRModule::getPromiseType(const HIRType *inner) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Promise));
  ID.AddPointer(inner);

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos)) {
    return static_cast<HIRPromiseType *>(existing);
  }

  auto *newType = new (typeAllocator) HIRPromiseType(inner);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

HIRViewType *HIRModule::getViewType(const HIRType *inner) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::View));
  ID.AddPointer(inner);
  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return static_cast<HIRViewType *>(existing);
  auto *newType = new (typeAllocator) HIRViewType(inner);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

HIRMutType *HIRModule::getMutType(const HIRType *inner) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Mut));
  ID.AddPointer(inner);
  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return static_cast<HIRMutType *>(existing);
  auto *newType = new (typeAllocator) HIRMutType(inner);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

HIRLockType *HIRModule::getLockType(const HIRType *inner) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Lock));
  ID.AddPointer(inner);
  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return static_cast<HIRLockType *>(existing);
  auto *newType = new (typeAllocator) HIRLockType(inner);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

HIRConstType *HIRModule::getConstType(const HIRType *inner) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Const));
  ID.AddPointer(inner);
  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return static_cast<HIRConstType *>(existing);
  auto *newType = new (typeAllocator) HIRConstType(inner);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

HIRVolatileType *HIRModule::getVolatileType(const HIRType *inner) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Volatile));
  ID.AddPointer(inner);
  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return static_cast<HIRVolatileType *>(existing);
  auto *newType = new (typeAllocator) HIRVolatileType(inner);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

UnionType *HIRModule::getUnionType(std::string name,
                                   std::vector<const HIRType *> fields,
                                   std::vector<std::string> fieldNames) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Union));
  ID.AddString(name);

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return llvm::cast<UnionType>(existing);

  auto *newType = new (typeAllocator)
      UnionType(std::move(name), std::move(fields), std::move(fieldNames));
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

PrimitiveType *HIRModule::getPrimitiveType(TypeKind kind) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(kind));
  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return llvm::cast<PrimitiveType>(existing);

  auto *newType = new (typeAllocator) PrimitiveType(kind);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

StructType *HIRModule::getStructType(std::string name,
                                     std::vector<const HIRType *> fields,
                                     std::vector<std::string> fieldNames) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Struct));
  ID.AddString(name);

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return llvm::cast<StructType>(existing);

  auto *newType = new (typeAllocator)
      StructType(std::move(name), std::move(fields), std::move(fieldNames));
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

PointerType *HIRModule::getPointerType(const HIRType *pointee, Ownership own,
                                       BorrowState state) {
  // Enforce invariant
  assert(pointee && "Cannot create PointerType with null pointee");

  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Pointer));
  ID.AddPointer(pointee);
  ID.AddInteger(static_cast<int>(own));
  ID.AddInteger(static_cast<int>(state)); // [NEW] Hash the borrow state!

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return llvm::cast<PointerType>(existing);

  auto *newType =
      new (typeAllocator) PointerType(pointee, own, state); // [NEW] Pass state
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

FunctionType *HIRModule::getFunctionType(const HIRType *ret,
                                         std::vector<const HIRType *> params,
                                         bool isVariadic, bool isInterrupt) {
  // Enforce invariant
  assert(ret && "Cannot create FunctionType with null return type");

  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Function));
  ID.AddPointer(ret);
  for (const auto *param : params) {
    assert(param && "Function parameter type cannot be null");
    ID.AddPointer(param);
  }

  // Ensure the FFI flags are factored into the unique Type ID Hash!
  ID.AddBoolean(isVariadic);
  ID.AddBoolean(isInterrupt);

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return llvm::cast<FunctionType>(existing);

  // Pass the flags into the constructor
  auto *newType = new (typeAllocator)
      FunctionType(ret, std::move(params), isVariadic, isInterrupt);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

HIRClosureType *HIRModule::getClosureType(const HIRType *ret,
                                          std::vector<const HIRType *> params) {
  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Closure));
  ID.AddPointer(ret);
  for (const auto *p : params) {
    ID.AddPointer(p);
  }

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return static_cast<HIRClosureType *>(existing);

  auto *newType = new (typeAllocator) HIRClosureType(ret, std::move(params));
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

HIRNullableType *HIRModule::getNullableType(const HIRType *inner) {
  assert(inner && "Cannot create a NullableType of null");

  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Nullable));
  ID.AddPointer(inner);

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return static_cast<HIRNullableType *>(existing);

  auto *newType = new (typeAllocator) HIRNullableType(inner);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

HIRWeakType *HIRModule::getWeakType(const HIRType *inner) {
  assert(inner && "Cannot create a WeakType of null");

  llvm::FoldingSetNodeID ID;
  ID.AddInteger(static_cast<int>(TypeKind::Weak));
  ID.AddPointer(inner);

  void *insertPos = nullptr;
  if (HIRType *existing = uniqueTypes.FindNodeOrInsertPos(ID, insertPos))
    return static_cast<HIRWeakType *>(existing);

  auto *newType = new (typeAllocator) HIRWeakType(inner);
  uniqueTypes.InsertNode(newType, insertPos);
  return newType;
}

// ============================================================================
// [Debugging]
// ============================================================================

void HIRModule::dump(llvm::raw_ostream &os) const {
  os << "Module: " << name << "\n";

  if (!classes.empty()) {
    os << "  Classes:\n";
    for (const auto &c : classes) {
      if (c)
        c->dump(os, 2);
    }
  }

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

// ============================================================================
// [Class Management]
// ============================================================================

void HIRModule::addClass(std::unique_ptr<HIRClass> cls) {
  classes.push_back(std::move(cls));
}

llvm::ArrayRef<HIRClass *> HIRModule::getClasses() const {
  classCache.clear();
  for (const auto &c : classes) {
    classCache.push_back(c.get());
  }
  return classCache;
}

HIRClass *HIRModule::getClass(llvm::StringRef name) const {
  for (const auto &c : classes) {
    if (c->getName() == name)
      return c.get();
  }
  return nullptr;
}
