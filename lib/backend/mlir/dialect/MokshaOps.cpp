#include "moksha/Dialect/MokshaOps.h"
#include "mlir/IR/OpImplementation.h"
#include "moksha/Dialect/MokshaDialect.h"

#define GET_OP_INTERFACE_DEFS
#include "moksha/Dialect/MokshaInterfaces.cpp.inc"

#define GET_OP_CLASSES
#include "moksha/Dialect/MokshaOps.cpp.inc"

namespace moksha {
namespace IR {

// RetainOp Implementation

::mlir::Value RetainOp::getARCManagedValue() { return getValue(); }

bool RetainOp::isRetain() { return true; }

bool RetainOp::isRelease() { return false; }

// ReleaseOp Implementation

::mlir::Value ReleaseOp::getARCManagedValue() { return getValue(); }

bool ReleaseOp::isRetain() { return false; }

bool ReleaseOp::isRelease() { return true; }

// ConstantOp Folding
::mlir::OpFoldResult ConstantOp::fold(FoldAdaptor adaptor) {
  return getValue();
}

bool SpawnOp::isBlocking() { return false; }

bool AwaitOp::isBlocking() { return true; }

} // namespace IR
} // namespace moksha
