#pragma once

#include "moksha/AST/ASTContext.h"
#include "moksha/AST/Type.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include <optional>
#include <vector>

namespace moksha {

class FunctionDecl;
enum class GenericError { ArityMismatch, ConstraintViolation };

/// Handles the logic for generic instantiation, substitution, and validation.
class GenericResolver {
public:
  explicit GenericResolver(ASTContext &ctx);

  /// Validates that provided generic arguments match the declaration's
  /// parameters. Checks for: Argument count mismatch, 'any' type constraint.
  /// \return std::nullopt if valid, or a GenericError if invalid.
  [[nodiscard]] std::optional<GenericError>
  validateGenericArgs(const std::vector<llvm::StringRef> &typeParams,
                      const std::vector<NamedType::GenericArg> &args);

  /// Creates a new Type with generic parameters substituted by concrete
  /// arguments. E.g., substitutes 'T' with 'int' in 'Box<T>'.
  /// This performs a deep recursive traversal of the type structure (including
  /// nested Arrays, Maps, and Function types) to ensure all occurrences of
  /// the generic parameters are replaced with their concrete counterparts.
  [[nodiscard]] TypePtr
  substituteType(const Type *type,
                 const llvm::StringMap<const Type *> &substitutions);

  /// Resolves a generic function signature into a concrete one.
  /// Used when calling 'update(T val)' on 'Box<int>'.
  struct [[nodiscard]] ConcreteSignature {
    TypePtr returnType;
    std::vector<TypePtr> paramTypes;
  };

  [[nodiscard]] ConcreteSignature
  resolveFunctionSignature(const FunctionDecl *funcDecl,
                           const llvm::StringMap<const Type *> &substitutions);

private:
  ASTContext &context;
};

} // namespace moksha
