#pragma once

#include "moksha/Support/SourceLocation.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRInst.h"
#include <cassert>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

namespace moksha {
namespace mir {

class MIRBuilder {
public:
  MIRBuilder();
  explicit MIRBuilder(MIRBlock *block);

  // ========================================================================
  // [Positioning]
  // ========================================================================

  void setInsertPoint(MIRBlock *block);
  MIRBlock *getInsertBlock() const;
  void clearInsertPoint();

  // ========================================================================
  // [Terminators]
  // ========================================================================

  BranchInst *createBr(MIRBlock *dest, SourceLocation loc = {});
  CondBranchInst *createCondBr(MIRValue *cond, MIRBlock *trueBlock,
                               MIRBlock *falseBlock, SourceLocation loc = {});
  ReturnInst *createRet(MIRValue *val, SourceLocation loc = {});
  ReturnInst *createRetVoid(SourceLocation loc = {});
  SwitchInst *createSwitch(MIRValue *cond, MIRBlock *defaultBlock,
                           SourceLocation loc = {});

  // ========================================================================
  // [Arithmetic & Logic]
  // ========================================================================

  // Generic Binary Op
  BinaryInst *createBinaryOp(Opcode op, MIRValue *lhs, MIRValue *rhs,
                             const std::string &name = "",
                             SourceLocation loc = {});

  // Integer Arithmetic
  BinaryInst *createAdd(MIRValue *lhs, MIRValue *rhs,
                        const std::string &name = "", SourceLocation loc = {});
  BinaryInst *createSub(MIRValue *lhs, MIRValue *rhs,
                        const std::string &name = "", SourceLocation loc = {});
  BinaryInst *createMul(MIRValue *lhs, MIRValue *rhs,
                        const std::string &name = "", SourceLocation loc = {});
  BinaryInst *createDiv(MIRValue *lhs, MIRValue *rhs,
                        const std::string &name = "", SourceLocation loc = {});
  BinaryInst *createMod(MIRValue *lhs, MIRValue *rhs,
                        const std::string &name = "", SourceLocation loc = {});

  // Floating Point Arithmetic
  BinaryInst *createFAdd(MIRValue *lhs, MIRValue *rhs,
                         const std::string &name = "", SourceLocation loc = {});
  BinaryInst *createFSub(MIRValue *lhs, MIRValue *rhs,
                         const std::string &name = "", SourceLocation loc = {});
  BinaryInst *createFMul(MIRValue *lhs, MIRValue *rhs,
                         const std::string &name = "", SourceLocation loc = {});
  BinaryInst *createFDiv(MIRValue *lhs, MIRValue *rhs,
                         const std::string &name = "", SourceLocation loc = {});

  // Bitwise Logic
  BinaryInst *createAnd(MIRValue *lhs, MIRValue *rhs,
                        const std::string &name = "", SourceLocation loc = {});
  BinaryInst *createOr(MIRValue *lhs, MIRValue *rhs,
                       const std::string &name = "", SourceLocation loc = {});
  BinaryInst *createXor(MIRValue *lhs, MIRValue *rhs,
                        const std::string &name = "", SourceLocation loc = {});
  BinaryInst *createShl(MIRValue *lhs, MIRValue *rhs,
                        const std::string &name = "", SourceLocation loc = {});
  BinaryInst *createShr(MIRValue *lhs, MIRValue *rhs,
                        const std::string &name = "", SourceLocation loc = {});

  // ========================================================================
  // [Comparison]
  // ========================================================================

  CompareInst *createICmp(CompareInst::Predicate pred, MIRValue *lhs,
                          MIRValue *rhs, const std::string &name = "",
                          SourceLocation loc = {});

  // ========================================================================
  // [Memory Operations]
  // ========================================================================

  AllocaInst *createAlloca(const hir::HIRType *type,
                           const std::string &name = "",
                           SourceLocation loc = {}, unsigned align = 0);
  LoadInst *createLoad(MIRValue *ptr, const std::string &name = "",
                       SourceLocation loc = {}, unsigned align = 0);
  StoreInst *createStore(MIRValue *val, MIRValue *ptr, SourceLocation loc = {},
                         unsigned align = 0);

  // Note: Accepts std::vector by value to enable move semantics into the
  // Instruction
  GetElementPtrInst *createGEP(MIRValue *ptr, std::vector<MIRValue *> indices,
                               const hir::HIRType *resType,
                               const std::string &name = "",
                               SourceLocation loc = {});

  // Convenience overload for brace-init-list: builder.createGEP(ptr, {idx1,
  // idx2}, type)
  GetElementPtrInst *createGEP(MIRValue *ptr,
                               std::initializer_list<MIRValue *> indices,
                               const hir::HIRType *resType,
                               const std::string &name = "",
                               SourceLocation loc = {});

  // ========================================================================
  // [Function Calls & Phis]
  // ========================================================================

  // Note: Accepts std::vector by value to enable move semantics into the
  // Instruction
  CallInst *createCall(MIRValue *callee, std::vector<MIRValue *> args,
                       const hir::HIRType *retType,
                       const std::string &name = "", bool isVarArg = false,
                       SourceLocation loc = {});

  // Creates a Phi node. Use phi->addIncoming(val, block) subsequently to
  // populate it.
  PhiInst *createPhi(const hir::HIRType *type, const std::string &name = "",
                     SourceLocation loc = {});

  // ========================================================================
  // [Casts & ARC]
  // ========================================================================

  CastInst *createBitCast(MIRValue *val, const hir::HIRType *destType,
                          const std::string &name = "",
                          SourceLocation loc = {});
  ARCInst *createRetain(MIRValue *obj, SourceLocation loc = {});
  ARCInst *createRelease(MIRValue *obj, SourceLocation loc = {});

  template <typename InstTy> InstTy *insert(std::unique_ptr<InstTy> inst) {
    assert(currentBlock && "Insert point not set for MIRBuilder");
    InstTy *rawPtr = inst.get();
    currentBlock->addInstruction(std::move(inst));
    return rawPtr;
  }

private:
  MIRBlock *currentBlock;
};

} // namespace mir
} // namespace moksha
