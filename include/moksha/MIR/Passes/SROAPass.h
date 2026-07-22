#pragma once

#include "moksha/MIR/Passes/MIRPass.h"

namespace moksha {
namespace mir {

class MIRModule;
class MIRFunction;

/** @brief Shatters structs into scalar allocations. */
class SROAPass : public MIRPass {
public:
  SROAPass() = default;
  ~SROAPass() override = default;

  llvm::StringRef getName() const override { return "SROAPass"; }
  bool runOnModule(MIRModule &M) override;
  bool runOnFunction(MIRFunction *F, MIRModule &M);
};

} // namespace mir
} // namespace moksha
