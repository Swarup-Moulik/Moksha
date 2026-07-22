#include "moksha/Dialect/MokshaDialect.h"
#include "moksha/Dialect/MokshaOps.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "moksha/Dialect/MokshaDialect.cpp.inc"
#include "llvm/ADT/TypeSwitch.h"

#define GET_TYPEDEF_CLASSES
#include "moksha/Dialect/MokshaOpsTypes.cpp.inc"

namespace moksha {
namespace IR {

void MokshaDialect::initialize() {
  addTypes<
#define GET_TYPEDEF_LIST
#include "moksha/Dialect/MokshaOpsTypes.cpp.inc"
      >();
  addOperations<
#define GET_OP_LIST
#include "moksha/Dialect/MokshaOps.cpp.inc"
      >();
}

mlir::Operation *MokshaDialect::materializeConstant(mlir::OpBuilder &builder,
                                                    mlir::Attribute value,
                                                    mlir::Type type,
                                                    mlir::Location loc) {
  return builder.create<::moksha::IR::ConstantOp>(loc, type, value);
}

} // namespace IR
} // namespace moksha
