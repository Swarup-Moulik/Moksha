#pragma once

namespace moksha {

class DiagnosticEngine;

namespace hir {
class HIRModule;
}

namespace mir {

/** @brief Verifies that the HIR module contains no unexpanded macros. */
[[nodiscard]] bool VerifyNoMacros(const hir::HIRModule *module,
                                  DiagnosticEngine &diags);

} // namespace mir
} // namespace moksha
