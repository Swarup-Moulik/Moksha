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

/// The Escape Analysis pass determines if dynamically allocated memory
/// (e.g., via __moksha_alloc) ever "escapes" the function it was created in.
/// If an object does not escape and its lifecycle is completely local,
/// this pass can elide the allocation entirely or promote it to the stack.
class EscapeAnalysisPass : public MIRPass {
public:
  llvm::StringRef getName() const override { return "EscapeAnalysisPass"; }

  /// Runs the pass on all functions within the module.
  bool runOnModule(MIRModule &M) override;

private:
  /// Analyzes a single function for non-escaping allocations.
  bool runOnFunction(MIRFunction *F, MIRModule &M);

  /// Traces all uses of a pointer to determine if it escapes the local scope.
  bool doesEscape(
      MIRValue *val,
      const std::unordered_map<MIRValue *, std::vector<MIRInst *>> &defUse);
};

} // namespace mir
} // namespace moksha
