#pragma once

#include "moksha/AST/ASTContext.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include <optional>
#include <vector>

namespace moksha {

class FunctionDecl;
enum class GenericError {
  ArityMismatch,
  AnyConstraintViolation,
  SharedConstraintViolation
};

/** @brief Handles the logic for generic instantiation, substitution, and validation. */
class GenericResolver {
public:
  explicit GenericResolver(ASTContext &ctx);

  /** @brief Validates that provided generic arguments match the declaration's parameters. */
  [[nodiscard]] std::optional<GenericError>
  validateGenericArgs(const std::vector<GenericDecl::GenericParam> &typeParams,
                      const std::vector<NamedType::GenericArg> &args);

  /** @brief Creates a new Type with generic parameters substituted by concrete arguments. */
  [[nodiscard]] TypePtr
  substituteType(const Type *type,
                 const llvm::StringMap<const Type *> &substitutions);

  /** @brief Resolves a generic function signature into a concrete one. */
  struct [[nodiscard]] ConcreteSignature {
    const FunctionDecl *decl;
    TypePtr returnType;
    std::vector<TypePtr> paramTypes;
  };

  [[nodiscard]] ConcreteSignature
  resolveFunctionSignature(const FunctionDecl *funcDecl,
                           const llvm::StringMap<const Type *> &substitutions);

  std::string getMangledName(llvm::StringRef baseName,
                             const std::vector<const Type *> &typeArgs);

  const ClassDecl *instantiateClass(const GenericDecl *genericTemplate,
                                    const std::vector<const Type *> &typeArgs);

  const FunctionDecl *
  instantiateFunction(const GenericDecl *genericTemplate,
                      const std::vector<const Type *> &typeArgs);

private:
  ASTContext &context;
  llvm::StringMap<const ClassDecl *> instantiatedClasses;
  llvm::StringMap<const FunctionDecl *> instantiatedFunctions;
};

} // namespace moksha
