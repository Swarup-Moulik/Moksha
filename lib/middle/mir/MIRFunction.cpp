#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRArgument.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRInst.h"
#include <iostream>

namespace moksha {
namespace mir {

// ============================================================================
// [Helpers]
// ============================================================================

static void printType(std::ostream &os, const hir::HIRType *type) {
  if (type) {
    // Assuming HIRType has a toString method.
    os << type->toString();
  } else {
    os << "void";
  }
}

// ============================================================================
// [Constructor & Destructor]
// ============================================================================

MIRFunction::MIRFunction(const hir::HIRType *returnType, std::string name,
                         Linkage linkage)
    : MIRValue(ValueKind::Function, returnType, std::move(name)),
      linkage(linkage) {}

// Defined here because MIRBlock and MIRArgument are fully defined in this file,
// allowing std::unique_ptr to correctly generate a deleter.
MIRFunction::~MIRFunction() = default;

// ============================================================================
// [Attributes]
// ============================================================================

bool MIRFunction::isDeclaration() const { return blocks.empty(); }

// ============================================================================
// [Block Management]
// ============================================================================

void MIRFunction::addBlock(std::unique_ptr<MIRBlock> block) {
  if (!block)
    return;

  // Enforce back-linkage to this function
  block->setParent(this);

  // Take ownership (Move the unique_ptr into the vector)
  blocks.push_back(std::move(block));
}

std::vector<MIRBlock *> MIRFunction::getRawBlocks() const {
  std::vector<MIRBlock *> raw;
  raw.reserve(blocks.size());
  for (const auto &b : blocks) {
    raw.push_back(b.get());
  }
  return raw;
}

// ============================================================================
// [Argument Management]
// ============================================================================

void MIRFunction::addArgument(std::unique_ptr<MIRArgument> arg) {
  if (!arg)
    return;

  // Note: MIRArgument sets its parent function in its constructor.
  // We assume the caller created the argument with the correct parent.

  // Take ownership (Move the unique_ptr into the vector)
  args.push_back(std::move(arg));
}

std::vector<MIRArgument *> MIRFunction::getRawArguments() const {
  std::vector<MIRArgument *> raw;
  raw.reserve(args.size());
  for (const auto &a : args) {
    raw.push_back(a.get());
  }
  return raw;
}

// ============================================================================
// [Debug / Dump]
// ============================================================================

void MIRFunction::dump(std::ostream &os) const {
  os << "define ";
  if (linkage == Linkage::Internal)
    os << "internal ";

  printType(os, getType());
  os << " @" << getName() << "(";

  // Dump Arguments
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0)
      os << ", ";
    args[i]->dump(os);
  }
  os << ")";

  if (isDeclaration()) {
    os << ";\n";
  } else {
    os << " {\n";
    for (const auto &bb : blocks) {
      bb->dump(os);
      // Add a newline between blocks for better readability
      os << "\n";
    }
    os << "}\n";
  }
}

} // namespace mir
} // namespace moksha
