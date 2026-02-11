#include "moksha/Backend/MLIR/TypeLowering.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "moksha/MIR/MIRValue.h" // [FIX] Added missing include

namespace moksha {
namespace backend {
namespace mlir {

TypeLowering::TypeLowering(::mlir::MLIRContext &ctx) : context(ctx) {}

::mlir::Type TypeLowering::lowerHIRType(const hir::HIRType &type) const {
  // [FIX] Use IntegerType::get via context
  return ::mlir::IntegerType::get(&context, 64);
}

::mlir::Type TypeLowering::lowerMIRType(const mir::MIRType *type) const {
  // [FIX] Use NoneType::get via context
  return ::mlir::NoneType::get(&context);
}

::mlir::Type TypeLowering::lowerMIRValueType(const mir::MIRValue &value) const {
  // This now works because MIRValue.h is included
  return lowerHIRType(*value.getType());
}

bool TypeLowering::isTriviallyCopyable(const hir::HIRType &type) const {
  return true;
}

bool TypeLowering::requiresOwnership(const hir::HIRType &type) const {
  return false;
}

} // namespace mlir
} // namespace backend
} // namespace moksha
