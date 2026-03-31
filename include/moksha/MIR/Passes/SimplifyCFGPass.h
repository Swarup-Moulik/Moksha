#pragma once

#include "moksha/MIR/Passes/MIRPass.h"
#include "llvm/ADT/StringRef.h"

namespace moksha {
namespace mir {

class MIRFunction;

/// The SimplifyCFG pass cleans up the Control Flow Graph (CFG).
/// It performs three main optimizations:
/// 1. Branch Folding: Converts conditional branches with identical targets into
/// unconditional branches.
/// 2. Empty Block Bypassing: If a block is empty and just jumps to another
/// block, predecessors are rerouted to skip it.
/// 3. Block Merging: If Block A unconditionally jumps to Block B, and A is B's
/// only predecessor, they are merged.
class SimplifyCFGPass : public MIRPass {
public:
  llvm::StringRef getName() const override { return "SimplifyCFGPass"; }

  /// Runs the pass on all functions within the module.
  bool runOnModule(MIRModule &M) override;

private:
  bool runOnFunction(MIRFunction *F);

  bool foldBranches(MIRFunction *F);
  bool bypassEmptyBlocks(MIRFunction *F);
  bool mergeBlocks(MIRFunction *F);

  void removeDeadBlocks(MIRFunction *F);
};

} // namespace mir
} // namespace moksha
