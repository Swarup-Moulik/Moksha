#pragma once

#include "moksha/MIR/Passes/MIRPass.h"

namespace moksha {
namespace mir {

/** @brief Performs constant folding on the MIR module. */
class ConstantFoldingPass : public MIRPass {
public:
  llvm::StringRef getName() const override { return "ConstantFoldingPass"; }
  bool runOnModule(MIRModule &M) override;
};

} // namespace mir
} // namespace moksha
