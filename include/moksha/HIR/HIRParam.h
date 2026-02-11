#pragma once

#include "moksha/HIR/HIRType.h"
#include "moksha/Support/SourceLocation.h"
#include <string>
#include <utility>

namespace moksha {
namespace hir {

struct HIRParam {
    std::string name;
    const HIRType *type;
    SourceLocation loc;

    HIRParam(std::string name, const HIRType *type, SourceLocation loc)
        : name(std::move(name)), type(type), loc(loc) {}

    // Add these getters if your code uses them as accessors
    const std::string& getName() const { return name; }
    const HIRType* getType() const { return type; }
    SourceLocation getLoc() const { return loc; }
};

} // namespace hir
} // namespace moksha
