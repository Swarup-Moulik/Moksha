#pragma once

#include "moksha/AST/ASTContext.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/Support/SourceLocation.h"
#include "llvm/ADT/StringRef.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace moksha {

/// Represents a single macro parameter.
struct MacroParam {
  std::string name;
  SourceLocation loc;

  MacroParam(llvm::StringRef n, SourceLocation l) : name(n.str()), loc(l) {}
};

/// Base class for all macro definitions.
class Macro {
public:
  enum class Kind {
    ObjectLike,  // Simple constant-like macros
    FunctionLike // Parameterized macros
  };

  Macro(Kind k, llvm::StringRef n, SourceLocation l)
      : kind(k), name(n.str()), loc(l) {}
  virtual ~Macro() = default;

  /// Expands the macro into a sequence of AST statements or expressions.
  virtual std::vector<std::unique_ptr<Stmt>>
  expand(const std::vector<std::unique_ptr<Expr>> &args,
         ASTContext &ctx) const = 0;

  Kind getKind() const { return kind; }
  llvm::StringRef getName() const { return name; }
  SourceLocation getLoc() const { return loc; }

protected:
  Kind kind;
  std::string name;
  SourceLocation loc;
};

/// Object-like macro (constant replacement)
class ObjectMacro : public Macro {
public:
  ObjectMacro(llvm::StringRef n, std::unique_ptr<Expr> val, SourceLocation l)
      : Macro(Kind::ObjectLike, n, l), value(std::move(val)) {}
  const Expr *getValue() const { return value.get(); }

  std::vector<std::unique_ptr<Stmt>>
  expand(const std::vector<std::unique_ptr<Expr>> &args,
         ASTContext &ctx) const override;

private:
  std::unique_ptr<Expr> value;
};

/// Function-like macro (parameterized)
class FunctionMacro : public Macro {
public:
  FunctionMacro(llvm::StringRef n, std::vector<MacroParam> params,
                std::vector<std::unique_ptr<Stmt>> body, SourceLocation l)
      : Macro(Kind::FunctionLike, n, l), params(std::move(params)),
        body(std::move(body)) {}

  const std::vector<MacroParam> &getParams() const { return params; }

  std::vector<std::unique_ptr<Stmt>>
  expand(const std::vector<std::unique_ptr<Expr>> &args,
         ASTContext &ctx) const override;

private:
  std::vector<MacroParam> params;
  std::vector<std::unique_ptr<Stmt>> body;
};

/// Macro Table: stores all defined macros for lookup
class MacroTable {
public:
  void addMacro(std::unique_ptr<Macro> macro);
  const Macro *lookup(llvm::StringRef name) const;
  bool contains(llvm::StringRef name) const;

private:
  std::unordered_map<std::string, std::unique_ptr<Macro>> table;
};

} // namespace moksha
