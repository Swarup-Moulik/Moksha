#include "moksha/Dialect/MokshaDialect.h"
#include "moksha/Dialect/MokshaOps.h"
#include "moksha/HIR/HIRType.h" // For hir::PointerType

using namespace mlir;
using namespace moksha;

// --- [INSERT THIS BLOCK STARTING HERE] ---
namespace moksha {
namespace detail {

// Define the storage for PointerType.
// This allows the compiler to know how to allocate and destroy it.
struct PointerTypeStorage : public mlir::TypeStorage {
  using KeyTy = mlir::Type;

  PointerTypeStorage(mlir::Type pointee) : pointee(pointee) {}

  /// The comparison function for the uniquer.
  bool operator==(const KeyTy &key) const { return key == pointee; }

  /// The construction function.
  static PointerTypeStorage *construct(mlir::TypeStorageAllocator &allocator,
                                       const KeyTy &key) {
    return new (allocator.allocate<PointerTypeStorage>())
        PointerTypeStorage(key);
  }

  mlir::Type pointee;
};

} // namespace detail
} // namespace moksha

#include "moksha/Dialect/MokshaDialect.cpp.inc"

void MokshaDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "moksha/Dialect/MokshaOps.cpp.inc"
      >();

  addTypes<
#define GET_TYPEDEF_LIST
#include "moksha/Dialect/MokshaOpsTypes.cpp.inc"
      >();
}
