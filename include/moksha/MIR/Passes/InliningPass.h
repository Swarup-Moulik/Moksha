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

/** @brief Inlines function calls within the MIR module. */
class InliningPass : public MIRPass {
public:
  llvm::StringRef getName() const override { return "InliningPass"; }
  bool runOnModule(MIRModule &M) override;

private:
  bool runOnFunction(MIRFunction *F, MIRModule &M);
  bool shouldInline(MIRFunction *callee);
  bool inlineCall(MIRFunction *caller, MIRBlock *block,
                  std::vector<std::unique_ptr<MIRInst>>::iterator &it,
                  CallInst *call, MIRModule &M);
  bool inlineInvoke(MIRFunction *caller, MIRBlock *block,
                    std::vector<std::unique_ptr<MIRInst>>::iterator &it,
                    InvokeInst *invoke, MIRModule &M);
};

} // namespace mir
} // namespace moksha
