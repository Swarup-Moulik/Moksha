#pragma once

namespace moksha {

class DiagnosticEngine;

namespace hir {
class HIRModule;
}

namespace mir {

/// \brief Verifies that the HIR module contains no unexpanded macros.
///
/// This is a hard pipeline barrier.
/// MIR lowering and all later passes assume macros do not exist.
[[nodiscard]] bool VerifyNoMacros(const hir::HIRModule *module,
                                  DiagnosticEngine &diags);

} // namespace mir
} // namespace moksha
