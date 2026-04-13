#include "moksha/Sema/SymbolTable.h"
#include "moksha/AST/ASTContext.h"
#include "moksha/AST/Stmt.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {

// === Internal Helpers ===

static llvm::StringRef scopeKindToString(ScopeKind kind) {
  switch (kind) {
  case ScopeKind::Global:
    return "Global";
  case ScopeKind::Function:
    return "Function";
  case ScopeKind::Block:
    return "Block";
  case ScopeKind::Class:
    return "Class";
  default:
    return "Unknown";
  }
}

// === Scope Implementation ===

void Scope::addSymbol(llvm::StringRef name, Symbol symbol) {
  auto result = symbols.insert({name, symbol});
  (void)result;
  assert(result.second && "Symbol already exists in scope!");
}

Symbol *Scope::findSymbol(llvm::StringRef name) {
  auto it = symbols.find(name);
  if (it != symbols.end()) {
    return &it->second;
  }
  return nullptr;
}

const Symbol *Scope::findSymbol(llvm::StringRef name) const {
  auto it = symbols.find(name);
  if (it != symbols.end()) {
    return &it->second;
  }
  return nullptr;
}

// === SymbolTable Implementation ===

SymbolTable::SymbolTable(DiagnosticEngine &Diags) : Diags(Diags) {
  enterScope(ScopeKind::Global);
}

SymbolTable::~SymbolTable() {
  while (!scopeStack.empty()) {
    exitScope();
  }
}

Scope *SymbolTable::currentScope() {
  return scopeStack.empty() ? nullptr : scopeStack.back().get();
}

void SymbolTable::enterScope(ScopeKind kind) {
  scopeStack.push_back(std::make_unique<Scope>(kind));
}

void SymbolTable::exitScope() {
  assert(!scopeStack.empty() && "Cannot exit scope: stack is empty");
  scopeStack.pop_back();
}

bool SymbolTable::addSymbol(llvm::StringRef name, Symbol symbol,
                            llvm::SMLoc loc) {
  if (scopeStack.empty())
    return false;

  Scope *currentScope = scopeStack.back().get();

  // Check for redefinition OR overload
  if (Symbol *existing = currentScope->findSymbol(name)) {

    // Allow redundant Module or Variable imports (Idempotency)
    if (existing->kind == symbol.kind &&
        (symbol.kind == SymbolKind::Module ||
         symbol.kind == SymbolKind::Variable)) {
      return true;
    }

    if (existing->kind == SymbolKind::Function &&
        symbol.kind == SymbolKind::Function) {
      existing->overloads.push_back(symbol);
      return true;
    }
    Diags.report(loc, DiagID::err_symbol_redefinition) << name;
    return false;
  }

  currentScope->addSymbol(name, symbol);
  return true;
}

Symbol *SymbolTable::lookup(llvm::StringRef name) {
  for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
    Symbol *sym = (*it)->findSymbol(name);
    if (sym)
      return sym;
  }
  return nullptr;
}

const Symbol *SymbolTable::lookup(llvm::StringRef name) const {
  for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
    const Symbol *sym = (*it)->findSymbol(name);
    if (sym)
      return sym;
  }
  return nullptr;
}

Symbol *SymbolTable::lookupGlobal(llvm::StringRef name) {
  if (scopeStack.empty())
    return nullptr;
  return scopeStack.front()->findSymbol(name);
}

bool SymbolTable::isDefinedInCurrentScope(llvm::StringRef name) const {
  if (scopeStack.empty())
    return false;
  return scopeStack.back()->findSymbol(name) != nullptr;
}

ScopeKind SymbolTable::getCurrentScopeKind() const {
  if (scopeStack.empty())
    return ScopeKind::Global;
  return scopeStack.back()->getKind();
}

// === Helper: Built-in Primitives ===

void SymbolTable::addPrimitiveTypes(ASTContext &ctx) {
  // 1. Define the Canonical Types
  addSymbol("i8", Symbol(SymbolKind::Type, "i8", ctx.getI8Type()));
  addSymbol("i16", Symbol(SymbolKind::Type, "i16", ctx.getI16Type()));
  addSymbol("i32", Symbol(SymbolKind::Type, "i32", ctx.getI32Type()));
  addSymbol("i64", Symbol(SymbolKind::Type, "i64", ctx.getI64Type()));
  addSymbol("u8", Symbol(SymbolKind::Type, "u8", ctx.getU8Type()));
  addSymbol("u16", Symbol(SymbolKind::Type, "u16", ctx.getU16Type()));
  addSymbol("u32", Symbol(SymbolKind::Type, "u32", ctx.getU32Type()));
  addSymbol("u64", Symbol(SymbolKind::Type, "u64", ctx.getU64Type()));

  addSymbol("f8", Symbol(SymbolKind::Type, "f8", ctx.getF8Type()));
  addSymbol("f16", Symbol(SymbolKind::Type, "f16", ctx.getF16Type()));
  addSymbol("f32", Symbol(SymbolKind::Type, "f32", ctx.getF32Type()));
  addSymbol("f64", Symbol(SymbolKind::Type, "f64", ctx.getF64Type()));
  addSymbol("float", Symbol(SymbolKind::Type, "float", ctx.getF32Type()));
  addSymbol("double", Symbol(SymbolKind::Type, "double", ctx.getF64Type()));

  addSymbol("boolean", Symbol(SymbolKind::Type, "boolean", ctx.getBoolType()));
  addSymbol("void", Symbol(SymbolKind::Type, "void", ctx.getVoidType()));
  addSymbol("any", Symbol(SymbolKind::Type, "any", ctx.getAnyType()));

  // 2. Define Aliases for C-Style Syntax
  addSymbol("int", Symbol(SymbolKind::Type, "int", ctx.getI32Type()));
  addSymbol("char", Symbol(SymbolKind::Type, "char", ctx.getCharType()));

  addSymbol("isize", Symbol(SymbolKind::Type, "isize", ctx.getISizeType()));
  addSymbol("usize", Symbol(SymbolKind::Type, "usize", ctx.getUSizeType()));

  addSymbol("string", Symbol(SymbolKind::Type, "string", ctx.getStringType()));
  addSymbol("bool", Symbol(SymbolKind::Type, "bool", ctx.getBoolType()));
}

void SymbolTable::dump() const {
  llvm::errs() << "=== Symbol Table Dump ===\n";
  int level = 0;
  for (const auto &scope : scopeStack) {
    llvm::errs() << "Scope Level " << level << " ("
                 << scopeKindToString(scope->getKind()) << "):\n";

    // [CHANGE] Create an indentation string based on level
    std::string indent(level * 2, ' ');

    for (const auto &entry : scope->symbols) {
      // [CHANGE] Use the indent
      llvm::errs() << indent << "  - " << entry.getKey() << " ["
                   << entry.getValue().kindToString() << "]\n";
    }
    level++;
  }
  llvm::errs() << "=========================\n";
}

} // namespace moksha
