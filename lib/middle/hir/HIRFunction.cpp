#include "moksha/HIR/HIRFunction.h"
#include "moksha/HIR/HIRParam.h"
#include "moksha/HIR/HIRStmt.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/HIR/HIRVisitor.h"
#include <cassert> // [FIX] Required for invariants
#include <iostream>

namespace moksha {
namespace hir {

HIRFunction::HIRFunction(std::string name, std::vector<std::string> typeParams,
                         std::vector<HIRParam> params,
                         const HIRType *returnType,
                         std::unique_ptr<HIRStmt> body, bool isAsync,
                         bool isVariadic, SourceLocation loc)
    : name(std::move(name)), typeParams(std::move(typeParams)),
      params(std::move(params)), returnType(returnType), body(std::move(body)),
      isAsync(isAsync), isVariadic(isVariadic), loc(loc) {

  // [FIX] Enforce canonical type invariants
  assert(returnType && "HIRFunction returnType must not be null");

#ifndef NDEBUG
  for (const auto &p : this->params) {
    assert(p.type && "HIRParam type must not be null");
  }
#endif
}

const std::string &HIRFunction::getName() const { return name; }
const std::vector<std::string> &HIRFunction::getTypeParams() const {
  return typeParams;
}
const std::vector<HIRParam> &HIRFunction::getParams() const { return params; }

const HIRStmt *HIRFunction::getBody() const { return body.get(); }
const HIRType *HIRFunction::getReturnType() const { return returnType; }

bool HIRFunction::isAsyncFunc() const { return isAsync; }
bool HIRFunction::isVariadicFunc() const { return isVariadic; }
SourceLocation HIRFunction::getLoc() const { return loc; }

void HIRFunction::accept(HIRVisitor &visitor) { visitor.visitFunction(*this); }

void HIRFunction::accept(ConstHIRVisitor &visitor) const {
  visitor.visitFunction(*this);
}

// [FIX] Updated to take ostream reference
static void printIndent(std::ostream &os, int indent) {
  for (int i = 0; i < indent; ++i)
    os << "  ";
}

// [FIX] Updated signature to use ostream
void HIRFunction::dump(std::ostream &os, int indent) const {
  printIndent(os, indent);
  os << "Func: " << name;

  // 1. Generic Params
  if (!typeParams.empty()) {
    os << "<";
    for (size_t i = 0; i < typeParams.size(); ++i) {
      os << typeParams[i] << (i < typeParams.size() - 1 ? ", " : "");
    }
    os << ">";
  }

  // 2. Flags
  if (isAsync)
    os << " [async]";
  if (isVariadic)
    os << " [variadic]";
  if (isExtern())
    os << " [extern]";

  os << "\n";

  // 3. Parameters
  if (!params.empty()) {
    printIndent(os, indent + 1);
    os << "Params: ";
    for (size_t i = 0; i < params.size(); ++i) {
      os << params[i].name << ": ";
      // [FIX] No longer need to check if params[i].type is null due to
      // constructor assert
      os << params[i].type->toString();

      if (i < params.size() - 1)
        os << ", ";
    }
    os << "\n";
  }

  // 4. Return Type
  if (returnType) {
    printIndent(os, indent + 1);
    os << "Return: " << returnType->toString() << "\n";
  }

  // 5. Body
  if (body) {
    printIndent(os, indent + 1);
    os << "Body:\n";
    body->dump(os, indent + 2);
  }
}

} // namespace hir
} // namespace moksha
