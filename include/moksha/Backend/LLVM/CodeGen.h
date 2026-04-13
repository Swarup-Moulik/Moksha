#pragma once

#include "llvm/IR/Module.h"
#include <string>

namespace moksha {

struct TargetConfig {
  std::string triple;
  std::string cpu;
  std::string features;
  int optLevel = 2;
};

bool emitObjectCode(llvm::Module &llvmModule, const std::string &outputFilename,
                    const TargetConfig &config = {});

} // namespace moksha
