#pragma once
#include "moksha/AST/Type.h" // Needed for Type* in Symbol
#include "moksha/Support/Diagnostics.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include <memory>
#include <string>
#include <vector>

namespace moksha {

class Decl;
class ASTContext;

enum class SymbolKind { Variable, Function, Type, Class, Module };

// [Requirement 1] Symbol Definition
struct Symbol {
  SymbolKind kind;
  std::string name;
  const Type *type = nullptr;
  const Decl *decl = nullptr;
  std::vector<Symbol> overloads;
  int bitWidth = -1;

  Symbol() : kind(SymbolKind::Variable) {}
  Symbol(SymbolKind k, std::string n, const Type *t = nullptr,
         const Decl *d = nullptr)
      : kind(k), name(std::move(n)), type(t), decl(d) {}

  std::string kindToString() const {
    switch (kind) {
    case SymbolKind::Variable:
      return "Variable";
    case SymbolKind::Function:
      return "Function";
    case SymbolKind::Type:
      return "Type";
    case SymbolKind::Class:
      return "Class";
    case SymbolKind::Module:
      return "Module";
    default:
      return "Unknown";
    }
  }
};

enum class ScopeKind { Global, Function, Block, Class };

// [Requirement 2] Scope Definition
class Scope {
public:
  // Public so SymbolTable.cpp can iterate over it in dump()
  llvm::StringMap<Symbol> symbols;

  Scope(ScopeKind k) : kind(k) {}

  void addSymbol(llvm::StringRef name, Symbol symbol);
  Symbol *findSymbol(llvm::StringRef name);
  const Symbol *findSymbol(llvm::StringRef name) const;
  ScopeKind getKind() const { return kind; }

private:
  ScopeKind kind;
};

class SymbolTable {
public:
  explicit SymbolTable(DiagnosticEngine &Diags);
  ~SymbolTable();

  void enterScope(ScopeKind kind);
  void exitScope();

  // Returns true if successful, false if redefinition
  bool addSymbol(llvm::StringRef name, Symbol symbol, llvm::SMLoc loc = {});

  Symbol *lookup(llvm::StringRef name);
  const Symbol *lookup(llvm::StringRef name) const;
  Symbol *lookupGlobal(llvm::StringRef name);

  bool isDefinedInCurrentScope(llvm::StringRef name) const;
  ScopeKind getCurrentScopeKind() const;
  Scope *currentScope();
  void addPrimitiveTypes(ASTContext &ctx);
  void dump() const;

private:
  void addPrimitiveTypes();

  std::vector<std::unique_ptr<Scope>> scopeStack;
  DiagnosticEngine &Diags;
};

} // namespace moksha
