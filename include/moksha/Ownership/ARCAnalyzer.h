#pragma once

namespace moksha {

class DiagnosticEngine;

namespace mir {

class MIRModule;
class MIRFunction;

/// \brief The ARC Optimization Pass.
///
/// Analyzes data flow to identify and remove redundant Reference Counting
/// operations inserted by the `ARCInserter`.
///
/// **Optimizations:**
/// 1. **Pair Elision:** Removes `Retain` + `Release` pairs that cancel each
/// other
///    out within the same basic block or across simple control flow.
/// 2. **Motion:** Moves `Release` operations later or `Retain` operations
/// earlier
///    to group them for removal.
///
/// **Preconditions:**
/// - `runARCInsertion` must have already run (MIR contains retains/releases).
///
/// \param module The MIR module to optimize (modified in-place).
/// \param diags The engine to report optimization warnings or errors.
/// \return true if the module was modified (instructions removed).
bool runARCOptimization(MIRModule *module, DiagnosticEngine &diags);

/// \brief Runs optimization on a single function (useful for testing).
bool runARCOptimization(MIRFunction *function, DiagnosticEngine &diags);

} // namespace mir
} // namespace moksha
