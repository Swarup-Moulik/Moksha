#pragma once

#include "mlir/Pass/Pass.h"
#include <memory>

namespace moksha {

// Creates the pass defined in MokshaPasses.td
std::unique_ptr<::mlir::Pass> createConvertMokshaToLLVMPass();

} // namespace moksha
