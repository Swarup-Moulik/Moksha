#include "moksha/MIR/Passes/PassManager.h"
#include "moksha/MIR/MIRModule.h"
#include "llvm/Support/raw_ostream.h" // Added for llvm::outs()
#include <typeinfo>

namespace moksha {
namespace mir {

void PassManager::addPass(std::unique_ptr<MIRPass> pass) {
  passes.push_back(std::move(pass));
}

void PassManager::run(MIRModule &module) {
  bool changed = true;
  int iterationCount = 0;
  const int MAX_ITERATIONS = 15; // Circuit breaker

  while (changed && iterationCount < MAX_ITERATIONS) {
    changed = false;
    for (auto &pass : passes) {
      bool passChanged = pass->runOnModule(module);
      changed |= passChanged;
    }
    iterationCount++;
  }
}

} // namespace mir
} // namespace moksha
