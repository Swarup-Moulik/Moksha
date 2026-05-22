#pragma once

#include "moksha/AST/ASTContext.h"
#include "moksha/AST/Stmt.h"
#include "moksha/Sema/SymbolTable.h"

namespace moksha {

class BuiltinRegistry {
public:
  static void registerBuiltins(ASTContext &ctx, SymbolTable &sym);

private:
  static void registerMathBuiltins(ASTContext &ctx, SymbolTable &sym);
  static void registerGenericArrayBuiltins(ASTContext &ctx, SymbolTable &sym);
  static void registerStringBuiltins(ASTContext &ctx, SymbolTable &sym);
  static void registerMapBuiltins(ASTContext &ctx, SymbolTable &sym);
  static void registerStandardIO(ASTContext &ctx, SymbolTable &sym);
  static void registerAtomics(ASTContext &ctx, SymbolTable &sym);
  static void registerAsyncBuiltins(ASTContext &ctx, SymbolTable &sym);
  static void registerFileBuiltins(ASTContext &ctx, SymbolTable &sym);
};

} // namespace moksha
