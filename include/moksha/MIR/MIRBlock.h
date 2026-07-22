#pragma once

#include "moksha/MIR/MIRInst.h"
#include "llvm/Support/raw_ostream.h"
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace moksha {
namespace mir {

// Forward declarations
class MIRFunction;
class MIRInst;

/** @brief Constructs a MIRBlock with the given name and parent function. */
class MIRBlock : public MIRValue {
public:
  MIRBlock(std::string name, MIRFunction *parent);

  ~MIRBlock() override = default;

  MIRBlock(MIRBlock &&) = default;
  MIRBlock &operator=(MIRBlock &&) = default;
  MIRBlock(const MIRBlock &) = delete;
  MIRBlock &operator=(const MIRBlock &) = delete;

  MIRFunction *getParent() const { return parent; }
  void setParent(MIRFunction *p) { parent = p; }

  const std::vector<MIRBlock *> &getPredecessors() const {
    return predecessors;
  }
  const std::vector<MIRBlock *> &getSuccessors() const { return successors; }

  std::vector<MIRBlock *> &getPredecessors() { return predecessors; }
  std::vector<MIRBlock *> &getSuccessors() { return successors; }

  void addPredecessor(MIRBlock *pred);
  void addSuccessor(MIRBlock *succ);

  void removePredecessor(MIRBlock *pred);
  void removeSuccessor(MIRBlock *succ);

  const std::vector<std::unique_ptr<MIRInst>> &getInstructions() const {
    return instructions;
  }

  std::vector<std::unique_ptr<MIRInst>> &getInstructionsMut() {
    return instructions;
  }

  std::vector<MIRInst *> getRawInstructions();
  std::vector<const MIRInst *> getRawInstructions() const;

  void addInstruction(std::unique_ptr<MIRInst> inst);
  auto begin() { return instructions.begin(); }
  auto end() { return instructions.end(); }
  auto begin() const { return instructions.cbegin(); }
  auto end() const { return instructions.cend(); }

  void dump(llvm::raw_ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::BasicBlock;
  }

private:
  MIRFunction *parent;
  std::vector<std::unique_ptr<MIRInst>> instructions;
  std::vector<MIRBlock *> predecessors;
  std::vector<MIRBlock *> successors;
};

} // namespace mir
} // namespace moksha
