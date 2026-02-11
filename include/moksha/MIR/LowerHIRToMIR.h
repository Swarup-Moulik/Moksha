#pragma once

#include <memory>

namespace moksha {

class DiagnosticEngine; // Forward declaration

namespace hir {
class HIRModule;
}

namespace mir {

class MIRModule;

/// Lowers an HIR module to MIR.
///
/// Converts high-level structured control flow (HIR) into a
/// Control Flow Graph (CFG) with SSA candidates (allocas).
///
/// \param hirModule The input verified HIR.
/// \param diags Diagnostic engine for reporting lowering errors.
/// \return A unique_ptr to the generated MIRModule, or nullptr on failure.
std::unique_ptr<MIRModule> LowerHIRToMIR(const hir::HIRModule *hirModule,
                                         DiagnosticEngine &diags);

} // namespace mir
} // namespace moksha
