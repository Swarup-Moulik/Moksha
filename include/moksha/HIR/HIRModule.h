#pragma once

#include "moksha/HIR/HIRType.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <string>
#include <vector>

namespace moksha {
namespace hir {

// Forward declarations avoid circular includes
class HIRFunction;
class HIRStmt;
class HIRClass;

class HIRModule {
public:
  explicit HIRModule(std::string moduleName);
  ~HIRModule(); // Implemented in .cpp

  PrimitiveType *getPrimitiveType(TypeKind kind);

  // Module metadata
  llvm::StringRef getName() const { return name; }

  // Function management
  void addFunction(std::unique_ptr<HIRFunction> func); // Implemented in .cpp
  llvm::ArrayRef<HIRFunction *> getFunctions() const;  // Implemented in .cpp
  HIRFunction *getFunction(llvm::StringRef name) const;

  // Type Interning / Canonicalization
  HIRType *getVoidType();
  HIRType *getBoolType();
  HIRStringType *getStringType();
  HIRIntType *getIntType(uint16_t width, bool isSigned,
                         bool isPtrWidth = false);
  HIRFloatType *getFloatType(uint16_t width);

  StructType *getStructType(std::string name,
                            std::vector<const HIRType *> fields);
  UnionType *getUnionType(std::string name,
                          std::vector<const HIRType *> fields);
  PointerType *getPointerType(const HIRType *pointee, Ownership own);
  FunctionType *getFunctionType(const HIRType *ret,
                                std::vector<const HIRType *> params);
  ArrayType *getArrayType(const HIRType *element, uint64_t size);
  HIRNullableType *getNullableType(const HIRType *inner);

  // Global Variable management
  void addGlobal(std::unique_ptr<HIRStmt> global);
  llvm::ArrayRef<HIRStmt *> getGlobals() const;

  void addClass(std::unique_ptr<HIRClass> cls);
  llvm::ArrayRef<HIRClass *> getClasses() const;
  HIRClass *getClass(llvm::StringRef name) const;

  void dump(llvm::raw_ostream &os) const;

private:
  std::string name;

  // Caches for returning ArrayRef (mutable allows modification in const
  // methods)
  mutable std::vector<HIRStmt *> globalCache;
  mutable std::vector<HIRFunction *> functionCache;

  // Owns the functions and globals
  std::vector<std::unique_ptr<HIRFunction>> functions;
  std::vector<std::unique_ptr<HIRStmt>> globals;

  // Type Storage
  llvm::BumpPtrAllocator typeAllocator;
  llvm::FoldingSet<HIRType> uniqueTypes;

  mutable std::vector<HIRClass *> classCache;
  std::vector<std::unique_ptr<HIRClass>> classes;
};

} // namespace hir
} // namespace moksha
