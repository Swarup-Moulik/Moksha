#pragma once

#include "mlir/IR/Types.h"
// Remove #include "moksha/HIR/HIRType.h" if it causes circular deps,
// or rely on the forward declaration below.

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

class TypeLowering {
public:
  explicit TypeLowering(::mlir::MLIRContext &ctx);

  // [FIX] Use hir::HIRType instead of bare HIRType
  ::mlir::Type lowerHIRType(const hir::HIRType &type) const;

  ::mlir::Type lowerMIRType(const mir::MIRType *type) const;

  ::mlir::Type lowerMIRValueType(const mir::MIRValue &value) const;

  // --- Semantic Queries ---

  // [FIX] Use hir::HIRType
  bool isTriviallyCopyable(const hir::HIRType &type) const;
  bool requiresOwnership(const hir::HIRType &type) const;

private:
  ::mlir::MLIRContext &context;
};

} // namespace mlir
} // namespace backend
} // namespace moksha
