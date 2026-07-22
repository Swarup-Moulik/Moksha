#include "moksha/MIR/MIRInst.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRArgument.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRGlobal.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace moksha {
namespace mir {

// Helpers
static std::string getBorrowString(mir::BorrowKind bk) {
  switch (bk) {
  case mir::BorrowKind::Mut:
    return " [mut]";
  case mir::BorrowKind::View:
    return " [view]";
  case mir::BorrowKind::Lock:
    return " [lock]";
  default:
    return "";
  }
}

static void printType(llvm::raw_ostream &os, const hir::HIRType *type) {
  if (type)
    os << type->toString();
  else
    os << "void";
}

static void printEscapedString(llvm::raw_ostream &os, const std::string &s) {
  os << "c\"";
  for (unsigned char c : s) {
    if (c == '\\')
      os << "\\\\";
    else if (c == '"')
      os << "\\22";
    else if (isprint(c))
      os << (char)c;
    else {
      os << "\\" << llvm::format_hex_no_prefix(c, 2);
    }
  }
  os << "\\00\"";
}

static void printOperand(llvm::raw_ostream &os, const MIRValue *val) {
  if (!val) {
    os << "null";
    return;
  }

  if (auto *arr = llvm::dyn_cast_or_null<ConstantArray>(val)) {
    arr->dump(os);
    return;
  }

  if (auto *mapConst = llvm::dyn_cast_or_null<ConstantMap>(val)) {
    mapConst->dump(os);
    return;
  }

  switch (val->getKind()) {
  case ValueKind::ConstantInt:
  case ValueKind::ConstantFloat:
  case ValueKind::ConstantBool:
  case ValueKind::ConstantString:
  case ValueKind::ConstantNull:
  case ValueKind::ConstantDecimal:
  case ValueKind::ConstantStruct:
  case ValueKind::ConstantUnion:
  case ValueKind::ConstantBitCast:
  case ValueKind::ConstantSlice:
  case ValueKind::ConstantUpcast:
  case ValueKind::ConstantAnyCast:
  case ValueKind::ConstantArrayToSlice:
  case ValueKind::ConstantSliceToArray:
    val->dump(os);
    break;
  case ValueKind::Function:
  case ValueKind::Global:
    os << "@\"" << val->getName() << "\"";
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
    return "shr";
  case Opcode::Pow:
    return "pow";
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
  case Opcode::ZExt:
    return "zext";
  case Opcode::SExt:
    return "sext";
  case Opcode::Trunc:
    return "trunc";
  case Opcode::PtrToInt:
    return "ptrtoint";
  case Opcode::IntToPtr:
    return "inttoptr";
  case Opcode::AnyCast:
    return "anycast";
  case Opcode::Upcast:
    return "upcast";
  case Opcode::ArrayToSlice:
    return "array_to_slice";
  case Opcode::SliceToArray:
    return "slice_to_array";
  case Opcode::Retain:
    return "retain";
  case Opcode::Release:
    return "release";
  case Opcode::Phi:
    return "phi";
  case Opcode::StoreWeak:
    return "store_weak";
  case Opcode::LoadWeak:
    return "load_weak";
  case Opcode::Unreachable:
    return "unreachable";
  case Opcode::Invoke:
    return "invoke";
  case Opcode::LandingPad:
    return "landingpad";
  case Opcode::Resume:
    return "resume";
  case Opcode::Throw:
    return "throw";
  case Opcode::InlineAsm:
    return "inlineasm";
  case Opcode::MakeClosure:
    return "make_closure";
  case Opcode::Spawn:
    return "spawn";
  case Opcode::Await:
    return "await";
  default:
    return "unknown";
  }
}

// Global & Argument
void MIRGlobal::dump(llvm::raw_ostream &os) const {
  os << "@\"" << getName() << "\" = ";
  if (getLinkage() == Linkage::Internal)
    os << "internal ";
  else if (getLinkage() == Linkage::Weak)
    os << "weak ";
  else if (getLinkage() == Linkage::LinkOnce)
    os << "linkonce ";
  else if (isExtern())
    os << "external ";
  if (isUsed())
    os << "used ";
  if (isThreadLocal())
    os << "thread_local ";
  if (isVolatile())
    os << "volatile ";
  if (isConstant())
    os << "constant ";
  else
    os << "global ";

  if (getType()) {
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(getType())) {
      os << ptrTy->getPointee()->toString() << " ";
    } else {
      os << getType()->toString() << " ";
    }
  }

  if (getInitializer()) {
    getInitializer()->dump(os);
  } else if (!isExtern()) {
    os << "zeroinitializer";
  }

  if (getAlignment() > 0) {
    os << ", align " << getAlignment();
  }

  if (!getSection().empty()) {
    os << ", section \"" << getSection() << "\"";
  }
}

void MIRArgument::dump(llvm::raw_ostream &os) const {
  printType(os, getType());
  if (!getName().empty()) {
    os << " %" << getName();
  }
}

// Constants
//
void ConstantInt::dump(llvm::raw_ostream &os) const { os << value; }

void ConstantFloat::dump(llvm::raw_ostream &os) const {
  os << llvm::format("%.15g", value);
}

void ConstantBool::dump(llvm::raw_ostream &os) const {
  os << (value ? "true" : "false");
}

void ConstantString::dump(llvm::raw_ostream &os) const {
  printEscapedString(os, value);
}

void ConstantNull::dump(llvm::raw_ostream &os) const { os << "null"; }

void ConstantUndef::dump(llvm::raw_ostream &os) const { os << "undef"; }

void ConstantArray::dump(llvm::raw_ostream &os) const {
  os << "[";
  for (size_t i = 0; i < elements.size(); ++i) {
    if (i > 0)
      os << ", ";
    if (elements[i]) {
      printType(os, elements[i]->getType());
      os << " ";
      printOperand(os, elements[i]);
    } else {
      os << "null";
    }
  }
  os << "]";
}

void ConstantSlice::dump(llvm::raw_ostream &os) const {
  os << "[";
  for (size_t i = 0; i < elements.size(); ++i) {
    if (i > 0)
      os << ", ";
    if (elements[i]) {
      printType(os, elements[i]->getType());
      os << " ";
      printOperand(os, elements[i]);
    } else {
      os << "null";
    }
  }
  os << "]";
}

void ConstantMap::dump(llvm::raw_ostream &os) const {
  os << "{";
  for (size_t i = 0; i < entries.size(); ++i) {
    if (i > 0)
      os << ", ";
    printOperand(os, entries[i].first);
    os << ": ";
    printOperand(os, entries[i].second);
  }
  os << "}";
}

void ConstantStruct::dump(llvm::raw_ostream &os) const {
  os << "{ ";
  for (size_t i = 0; i < fields.size(); ++i) {
    if (i > 0)
      os << ", ";

    if (fields[i]) {
      printType(os, fields[i]->getType());
      os << " ";
      printOperand(os, fields[i]);
    } else {
      os << "null";
    }
  }
  os << " }";
}

void ConstantUnion::dump(llvm::raw_ostream &os) const {
  os << "union " << getType()->toString() << " { ";
  if (activeField) {
    activeField->dump(os);
  } else {
    os << "zeroinitializer";
  }
  os << " }";
}

void ConstantBitCast::dump(llvm::raw_ostream &os) const {
  os << "bitcast (";
  printType(os, value->getType());
  os << " ";
  printOperand(os, value);
  os << " to ";
  printType(os, getType());
  os << ")";
}

void ConstantUpcast::dump(llvm::raw_ostream &os) const {
  os << "upcast (";
  printType(os, value->getType());
  os << " ";
  printOperand(os, value);
  os << " to ";
  printType(os, getType());
  os << ")";
}

void ConstantAnyCast::dump(llvm::raw_ostream &os) const {
  os << "anycast (";
  printType(os, value->getType());
  os << " ";
  printOperand(os, value);
  os << " to ";
  printType(os, getType());
  os << ")";
}

void ConstantArrayToSlice::dump(llvm::raw_ostream &os) const {
  os << "array_to_slice (";
  printType(os, value->getType());
  os << " ";
  printOperand(os, value);
  os << " to ";
  printType(os, getType());
  os << ")";
}

void ConstantSliceToArray::dump(llvm::raw_ostream &os) const {
  os << "slice_to_array (";
  printType(os, value->getType());
  os << " ";
  printOperand(os, value);
  os << " to ";
  printType(os, getType());
  os << ")";
}

// Terminators

BranchInst::BranchInst(MIRBlock *target, SourceLocation loc)
    : MIRInst(Opcode::Br, nullptr, "", loc), target(target) {}

void BranchInst::dump(llvm::raw_ostream &os) const {
  os << "br label ";
  printOperand(os, target);
}

void BranchInst::replaceOperand(MIRValue *oldVal, MIRValue *newVal) {
  if (target == oldVal) {
    target = static_cast<MIRBlock *>(newVal);
  }
}

CondBranchInst::CondBranchInst(MIRValue *cond, MIRBlock *trueBlock,
                               MIRBlock *falseBlock, SourceLocation loc)
    : MIRInst(Opcode::CondBr, nullptr, "", loc), cond(cond),
      trueBlock(trueBlock), falseBlock(falseBlock) {}

void CondBranchInst::dump(llvm::raw_ostream &os) const {
  os << "br ";
  printType(os, cond->getType());
  os << " ";
  printOperand(os, cond);
  os << ", label ";
  printOperand(os, trueBlock);
  os << ", label ";
  printOperand(os, falseBlock);
}

void CondBranchInst::replaceOperand(MIRValue *oldVal, MIRValue *newVal) {
  if (cond == oldVal)
    cond = newVal;
  if (trueBlock == oldVal)
    trueBlock = static_cast<MIRBlock *>(newVal);
  if (falseBlock == oldVal)
    falseBlock = static_cast<MIRBlock *>(newVal);
}

SwitchInst::SwitchInst(MIRValue *cond, MIRBlock *defaultBlock,
                       SourceLocation loc)
    : MIRInst(Opcode::Switch, nullptr, "", loc), cond(cond),
      defaultBlock(defaultBlock) {}

void SwitchInst::addCase(MIRValue *value, MIRBlock *target) {
  cases.emplace_back(value, target);
}

void SwitchInst::dump(llvm::raw_ostream &os) const {
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

void SwitchInst::replaceOperand(MIRValue *oldVal, MIRValue *newVal) {
  // 1. Update the condition
  if (cond == oldVal)
    cond = newVal;

  // 2. Update the case values
  for (auto &c : cases) {
    if (c.first == oldVal)
      c.first = newVal;
  }

  // 3. Update the target blocks (if SimplifyCFG is bypassing them)
  if (auto *newBlock = llvm::dyn_cast_or_null<MIRBlock>(newVal)) {
    if (static_cast<MIRValue *>(defaultBlock) == oldVal) {
      defaultBlock = newBlock;
    }
    for (auto &c : cases) {
      if (static_cast<MIRValue *>(c.second) == oldVal) {
        c.second = newBlock;
      }
    }
  }
}

ReturnInst::ReturnInst(MIRValue *val, SourceLocation loc)
    : MIRInst(Opcode::Return, nullptr, "", loc), val(val) {}

void ReturnInst::dump(llvm::raw_ostream &os) const {
  os << "ret ";
  if (val) {
    printType(os, val->getType());
    os << " ";
    printOperand(os, val);
  } else {
    os << "void";
  }
}

void UnreachableInst::dump(llvm::raw_ostream &os) const { os << "unreachable"; }

// Memory Ops

AllocaInst::AllocaInst(const hir::HIRType *ptrType,
                       const hir::HIRType *allocType, std::string name,
                       SourceLocation loc, unsigned align)
    : MIRInst(Opcode::Alloca, ptrType, std::move(name), loc),
      allocatedType(allocType), alignment(align) {}

void AllocaInst::dump(llvm::raw_ostream &os) const {
  os << "  %" << getName() << " = alloca ";
  printType(os, allocatedType);
  os << getBorrowString(getBorrowKind());
  if (alignment > 0) {
    os << ", align " << alignment;
  }
}

static const hir::HIRType *determineLoadType(const MIRValue *ptr) {
  if (!ptr || !ptr->getType())
    return nullptr;

  if (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(ptr->getType())) {
    return pTy->getPointee();
  }

  return ptr->getType();
}

LoadInst::LoadInst(MIRValue *ptr, std::string name, SourceLocation loc,
                   unsigned align)
    : MIRInst(Opcode::Load, determineLoadType(ptr), std::move(name), loc),
      ptr(ptr), alignment(align) {}

void LoadInst::dump(llvm::raw_ostream &os) const {
  os << "  %" << getName() << " = load ";
  if (isVolatile())
    os << "volatile ";
  printType(os, getType());
  os << getBorrowString(getBorrowKind());
  os << ", ";
  printType(os, getType());
  os << "* ";
  printOperand(os, ptr);

  if (alignment > 0) {
    os << ", align " << alignment;
  }
}

StoreInst::StoreInst(MIRValue *val, MIRValue *ptr, SourceLocation loc,
                     unsigned align)
    : MIRInst(Opcode::Store, nullptr, "", loc), val(val), ptr(ptr),
      alignment(align) {}

void StoreInst::dump(llvm::raw_ostream &os) const {
  os << "store ";
  if (isVolatile())
    os << "volatile ";
  printType(os, val->getType());
  os << " ";
  printOperand(os, val);
  os << ", ";
  printType(os, val->getType());
  os << "* ";
  printOperand(os, ptr);

  if (alignment > 0) {
    os << ", align " << alignment;
  }
}

// Weak Memory Instructions

StoreWeakInst::StoreWeakInst(MIRValue *val, MIRValue *ptr, SourceLocation loc)
    : MIRInst(Opcode::StoreWeak, nullptr, "", loc), val(val), ptr(ptr) {}

void StoreWeakInst::dump(llvm::raw_ostream &os) const {
  os << "store_weak ";
  printType(os, val->getType());
  os << " ";
  printOperand(os, val);
  os << ", ";
  printType(os, ptr->getType());
  os << " ";
  printOperand(os, ptr);
}

LoadWeakInst::LoadWeakInst(MIRValue *ptr, const hir::HIRType *resType,
                           std::string name, SourceLocation loc)
    : MIRInst(Opcode::LoadWeak, resType, std::move(name), loc), ptr(ptr) {}

void LoadWeakInst::dump(llvm::raw_ostream &os) const {
  if (!getName().empty()) {
    os << "%" << getName() << " = ";
  }
  os << "load_weak ";
  printType(os, getType());
  os << ", ";
  printType(os, ptr->getType());
  os << " ";
  printOperand(os, ptr);
}

GetElementPtrInst::GetElementPtrInst(MIRValue *ptr,
                                     std::vector<MIRValue *> &&indices,
                                     const hir::HIRType *ptrType,
                                     const hir::HIRType *resType,
                                     std::string name, SourceLocation loc)
    : MIRInst(Opcode::GetElementPtr, ptrType, std::move(name), loc), ptr(ptr),
      indices(std::move(indices)) {}

void GetElementPtrInst::dump(llvm::raw_ostream &os) const {
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

// Aggregates

InsertValueInst::InsertValueInst(MIRValue *agg, MIRValue *val, uint32_t index,
                                 std::string name, SourceLocation loc)
    : MIRInst(Opcode::InsertValue, agg->getType(), std::move(name), loc),
      agg(agg), val(val), index(index) {}

void InsertValueInst::dump(llvm::raw_ostream &os) const {
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

void ExtractValueInst::dump(llvm::raw_ostream &os) const {
  os << "%" << getName() << " = extractvalue ";
  printType(os, agg->getType());
  os << " ";
  printOperand(os, agg);
  os << ", " << index;
}

// Others

ARCInst::ARCInst(Opcode op, MIRValue *obj, MIRFunction *dropFunc,
                 SourceLocation loc)
    : MIRInst(op, nullptr, "", loc), opcode(op), object(obj),
      dropFunc(dropFunc) {}

void ARCInst::dump(llvm::raw_ostream &os) const {
  os << (opcode == Opcode::Retain ? "retain " : "release ");
  printType(os, object->getType());
  os << " ";
  printOperand(os, object);
  if (dropFunc) {
    os << " [drop: @" << dropFunc->getName() << "]";
  }
}

BinaryInst::BinaryInst(Opcode op, MIRValue *lhs, MIRValue *rhs,
                       std::string name, SourceLocation loc)
    : MIRInst(op, lhs->getType(), std::move(name), loc), lhs(lhs), rhs(rhs) {}

void BinaryInst::dump(llvm::raw_ostream &os) const {
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

void CastInst::dump(llvm::raw_ostream &os) const {
  os << "%" << getName() << " = ";
  os << getOpcodeName(opcode) << " ";
  printType(os, value->getType());
  os << " ";
  printOperand(os, value);
  os << " to ";
  printType(os, getType());
  os << getBorrowString(getBorrowKind());
}

CompareInst::CompareInst(Predicate pred, MIRValue *lhs, MIRValue *rhs,
                         const hir::HIRType *resType, std::string name,
                         SourceLocation loc)
    : MIRInst(Opcode::ICmp, resType, std::move(name), loc), pred(pred),
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

void CompareInst::dump(llvm::raw_ostream &os) const {
  os << "%" << getName() << " = icmp " << getPredicateString(pred) << " ";
  printType(os, lhs->getType());
  os << " ";
  printOperand(os, lhs);
  os << ", ";
  printOperand(os, rhs);
}

FCmpInst::FCmpInst(Predicate pred, MIRValue *lhs, MIRValue *rhs,
                   const hir::HIRType *resType, std::string name,
                   SourceLocation loc)
    : MIRInst(Opcode::FCmp, resType, std::move(name), loc), pred(pred),
      lhs(lhs), rhs(rhs) {}

void FCmpInst::dump(llvm::raw_ostream &os) const {
  if (getType()) {
    os << "%" << getName() << " = ";
  }
  os << "fcmp ";

  switch (pred) {
  case Predicate::OEQ:
    os << "oeq ";
    break;
  case Predicate::ONE:
    os << "one ";
    break;
  case Predicate::OLT:
    os << "olt ";
    break;
  case Predicate::OLE:
    os << "ole ";
    break;
  case Predicate::OGT:
    os << "ogt ";
    break;
  case Predicate::OGE:
    os << "oge ";
    break;
  case Predicate::UEQ:
    os << "ueq ";
    break;
  case Predicate::UNE:
    os << "une ";
    break;
  case Predicate::ULT:
    os << "ult ";
    break;
  case Predicate::ULE:
    os << "ule ";
    break;
  case Predicate::UGT:
    os << "ugt ";
    break;
  case Predicate::UGE:
    os << "uge ";
    break;
  }

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

void PhiInst::removeIncoming(MIRBlock *block) {
  incoming.erase(
      std::remove_if(incoming.begin(), incoming.end(),
                     [block](const std::pair<MIRValue *, MIRBlock *> &inc) {
                       return inc.second == block;
                     }),
      incoming.end());
}

void PhiInst::dump(llvm::raw_ostream &os) const {
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

void PhiInst::replaceOperand(MIRValue *oldVal, MIRValue *newVal) {
  for (auto &inc : incoming) {
    // 1. Remap the incoming value
    if (inc.first == oldVal)
      inc.first = newVal;

    // 2. Remap the incoming block
    if (static_cast<MIRValue *>(inc.second) == oldVal) {
      if (auto *newBlock = llvm::dyn_cast_or_null<MIRBlock>(newVal)) {
        inc.second = newBlock;
      }
    }
  }
}

CallInst::CallInst(MIRValue *callee, std::vector<MIRValue *> &&args,
                   const hir::HIRType *retType, std::string name, bool isVarArg,
                   SourceLocation loc)
    : MIRInst(Opcode::Call, retType, std::move(name), loc), callee(callee),
      args(std::move(args)), isVarArg(isVarArg) {}

void CallInst::dump(llvm::raw_ostream &os) const {
  if (getType() && getType()->getKind() != hir::TypeKind::Void) {
    if (!getName().empty()) {
      os << "%" << getName() << " = ";
    }
  }
  os << "call ";
  printType(os, getType());
  os << getBorrowString(getBorrowKind());
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

// Exceptions & Stack Unwinding

InvokeInst::InvokeInst(MIRValue *callee, std::vector<MIRValue *> &&args,
                       MIRBlock *normalDest, MIRBlock *unwindDest,
                       const hir::HIRType *retType, std::string name,
                       SourceLocation loc)
    : MIRInst(Opcode::Invoke, retType, std::move(name), loc), callee(callee),
      args(std::move(args)), normalDest(normalDest), unwindDest(unwindDest) {}

void InvokeInst::dump(llvm::raw_ostream &os) const {
  if (getType() && getType()->getKind() != hir::TypeKind::Void) {
    os << "%" << getName() << " = ";
  }
  os << "invoke ";
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
  os << ") to label ";
  printOperand(os, normalDest);
  os << " unwind label ";
  printOperand(os, unwindDest);
}

void InvokeInst::replaceOperand(MIRValue *oldVal, MIRValue *newVal) {
  if (callee == oldVal)
    callee = newVal;
  for (auto &arg : args) {
    if (arg == oldVal)
      arg = newVal;
  }
  if (normalDest == oldVal)
    normalDest = static_cast<MIRBlock *>(newVal);
  if (unwindDest == oldVal)
    unwindDest = static_cast<MIRBlock *>(newVal);
}

LandingPadInst::LandingPadInst(const hir::HIRType *resultType, std::string name,
                               SourceLocation loc)
    : MIRInst(Opcode::LandingPad, resultType, std::move(name), loc) {}

void LandingPadInst::dump(llvm::raw_ostream &os) const {
  os << "%" << getName() << " = landingpad ";
  printType(os, getType());

  if (catchTypes.empty()) {
    os << " cleanup";
  } else {
    for (const auto *ct : catchTypes) {
      os << "\n          catch ";
      printType(os, ct);
    }
  }
}

ResumeInst::ResumeInst(MIRValue *exception, SourceLocation loc)
    : MIRInst(Opcode::Resume, nullptr, "", loc), exception(exception) {}

void ResumeInst::dump(llvm::raw_ostream &os) const {
  os << "resume ";
  if (exception) {
    printType(os, exception->getType());
    os << " ";
    printOperand(os, exception);
  } else {
    os << "none <null exception>";
  }
}

ThrowInst::ThrowInst(MIRValue *exception, MIRBlock *unwindDest,
                     SourceLocation loc)
    : MIRInst(Opcode::Throw, nullptr, "", loc), exception(exception),
      unwindDest(unwindDest) {}

void ThrowInst::dump(llvm::raw_ostream &os) const {
  os << "throw ";
  printType(os, exception->getType());
  os << " ";
  printOperand(os, exception);
  if (unwindDest) {
    os << " unwind label ";
    printOperand(os, unwindDest);
  }
}

void ThrowInst::replaceOperand(MIRValue *oldVal, MIRValue *newVal) {
  if (exception == oldVal)
    exception = newVal;
  if (unwindDest == oldVal)
    unwindDest = static_cast<MIRBlock *>(newVal);
}

// Inline Assembly & Decimal Constants

InlineAsmInst::InlineAsmInst(std::string asmStr, std::string constraints,
                             std::vector<MIRValue *> &&args, bool isVolatile,
                             const hir::HIRType *retType, SourceLocation loc)
    : MIRInst(Opcode::InlineAsm, retType, "", loc),
      asmString(std::move(asmStr)), constraints(std::move(constraints)),
      args(std::move(args)), isVolatile(isVolatile) {}

void InlineAsmInst::dump(llvm::raw_ostream &os) const {
  if (getType() && getType()->getKind() != hir::TypeKind::Void) {
    os << "%" << getName() << " = ";
  }
  os << "call ";
  printType(os, getType());
  os << " asm ";

  if (isVolatile) {
    os << "sideeffect ";
  }

  os << "\"" << asmString << "\" (\"" << constraints << "\")(";
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0)
      os << ", ";
    printType(os, args[i]->getType());
    os << " ";
    printOperand(os, args[i]);
  }
  os << ")";
}

void ConstantDecimal::dump(llvm::raw_ostream &os) const { os << value; }

static const char *memoryOrderToString(MemoryOrder order) {
  switch (order) {
  case MemoryOrder::Relaxed:
    return "unordered";
  case MemoryOrder::Consume:
    return "consume";
  case MemoryOrder::Acquire:
    return "acquire";
  case MemoryOrder::Release:
    return "release";
  case MemoryOrder::AcqRel:
    return "acq_rel";
  case MemoryOrder::SeqCst:
    return "seq_cst";
  }
  return "seq_cst";
}

static const char *atomicOpToString(AtomicOp op) {
  switch (op) {
  case AtomicOp::Xchg:
    return "xchg";
  case AtomicOp::Add:
    return "add";
  case AtomicOp::Sub:
    return "sub";
  case AtomicOp::And:
    return "and";
  case AtomicOp::Nand:
    return "nand";
  case AtomicOp::Or:
    return "or";
  case AtomicOp::Xor:
    return "xor";
  case AtomicOp::Max:
    return "max";
  case AtomicOp::Min:
    return "min";
  case AtomicOp::UMax:
    return "umax";
  case AtomicOp::UMin:
    return "umin";
  }
  return "add";
}

// Concurrency & Closures

void MakeClosureInst::dump(llvm::raw_ostream &os) const {
  os << "%" << getName() << " = make_closure ";
  printType(os, getType());
  os << " ";
  printOperand(os, getFunctionPointer());

  os << " [";
  for (size_t i = 0; i < captures.size(); ++i) {
    if (i > 0)
      os << ", ";
    printType(os, captures[i]->getType());
    os << " ";
    printOperand(os, captures[i]);
  }
  os << "]";
}

void MakeSharedInst::dump(llvm::raw_ostream &os) const {
  os << "%" << getName() << " = make_shared ";
  printType(os, operand->getType());
  os << " ";
  printOperand(os, operand);
}

void SpawnInst::dump(llvm::raw_ostream &os) const {
  if (getType() && getType()->getKind() != hir::TypeKind::Void) {
    os << "%" << getName() << " = ";
  }

  os << "spawn ";
  if (threadKind == hir::ThreadKind::Weak)
    os << "weak ";
  else if (threadKind == hir::ThreadKind::Detached)
    os << "detached ";

  printType(os, closure->getType());
  os << " ";
  printOperand(os, closure);
}

void AwaitInst::dump(llvm::raw_ostream &os) const {
  if (getType() && getType()->getKind() != hir::TypeKind::Void) {
    os << "%" << getName() << " = ";
  }
  os << "await ";
  printType(os, promise->getType());
  os << " ";
  printOperand(os, promise);
}

AtomicLoadInst::AtomicLoadInst(MIRValue *pointer, MemoryOrder order,
                               SourceLocation loc)
    : MIRInst(Opcode::AtomicLoad, nullptr, "", loc), pointer(pointer),
      order(order) {
  if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(pointer->getType())) {
    setType(ptrTy->getPointee());
  }
}

void AtomicLoadInst::dump(llvm::raw_ostream &os) const {
  os << "%" << getName() << " = load atomic ";
  printType(os, getType());
  os << ", ";
  printType(os, pointer->getType());
  os << " ";
  printOperand(os, pointer);
  os << " " << memoryOrderToString(order);
}

AtomicStoreInst::AtomicStoreInst(MIRValue *value, MIRValue *pointer,
                                 MemoryOrder order, SourceLocation loc)
    : MIRInst(Opcode::AtomicStore, nullptr, "", loc), value(value),
      pointer(pointer), order(order) {}

void AtomicStoreInst::dump(llvm::raw_ostream &os) const {
  os << "store atomic ";
  printType(os, value->getType());
  os << " ";
  printOperand(os, value);
  os << ", ";
  printType(os, pointer->getType());
  os << " ";
  printOperand(os, pointer);
  os << " " << memoryOrderToString(order);
}

AtomicRMWInst::AtomicRMWInst(AtomicOp op, MIRValue *pointer, MIRValue *value,
                             MemoryOrder order, SourceLocation loc)
    : MIRInst(Opcode::AtomicRMW, value->getType(), "", loc), op(op),
      pointer(pointer), value(value), order(order) {}

void AtomicRMWInst::dump(llvm::raw_ostream &os) const {
  os << "%" << getName() << " = atomicrmw " << atomicOpToString(op) << " ";
  printType(os, pointer->getType());
  os << " ";
  printOperand(os, pointer);
  os << ", ";
  printType(os, value->getType());
  os << " ";
  printOperand(os, value);
  os << " " << memoryOrderToString(order);
}

AtomicCmpXchgInst::AtomicCmpXchgInst(MIRValue *pointer, MIRValue *expected,
                                     MIRValue *desired,
                                     MemoryOrder successOrder,
                                     MemoryOrder failureOrder,
                                     SourceLocation loc)
    : MIRInst(Opcode::AtomicCmpXchg, expected->getType(), "", loc),
      pointer(pointer), expected(expected), desired(desired),
      successOrder(successOrder), failureOrder(failureOrder) {}

void AtomicCmpXchgInst::dump(llvm::raw_ostream &os) const {
  os << "%" << getName() << " = cmpxchg ";
  printType(os, pointer->getType());
  os << " ";
  printOperand(os, pointer);
  os << ", ";
  printType(os, expected->getType());
  os << " ";
  printOperand(os, expected);
  os << ", ";
  printType(os, desired->getType());
  os << " ";
  printOperand(os, desired);
  os << " " << memoryOrderToString(successOrder) << " "
     << memoryOrderToString(failureOrder);
}

FenceInst::FenceInst(MemoryOrder order, SourceLocation loc)
    : MIRInst(Opcode::Fence, nullptr, "", loc), order(order) {}

void FenceInst::dump(llvm::raw_ostream &os) const {
  os << "fence " << memoryOrderToString(order);
}

} // namespace mir
} // namespace moksha
