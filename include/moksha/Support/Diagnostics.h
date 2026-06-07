/**
 * @file Diagnostics.h
 * @brief Defines the diagnostic reporting engine for the Moksha compiler.
 * * This module provides the infrastructure for emitting errors, warnings,
 * and notes. It uses a builder pattern with overloaded stream operators
 * to easily format complex error messages.
 */

#ifndef MOKSHA_SUPPORT_DIAGNOSTICS_H
#define MOKSHA_SUPPORT_DIAGNOSTICS_H

#include "moksha/Support/SourceLocation.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include <string>

namespace moksha {

/// @brief Defines all unique diagnostic IDs for the compiler.
/// @note These IDs map to format strings and severity levels in the
/// DiagnosticEngine implementation.
enum class DiagID {
  // --- Lexer / Parser Errors ---
  err_unexpected_char,
  err_unexpected_token,
  err_expected_expression,
  err_expected_token,

  // --- Sema / Symbol Table Errors ---
  err_symbol_redefinition,
  err_undeclared_identifier,
  err_variable_redeclaration,
  err_function_redeclaration,

  // --- Type Checking Errors ---
  err_type_mismatch,
  err_type_incompatible_assignment,
  err_type_incompatible_return,
  err_const_violation,
  err_if_condition_bool,
  err_invalid_bin_op,
  err_invalid_unary_op,
  err_invalid_cast,
  err_missing_return,
  err_invalid_access,
  err_member_collision,
  err_argument_count_mismatch,
  err_unreachable_code,
  err_decimal_precision_loss,
  err_decimal_overflow,
  err_array_length,
  err_spread_fixed_array,
  err_ambiguous_inheritance,
  err_ambiguous_reference,
  err_null_assignment,
  err_infinite_size,
  err_generic_constraint,
  err_generic_arity,
  err_uninitialized_var,

  // --- MIR / Lowering Errors ---
  err_unexpanded_macro,
  err_invalid_type,
  err_not_implemented,

  // --- Borrow Checker Errors ---
  err_borrow_violation,
  err_mutation_while_borrowed,
  err_borrow_escape,
  err_use_after_move,
  err_partial_move,
  note_borrow_occurred_here,

  // --- Warnings ---
  warn_switch_not_exhaustive,
  warn_not_implemented,
  warn_unused_variable,
  warn_implicit_bool_conv,

  // --- Concurrency & Async Errors ---
  err_await_in_sync_lock,
  err_thread_in_async_lock,
  err_async_lock_target,

  // --- General / Notes ---
  note_previous_definition,
  err_no_member,
  err_unknown_type,
  err_break_outside_loop,
  err_continue_outside_loop,
  err_invalid_this,
  err_invalid_super,
  err_internal,
  err_missing_builtin,
};

class DiagnosticEngine;

/**
 * @brief A temporary object that collects format arguments via the `<<`
 * operator.
 * * @note This class uses the RAII pattern. The diagnostic is not emitted to
 * the engine when report() is called, but rather when this builder object is
 * destroyed at the end of the C++ statement.
 * @example Diags.report(Loc, DiagID::err_foo) << "argument"; // Emitted at the
 * semicolon
 */
class DiagnosticBuilder {
public:
  DiagnosticBuilder(DiagnosticEngine &Engine, SourceLocation Loc, DiagID ID,
                    llvm::SourceMgr::DiagKind Kind);

  /// @brief Destructor triggers the actual emission of the formatted
  /// diagnostic.
  ~DiagnosticBuilder();

  /// @brief Streams dynamic arguments into the diagnostic message.
  template <typename T> DiagnosticBuilder &operator<<(const T &Val) {
    llvm::raw_string_ostream(Message) << Val;
    return *this;
  }

private:
  DiagnosticEngine &Engine;
  SourceLocation Loc;
  DiagID ID;
  llvm::SourceMgr::DiagKind Kind;
  std::string Message;
};

/**
 * @brief The main entry point for tracking and reporting compiler diagnostics.
 * * This engine wraps `llvm::SourceMgr` and tracks the total number of errors
 * encountered to determine if compilation should halt before the next phase.
 */
class DiagnosticEngine {
public:
  explicit DiagnosticEngine(llvm::SourceMgr &SrcMgr) : SrcMgr(SrcMgr) {}

  /**
   * @brief Initiates a diagnostic report at the specified location.
   * @param Loc The source location where the diagnostic points.
   * @param ID The specific diagnostic identifier.
   * @return A DiagnosticBuilder to accept streaming arguments.
   */
  DiagnosticBuilder report(SourceLocation Loc, DiagID ID);

  /// @brief Low-level emitter called by the DiagnosticBuilder's destructor.
  void emit(SourceLocation Loc, DiagID ID, llvm::SourceMgr::DiagKind Kind,
            const std::string &ExtraMsg);

  /// @brief Returns true if any errors (excluding warnings) have been emitted.
  bool hasErrors() const { return NumErrors > 0; }

  /// @brief Returns the total count of emitted errors.
  unsigned getNumErrors() const { return NumErrors; }

  /// @brief Retrieves the base format string for a specific diagnostic ID.
  static const char *getDiagnosticText(DiagID ID);

  /// @brief Maps a diagnostic ID to its severity level (Error, Warning, Note).
  static llvm::SourceMgr::DiagKind getDiagnosticKind(DiagID ID);

private:
  llvm::SourceMgr &SrcMgr;
  unsigned NumErrors = 0;
};

} // namespace moksha

#endif // MOKSHA_SUPPORT_DIAGNOSTICS_H
