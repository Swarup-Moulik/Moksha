#pragma once

#include "moksha/MIR/Passes/MIRPass.h"
#include "llvm/ADT/StringRef.h"

namespace moksha {
namespace mir {

class MIRFunction;
class MIRDominance;

/** @brief Promotes allocations to registers within a single function from
 * stack. */
class Mem2RegPass : public MIRPass {
public:
  llvm::StringRef getName() const override { return "Mem2RegPass"; }
  bool runOnModule(MIRModule &M) override;

private:
  bool runOnFunction(MIRFunction *F, MIRModule &M);
};

} // namespace mir
} // namespace moksha
