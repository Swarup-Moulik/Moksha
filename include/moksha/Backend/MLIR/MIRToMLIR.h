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

/// \brief Lowers finalized MIR to MLIR (Moksha Dialect).
///
/// Converts the Control Flow Graph (MIR) into the corresponding MLIR
/// operations.
///
/// **Preconditions:**
/// - MIR must be verified (MIRVerifier).
/// - ARCInserter and ARCAnalyzer must have already run (ownership is resolved).
///
/// \param mirModule The input MIR module. Passed by reference as it is
/// guaranteed to exist.
/// \param context The MLIR context where the new module will be created.
/// \param diags The engine to report lowering errors.
/// \return An owning reference to the generated MLIR module, or null on
/// failure.
::mlir::OwningOpRef<::mlir::ModuleOp>
convertMIRToMLIR(mir::MIRModule &mirModule, ::mlir::MLIRContext &context,
                 DiagnosticEngine &diags);

} // namespace mlir
} // namespace backend
} // namespace moksha
