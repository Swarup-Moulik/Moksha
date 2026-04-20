#include "moksha/Dialect/MokshaOps.h"
#include "mlir/IR/OpImplementation.h"
#include "moksha/Dialect/MokshaDialect.h"

// 1. Generate the C++ definitions for the custom interfaces FIRST
#define GET_OP_INTERFACE_DEFS
#include "moksha/Dialect/MokshaInterfaces.cpp.inc"

// 2. Generate the C++ classes for every Moksha operation
#define GET_OP_CLASSES
#include "moksha/Dialect/MokshaOps.cpp.inc"

namespace moksha {
namespace IR {

// ============================================================================
// Custom Logic for ARCOpInterface
// ============================================================================

// --- RetainOp Implementation ---

::mlir::Value RetainOp::getARCManagedValue() { return getValue(); }

bool RetainOp::isRetain() { return true; }

bool RetainOp::isRelease() { return false; }

// --- ReleaseOp Implementation ---

::mlir::Value ReleaseOp::getARCManagedValue() { return getValue(); }

bool ReleaseOp::isRetain() { return false; }

bool ReleaseOp::isRelease() { return true; }

// ============================================================================
// ConstantOp Folding
// ============================================================================

::mlir::OpFoldResult ConstantOp::fold(FoldAdaptor adaptor) {
  return getValue();
}

// ============================================================================
// Custom Logic for AsyncOpInterface
// ============================================================================

bool SpawnOp::isBlocking() {
  return false; // Spawning tasks returns immediately
}

bool AwaitOp::isBlocking() {
  return true; // Await suspends/blocks execution
}

} // namespace IR
} // namespace moksha
