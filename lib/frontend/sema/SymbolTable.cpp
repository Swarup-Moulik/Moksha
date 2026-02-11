#include "moksha/Sema/SymbolTable.h"
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
  addPrimitiveTypes();
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
  // [CHANGE] Add assertion to crash early in Debug mode if logic is wrong
  assert(!scopeStack.empty() && "Cannot exit scope: stack is empty");

  // Keep your existing safety check for runtime/release safety
  if (scopeStack.size() <= 1) {
    llvm::errs()
        << "Compiler Internal Error: Attempting to pop Global scope.\n";
    return;
  }
  scopeStack.pop_back();
}

bool SymbolTable::addSymbol(llvm::StringRef name, Symbol symbol,
                            llvm::SMLoc loc) {
  if (scopeStack.empty())
    return false;

  Scope *currentScope = scopeStack.back().get();

  // Check for redefinition
  if (currentScope->findSymbol(name)) {
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

void SymbolTable::addPrimitiveTypes() {
  // NOTE: Even with 'int a = 10' syntax, 'int' must be defined here
  // so the Parser can look it up to verify it is a valid type.

  // 1. Define the Canonical Types (The "Real" Types)
  // In a real compiler, these would point to a Type* object.
  addSymbol("i8", Symbol(SymbolKind::Type, "i8"));
  addSymbol("i16", Symbol(SymbolKind::Type, "i16"));
  addSymbol("i32", Symbol(SymbolKind::Type, "i32"));
  addSymbol("i64", Symbol(SymbolKind::Type, "i64"));

  addSymbol("u8", Symbol(SymbolKind::Type, "u8"));
  addSymbol("u16", Symbol(SymbolKind::Type, "u16"));
  addSymbol("u32", Symbol(SymbolKind::Type, "u32"));
  addSymbol("u64", Symbol(SymbolKind::Type, "u64"));

  addSymbol("float", Symbol(SymbolKind::Type, "float"));
  addSymbol("double", Symbol(SymbolKind::Type, "double"));
  addSymbol("quarter", Symbol(SymbolKind::Type, "quarter")); // f8
    addSymbol("half",    Symbol(SymbolKind::Type, "half"));    // f16
  addSymbol("boolean", Symbol(SymbolKind::Type, "boolean"));
  addSymbol("void", Symbol(SymbolKind::Type, "void"));

  // 2. Define Aliases for C-Style Syntax
  // If your syntax is 'int a = 10', 'int' acts as an alias for 'i32'.
  // Both symbols should ideally point to the same internal Type representation.

  addSymbol("int", Symbol(SymbolKind::Type, "int"));   // Helper for i32
  addSymbol("char", Symbol(SymbolKind::Type, "char")); // Helper for u8 or i8
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
