#include "moksha/Support/Diagnostics.h"
#include "llvm/ADT/Twine.h"

namespace moksha {

// === Diagnostic Builder ===

DiagnosticBuilder::DiagnosticBuilder(DiagnosticEngine &Engine,
                                     SourceLocation Loc, DiagID ID,
                                     llvm::SourceMgr::DiagKind Kind)
    : Engine(Engine), Loc(Loc), ID(ID), Kind(Kind) {}

DiagnosticBuilder::~DiagnosticBuilder() {
  // When the builder goes out of scope (end of line), emit the diagnostic.
  Engine.emit(Loc, ID, Kind, Message);
}

// === Diagnostic Engine ===

DiagnosticBuilder DiagnosticEngine::report(SourceLocation Loc, DiagID ID) {
  return DiagnosticBuilder(*this, Loc, ID, getDiagnosticKind(ID));
}

void DiagnosticEngine::emit(SourceLocation Loc, DiagID ID,
                            llvm::SourceMgr::DiagKind Kind,
                            const std::string &ExtraMsg) {
  // 1. Get the base message template (e.g., "Redefinition of symbol")
  llvm::StringRef BaseMsg = getDiagnosticText(ID);

  // 2. Combine base message with any streamed arguments
  std::string FinalMsg = BaseMsg.str();
  if (!ExtraMsg.empty()) {
    FinalMsg += ": " + ExtraMsg;
  }

  // 3. Update error count
  if (Kind == llvm::SourceMgr::DK_Error) {
    NumErrors++;
  }

  // 4. Delegate to LLVM's SourceMgr to print the pretty error ONCE
  SrcMgr.PrintMessage(Loc, Kind, llvm::Twine(FinalMsg));
}

// === Diagnostic Registry ===

llvm::SourceMgr::DiagKind DiagnosticEngine::getDiagnosticKind(DiagID ID) {
  switch (ID) {
  case DiagID::note_previous_definition:
    return llvm::SourceMgr::DK_Note;
  case DiagID::warn_unused_variable:
  case DiagID::warn_switch_not_exhaustive:
  case DiagID::warn_not_implemented:
  case DiagID::warn_implicit_bool_conv:
    return llvm::SourceMgr::DK_Warning;
  default:
    return llvm::SourceMgr::DK_Error;
  }
}

const char *DiagnosticEngine::getDiagnosticText(DiagID ID) {
  switch (ID) {
  // Parser
  case DiagID::err_unexpected_char:
    return "Unexpected character";
  case DiagID::err_unexpected_token:
    return "Unexpected token";
  case DiagID::err_expected_expression:
    return "Expected expression";
  case DiagID::err_expected_token:
    return "Expected token";

  // Sema
  case DiagID::err_symbol_redefinition:
    return "Redefinition of symbol";
  case DiagID::err_undeclared_identifier:
    return "Undeclared identifier";
  case DiagID::err_variable_redeclaration:
    return "Variable already declared in this scope";
  case DiagID::err_function_redeclaration:
    return "Function already declared";
  case DiagID::err_internal:
    return "Internal compiler error";

  // Type Checking
  case DiagID::err_type_mismatch:
    return "Type mismatch";
  case DiagID::err_const_violation:
    return "Cannot assign to const variable";
  case DiagID::err_type_incompatible_assignment:
    return "Incompatible types in assignment";
  case DiagID::err_type_incompatible_return:
    return "Return value type does not match function return type";
  case DiagID::err_if_condition_bool:
    return "'if' condition must be boolean";
  case DiagID::err_invalid_bin_op:
    return "Invalid operands for binary operator";
  case DiagID::err_invalid_unary_op:
    return "Invalid operand for unary operator";
  case DiagID::err_invalid_cast:
    return "Invalid type cast";
  case DiagID::err_missing_return:
    return "Non-void function may end without returning a value";
  case DiagID::err_invalid_access:
    return "Invalid access to member";
  case DiagID::warn_implicit_bool_conv:
    return "Implicit conversion to boolean";
  case DiagID::err_array_length:
    return "Length mismatch";
  case DiagID::err_spread_fixed_array:
    return "Cannot use spread operator (...) on fixed-size arrays";
  case DiagID::err_ambiguous_inheritance:
    return "Ambiguous inheritance";
  case DiagID::err_ambiguous_reference:
    return "Ambiguous reference";
  case DiagID::err_null_assignment:
    return "Cannot assign 'null' to non-nullable type";
  case DiagID::err_infinite_size:
    return "Recursive type has infinite size";
  case DiagID::err_generic_constraint:
    return "Generic constraint violation";
  case DiagID::err_generic_arity:
    return "Generic arity mismatch";
  case DiagID::err_uninitialized_var:
    return "Uninitialized variable";
  case DiagID::err_unreachable_code:
    return "Unreachable code";

  // MIR / Lowering Errors (Added)
  case DiagID::err_unexpanded_macro:
    return "Unexpanded macro reached MIR lowering phase";
  case DiagID::err_invalid_type:
    return "Invalid or unsupported type found";
  case DiagID::err_not_implemented:
    return "Feature not yet implemented";

  // Borrow Checker Errors
  case DiagID::err_borrow_violation:
    return "Borrow checker violation";
  case DiagID::err_mutation_while_borrowed:
    return "Cannot mutate variable while it is actively borrowed";

  // Warnings
  case DiagID::warn_switch_not_exhaustive:
    return "Switch statement is not exhaustive";
  case DiagID::warn_not_implemented:
    return "Feature not yet implemented (Warning)";
  case DiagID::warn_unused_variable:
    return "Variable declared but never used";

  // Notes
  case DiagID::note_previous_definition:
    return "Previous definition is here";

  default:
    return "Unknown error";
  }
}

} // namespace moksha
