#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h" // Required to access parent Function if needed
#include "moksha/MIR/MIRInst.h" // Required for MIRInst definitions
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

// ============================================================================
// [Debug / Dump]
// ============================================================================

void MIRBlock::dump(std::ostream &os) const {
  // Print the block label (e.g., "entry:")
  os << getName() << ":\n";

  // Explicit check for empty blocks to aid debugging
  if (instructions.empty()) {
    // [FIX] Added extra newline for better visual separation in the dump
    os << "  ; <empty>\n\n";
    return;
  }

  // Print all instructions indented
  for (const auto &inst : instructions) {
    os << "  ";
    inst->dump(os);
    os << "\n";
  }
}

} // namespace mir
} // namespace moksha
