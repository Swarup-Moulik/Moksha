#pragma once

#include "moksha/MIR/Passes/MIRPass.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRModule.h"

namespace moksha {
namespace mir {

/// The Drop Elision Pass runs after the Borrow Checker.
/// It identifies implicit cleanup instructions (like ARC Releases or free() calls)
/// that target memory which has already been moved, and securely deletes them
/// to prevent Double-Free and Use-After-Free runtime crashes.
class DropElisionPass : public MIRPass {
public:
  // Implement the required virtual method
  llvm::StringRef getName() const override { return "DropElisionPass"; }

  // Override the run method
  bool runOnModule(MIRModule &M) override;

private:
  bool runOnFunction(MIRFunction *F);
};

} // namespace mir
} // namespace moksha
