#pragma once

#include "moksha/HIR/HIRParam.h"
#include "moksha/Support/SourceLocation.h"
#include "llvm/Support/raw_ostream.h"
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
              bool isInterrupt, bool isNaked, bool isNoReturn,
              std::string sectionName, SourceLocation loc);

  const std::string &getName() const;
  const std::vector<std::string> &getTypeParams() const;
  const std::vector<HIRParam> &getParams() const;
  const HIRStmt *getBody() const;
  const HIRType *getReturnType() const;

  bool isAsyncFunc() const;
  bool isVariadicFunc() const;
  bool isExtern() const { return !body; }
  bool isInterruptFunc() const { return isInterrupt; }
  bool isNakedFunc() const { return isNaked; }
  bool isNoReturnFunc() const { return isNoReturn; }
  const std::string &getSection() const { return sectionName; }
  SourceLocation getLoc() const;

  void accept(HIRVisitor &visitor);
  void accept(ConstHIRVisitor &visitor) const;

  void dump(llvm::raw_ostream &os, int indent = 0) const;

private:
  std::string name;
  std::vector<std::string> typeParams;
  std::vector<HIRParam> params;
  const HIRType *returnType;
  std::unique_ptr<HIRStmt> body;
  bool isAsync;
  bool isVariadic;
  bool isInterrupt;
  bool isNaked;
  bool isNoReturn;
  std::string sectionName;
  SourceLocation loc;
};

class HIRClass {
public:
  HIRClass(std::string name, const HIRType *structType,
           std::vector<std::unique_ptr<HIRFunction>> methods)
      : name(std::move(name)), structType(structType),
        methods(std::move(methods)) {}

  const std::string &getName() const { return name; }
  const HIRType *getType() const { return structType; }
  const std::vector<std::unique_ptr<HIRFunction>> &getMethods() const {
    return methods;
  }

  void dump(llvm::raw_ostream &os, int indent = 0) const;

private:
  std::string name;
  const HIRType *structType;
  std::vector<std::unique_ptr<HIRFunction>> methods;
};

} // namespace hir
} // namespace moksha
