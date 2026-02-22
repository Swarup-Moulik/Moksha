#ifndef MOKSHA_SUPPORT_DIAGNOSTICS_H
#define MOKSHA_SUPPORT_DIAGNOSTICS_H

#include "moksha/Support/SourceLocation.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include <string>

namespace moksha {

/// Defines all unique diagnostic IDs for the compiler.
enum class DiagID {
  // Lexer / Parser Errors
  err_unexpected_char,
  err_unexpected_token,
  err_expected_expression,
  err_expected_token,

  // Sema / Symbol Table Errors
  err_symbol_redefinition,
  err_undeclared_identifier,
  err_variable_redeclaration,
  err_function_redeclaration,

  // Type Checking Errors
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
  err_unreachable_code,
  err_array_length,
  err_ambiguous_inheritance,
  err_ambiguous_reference,
  err_null_assignment,
  err_infinite_size,
  err_generic_constraint,
  err_generic_arity,
  err_uninitialized_var,

  // MIR / Lowering Errors (Added these to fix the build error)
  err_unexpanded_macro,
  err_invalid_type,
  err_not_implemented,

  // Warnings
  warn_switch_not_exhaustive,
  warn_not_implemented,
  warn_unused_variable,

  // General / Notes
  note_previous_definition,
  err_argument_count_mismatch,
  err_no_member,
  err_unknown_type,
  err_break_outside_loop,
  err_continue_outside_loop,
  err_invalid_this,
  err_invalid_super,
  warn_implicit_bool_conv,
  err_internal,
};

/// Forward declaration
class DiagnosticEngine;

/// A temporary object that collects arguments (<< "str") and emits
/// the diagnostic when it is destroyed.
class DiagnosticBuilder {
public:
  // CHANGE: Remove the body { ... } and end with a semicolon.
  DiagnosticBuilder(DiagnosticEngine &Engine, SourceLocation Loc, DiagID ID,
                    llvm::SourceMgr::DiagKind Kind);

  ~DiagnosticBuilder();

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

/// The main entry point for reporting errors.
class DiagnosticEngine {
public:
  explicit DiagnosticEngine(llvm::SourceMgr &SrcMgr) : SrcMgr(SrcMgr) {}

  /// Reports an error/warning at the given location.
  /// Usage: Diags.report(Loc, DiagID::err_foo) << "arg";
  DiagnosticBuilder report(SourceLocation Loc, DiagID ID);

  /// Low-level emitter called by the Builder's destructor.
  void emit(SourceLocation Loc, DiagID ID, llvm::SourceMgr::DiagKind Kind,
            const std::string &ExtraMsg);

  bool hasErrors() const { return NumErrors > 0; }
  unsigned getNumErrors() const { return NumErrors; }

  /// Helper to get the format string for an ID (e.g. "expected '{}'")
  static const char *getDiagnosticText(DiagID ID);

  /// Helper to get the severity (Error/Warning/Note) for an ID
  static llvm::SourceMgr::DiagKind getDiagnosticKind(DiagID ID);

private:
  llvm::SourceMgr &SrcMgr;
  unsigned NumErrors = 0;
};

} // namespace moksha

#endif // MOKSHA_SUPPORT_DIAGNOSTICS_H
