#pragma once

#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRModule.h"
#include "moksha/MIR/Passes/MIRPass.h"

namespace moksha {
namespace mir {

/** @brief Eliminate redundant or duplicate calls to destructor functions */
class DropElisionPass : public MIRPass {
public:
  llvm::StringRef getName() const override { return "DropElisionPass"; }
  bool runOnModule(MIRModule &M) override;

private:
  bool runOnFunction(MIRFunction *F);
};

} // namespace mir
} // namespace moksha
