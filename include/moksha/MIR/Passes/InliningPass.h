#pragma once

#include "moksha/MIR/Passes/MIRPass.h"
#include "llvm/ADT/StringRef.h"
#include <memory>
#include <vector>

namespace moksha {
namespace mir {

class MIRFunction;
class MIRBlock;
class MIRInst;
class CallInst;
class InvokeInst;

/// The Inlining Pass replaces function calls with the actual body of the
/// called function. This is critical for exposing constructors and small
/// methods to the Escape Analyzer and Dead Code Eliminator.
class InliningPass : public MIRPass {
public:
  llvm::StringRef getName() const override { return "InliningPass"; }

  /// Runs the pass on all functions within the module.
  bool runOnModule(MIRModule &M) override;

private:
  /// Attempts to inline eligible calls within a specific function.
  bool runOnFunction(MIRFunction *F, MIRModule &M);

  /// Determines if a specific function is small/simple enough to be inlined.
  bool shouldInline(MIRFunction *callee);

  /// Performs the actual cloning of blocks and instructions into the caller.
  bool inlineCall(MIRFunction *caller, MIRBlock *block,
                  std::vector<std::unique_ptr<MIRInst>>::iterator &it,
                  CallInst *call, MIRModule &M);

  bool inlineInvoke(MIRFunction *caller, MIRBlock *block,
                    std::vector<std::unique_ptr<MIRInst>>::iterator &it,
                    InvokeInst *invoke, MIRModule &M);
};

} // namespace mir
} // namespace moksha
