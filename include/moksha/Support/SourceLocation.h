/**
 * @file SourceLocation.h
 * @brief Defines wrappers for LLVM source tracking primitives.
 * * By wrapping LLVM's SourceMgr types, the Moksha compiler isolates its AST
 * and Lexer from direct LLVM API dependencies where possible, making it easier
 * to maintain or swap out the underlying source management in the future.
 */

#ifndef MOKSHA_SUPPORT_SOURCELOCATION_H
#define MOKSHA_SUPPORT_SOURCELOCATION_H

#include "llvm/Support/SMLoc.h"
#include "llvm/Support/SourceMgr.h" // Required for SMRange definitions

namespace moksha {

/// @brief Wrapper around LLVM's Source Location (pointer to a specific byte in
/// the buffer).
using SourceLocation = llvm::SMLoc;

/// @brief Represents a continuous range of characters in the source code (Start
/// -> End).
using SourceRange = llvm::SMRange;

} // namespace moksha

#endif // MOKSHA_SUPPORT_SOURCELOCATION_H
