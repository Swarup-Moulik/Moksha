#include "moksha/MIR/MIRInst.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRArgument.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRGlobal.h"
#include <iomanip>
#include <iostream>
#include <sstream>

namespace moksha {
namespace mir {

// ============================================================================
// [Helpers]
// ============================================================================

static void printType(std::ostream &os, const hir::HIRType *type) {
  if (type) {
    os << type->toString();
  } else {
    os << "void";
  }
}

static void printEscapedString(std::ostream &os, const std::string &s) {
  os << "c\"";
  for (unsigned char c : s) {
    if (c == '\\')
      os << "\\\\";
    else if (c == '"')
      os << "\\22";
    else if (isprint(c))
      os << c;
    else {
      os << "\\";
      os << std::hex << std::setw(2) << std::setfill('0') << (int)c;
      os << std::dec; // restore decimal
    }
  }
  os << "\\00\"";
}

static void printOperand(std::ostream &os, const MIRValue *val) {
  if (!val) {
    os << "null";
    return;
  }

  switch (val->getKind()) {
  case ValueKind::ConstantInt:
  case ValueKind::ConstantFloat:
  case ValueKind::ConstantBool:
  case ValueKind::ConstantString:
  case ValueKind::ConstantNull:
    val->dump(os); // Constants dump their value directly
    break;
  case ValueKind::Global:
    os << "@" << val->getName();
    break;
  case ValueKind::BasicBlock:
    os << "%" << val->getName();
    break;
  default:
    if (val->getName().empty()) {
      os << "%" << (const void *)val;
    } else {
      os << "%" << val->getName();
    }
    break;
  }
}

static std::string getOpcodeName(Opcode op) {
  switch (op) {
  case Opcode::Alloca:
    return "alloca";
  case Opcode::Load:
    return "load";
  case Opcode::Store:
    return "store";
  case Opcode::GetElementPtr:
    return "getelementptr";
  case Opcode::InsertValue:
    return "insertvalue";
  case Opcode::ExtractValue:
    return "extractvalue";
  case Opcode::Add:
    return "add";
  case Opcode::Sub:
    return "sub";
  case Opcode::Mul:
    return "mul";
  case Opcode::Div:
    return "sdiv";
  case Opcode::Mod:
    return "srem";
  case Opcode::FAdd:
    return "fadd";
  case Opcode::FSub:
    return "fsub";
  case Opcode::FMul:
    return "fmul";
  case Opcode::FDiv:
    return "fdiv";
  case Opcode::And:
    return "and";
  case Opcode::Or:
    return "or";
  case Opcode::Xor:
    return "xor";
  case Opcode::Shl:
    return "shl";
  case Opcode::Shr:
    return "ashr";
  case Opcode::ICmp:
    return "icmp";
  case Opcode::FCmp:
    return "fcmp";
  case Opcode::Br:
    return "br";
  case Opcode::CondBr:
    return "br";
  case Opcode::Return:
    return "ret";
  case Opcode::Call:
    return "call";
  case Opcode::Switch:
    return "switch";
  case Opcode::BitCast:
    return "bitcast";
  case Opcode::IntToFloat:
    return "sitofp";
  case Opcode::FloatToInt:
    return "fptosi";
  case Opcode::Retain:
    return "retain";
  case Opcode::Release:
    return "release";
  case Opcode::Phi:
    return "phi";
  default:
    return "unknown";
  }
}

// ============================================================================
// [Global & Argument] (MIRFunction/MIRBlock/MIRModule moved to respective
// files)
// ============================================================================

void MIRGlobal::dump(std::ostream &os) const {
  os << "@" << getName() << " = ";
  if (linkage == Linkage::Internal)
    os << "internal ";

  // [FIX] Use correct member name 'isConstantFlag'
  os << (isConstantFlag ? "constant " : "global ");
  printType(os, getType());
  os << " ";

  // [FIX] Commented out initializer until supported in header
  /*
  if (initializer) {
    initializer->dump(os);
  } else {
    os << "zeroinitializer";
  }
  */
  os << "zeroinitializer";
}

void MIRArgument::dump(std::ostream &os) const {
  printType(os, getType());
  os << " %" << getName();
}

// ============================================================================
// [Constants]
// ============================================================================

void ConstantInt::dump(std::ostream &os) const { os << value; }

void ConstantFloat::dump(std::ostream &os) const {
  os << std::setprecision(15) << value;
}

void ConstantBool::dump(std::ostream &os) const {
  os << (value ? "true" : "false");
}

void ConstantString::dump(std::ostream &os) const {
  printEscapedString(os, value);
}

void ConstantNull::dump(std::ostream &os) const { os << "null"; }

// ============================================================================
// [Terminators]
// ============================================================================

BranchInst::BranchInst(MIRBlock *target, SourceLocation loc)
    : MIRInst(Opcode::Br, nullptr, "", loc), target(target) {}

void BranchInst::dump(std::ostream &os) const {
  os << "br label ";
  printOperand(os, target);
}

CondBranchInst::CondBranchInst(MIRValue *cond, MIRBlock *trueBlock,
                               MIRBlock *falseBlock, SourceLocation loc)
    : MIRInst(Opcode::CondBr, nullptr, "", loc), cond(cond),
      trueBlock(trueBlock), falseBlock(falseBlock) {}

void CondBranchInst::dump(std::ostream &os) const {
  os << "br ";
  printType(os, cond->getType());
  os << " ";
  printOperand(os, cond);
  os << ", label ";
  printOperand(os, trueBlock);
  os << ", label ";
  printOperand(os, falseBlock);
}

SwitchInst::SwitchInst(MIRValue *cond, MIRBlock *defaultBlock,
                       SourceLocation loc)
    : MIRInst(Opcode::Switch, nullptr, "", loc), cond(cond),
      defaultBlock(defaultBlock) {}

void SwitchInst::addCase(MIRValue *value, MIRBlock *target) {
  cases.emplace_back(value, target);
}

void SwitchInst::dump(std::ostream &os) const {
  os << "switch ";
  printType(os, cond->getType());
  os << " ";
  printOperand(os, cond);
  os << ", label ";
  printOperand(os, defaultBlock);
  os << " [\n";

  if (cases.empty()) {
    os << "    ; no cases\n";
  }

  for (const auto &c : cases) {
    os << "    ";
    printType(os, c.first->getType());
    os << " ";
    printOperand(os, c.first);
    os << ", label ";
    printOperand(os, c.second);
    os << "\n";
  }
  os << "  ]";
}

ReturnInst::ReturnInst(MIRValue *val, SourceLocation loc)
    : MIRInst(Opcode::Return, nullptr, "", loc), val(val) {}

void ReturnInst::dump(std::ostream &os) const {
  os << "ret ";
  if (val) {
    printType(os, val->getType());
    os << " ";
    printOperand(os, val);
  } else {
    os << "void";
  }
}

// ============================================================================
// [Memory Ops]
// ============================================================================

AllocaInst::AllocaInst(const hir::HIRType *allocType, std::string name,
                       SourceLocation loc, unsigned align)
    : MIRInst(Opcode::Alloca, allocType, std::move(name), loc),
      allocatedType(allocType), alignment(align) {}

void AllocaInst::dump(std::ostream &os) const {
  os << "%" << getName() << " = alloca ";
  printType(os, allocatedType);

  if (alignment > 0) {
    os << ", align " << alignment;
  }
}

LoadInst::LoadInst(MIRValue *ptr, std::string name, SourceLocation loc,
                   unsigned align)
    : MIRInst(Opcode::Load, nullptr, std::move(name), loc), ptr(ptr),
      alignment(align) {}

void LoadInst::dump(std::ostream &os) const {
  os << "%" << getName() << " = load ";
  printType(os, getType());
  os << ", ";
  printType(os, ptr->getType());
  os << " ";
  printOperand(os, ptr);

  if (alignment > 0) {
    os << ", align " << alignment;
  }
}

StoreInst::StoreInst(MIRValue *val, MIRValue *ptr, SourceLocation loc,
                     unsigned align)
    : MIRInst(Opcode::Store, nullptr, "", loc), val(val), ptr(ptr),
      alignment(align) {}

void StoreInst::dump(std::ostream &os) const {
  os << "store ";
  printType(os, val->getType());
  os << " ";
  printOperand(os, val);
  os << ", ";
  printType(os, ptr->getType());
  os << " ";
  printOperand(os, ptr);

  if (alignment > 0) {
    os << ", align " << alignment;
  }
}

GetElementPtrInst::GetElementPtrInst(MIRValue *ptr,
                                     std::vector<MIRValue *> &&indices,
                                     const hir::HIRType *resultType,
                                     std::string name, SourceLocation loc)
    : MIRInst(Opcode::GetElementPtr, resultType, std::move(name), loc),
      ptr(ptr), indices(std::move(indices)) {}

void GetElementPtrInst::dump(std::ostream &os) const {
  os << "%" << getName() << " = getelementptr ";
  printType(os, getType());
  os << ", ";
  printType(os, ptr->getType());
  os << " ";
  printOperand(os, ptr);
  for (auto *idx : indices) {
    os << ", ";
    printType(os, idx->getType());
    os << " ";
    printOperand(os, idx);
  }
}

// ============================================================================
// [Aggregates]
// ============================================================================

InsertValueInst::InsertValueInst(MIRValue *agg, MIRValue *val, uint32_t index,
                                 std::string name, SourceLocation loc)
    : MIRInst(Opcode::InsertValue, agg->getType(), std::move(name), loc),
      agg(agg), val(val), index(index) {}

void InsertValueInst::dump(std::ostream &os) const {
  os << "%" << getName() << " = insertvalue ";
  printType(os, agg->getType());
  os << " ";
  printOperand(os, agg);
  os << ", ";
  printType(os, val->getType());
  os << " ";
  printOperand(os, val);
  os << ", " << index;
}

ExtractValueInst::ExtractValueInst(MIRValue *agg, uint32_t index,
                                   const hir::HIRType *resType,
                                   std::string name, SourceLocation loc)
    : MIRInst(Opcode::ExtractValue, resType, std::move(name), loc), agg(agg),
      index(index) {}

void ExtractValueInst::dump(std::ostream &os) const {
  os << "%" << getName() << " = extractvalue ";
  printType(os, agg->getType());
  os << " ";
  printOperand(os, agg);
  os << ", " << index;
}

// ============================================================================
// [Others]
// ============================================================================

ARCInst::ARCInst(Opcode op, MIRValue *obj, SourceLocation loc)
    : MIRInst(op, nullptr, "", loc), obj(obj) {}

void ARCInst::dump(std::ostream &os) const {
  os << getOpcodeName(opcode) << " ";
  printType(os, obj->getType());
  os << " ";
  printOperand(os, obj);
}

BinaryInst::BinaryInst(Opcode op, MIRValue *lhs, MIRValue *rhs,
                       std::string name, SourceLocation loc)
    : MIRInst(op, lhs->getType(), std::move(name), loc), lhs(lhs), rhs(rhs) {}

void BinaryInst::dump(std::ostream &os) const {
  os << "%" << getName() << " = " << getOpcodeName(opcode) << " ";
  printType(os, getType());
  os << " ";
  printOperand(os, lhs);
  os << ", ";
  printOperand(os, rhs);
}

CastInst::CastInst(Opcode op, MIRValue *value, const hir::HIRType *destType,
                   std::string name, SourceLocation loc)
    : MIRInst(op, destType, std::move(name), loc), value(value) {}

void CastInst::dump(std::ostream &os) const {
  os << "%" << getName() << " = " << getOpcodeName(opcode) << " ";
  printType(os, value->getType());
  os << " ";
  printOperand(os, value);
  os << " to ";
  printType(os, getType());
}

CompareInst::CompareInst(Predicate pred, MIRValue *lhs, MIRValue *rhs,
                         std::string name, SourceLocation loc)
    : MIRInst(Opcode::ICmp, nullptr, std::move(name), loc), pred(pred),
      lhs(lhs), rhs(rhs) {}

static const char *getPredicateString(CompareInst::Predicate p) {
  switch (p) {
  case CompareInst::Predicate::EQ:
    return "eq";
  case CompareInst::Predicate::NE:
    return "ne";
  case CompareInst::Predicate::LT:
    return "slt";
  case CompareInst::Predicate::LE:
    return "sle";
  case CompareInst::Predicate::GT:
    return "sgt";
  case CompareInst::Predicate::GE:
    return "sge";
  case CompareInst::Predicate::ULT:
    return "ult";
  case CompareInst::Predicate::ULE:
    return "ule";
  case CompareInst::Predicate::UGT:
    return "ugt";
  case CompareInst::Predicate::UGE:
    return "uge";
  default:
    return "unknown";
  }
}

void CompareInst::dump(std::ostream &os) const {
  os << "%" << getName() << " = icmp " << getPredicateString(pred) << " ";
  printType(os, lhs->getType());
  os << " ";
  printOperand(os, lhs);
  os << ", ";
  printOperand(os, rhs);
}

PhiInst::PhiInst(const hir::HIRType *type, std::string name, SourceLocation loc)
    : MIRInst(Opcode::Phi, type, std::move(name), loc) {}

void PhiInst::addIncoming(MIRValue *val, MIRBlock *block) {
  incoming.emplace_back(val, block);
}

void PhiInst::dump(std::ostream &os) const {
  os << "%" << getName() << " = phi ";
  printType(os, getType());
  os << " ";

  bool multiline = incoming.size() > 3;

  for (size_t i = 0; i < incoming.size(); ++i) {
    if (i > 0) {
      os << ", ";
      if (multiline)
        os << "\n    ";
    }
    os << "[ ";
    printOperand(os, incoming[i].first);
    os << ", ";
    printOperand(os, incoming[i].second);
    os << " ]";
  }
}

CallInst::CallInst(MIRValue *callee, std::vector<MIRValue *> &&args,
                   const hir::HIRType *retType, std::string name, bool isVarArg,
                   SourceLocation loc)
    : MIRInst(Opcode::Call, retType, std::move(name), loc), callee(callee),
      args(std::move(args)), isVarArg(isVarArg) {}

void CallInst::dump(std::ostream &os) const {
  if (getType()) {
    os << "%" << getName() << " = ";
  }
  os << "call ";
  printType(os, getType());
  os << " ";
  printOperand(os, callee);
  os << "(";
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0)
      os << ", ";
    printType(os, args[i]->getType());
    os << " ";
    printOperand(os, args[i]);
  }
  if (isVarArg)
    os << ", ...";
  os << ")";
}

} // namespace mir
} // namespace moksha
