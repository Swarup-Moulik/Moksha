#pragma once

#include "moksha/MIR/Passes/MIRPass.h"
#include "llvm/ADT/StringRef.h"
#include <unordered_map>
#include <vector>

namespace moksha {
namespace mir {

class MIRFunction;
class MIRInst;
class MIRValue;

/** @brief Determines whether a variable's lifetime extends beyond its enclosing
 * stack frame  */
class EscapeAnalysisPass : public MIRPass {
public:
  llvm::StringRef getName() const override { return "EscapeAnalysisPass"; }
  bool runOnModule(MIRModule &M) override;

private:
  bool runOnFunction(MIRFunction *F, MIRModule &M);
  bool doesEscape(
      MIRValue *val,
      const std::unordered_map<MIRValue *, std::vector<MIRInst *>> &defUse);
};

} // namespace mir
} // namespace moksha
