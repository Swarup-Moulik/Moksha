#pragma once

#include "llvm/Support/SMLoc.h"
#include "llvm/Support/SourceMgr.h"

namespace moksha {

/// @brief Wrapper around LLVM's Source Location (pointer to a specific byte in
/// the buffer).
using SourceLocation = llvm::SMLoc;

/// @brief Represents a continuous range of characters in the source code (Start
/// -> End).
using SourceRange = llvm::SMRange;

} // namespace moksha
