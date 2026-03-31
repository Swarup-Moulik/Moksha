#pragma once

#include "moksha/MIR/Passes/MIRPass.h"

namespace moksha {
namespace mir {

class MIRModule;
class MIRFunction;

/// Scalar Replacement of Aggregates (SROA)
/// Finds aggregate allocations (like structs) that are only accessed via
/// constant-indexed GEPs and "shatters" them into individual scalar
/// allocations. This allows subsequent passes (like Mem2Reg) to promote struct
/// fields into SSA registers.
class SROAPass : public MIRPass {
public:
  SROAPass() = default;
  ~SROAPass() override = default;

  llvm::StringRef getName() const override { return "SROAPass"; }

  /// Runs the SROA pass on all functions within the module.
  /// Returns true if the module was modified.
  bool runOnModule(MIRModule &M) override;

  /// Runs the SROA pass on a specific function.
  /// Returns true if the function was modified.
  bool runOnFunction(MIRFunction *F, MIRModule &M);
};

} // namespace mir
} // namespace moksha
