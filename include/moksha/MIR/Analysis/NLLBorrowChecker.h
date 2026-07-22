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

/** @brief Represents a specific memory location (Base + GEP projections). */
struct Place {
  MIRValue *base;
  std::vector<uint64_t> projections;
  bool conflictsWith(const Place &other) const;
  bool operator==(const Place &other) const {
    return base == other.base && projections == other.projections;
  }
};

/** @brief Represents an active borrow of a specific memory location. */
struct Loan {
  Place borrowedPlace;
  MIRValue *pointer;
  bool isMut;

  bool operator==(const Loan &other) const {
    // A loan is unique to the place it borrows AND the pointer holding it
    return borrowedPlace.base == other.borrowedPlace.base &&
           pointer == other.pointer;
  }
};

/** @brief Implements Non-Lexical Lifetimes (NLL) via CFG Dataflow Analysis. */
class NLLBorrowChecker {
public:
  explicit NLLBorrowChecker(DiagnosticEngine &diags);

  void checkModule(MIRModule *module);
  void checkFunction(MIRFunction *func);

private:
  DiagnosticEngine &diags;

  std::unordered_map<const MIRValue *, const MIRInst *> lastUses;
  std::unordered_map<const MIRBlock *, std::vector<Loan>> blockIn;
  std::unordered_map<const MIRBlock *, std::vector<Loan>> blockOut;
  std::unordered_map<const MIRInst *, std::vector<Loan>> activeLoansAtInst;

  void computeLiveness(MIRFunction *func);
  void computeDataflow(MIRFunction *func);
  void checkConflicts(MIRFunction *func);
  std::vector<Loan> mergeLoans(const std::vector<Loan> &a,
                               const std::vector<Loan> &b);
  std::vector<Place> resolvePlace(MIRValue *val) const;
};

} // namespace mir
} // namespace moksha

// Hash Specialization for Place
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
