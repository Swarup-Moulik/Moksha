#pragma once

#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "moksha/Support/Diagnostics.h"
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace moksha {
namespace mir {

// ============================================================================
// [Path Sensitivity (Places)]
// ============================================================================

/// Represents a specific memory location (Base + GEP projections).
struct Place {
  MIRValue *base;
  std::vector<uint64_t> projections;

  /// True if same base AND one path is a prefix of the other.
  /// (e.g., `x` conflicts with `x.a`, but `x.a` does not conflict with `x.b`)
  bool conflictsWith(const Place &other) const;

  bool operator==(const Place &other) const {
    return base == other.base && projections == other.projections;
  }
};

// ============================================================================
// [Borrow Tracking]
// ============================================================================

// Represents an active borrow of a specific memory location
struct Loan {
  Place
      borrowedPlace; // The memory being borrowed (e.g., the AllocaInst for 'p')
  MIRValue *pointer; // The variable holding the borrow (e.g., 'v', 'm1')
  bool isMut;        // true if *mut, false if view (immutable)

  bool operator==(const Loan &other) const {
    // A loan is unique to the place it borrows AND the pointer holding it
    return borrowedPlace.base == other.borrowedPlace.base &&
           pointer == other.pointer;
  }
};

// ============================================================================
// [The Checker]
// ============================================================================

/// Implements Non-Lexical Lifetimes (NLL) via CFG Dataflow Analysis.
class NLLBorrowChecker {
public:
  explicit NLLBorrowChecker(DiagnosticEngine &diags);

  void checkModule(MIRModule *module);
  void checkFunction(MIRFunction *func);

private:
  DiagnosticEngine &diags;

  // ========================================================================
  // [Dataflow State]
  // ========================================================================

  // 1. Liveness: Maps a pointer value to the LAST instruction that uses it.
  // This tells us exactly when a Loan should "die" (The NLL rule).
  std::unordered_map<const MIRValue *, const MIRInst *> lastUses;

  // 2. Block Boundaries: The state of active loans entering and exiting blocks
  std::unordered_map<const MIRBlock *, std::vector<Loan>> blockIn;
  std::unordered_map<const MIRBlock *, std::vector<Loan>> blockOut;

  // 3. Instruction State: The active loans immediately *before* an instruction
  // executes
  std::unordered_map<const MIRInst *, std::vector<Loan>> activeLoansAtInst;

  // ========================================================================
  // [Analysis Phases]
  // ========================================================================

  // Phase 1: Walk the function backwards to find the last use of every pointer
  void computeLiveness(MIRFunction *func);

  // Phase 2: Run the worklist algorithm to propagate In/Out sets across
  // branches
  void computeDataflow(MIRFunction *func);

  // Phase 3: Check for mutable aliasing violations using the computed dataflow
  void checkConflicts(MIRFunction *func);

  // Helper to merge two sets of loans (used when blocks have multiple
  // predecessors)
  std::vector<Loan> mergeLoans(const std::vector<Loan> &a,
                               const std::vector<Loan> &b);

  // Helper to resolve memory origin
  std::vector<Place> resolvePlace(MIRValue *val) const;
};

} // namespace mir
} // namespace moksha

// ============================================================================
// [Hash Specialization for Place]
// ============================================================================

namespace std {
template <> struct hash<moksha::mir::Place> {
  size_t operator()(const moksha::mir::Place &p) const {
    size_t h = std::hash<void *>()(p.base);
    for (uint64_t proj : p.projections) {
      size_t proj_hash = std::hash<uint64_t>()(proj);
      // Magic number prevents symmetrical cancellation and shifts entropy
      h ^= proj_hash + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
  }
};
} // namespace std
