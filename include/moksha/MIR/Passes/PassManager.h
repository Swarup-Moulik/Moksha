#pragma once

#include "moksha/MIR/Passes/MIRPass.h"
#include <memory>
#include <vector>

namespace moksha {
namespace mir {

class MIRModule;

/** @brief Manages a collection of MIR passes to be run on a module. */
class PassManager {
public:
  void addPass(std::unique_ptr<MIRPass> pass);
  void run(MIRModule &module);

private:
  std::vector<std::unique_ptr<MIRPass>> passes;
};

} // namespace mir
} // namespace moksha
