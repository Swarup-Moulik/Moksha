#include "moksha/Support/Diagnostics.h"
#include "llvm/ADT/Twine.h"

namespace moksha {

// Diagnostic Builder

DiagnosticBuilder::DiagnosticBuilder(DiagnosticEngine &Engine,
                                     SourceLocation Loc, DiagID ID,
                                     llvm::SourceMgr::DiagKind Kind)
    : Engine(Engine), Loc(Loc), ID(ID), Kind(Kind) {}

DiagnosticBuilder::~DiagnosticBuilder() { Engine.emit(Loc, ID, Kind, Message); }

// Diagnostic Engine

DiagnosticBuilder DiagnosticEngine::report(SourceLocation Loc, DiagID ID) {
  return DiagnosticBuilder(*this, Loc, ID, getDiagnosticKind(ID));
}

void DiagnosticEngine::emit(SourceLocation Loc, DiagID ID,
                            llvm::SourceMgr::DiagKind Kind,
                            const std::string &ExtraMsg) {

  llvm::StringRef BaseMsg = getDiagnosticText(ID);

  std::string FinalMsg = BaseMsg.str();
  if (!ExtraMsg.empty()) {
    FinalMsg += ": " + ExtraMsg;
  }

  if (Kind == llvm::SourceMgr::DK_Error) {
    NumErrors++;
  }

  SrcMgr.PrintMessage(Loc, Kind, llvm::Twine(FinalMsg));
}

// Diagnostic Registry
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
  // Parser errors
  case DiagID::err_unexpected_char:
    return "Unexpected character";
  case DiagID::err_unexpected_token:
    return "Unexpected token";
  case DiagID::err_expected_expression:
    return "Expected expression";
  case DiagID::err_expected_token:
    return "Expected token";

  // Sema & Symbol Table errors
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

  // Type checking errors
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
  case DiagID::err_member_collision:
    return "Member collision";
  case DiagID::err_argument_count_mismatch:
    return "Function argument count mismatch: expected vs provided";
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
  case DiagID::err_decimal_precision_loss:
    return "Decimal scale truncation";
  case DiagID::err_decimal_overflow:
    return "Possible decimal overflow";
  case DiagID::err_builtin_shadowing:
    return "Builtin shadowing";

  // MIR / Lowering Errors
  case DiagID::err_unexpanded_macro:
    return "Unexpanded macro reached MIR lowering phase";
  case DiagID::err_invalid_type:
    return "Invalid or unsupported type found";
  case DiagID::err_not_implemented:
    return "Feature not yet implemented";

  // Borrow checker errors
  case DiagID::err_borrow_violation:
    return "Borrow checker violation";
  case DiagID::err_mutation_while_borrowed:
    return "Cannot mutate variable while it is actively borrowed";
  case DiagID::err_borrow_escape:
    return "Borrow escape detected";
  case DiagID::err_use_after_move:
    return "Use of moved value";
  case DiagID::err_partial_move:
    return "Use of partially moved value";
  case DiagID::note_borrow_occurred_here:
    return "Borrow occurred here";

  // Warnings
  case DiagID::warn_switch_not_exhaustive:
    return "Switch statement is not exhaustive";
  case DiagID::warn_not_implemented:
    return "Feature not yet implemented (Warning)";
  case DiagID::warn_unused_variable:
    return "Variable declared but never used";
  case DiagID::err_missing_builtin:
    return "Missing required compiler builtin or runtime function";

  // Concurrency & Async errors
  case DiagID::err_await_in_sync_lock:
    return "Cannot use 'await' inside a synchronous 'lock' block (causes OS "
           "thread deadlock)";
  case DiagID::err_thread_in_async_lock:
    return "Cannot spawn an OS thread inside an 'async lock' (causes scheduler "
           "starvation)";
  case DiagID::err_async_lock_target:
    return "Target of 'async lock' must be an 'AsyncMutex'";

  // General notes
  case DiagID::note_previous_definition:
    return "Previous definition is here";
  case DiagID::err_no_member:
    return "No such member";
  case DiagID::err_unknown_type:
    return "Unknown type";
  case DiagID::err_break_outside_loop:
    return "'break' statement outside of loop or switch";
  case DiagID::err_continue_outside_loop:
    return "'continue' statement outside of loop";
  case DiagID::err_invalid_this:
    return "Invalid use of 'this' outside of a class method";
  case DiagID::err_invalid_super:
    return "Invalid use of 'super'";

  default:
    return "Unknown error";
  }
}

} // namespace moksha
