#pragma once

#include "moksha/HIR/HIRType.h"
#include "moksha/Support/SourceLocation.h"
#include <string>
#include <utility>
#include <memory>

namespace moksha {
namespace hir {

class HIRExpr;

struct HIRParam {
  std::string name;
  const HIRType *type;
  SourceLocation loc;
  std::unique_ptr<HIRExpr> defaultValue;

  HIRParam(std::string name, const HIRType *type, SourceLocation loc,
           std::unique_ptr<HIRExpr> defVal = nullptr)
      : name(std::move(name)), type(type), loc(loc),
        defaultValue(std::move(defVal)) {}

  // Add these getters if your code uses them as accessors
  const std::string &getName() const { return name; }
  const HIRType *getType() const { return type; }
  SourceLocation getLoc() const { return loc; }
  const HIRExpr *getDefaultValue() const { return defaultValue.get(); }
};

} // namespace hir
} // namespace moksha
