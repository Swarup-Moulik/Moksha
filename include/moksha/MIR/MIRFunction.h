#pragma once

#include "moksha/MIR/MIRInst.h" // Required for MIRValue inheritance and Linkage enum
#include <iosfwd>
#include <memory>
#include <string>
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

  std::vector<MIRBlock *> getRawBlocks() const;

  void addBlock(std::unique_ptr<MIRBlock> block);

  // ------------------------------------------------------------------------
  // Argument Management
  // ------------------------------------------------------------------------

  const std::vector<std::unique_ptr<MIRArgument>> &getArguments() const {
    return args;
  }

  std::vector<MIRArgument *> getRawArguments() const;

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

  void dump(std::ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::Function;
  }

private:
  Linkage linkage;
  std::vector<std::unique_ptr<MIRArgument>> args;
  std::vector<std::unique_ptr<MIRBlock>> blocks;
};

} // namespace mir
} // namespace moksha
