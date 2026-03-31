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

static void printType(llvm::raw_ostream &os, const hir::HIRType *type) {
  if (type) {
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

void MIRFunction::numberUnnamedValues() {
  unsigned counter = 0;

  // 1. Number unnamed arguments
  for (auto &arg : args) {
    if (arg->getName().empty()) {
      arg->setName(std::to_string(counter++));
    }
  }

  // 2. Number unnamed blocks and instructions
  for (auto &block : blocks) {
    // Number the block itself (e.g., if it's not "entry" or "if.then")
    if (block->getName().empty()) {
      block->setName(std::to_string(counter++));
    }

    for (auto &inst : block->getInstructions()) {
      bool producesValue = inst->getType() != nullptr &&
                           inst->getType()->getKind() != hir::TypeKind::Void;

      if (producesValue) {
        if (inst->getName().empty()) {
          inst->setName(std::to_string(counter++));
        } else {
          inst->setName(getUniqueName(inst->getName()));
        }
      }
    }
  }
}

// ============================================================================
// [Debug / Dump]
// ============================================================================

void MIRFunction::dump(llvm::raw_ostream &os) const {
  // Check if it's just a prototype
  if (isDeclaration()) {
    os << "declare ";
  } else {
    os << "define ";
  }
  // [FIX] Handle Linkage
  if (linkage == Linkage::Internal)
    os << "internal ";
  else if (linkage == Linkage::Weak)
    os << "weak ";

  // [FIX] Handle Calling Convention
  switch (callingConv) {
  case CallingConv::StdCall:
    os << "x86_stdcallcc ";
    break;
  case CallingConv::FastCall:
    os << "x86_fastcallcc ";
    break;
  case CallingConv::Interrupt:
    os << "x86_intrcc ";
    break;
  default:
    break;
  }
  printType(os, getType());
  os << " @\"" << getName() << "\"(";

  // Dump Arguments
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0)
      os << ", ";
    args[i]->dump(os);
  }

  // [FIX 1] Append the variadic ellipsis to the parameters
  if (isVariadicFlag) {
    if (!args.empty())
      os << ", ";
    os << "...";
  }

  os << ")";

  // [FIX 2] Print the low-level system attributes
  if (isInterruptFlag)
    os << " interrupt";
  if (isNakedFlag)
    os << " naked";
  if (isNoReturnFlag)
    os << " noreturn";
  if (isNoInlineFlag)
    os << " noinline";
  if (isInlineFlag)
    os << " inline";
  if (isPureFlag)
    os << " pure";
  if (isColdFlag)
    os << " cold";
  if (isUsedFlag)
    os << " used";
  if (!sectionName.empty())
    os << " section(\"" << sectionName << "\")";

  if (isDeclaration()) {
    os << ";\n";
    return;
  }

  os << " {\n";
  for (const auto &block : blocks) {
    block->dump(os);
  }
  os << "}\n\n";
}

// ============================================================================
// [SSA Name Resolution]
// ============================================================================

std::string MIRFunction::getUniqueName(const std::string &baseName) {
  if (baseName.empty()) {
    return ""; // Anonymous values don't need unique names
  }

  // Increment the counter for this specific base name
  unsigned count = nameCounters[baseName]++;

  if (count == 0) {
    return baseName; // First use gets the clean, unnumbered name
  }

  // Subsequent uses get a dot-number suffix (e.g., closure.val.1)
  return baseName + "." + std::to_string(count);
}

} // namespace mir
} // namespace moksha
