#pragma once

#include "llvm/ADT/StringRef.h"

namespace moksha {
namespace mir {

class MIRModule;

/** @brief Base class for all MIR passes. */
class MIRPass {
public:
  virtual ~MIRPass() = default;
  virtual llvm::StringRef getName() const = 0;
  virtual bool runOnModule(MIRModule &M) = 0;
};

} // namespace mir
} // namespace moksha
