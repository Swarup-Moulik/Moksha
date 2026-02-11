#include "moksha/MIR/MIRBuilder.h"
#include <memory>

namespace moksha {
namespace mir {

// ============================================================================
// [Constructors & Positioning]
// ============================================================================

MIRBuilder::MIRBuilder() : currentBlock(nullptr) {}

MIRBuilder::MIRBuilder(MIRBlock *block) : currentBlock(block) {}

void MIRBuilder::setInsertPoint(MIRBlock *block) { currentBlock = block; }

MIRBlock *MIRBuilder::getInsertBlock() const { return currentBlock; }

void MIRBuilder::clearInsertPoint() { currentBlock = nullptr; }

// Note: The generic 'insert' method is now a private template defined in the
// header to handle ownership transfer (std::unique_ptr) correctly.

// ============================================================================
// [Terminators]
// ============================================================================

BranchInst *MIRBuilder::createBr(MIRBlock *dest, SourceLocation loc) {
  return insert(std::make_unique<BranchInst>(dest, loc));
}

CondBranchInst *MIRBuilder::createCondBr(MIRValue *cond, MIRBlock *trueBlock,
                                         MIRBlock *falseBlock,
                                         SourceLocation loc) {
  return insert(
      std::make_unique<CondBranchInst>(cond, trueBlock, falseBlock, loc));
}

ReturnInst *MIRBuilder::createRet(MIRValue *val, SourceLocation loc) {
  return insert(std::make_unique<ReturnInst>(val, loc));
}

ReturnInst *MIRBuilder::createRetVoid(SourceLocation loc) {
  return insert(std::make_unique<ReturnInst>(nullptr, loc));
}

SwitchInst *MIRBuilder::createSwitch(MIRValue *cond, MIRBlock *defaultBlock,
                                     SourceLocation loc) {
  return insert(std::make_unique<SwitchInst>(cond, defaultBlock, loc));
}

// ============================================================================
// [Arithmetic & Logic]
// ============================================================================

BinaryInst *MIRBuilder::createBinaryOp(Opcode op, MIRValue *lhs, MIRValue *rhs,
                                       const std::string &name,
                                       SourceLocation loc) {
  return insert(std::make_unique<BinaryInst>(op, lhs, rhs, name, loc));
}

// Integer Arithmetic
BinaryInst *MIRBuilder::createAdd(MIRValue *lhs, MIRValue *rhs,
                                  const std::string &name, SourceLocation loc) {
  return createBinaryOp(Opcode::Add, lhs, rhs, name, loc);
}

BinaryInst *MIRBuilder::createSub(MIRValue *lhs, MIRValue *rhs,
                                  const std::string &name, SourceLocation loc) {
  return createBinaryOp(Opcode::Sub, lhs, rhs, name, loc);
}

BinaryInst *MIRBuilder::createMul(MIRValue *lhs, MIRValue *rhs,
                                  const std::string &name, SourceLocation loc) {
  return createBinaryOp(Opcode::Mul, lhs, rhs, name, loc);
}

BinaryInst *MIRBuilder::createDiv(MIRValue *lhs, MIRValue *rhs,
                                  const std::string &name, SourceLocation loc) {
  return createBinaryOp(Opcode::Div, lhs, rhs, name, loc);
}

BinaryInst *MIRBuilder::createMod(MIRValue *lhs, MIRValue *rhs,
                                  const std::string &name, SourceLocation loc) {
  return createBinaryOp(Opcode::Mod, lhs, rhs, name, loc);
}

// Floating Point Arithmetic
BinaryInst *MIRBuilder::createFAdd(MIRValue *lhs, MIRValue *rhs,
                                   const std::string &name,
                                   SourceLocation loc) {
  return createBinaryOp(Opcode::FAdd, lhs, rhs, name, loc);
}

BinaryInst *MIRBuilder::createFSub(MIRValue *lhs, MIRValue *rhs,
                                   const std::string &name,
                                   SourceLocation loc) {
  return createBinaryOp(Opcode::FSub, lhs, rhs, name, loc);
}

BinaryInst *MIRBuilder::createFMul(MIRValue *lhs, MIRValue *rhs,
                                   const std::string &name,
                                   SourceLocation loc) {
  return createBinaryOp(Opcode::FMul, lhs, rhs, name, loc);
}

BinaryInst *MIRBuilder::createFDiv(MIRValue *lhs, MIRValue *rhs,
                                   const std::string &name,
                                   SourceLocation loc) {
  return createBinaryOp(Opcode::FDiv, lhs, rhs, name, loc);
}

// Bitwise Logic
BinaryInst *MIRBuilder::createAnd(MIRValue *lhs, MIRValue *rhs,
                                  const std::string &name, SourceLocation loc) {
  return createBinaryOp(Opcode::And, lhs, rhs, name, loc);
}

BinaryInst *MIRBuilder::createOr(MIRValue *lhs, MIRValue *rhs,
                                 const std::string &name, SourceLocation loc) {
  return createBinaryOp(Opcode::Or, lhs, rhs, name, loc);
}

BinaryInst *MIRBuilder::createXor(MIRValue *lhs, MIRValue *rhs,
                                  const std::string &name, SourceLocation loc) {
  return createBinaryOp(Opcode::Xor, lhs, rhs, name, loc);
}

BinaryInst *MIRBuilder::createShl(MIRValue *lhs, MIRValue *rhs,
                                  const std::string &name, SourceLocation loc) {
  return createBinaryOp(Opcode::Shl, lhs, rhs, name, loc);
}

BinaryInst *MIRBuilder::createShr(MIRValue *lhs, MIRValue *rhs,
                                  const std::string &name, SourceLocation loc) {
  return createBinaryOp(Opcode::Shr, lhs, rhs, name, loc);
}

// ============================================================================
// [Comparison]
// ============================================================================

CompareInst *MIRBuilder::createICmp(CompareInst::Predicate pred, MIRValue *lhs,
                                    MIRValue *rhs, const std::string &name,
                                    SourceLocation loc) {
  return insert(std::make_unique<CompareInst>(pred, lhs, rhs, name, loc));
}

// ============================================================================
// [Memory Operations]
// ============================================================================

AllocaInst *MIRBuilder::createAlloca(const hir::HIRType *type,
                                     const std::string &name,
                                     SourceLocation loc, unsigned align) {
  return insert(std::make_unique<AllocaInst>(type, name, loc, align));
}

LoadInst *MIRBuilder::createLoad(MIRValue *ptr, const std::string &name,
                                 SourceLocation loc, unsigned align) {
  return insert(std::make_unique<LoadInst>(ptr, name, loc, align));
}

StoreInst *MIRBuilder::createStore(MIRValue *val, MIRValue *ptr,
                                   SourceLocation loc, unsigned align) {
  return insert(std::make_unique<StoreInst>(val, ptr, loc, align));
}

// Update: Pass std::vector by value (moved in header declaration) to enable
// move semantics
GetElementPtrInst *MIRBuilder::createGEP(MIRValue *ptr,
                                         std::vector<MIRValue *> indices,
                                         const hir::HIRType *resType,
                                         const std::string &name,
                                         SourceLocation loc) {
  // Explicitly move indices into the constructor
  return insert(std::make_unique<GetElementPtrInst>(ptr, std::move(indices),
                                                    resType, name, loc));
}

GetElementPtrInst *
MIRBuilder::createGEP(MIRValue *ptr, std::initializer_list<MIRValue *> indices,
                      const hir::HIRType *resType, const std::string &name,
                      SourceLocation loc) {
  // Convert initializer_list to vector and delegate to the vector overload
  return createGEP(ptr, std::vector<MIRValue *>(indices), resType, name, loc);
}

// ============================================================================
// [Function Calls & Phis]
// ============================================================================

// Update: Pass std::vector by value to enable move semantics
CallInst *MIRBuilder::createCall(MIRValue *callee, std::vector<MIRValue *> args,
                                 const hir::HIRType *retType,
                                 const std::string &name, bool isVarArg,
                                 SourceLocation loc) {
  // Explicitly move args into the constructor
  return insert(std::make_unique<CallInst>(callee, std::move(args), retType,
                                           name, isVarArg, loc));
}

PhiInst *MIRBuilder::createPhi(const hir::HIRType *type,
                               const std::string &name, SourceLocation loc) {
  return insert(std::make_unique<PhiInst>(type, name, loc));
}

// ============================================================================
// [Casts & ARC]
// ============================================================================

CastInst *MIRBuilder::createBitCast(MIRValue *val, const hir::HIRType *destType,
                                    const std::string &name,
                                    SourceLocation loc) {
  return insert(
      std::make_unique<CastInst>(Opcode::BitCast, val, destType, name, loc));
}

ARCInst *MIRBuilder::createRetain(MIRValue *obj, SourceLocation loc) {
  return insert(std::make_unique<ARCInst>(Opcode::Retain, obj, loc));
}

ARCInst *MIRBuilder::createRelease(MIRValue *obj, SourceLocation loc) {
  return insert(std::make_unique<ARCInst>(Opcode::Release, obj, loc));
}

} // namespace mir
} // namespace moksha
