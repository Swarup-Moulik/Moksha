#pragma once

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OwningOpRef.h"

namespace mlir {
class MLIRContext;
}

namespace moksha {

class DiagnosticEngine;

namespace mir {
class MIRModule;
}

namespace backend {
namespace mlir {

/** @brief Lowers finalized MIR to MLIR (Moksha Dialect). */
::mlir::OwningOpRef<::mlir::ModuleOp>
convertMIRToMLIR(mir::MIRModule &mirModule, ::mlir::MLIRContext &context,
                 DiagnosticEngine &diags);

} // namespace mlir
} // namespace backend
} // namespace moksha
