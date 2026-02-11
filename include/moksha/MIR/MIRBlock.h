#pragma once

#include "moksha/MIR/MIRInst.h" // Required for MIRValue inheritance and Linkage enum
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace moksha {
namespace mir {

// Forward declarations
class MIRFunction;
class MIRInst;

class MIRBlock : public MIRValue {
public:
  // ------------------------------------------------------------------------
  // Constructors / Destructors
  // ------------------------------------------------------------------------

  MIRBlock(std::string name, MIRFunction *parent);

  ~MIRBlock() override = default;

  // Move Semantics
  MIRBlock(MIRBlock &&) = default;
  MIRBlock &operator=(MIRBlock &&) = default;

  // Copy Semantics
  MIRBlock(const MIRBlock &) = delete;
  MIRBlock &operator=(const MIRBlock &) = delete;

  // ------------------------------------------------------------------------
  // Parent Linkage
  // ------------------------------------------------------------------------

  MIRFunction *getParent() const { return parent; }
  void setParent(MIRFunction *p) { parent = p; }

  // ------------------------------------------------------------------------
  // CFG / Predecessors
  // ------------------------------------------------------------------------

  // [ADDED] Accessor for predecessors (needed by MIRDominance)
  const std::vector<MIRBlock *> &getPredecessors() const {
    return predecessors;
  }

  // [ADDED] Helper to mutable predecessors if needed
  std::vector<MIRBlock *> &getPredecessors() { return predecessors; }

  // [ADDED] Helper to add a predecessor edge
  void addPredecessor(MIRBlock *pred) { predecessors.push_back(pred); }

  // ------------------------------------------------------------------------
  // Instruction Management
  // ------------------------------------------------------------------------

  const std::vector<std::unique_ptr<MIRInst>> &getInstructions() const {
    return instructions;
  }

  std::vector<MIRInst *> getRawInstructions();
  std::vector<const MIRInst *> getRawInstructions() const;

  void addInstruction(std::unique_ptr<MIRInst> inst);

  // ------------------------------------------------------------------------
  // Iterators
  // ------------------------------------------------------------------------

  auto begin() { return instructions.begin(); }
  auto end() { return instructions.end(); }
  auto begin() const { return instructions.cbegin(); }
  auto end() const { return instructions.cend(); }

  // ------------------------------------------------------------------------
  // Debugging / RTTI
  // ------------------------------------------------------------------------

  void dump(std::ostream &os) const override;

  static bool classof(const MIRValue *v) {
    return v->getKind() == ValueKind::BasicBlock;
  }

private:
  MIRFunction *parent;
  std::vector<std::unique_ptr<MIRInst>> instructions;

  // [ADDED] Storage for Control Flow Graph edges
  std::vector<MIRBlock *> predecessors;
};

} // namespace mir
} // namespace moksha
