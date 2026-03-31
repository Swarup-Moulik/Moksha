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

struct HIRGenericParam {
  std::string name;
  bool isShared;
};

class HIRFunction {
public:
  HIRFunction(std::string name, std::vector<HIRGenericParam> typeParams,
              std::vector<HIRParam> params, const HIRType *returnType,
              std::unique_ptr<HIRStmt> body, bool isAsync, bool isVariadic,
              bool isInterrupt, bool isNaked, bool isNoReturn,
              std::string sectionName, SourceLocation loc);

  const std::string &getName() const;
  const std::vector<HIRGenericParam> &getTypeParams() const;
  const std::vector<HIRParam> &getParams() const;
  const HIRStmt *getBody() const;
  const HIRType *getReturnType() const;

  bool isAsyncFunc() const;
  bool isVariadicFunc() const;
  bool isExtern() const { return !body; }
  bool isInterruptFunc() const { return isInterrupt; }
  bool isNakedFunc() const { return isNaked; }
  bool isNoReturnFunc() const { return isNoReturn; }
  bool isNoInlineFunc() const { return isNoInline; }
  void setNoInline(bool v) { isNoInline = v; }

  bool isInlineFunc() const { return isInline; }
  void setInline(bool v) { isInline = v; }

  bool isPureFunc() const { return isPure; }
  void setPure(bool v) { isPure = v; }

  bool isColdFunc() const { return isCold; }
  void setCold(bool v) { isCold = v; }

  bool isUsedFunc() const { return isUsed; }
  void setUsed(bool v) { isUsed = v; }

  bool isStaticFunc() const { return isStatic; }
  void setStatic(bool v) { isStatic = v; }
  bool isWeak() const { return isWeakFlag; }
  void setWeak(bool v) { isWeakFlag = v; }
  const std::string &getABI() const { return abi; }
  void setABI(std::string abiStr) { abi = std::move(abiStr); }
  const std::string &getSection() const { return sectionName; }
  SourceLocation getLoc() const;

  void accept(HIRVisitor &visitor);
  void accept(ConstHIRVisitor &visitor) const;

  bool isVirtualFunc() const { return isVirtual; }
  void setVirtual(bool v) { isVirtual = v; }

  bool isOverrideFunc() const { return isOverride; }
  void setOverride(bool o) { isOverride = o; }

  int getVTableIndex() const { return vtableIndex; }
  void setVTableIndex(int idx) { vtableIndex = idx; }

  void dump(llvm::raw_ostream &os, int indent = 0) const;

private:
  std::string name;
  std::vector<HIRGenericParam> typeParams;
  std::vector<HIRParam> params;
  const HIRType *returnType;
  std::unique_ptr<HIRStmt> body;
  bool isAsync;
  bool isVariadic;
  bool isInterrupt;
  bool isNaked;
  bool isNoReturn;
  bool isWeakFlag = false;
  std::string abi = "C";
  std::string sectionName;
  bool isNoInline = false;
  bool isInline = false;
  bool isPure = false;
  bool isCold = false;
  bool isUsed = false;
  bool isStatic = false;
  bool isVirtual = false;
  bool isOverride = false;
  int vtableIndex = -1;
  SourceLocation loc;
};

class HIRClass {
public:
  HIRClass(std::string name, std::vector<HIRGenericParam> typeParams,
           const HIRType *structType,
           std::vector<std::unique_ptr<HIRFunction>> methods,
           bool isPacked = false, int align = 0, std::string section = "")
      : name(std::move(name)), typeParams(std::move(typeParams)),
        structType(structType), methods(std::move(methods)),
        isPackedFlag(isPacked), alignment(align),
        sectionName(std::move(section)) {}

  HIRClass(std::string name, const HIRType *structType,
           std::vector<std::unique_ptr<HIRFunction>> methods,
           bool isPacked = false, int align = 0, std::string section = "")
      : name(std::move(name)), structType(structType),
        methods(std::move(methods)), isPackedFlag(isPacked), alignment(align),
        sectionName(std::move(section)) {}

  const std::string &getName() const { return name; }
  const std::vector<HIRGenericParam> &getTypeParams() const {
    return typeParams;
  }
  const HIRType *getType() const { return structType; }
  const std::vector<std::unique_ptr<HIRFunction>> &getMethods() const {
    return methods;
  }
  bool isPacked() const { return isPackedFlag; }
  int getAlignment() const { return alignment; }
  const std::string &getSection() const { return sectionName; }
  bool hasVTable() const { return hasVTableFlag; }
  void setHasVTable(bool v) { hasVTableFlag = v; }
  const std::vector<const HIRType *> &getParentTypes() const {
    return parentTypes;
  }
  void setParentTypes(std::vector<const HIRType *> parents) {
    parentTypes = std::move(parents);
  }
  void dump(llvm::raw_ostream &os, int indent = 0) const;

private:
  std::string name;
  std::vector<HIRGenericParam> typeParams;
  const HIRType *structType;
  std::vector<std::unique_ptr<HIRFunction>> methods;
  bool isPackedFlag;
  int alignment;
  std::string sectionName;
  bool hasVTableFlag = false;
  std::vector<const HIRType *> parentTypes;
};

} // namespace hir
} // namespace moksha
