#ifndef MOKSHA_SUPPORT_SOURCELOCATION_H
#define MOKSHA_SUPPORT_SOURCELOCATION_H

#include "llvm/Support/SMLoc.h"
#include "llvm/Support/SourceMgr.h" // Required for SMRange definitions

namespace moksha {

/// Wrapper around LLVM's Source Location (pointer to buffer).
/// We use this instead of raw pointers to abstract the underlying source manager.
using SourceLocation = llvm::SMLoc;

/// Represents a range of characters in the source code (Start -> End).
using SourceRange = llvm::SMRange;

} // namespace moksha

#endif // MOKSHA_SUPPORT_SOURCELOCATION_H
