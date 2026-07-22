#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRArgument.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRInst.h"
#include <iostream>

namespace moksha {
namespace mir {

// Helpers
static void printType(llvm::raw_ostream &os, const hir::HIRType *type) {
  if (type) {
    os << type->toString();
  } else {
    os << "void";
  }
}

// Constructor & Destructor
MIRFunction::MIRFunction(const hir::HIRType *returnType, std::string name,
                         Linkage linkage)
    : MIRValue(ValueKind::Function, returnType, std::move(name)),
      linkage(linkage) {}

MIRFunction::~MIRFunction() = default;

// Attributes
bool MIRFunction::isDeclaration() const { return blocks.empty(); }

// Block Management
void MIRFunction::addBlock(std::unique_ptr<MIRBlock> block) {
  if (!block)
    return;

  block->setParent(this);
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

// Argument Management
void MIRFunction::addArgument(std::unique_ptr<MIRArgument> arg) {
  if (!arg)
    return;
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

// Debug / Dump
void MIRFunction::dump(llvm::raw_ostream &os) const {
  if (isDeclaration()) {
    os << "declare ";
  } else {
    os << "define ";
  }

  if (linkage == Linkage::Internal)
    os << "internal ";
  else if (linkage == Linkage::Weak)
    os << "weak ";

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

  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0)
      os << ", ";
    args[i]->dump(os);
  }

  if (isVariadicFlag) {
    if (!args.empty())
      os << ", ";
    os << "...";
  }

  os << ")";

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

// SSA Name Resolution
std::string MIRFunction::getUniqueName(const std::string &baseName) {
  if (baseName.empty()) {
    return "";
  }

  unsigned count = nameCounters[baseName]++;

  if (count == 0) {
    return baseName;
  }

  return baseName + "." + std::to_string(count);
}

} // namespace mir
} // namespace moksha
