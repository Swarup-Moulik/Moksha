#pragma once

namespace moksha {

class DiagnosticEngine;

namespace mir {

class MIRModule;

/// \brief The ARC Insertion Pass.
///
/// Transforms raw MIR into "safe" MIR by injecting reference counting
/// operations.
///
/// **Logic:**
/// 1. Identifies ref-counted values (Classes, Arrays, Strings).
/// 2. Inserts `Retain` on creation/copy and `Release` at scope exits.
///
/// **Preconditions:**
/// - MIR must be verified (MIRVerifier).
/// - Basic ownership analysis (ARCAnalyzer) helps optimize placement.
///
/// \param module The MIR module to transform (modified in-place).
/// \param diags The engine to report ownership errors.
/// \return true if the module was modified.
bool runARCInsertion(MIRModule *module, DiagnosticEngine &diags);

} // namespace mir
} // namespace moksha
