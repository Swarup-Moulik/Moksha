#pragma once

#include "moksha/MIR/Passes/MIRPass.h"
#include "llvm/ADT/StringRef.h"

namespace moksha {
namespace mir {

class MIRFunction;
class MIRDominance;

/// The Mem2Reg pass promotes stack allocations (alloca) to registers
/// by constructing SSA form. It uses the Dominance Tree to compute
/// Dominance Frontiers, inserts Phi nodes, and renames variables.
class Mem2RegPass : public MIRPass {
public:
  llvm::StringRef getName() const override { return "Mem2RegPass"; }

  /// Runs the pass on all functions within the module.
  /// Returns true if the module was modified.
  bool runOnModule(MIRModule &M) override;

private:
  /// Promotes allocations to registers within a single function.
  bool runOnFunction(MIRFunction *F, MIRModule &M);
};

} // namespace mir
} // namespace moksha
