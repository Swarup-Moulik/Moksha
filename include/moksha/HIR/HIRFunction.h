#pragma once

#include "moksha/HIR/HIRParam.h"
#include "moksha/Support/SourceLocation.h"
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace moksha {
namespace hir {

class HIRStmt;
class HIRType;

class HIRVisitor;
class ConstHIRVisitor;

class HIRFunction {
public:
  HIRFunction(std::string name, std::vector<std::string> typeParams,
              std::vector<HIRParam> params, const HIRType *returnType,
              std::unique_ptr<HIRStmt> body, bool isAsync, bool isVariadic,
              SourceLocation loc);

  const std::string &getName() const;
  const std::vector<std::string> &getTypeParams() const;
  const std::vector<HIRParam> &getParams() const;
  const HIRStmt *getBody() const;
  const HIRType *getReturnType() const;

  bool isAsyncFunc() const;
  bool isVariadicFunc() const;
  bool isExtern() const { return !body; }
  SourceLocation getLoc() const;

  void accept(HIRVisitor &visitor);
  void accept(ConstHIRVisitor &visitor) const;

  void dump(std::ostream &os, int indent = 0) const;

private:
  std::string name;
  std::vector<std::string> typeParams;
  std::vector<HIRParam> params;
  const HIRType *returnType;
  std::unique_ptr<HIRStmt> body;
  bool isAsync;
  bool isVariadic;
  SourceLocation loc;
};

} // namespace hir
} // namespace moksha
