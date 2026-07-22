#pragma once

#include "moksha/MIR/Passes/MIRPass.h"

namespace moksha {
namespace mir {

/** @brief Performs dead code elimination on the MIR module. */
class DeadCodeEliminationPass : public MIRPass {
public:
  llvm::StringRef getName() const override { return "DeadCodeEliminationPass"; }
  bool runOnModule(MIRModule &M) override;
};

} // namespace mir
} // namespace moksha
