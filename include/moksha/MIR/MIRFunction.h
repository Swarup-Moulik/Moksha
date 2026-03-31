#pragma once

#include "moksha/MIR/MIRInst.h"
#include "llvm/Support/raw_ostream.h"
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace moksha {
namespace mir {

// Forward declarations
class MIRBlock;
class MIRArgument;

class MIRFunction : public MIRValue {
public:
  // ------------------------------------------------------------------------
  // Constructors / Destructors
  // ------------------------------------------------------------------------

  MIRFunction(const hir::HIRType *returnType, std::string name,
              Linkage linkage);

  ~MIRFunction() override;

  // Move Semantics
  MIRFunction(MIRFunction &&) = default;
  MIRFunction &operator=(MIRFunction &&) = default;

  // Copy Semantics
  MIRFunction(const MIRFunction &) = delete;
  MIRFunction &operator=(const MIRFunction &) = delete;

  // ------------------------------------------------------------------------
  // Attributes
  // ------------------------------------------------------------------------

  Linkage getLinkage() const { return linkage; }

  bool isDeclaration() const;

  // ------------------------------------------------------------------------
  // Block Management
  // ------------------------------------------------------------------------

  // [ADDED] Accessor for the entry block (needed by MIRDominance)
  MIRBlock *getEntryBlock() const {
    return blocks.empty() ? nullptr : blocks.front().get();
  }

  const std::vector<std::unique_ptr<MIRBlock>> &getBlocks() const {
    return blocks;
  }

  std::vector<std::unique_ptr<MIRBlock>> &getBlocksMut() { return blocks; }

  std::vector<MIRBlock *> getRawBlocks() const;

  void addBlock(std::unique_ptr<MIRBlock> block);

  // ------------------------------------------------------------------------
  // Argument Management
  // ------------------------------------------------------------------------

  const std::vector<std::unique_ptr<MIRArgument>> &getArguments() const {
    return args;
  }

  std::vector<MIRArgument *> getRawArguments() const;

  void numberUnnamedValues();

  void addArgument(std::unique_ptr<MIRArgument> arg);

  // ------------------------------------------------------------------------
  // Iterators
  // ------------------------------------------------------------------------

  auto begin() { return blocks.begin(); }
  auto end() { return blocks.end(); }
  auto begin() const { return blocks.cbegin(); }
  auto end() const { return blocks.cend(); }

  auto arg_begin() { return args.begin(); }
  auto arg_end() { return args.end(); }
  auto arg_begin() const { return args.cbegin(); }
  auto arg_end() const { return args.cend(); }

  // ------------------------------------------------------------------------
  // Debugging / RTTI
  // ------------------------------------------------------------------------

  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::Function;
  }

  // ------------------------------------------------------------------------
  // System ABI & Attributes
  // ------------------------------------------------------------------------
  bool isVariadic() const { return isVariadicFlag; }
  void setVariadic(bool v) { isVariadicFlag = v; }

  bool isInterrupt() const { return isInterruptFlag; }
  void setInterrupt(bool v) { isInterruptFlag = v; }

  bool isNaked() const { return isNakedFlag; }
  void setNaked(bool v) { isNakedFlag = v; }

  bool isNoReturn() const { return isNoReturnFlag; }
  void setNoReturn(bool v) { isNoReturnFlag = v; }

  bool isNoInline() const { return isNoInlineFlag; }
  void setNoInline(bool v) { isNoInlineFlag = v; }

  bool isInline() const { return isInlineFlag; }
  void setInline(bool v) { isInlineFlag = v; }

  bool isPure() const { return isPureFlag; }
  void setPure(bool v) { isPureFlag = v; }

  bool isCold() const { return isColdFlag; }
  void setCold(bool v) { isColdFlag = v; }

  bool isUsed() const { return isUsedFlag; }
  void setUsed(bool v) { isUsedFlag = v; }

  const std::string &getSection() const { return sectionName; }
  void setSection(std::string s) { sectionName = std::move(s); }

  CallingConv getCallingConv() const { return callingConv; }
  void setCallingConv(CallingConv cc) { callingConv = cc; }

  std::string getUniqueName(const std::string &baseName);

private:
  Linkage linkage;
  CallingConv callingConv = CallingConv::C;
  std::vector<std::unique_ptr<MIRArgument>> args;
  std::vector<std::unique_ptr<MIRBlock>> blocks;
  bool isVariadicFlag = false;
  bool isInterruptFlag = false;
  bool isNakedFlag = false;
  bool isNoReturnFlag = false;
  std::string sectionName = "";
  bool isNoInlineFlag = false;
  bool isInlineFlag = false;
  bool isPureFlag = false;
  bool isColdFlag = false;
  bool isUsedFlag = false;
  std::unordered_map<std::string, unsigned> nameCounters;
};

} // namespace mir
} // namespace moksha
