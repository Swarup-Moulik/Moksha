#include "moksha/HIR/HIRFunction.h"
#include "moksha/HIR/HIRExpr.h"
#include "moksha/HIR/HIRParam.h"
#include "moksha/HIR/HIRStmt.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/HIR/HIRVisitor.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <iostream>

namespace moksha {
namespace hir {

HIRFunction::HIRFunction(std::string name,
                         std::vector<HIRGenericParam> typeParams,
                         std::vector<HIRParam> params,
                         const HIRType *returnType,
                         std::unique_ptr<HIRStmt> body, bool isAsync,
                         bool isVariadic, bool isInterrupt, bool isNaked,
                         bool isNoReturn, std::string sectionName,
                         SourceLocation loc)
    : name(std::move(name)), typeParams(std::move(typeParams)),
      params(std::move(params)), returnType(returnType), body(std::move(body)),
      isAsync(isAsync), isVariadic(isVariadic), isInterrupt(isInterrupt),
      isNaked(isNaked), isNoReturn(isNoReturn),
      sectionName(std::move(sectionName)), loc(loc) {

  assert(returnType && "HIRFunction returnType must not be null");

#ifndef NDEBUG
  for (const auto &p : this->params) {
    assert(p.type && "HIRParam type must not be null");
  }
#endif
}

const std::string &HIRFunction::getName() const { return name; }
const std::vector<HIRGenericParam> &HIRFunction::getTypeParams() const {
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

// ============================================================================
// [HIRParam Implementation]
// ============================================================================
HIRParam::HIRParam(std::string name, const HIRType *type, SourceLocation loc,
                   std::unique_ptr<HIRExpr> defVal)
    : name(std::move(name)), type(type), loc(loc),
      defaultValue(std::move(defVal)) {}

HIRParam::~HIRParam() = default;
HIRParam::HIRParam(HIRParam &&) noexcept = default;
HIRParam &HIRParam::operator=(HIRParam &&) noexcept = default;

// [FIX] Updated to take ostream reference
static void printIndent(llvm::raw_ostream &os, int indent) {
  for (int i = 0; i < indent; ++i)
    os << "  ";
}

void HIRFunction::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  os << "Func: " << name;

  // 1. Generic Params
  if (!typeParams.empty()) {
    os << "<";
    for (size_t i = 0; i < typeParams.size(); ++i) {
      // [FIX] Safely unpack the new struct
      if (typeParams[i].isShared)
        os << "shared ";
      os << typeParams[i].name << (i < typeParams.size() - 1 ? ", " : "");
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
  if (isInterrupt)
    os << " [interrupt]";
  if (isNaked)
    os << " [naked]";
  if (isNoReturn)
    os << " [noreturn]";
  if (isStaticFunc())
    os << " [static]";
  if (isInlineFunc())
    os << " [inline]";
  if (isNoInlineFunc())
    os << " [noinline]";
  if (isPureFunc())
    os << " [pure]";
  if (isColdFunc())
    os << " [cold]";
  if (isUsedFunc())
    os << " [used]";
  if (isWeakFlag)
    os << " [weak]";
  if (!abi.empty() && abi != "C")
    os << " [abi(\"" << abi << "\")]";
  if (!sectionName.empty())
    os << " [section(\"" << sectionName << "\")]";
  if (isVirtual)
    os << " [virtual]";
  if (isOverride)
    os << " [override]";
  if (vtableIndex >= 0)
    os << " [vtable_idx=" << vtableIndex << "]";

  os << "\n";

  // 3. Parameters
  if (!params.empty()) {
    printIndent(os, indent + 1);
    os << "Params: ";
    for (size_t i = 0; i < params.size(); ++i) {
      os << params[i].name << ": " << params[i].type->toString();

      if (params[i].getDefaultValue()) {
        os << " = <default_val>";
      }

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

// ============================================================================
// [HIRClass Implementation]
// ============================================================================

void HIRClass::dump(llvm::raw_ostream &os, int indent) const {
  printIndent(os, indent);
  if (isRefClass()) {
    os << "RefClass: " << name;
  } else {
    os << "Class: " << name;
  }
  if (isPacked()) {
    os << " [packed]";
  }
  if (alignment > 0)
    os << " [align(" << alignment << ")]";
  if (!sectionName.empty())
    os << " [section(\"" << sectionName << "\")]";
  if (hasVTableFlag)
    os << " [vtable]";
  if (!parentTypes.empty()) {
    os << " : ";
    for (size_t i = 0; i < parentTypes.size(); ++i) {
      os << parentTypes[i]->toString();
      if (i < parentTypes.size() - 1)
        os << ", ";
    }
  }
  os << "\n";

  for (const auto &method : methods) {
    if (method)
      method->dump(os, indent + 1);
  }
}

} // namespace hir
} // namespace moksha
