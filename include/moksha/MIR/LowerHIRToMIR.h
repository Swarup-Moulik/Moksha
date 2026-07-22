#pragma once

#include <memory>

namespace moksha {

class DiagnosticEngine; // Forward declaration

namespace hir {
class HIRModule;
}

namespace mir {

class MIRModule;

/** @brief Lowers an HIR module to MIR. */
std::unique_ptr<MIRModule> LowerHIRToMIR(const hir::HIRModule *hirModule,
                                         DiagnosticEngine &diags);

} // namespace mir
} // namespace moksha
