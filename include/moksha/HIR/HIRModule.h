#pragma once

#include "moksha/HIR/HIRType.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Allocator.h"
#include <iosfwd> // Forward declaration for std::ostream
#include <memory>
#include <string>
#include <vector>

namespace moksha {
namespace hir {

// Forward declarations avoid circular includes
class HIRFunction;
class HIRStmt;

class HIRModule {
public:
  explicit HIRModule(std::string moduleName);
  ~HIRModule(); // Implemented in .cpp

  // Module metadata
  llvm::StringRef getName() const { return name; }

  // Function management
  void addFunction(std::unique_ptr<HIRFunction> func); // Implemented in .cpp
  llvm::ArrayRef<HIRFunction *> getFunctions() const;  // Implemented in .cpp
  HIRFunction *getFunction(llvm::StringRef name) const;

  // Type Interning / Canonicalization
  PrimitiveType *getPrimitiveType(TypeKind kind);
  StructType *getStructType(std::string name,
                            std::vector<const HIRType *> fields);
  PointerType *getPointerType(const HIRType *pointee, Ownership own);
  FunctionType *getFunctionType(const HIRType *ret,
                                std::vector<const HIRType *> params);

  // Global Variable management
  // [MOVED TO CPP] To avoid including HIRStmt.h here
  void addGlobal(std::unique_ptr<HIRStmt> global);
  llvm::ArrayRef<HIRStmt *> getGlobals() const;

  // Debugging
  // [FIXED] Signature now matches .cpp implementation
  void dump(std::ostream &os) const;

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
};

} // namespace hir
} // namespace moksha
