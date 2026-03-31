#pragma once

#include "moksha/MIR/Passes/MIRPass.h"
#include "llvm/ADT/StringRef.h"

namespace moksha {
namespace mir {

class MIRModule;

/// The Jump Threading pass optimizes control flow by identifying conditional
/// branches whose outcomes are fully determined by a specific incoming CFG
/// edge.
///
/// For example, if a Phi node merges a known `null` value from an earlier
/// block, and that Phi is subsequently checked for `null`, this pass "threads"
/// the jump directly from the earlier block to the bailout destination. This is
/// highly effective at collapsing the redundant cascading null checks generated
/// by optional chaining (`?.`).
class JumpThreadingPass : public MIRPass {
public:
  llvm::StringRef getName() const override { return "JumpThreadingPass"; }

  /// Runs the jump threading pass on all functions within the module.
  /// Returns true if the module's control flow graph was modified.
  bool runOnModule(MIRModule &M) override;
};

} // namespace mir
} // namespace moksha
