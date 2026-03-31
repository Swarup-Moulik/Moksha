#pragma once

#include "llvm/ADT/StringRef.h"

namespace moksha {
namespace mir {

class MIRModule;

class MIRPass {
public:
  virtual ~MIRPass() = default;
  virtual llvm::StringRef getName() const = 0;

  // Returns true if the pass modified the module
  virtual bool runOnModule(MIRModule &M) = 0;
};

} // namespace mir
} // namespace moksha
