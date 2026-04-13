#include "moksha/Dialect/MokshaDialect.h"
#include "moksha/Dialect/MokshaOps.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "llvm/ADT/TypeSwitch.h"

// Pull in the auto-generated Dialect construction logic
#include "moksha/Dialect/MokshaDialect.cpp.inc"

#define GET_TYPEDEF_CLASSES
#include "moksha/Dialect/MokshaOpsTypes.cpp.inc"

namespace moksha {
namespace IR {

void MokshaDialect::initialize() {
  // 1. Register the custom types defined in MokshaTypes.td
  addTypes<
#define GET_TYPEDEF_LIST
#include "moksha/Dialect/MokshaOpsTypes.cpp.inc"
      >();

  // 2. Register the operations defined in MokshaOps.td
  addOperations<
#define GET_OP_LIST
#include "moksha/Dialect/MokshaOps.cpp.inc"
      >();
}

// Since you enabled hasConstantMaterializer = 1 in MokshaDialect.td,
// you must provide this function. It allows MLIR to "fold" constants.
mlir::Operation *MokshaDialect::materializeConstant(mlir::OpBuilder &builder,
                                                    mlir::Attribute value,
                                                    mlir::Type type,
                                                    mlir::Location loc) {
  // Use the new ConstantOp we defined in TableGen
  return builder.create<::moksha::IR::ConstantOp>(loc, type, value);
}

} // namespace IR
} // namespace moksha
