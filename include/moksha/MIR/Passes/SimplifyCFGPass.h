#pragma once

#include "moksha/MIR/Passes/MIRPass.h"
#include "llvm/ADT/StringRef.h"

namespace moksha {
namespace mir {

class MIRFunction;

/** @brief Simplifies the Control Flow Graph (CFG) of a function. */
class SimplifyCFGPass : public MIRPass {
public:
  llvm::StringRef getName() const override { return "SimplifyCFGPass"; }
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
