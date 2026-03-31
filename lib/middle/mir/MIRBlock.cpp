#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h" // Required to access parent Function if needed
#include "moksha/MIR/MIRInst.h"     // Required for MIRInst definitions
#include <algorithm>
#include <iostream>

namespace moksha {
namespace mir {

// ============================================================================
// [Constructor]
// ============================================================================

MIRBlock::MIRBlock(std::string name, MIRFunction *parent)
    : MIRValue(ValueKind::BasicBlock, nullptr, std::move(name)),
      parent(parent) {
  // Note: We don't automatically add ourselves to the parent here.
  // The parent (MIRFunction) is responsible for adding/owning the block
  // via MIRFunction::addBlock().
}

// ============================================================================
// [Instruction Management]
// ============================================================================

void MIRBlock::addInstruction(std::unique_ptr<MIRInst> inst) {
  if (!inst)
    return;

  // Set the parent of the instruction to this block
  // This ensures back-linkage is always valid upon insertion.
  inst->setParent(this);

  // Auto-wire the explicit CFG edges for all terminators!
  if (auto *throwInst = llvm::dyn_cast<ThrowInst>(inst.get())) {
    if (MIRBlock *dest = throwInst->getUnwindDest()) {
      this->addSuccessor(dest);
      dest->addPredecessor(this);
    }
  } else if (auto *invokeInst = llvm::dyn_cast<InvokeInst>(inst.get())) {
    if (MIRBlock *dest = invokeInst->getUnwindDest()) {
      this->addSuccessor(dest);
      dest->addPredecessor(this);
    }
  } else if (auto *br = llvm::dyn_cast<BranchInst>(inst.get())) {
    if (MIRBlock *dest = br->getTarget()) {
      this->addSuccessor(dest);
      dest->addPredecessor(this);
    }
  } else if (auto *cbr = llvm::dyn_cast<CondBranchInst>(inst.get())) {
    if (MIRBlock *tBlock = cbr->getTrueBlock()) {
      this->addSuccessor(tBlock);
      tBlock->addPredecessor(this);
    }
    if (MIRBlock *fBlock = cbr->getFalseBlock()) {
      this->addSuccessor(fBlock);
      fBlock->addPredecessor(this);
    }
  } else if (auto *sw = llvm::dyn_cast<SwitchInst>(inst.get())) {
    if (MIRBlock *defBlock = sw->getDefaultBlock()) {
      this->addSuccessor(defBlock);
      defBlock->addPredecessor(this);
    }
    for (auto const &[val, caseBlock] : sw->getCases()) {
      if (caseBlock) {
        this->addSuccessor(caseBlock);
        caseBlock->addPredecessor(this);
      }
    }
  }

  // Take ownership of the instruction
  instructions.push_back(std::move(inst));
}

// [Mutable] Returns mutable raw pointers
std::vector<MIRInst *> MIRBlock::getRawInstructions() {
  std::vector<MIRInst *> raw;
  raw.reserve(instructions.size());
  for (auto &inst : instructions) {
    raw.push_back(inst.get());
  }
  return raw;
}

// [Read-Only] Returns const raw pointers
std::vector<const MIRInst *> MIRBlock::getRawInstructions() const {
  std::vector<const MIRInst *> raw;
  raw.reserve(instructions.size());
  for (const auto &inst : instructions) {
    raw.push_back(inst.get());
  }
  return raw;
}

void MIRBlock::addPredecessor(MIRBlock *pred) {
  if (std::find(predecessors.begin(), predecessors.end(), pred) ==
      predecessors.end()) {
    predecessors.push_back(pred);
  }
}

void MIRBlock::addSuccessor(MIRBlock *succ) {
  if (std::find(successors.begin(), successors.end(), succ) ==
      successors.end()) {
    successors.push_back(succ);
  }
}

void MIRBlock::removePredecessor(MIRBlock *pred) {
  predecessors.erase(
      std::remove(predecessors.begin(), predecessors.end(), pred),
      predecessors.end());
}
void MIRBlock::removeSuccessor(MIRBlock *succ) {
  successors.erase(std::remove(successors.begin(), successors.end(), succ),
                   successors.end());
}

// ============================================================================
// [Debug / Dump]
// ============================================================================

void MIRBlock::dump(llvm::raw_ostream &os) const {
  os << getName() << ":\n";
  if (instructions.empty()) {
    os << "  ; <empty>\n\n";
    return;
  }
  for (const auto &inst : instructions) {
    os << "  ";
    inst->dump(os);
    os << "\n";
  }
}

} // namespace mir
} // namespace moksha
