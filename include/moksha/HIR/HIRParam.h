#pragma once

#include "moksha/HIR/HIRType.h"
#include "moksha/Support/SourceLocation.h"
#include <memory>
#include <string>
#include <utility>

namespace moksha {
namespace hir {

class HIRExpr;

/** @brief Represents a function parameter */
struct HIRParam {
  std::string name;
  const HIRType *type;
  SourceLocation loc;
  std::unique_ptr<HIRExpr> defaultValue;

  HIRParam(std::string name, const HIRType *type, SourceLocation loc,
           std::unique_ptr<HIRExpr> defVal = nullptr);

  ~HIRParam();
  HIRParam(HIRParam &&) noexcept;
  HIRParam &operator=(HIRParam &&) noexcept;
  const std::string &getName() const { return name; }
  const HIRType *getType() const { return type; }
  SourceLocation getLoc() const { return loc; }
  const HIRExpr *getDefaultValue() const { return defaultValue.get(); }
};

} // namespace hir
} // namespace moksha
