#pragma once

#include "mlir/IR/Types.h"

namespace mlir {
class MLIRContext;
}

namespace moksha {

// Forward declarations
namespace hir {
class HIRType;
}
namespace mir {
class MIRType;
class MIRValue;
} // namespace mir

namespace backend {
namespace mlir {

/** @brief Lowers HIR/MIR types to MLIR types. */
class TypeLowering {
public:
  explicit TypeLowering(::mlir::MLIRContext &ctx, unsigned ptrSize = 8);
  ::mlir::Type lowerHIRType(const hir::HIRType &type) const;
  ::mlir::Type lowerMIRType(const mir::MIRType *type) const;
  ::mlir::Type lowerMIRValueType(const mir::MIRValue &value) const;

  bool isTriviallyCopyable(const hir::HIRType &type) const;
  bool requiresOwnership(const hir::HIRType &type) const;

private:
  ::mlir::MLIRContext &context;
  unsigned pointerSize;
};

} // namespace mlir
} // namespace backend
} // namespace moksha
