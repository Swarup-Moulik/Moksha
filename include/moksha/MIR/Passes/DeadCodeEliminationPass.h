#pragma once

#include "moksha/MIR/Passes/MIRPass.h"

namespace moksha {
namespace mir {

class DeadCodeEliminationPass : public MIRPass {
public:
  llvm::StringRef getName() const override { return "DeadCodeEliminationPass"; }
  bool runOnModule(MIRModule &M) override;
};

} // namespace mir
} // namespace moksha
