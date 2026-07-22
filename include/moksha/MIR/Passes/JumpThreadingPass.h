#pragma once

#include "moksha/MIR/Passes/MIRPass.h"
#include "llvm/ADT/StringRef.h"

namespace moksha {
namespace mir {

class MIRModule;

/** @brief Redirects conditional branches past redundant basic blocks to
 * streamline control flow and eliminate unnecessary jumps. */
class JumpThreadingPass : public MIRPass {
public:
  llvm::StringRef getName() const override { return "JumpThreadingPass"; }
  bool runOnModule(MIRModule &M) override;
};

} // namespace mir
} // namespace moksha
