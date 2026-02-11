#pragma once

#include "mlir/Pass/Pass.h"
#include <memory>

namespace mlir {
class Pass;
} // namespace mlir

namespace moksha {
namespace backend {
namespace mlir {

/// \brief Creates an MLIR pass that canonicalizes ARC operations.
///
/// This pass performs local and block-level optimizations on ARC operations:
/// 1. Removes redundant retain/release pairs on the same value.
/// 2. Hoists retains above independent operations to increase optimization
///    windows.
/// 3. Sinks releases as far as possible to reduce the "live" range of
///    strong references.
std::unique_ptr<::mlir::Pass> createCanonicalizeARCPass();

} // namespace mlir
} // namespace backend
} // namespace moksha
