#pragma once

#include "moksha/MIR/Passes/MIRPass.h"
#include <memory>
#include <vector>

namespace moksha {
namespace mir {

class MIRModule;

class PassManager {
public:
  void addPass(std::unique_ptr<MIRPass> pass);
  void run(MIRModule &module);

private:
  std::vector<std::unique_ptr<MIRPass>> passes;
};

} // namespace mir
} // namespace moksha
