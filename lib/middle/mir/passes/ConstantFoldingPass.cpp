#include "moksha/MIR/Passes/ConstantFoldingPass.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRGlobal.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "llvm/Support/Casting.h"
#include <unordered_set>

namespace moksha {
namespace mir {

// Helper to replace all uses of a value within a function
static void replaceAllUsesInFunction(MIRFunction *F, MIRValue *oldVal,
                                     MIRValue *newVal) {
  for (auto &blockPtr : F->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructionsMut()) {
      instPtr->replaceOperand(oldVal, newVal);
    }
  }
}

// Traces a pointer through arithmetic and casts to find its base allocation
static MIRValue *getBasePointer(MIRValue *val) {
  while (val) {
    if (auto *gep = llvm::dyn_cast_or_null<GetElementPtrInst>(val)) {
      val = gep->getPointer();
    } else if (auto *cast = llvm::dyn_cast_or_null<CastInst>(val)) {
      val = cast->getValue();
    } else if (auto *cCast = llvm::dyn_cast_or_null<ConstantBitCast>(val)) {
      val = cCast->getValue();
    } else if (auto *cAny = llvm::dyn_cast_or_null<ConstantAnyCast>(val)) {
      val = cAny->getValue();
    } else if (auto *cArr = llvm::dyn_cast_or_null<ConstantArrayToSlice>(val)) {
      val = cArr->getValue();
    } else if (auto *cSlice = llvm::dyn_cast_or_null<ConstantSliceToArray>(val)) {
      val = cSlice->getValue();
    } else {
      break;
    }
  }
  return val;
}

bool ConstantFoldingPass::runOnModule(MIRModule &M) {
  bool changed = false;

  // Global Constant Propagation
  std::unordered_set<MIRGlobal *> mutatedGlobals;

  // 1. Scan for any potential mutation of a global
  for (auto &func : M.getFunctions()) {
    if (func->isDeclaration())
      continue;
    for (auto &block : func->getBlocks()) {
      for (auto &inst : block->getInstructions()) {

        // 1. Scan for any potential mutation or escape of a global
        if (auto *store = llvm::dyn_cast_or_null<StoreInst>(inst.get())) {
          // Direct/Indirect mutation: Storing TO the global
          if (auto *g = llvm::dyn_cast_or_null<MIRGlobal>(
                  getBasePointer(store->getPointer()))) {
            mutatedGlobals.insert(g);
          }
          // Escape: Storing the global's address INTO something else
          if (auto *g = llvm::dyn_cast_or_null<MIRGlobal>(
                  getBasePointer(store->getValue()))) {
            mutatedGlobals.insert(g);
          }
        }
        // Escape: Passing the global to a function
        else if (auto *call = llvm::dyn_cast_or_null<CallInst>(inst.get())) {
          for (MIRValue *arg : call->getArgs()) {
            if (auto *g =
                    llvm::dyn_cast_or_null<MIRGlobal>(getBasePointer(arg))) {
              mutatedGlobals.insert(g);
            }
          }
        } else if (auto *invoke = llvm::dyn_cast_or_null<InvokeInst>(inst.get())) {
          for (MIRValue *arg : invoke->getArgs()) {
            if (auto *g =
                    llvm::dyn_cast_or_null<MIRGlobal>(getBasePointer(arg))) {
              mutatedGlobals.insert(g);
            }
          }
        } else if (auto *closure =
                       llvm::dyn_cast_or_null<MakeClosureInst>(inst.get())) {
          for (MIRValue *cap : closure->getCaptures()) {
            if (auto *g =
                    llvm::dyn_cast_or_null<MIRGlobal>(getBasePointer(cap))) {
              mutatedGlobals.insert(g);
            }
          }
        }
      }
    }
  }

  // 2. Replace loads of unmodified globals with their constant initializer
  for (auto &globalPtr : M.getGlobalsMut()) {
    MIRGlobal *g = globalPtr.get();
    if (g->getInitializer() && !g->isVolatile() &&
        mutatedGlobals.find(g) == mutatedGlobals.end()) {
      // Prevent inlining of heap-allocated and aggregate globals
      auto *initVal = g->getInitializer();
      if (llvm::isa<ConstantMap>(initVal) ||
          llvm::isa<ConstantSlice>(initVal) ||
          llvm::isa<ConstantArray>(initVal) ||
          llvm::isa<ConstantStruct>(initVal) ||
          llvm::isa<ConstantString>(initVal)) {
        continue;
      }
      for (auto &func : M.getFunctions()) {
        if (func->isDeclaration())
          continue;
        for (auto &block : func->getBlocks()) {
          auto &insts = block->getInstructionsMut();
          for (auto it = insts.begin(); it != insts.end();) {
            if (auto *load = llvm::dyn_cast_or_null<LoadInst>(it->get())) {
              if (!load->isVolatile() && load->getPointer() == g) {
                MIRValue *replacement = g->getInitializer();

                if (replacement->getType() != load->getType()) {
                  if (llvm::isa<ConstantNull>(replacement)) {
                    replacement =
                        M.getOrInsertConstant<ConstantNull>(load->getType());
                  } else {
                    replacement = M.getOrInsertConstant<ConstantBitCast>(
                        replacement, load->getType());
                  }
                }

                replaceAllUsesInFunction(func, load, replacement);
                it = insts.erase(it); // Remove the LoadInst
                changed = true;
                continue;
              }
            }
            ++it;
          }
        }
      }
    }
  }

  for (auto &func : M.getFunctions()) {
    if (func->isDeclaration())
      continue;

    for (auto &block : func->getBlocks()) {
      auto &insts = block->getInstructionsMut();

      // 1. Fold individual instructions (Slice access, arithmetic, etc.)
      for (auto it = insts.begin(); it != insts.end();) {
        MIRInst *inst = it->get();
        MIRValue *folded = nullptr;

        // Fold Slice Property Accesses
        if (auto *ext = llvm::dyn_cast_or_null<ExtractValueInst>(inst)) {
          if (auto *constSlice =
                  llvm::dyn_cast_or_null<ConstantSlice>(ext->getAggregate())) {

            if (ext->getIndex() == 1) { // Index 1 is the .length/size
              folded = M.getOrInsertConstant<ConstantInt>(
                  constSlice->getElements().size(), ext->getType());
            } else if (ext->getIndex() == 0) { // Index 0 is the data pointer
              // To fold the pointer, we create a ConstantArray of the elements
              // and return a BitCast of that array to the expected pointer type
              if (auto *sliceTy =
                      llvm::dyn_cast_or_null<hir::SliceType>(constSlice->getType())) {
                auto *elemTy = sliceTy->getElementType();
                auto *arrayTy =
                    M.getArrayType(elemTy, constSlice->getElements().size());

                // 1. Get the elements from the slice
                auto elementsRef = constSlice->getElements();

                // 2. Convert ArrayRef<MIRConstant*> to std::vector<MIRValue*>
                std::vector<mir::MIRValue *> elementsVec(elementsRef.begin(),
                                                         elementsRef.end());

                // 3. Pass the Type FIRST, then the vector!
                auto *constArray = M.getOrInsertConstant<ConstantArray>(
                    arrayTy, std::move(elementsVec));

                folded = M.getOrInsertConstant<ConstantBitCast>(constArray,
                                                                ext->getType());
              }
            }
          }
        }

        // Fold Compare Instructions (e.g., icmp ne null, null OR icmp sgt 10,
        // 20)
        if (auto *icmp = llvm::dyn_cast_or_null<CompareInst>(inst)) {

          // Strip away BitCasts so we can see the raw Constants underneath
          auto unwrapCasts = [](MIRValue *val) -> MIRValue * {
            while (val) {
              if (auto *cast = llvm::dyn_cast_or_null<CastInst>(val)) {
                val = cast->getValue();
              } else if (auto *cCast =
                             llvm::dyn_cast_or_null<ConstantBitCast>(val)) {
                val = cCast->getValue();
              } else if (auto *cAny =
                             llvm::dyn_cast_or_null<ConstantAnyCast>(val)) {
                val = cAny->getValue();
              } else if (auto *cArr =
                             llvm::dyn_cast_or_null<ConstantArrayToSlice>(
                                 val)) {
                val = cArr->getValue();
              } else if (auto *cSlice =
                             llvm::dyn_cast_or_null<ConstantSliceToArray>(
                                 val)) {
                val = cSlice->getValue();
              } else {
                break;
              }
            }
            return val;
          };

          MIRValue *lhs = unwrapCasts(icmp->getLHS());
          MIRValue *rhs = unwrapCasts(icmp->getRHS());

          // 1. Fold identical values or Nulls
          if (lhs == rhs ||
              (llvm::isa<ConstantNull>(lhs) && llvm::isa<ConstantNull>(rhs))) {

            if (icmp->getPredicate() == CompareInst::Predicate::EQ) {
              folded =
                  M.getOrInsertConstant<ConstantBool>(true, icmp->getType());
            } else if (icmp->getPredicate() == CompareInst::Predicate::NE) {
              folded =
                  M.getOrInsertConstant<ConstantBool>(false, icmp->getType());
            }
          }
          // 2. Fold Constant Integer Comparisons
          else if (auto *lInt = llvm::dyn_cast_or_null<ConstantInt>(lhs)) {
            if (auto *rInt = llvm::dyn_cast_or_null<ConstantInt>(rhs)) {

              // Determine if the operands are signed based on LHS type
              bool isSigned = false;
              if (lhs->getType()) {
                if (auto *hirIntTy = llvm::dyn_cast_or_null<hir::HIRIntType>(
                        lhs->getType())) {
                  isSigned = hirIntTy->isSigned();
                } else if (lhs->getType()->toString().find("i") !=
                           std::string::npos) {
                  isSigned = true; // Fallback heuristic for "i32", "i64"
                }
              }

              bool result = false;

              if (isSigned) {
                // Evaluate using C++ SIGNED comparisons
                int64_t lVal = static_cast<int64_t>(lInt->getValue());
                int64_t rVal = static_cast<int64_t>(rInt->getValue());

                switch (icmp->getPredicate()) {
                case CompareInst::Predicate::EQ:
                  result = (lVal == rVal);
                  break;
                case CompareInst::Predicate::NE:
                  result = (lVal != rVal);
                  break;
                case CompareInst::Predicate::LT:
                  result = (lVal < rVal);
                  break;
                case CompareInst::Predicate::LE:
                  result = (lVal <= rVal);
                  break;
                case CompareInst::Predicate::GT:
                  result = (lVal > rVal);
                  break;
                case CompareInst::Predicate::GE:
                  result = (lVal >= rVal);
                  break;
                default:
                  break;
                }
              } else {
                // Evaluate using C++ UNSIGNED comparisons
                uint64_t lVal = lInt->getValue();
                uint64_t rVal = rInt->getValue();

                switch (icmp->getPredicate()) {
                case CompareInst::Predicate::EQ:
                  result = (lVal == rVal);
                  break;
                case CompareInst::Predicate::NE:
                  result = (lVal != rVal);
                  break;
                case CompareInst::Predicate::LT:
                  result = (lVal < rVal);
                  break;
                case CompareInst::Predicate::LE:
                  result = (lVal <= rVal);
                  break;
                case CompareInst::Predicate::GT:
                  result = (lVal > rVal);
                  break;
                case CompareInst::Predicate::GE:
                  result = (lVal >= rVal);
                  break;
                default:
                  break;
                }
              }

              folded =
                  M.getOrInsertConstant<ConstantBool>(result, icmp->getType());
            }
          }
        }

        // --------------------------------------------------------------------
        // Fold Floating-Point Comparisons
        // --------------------------------------------------------------------
        if (auto *fcmp = llvm::dyn_cast_or_null<FCmpInst>(inst)) {
          auto unwrapCasts = [](MIRValue *val) -> MIRValue * {
            while (val) {
              if (auto *cast = llvm::dyn_cast_or_null<CastInst>(val)) {
                val = cast->getValue();
              } else if (auto *cCast =
                             llvm::dyn_cast_or_null<ConstantBitCast>(val)) {
                val = cCast->getValue();
              } else if (auto *cAny =
                             llvm::dyn_cast_or_null<ConstantAnyCast>(val)) {
                val = cAny->getValue();
              } else if (auto *cArr =
                             llvm::dyn_cast_or_null<ConstantArrayToSlice>(
                                 val)) {
                val = cArr->getValue();
              } else if (auto *cSlice =
                             llvm::dyn_cast_or_null<ConstantSliceToArray>(
                                 val)) {
                val = cSlice->getValue();
              } else {
                break;
              }
            }
            return val;
          };

          MIRValue *lhs = unwrapCasts(fcmp->getLHS());
          MIRValue *rhs = unwrapCasts(fcmp->getRHS());

          if (auto *lFloat = llvm::dyn_cast_or_null<ConstantFloat>(lhs)) {
            if (auto *rFloat = llvm::dyn_cast_or_null<ConstantFloat>(rhs)) {
              double lVal = lFloat->getValue();
              double rVal = rFloat->getValue();
              bool result = false;

              switch (fcmp->getPredicate()) {
              case FCmpInst::Predicate::OEQ:
              case FCmpInst::Predicate::UEQ:
                result = (lVal == rVal);
                break;
              case FCmpInst::Predicate::ONE:
              case FCmpInst::Predicate::UNE:
                result = (lVal != rVal);
                break;
              case FCmpInst::Predicate::OLT:
              case FCmpInst::Predicate::ULT:
                result = (lVal < rVal);
                break;
              case FCmpInst::Predicate::OLE:
              case FCmpInst::Predicate::ULE:
                result = (lVal <= rVal);
                break;
              case FCmpInst::Predicate::OGT:
              case FCmpInst::Predicate::UGT:
                result = (lVal > rVal);
                break;
              case FCmpInst::Predicate::OGE:
              case FCmpInst::Predicate::UGE:
                result = (lVal >= rVal);
                break;
              }

              folded =
                  M.getOrInsertConstant<ConstantBool>(result, fcmp->getType());
            }
          }
        }

        // --------------------------------------------------------------------
        // Fold Binary Math (Arithmetic)
        // --------------------------------------------------------------------
        if (auto *binOp = llvm::dyn_cast_or_null<BinaryInst>(inst)) {
          auto unwrapCasts = [](MIRValue *val) -> MIRValue * {
            while (val) {
              if (auto *cast = llvm::dyn_cast_or_null<CastInst>(val)) {
                val = cast->getValue();
              } else if (auto *cCast =
                             llvm::dyn_cast_or_null<ConstantBitCast>(val)) {
                val = cCast->getValue();
              } else if (auto *cAny =
                             llvm::dyn_cast_or_null<ConstantAnyCast>(val)) {
                val = cAny->getValue();
              } else if (auto *cArr =
                             llvm::dyn_cast_or_null<ConstantArrayToSlice>(
                                 val)) {
                val = cArr->getValue();
              } else if (auto *cSlice =
                             llvm::dyn_cast_or_null<ConstantSliceToArray>(
                                 val)) {
                val = cSlice->getValue();
              } else {
                break;
              }
            }
            return val;
          };

          MIRValue *lhs = unwrapCasts(binOp->getLHS());
          MIRValue *rhs = unwrapCasts(binOp->getRHS());

          // Integer Math
          if (auto *lInt = llvm::dyn_cast_or_null<ConstantInt>(lhs)) {
            if (auto *rInt = llvm::dyn_cast_or_null<ConstantInt>(rhs)) {

              // Check if the Moksha type is a signed integer
              bool isSigned = false;
              if (auto *hirIntTy = llvm::dyn_cast_or_null<hir::HIRIntType>(
                      binOp->getType())) {
                isSigned = hirIntTy->isSigned();
              } else if (binOp->getType()->toString().find("i") !=
                         std::string::npos) {
                isSigned = true; // Fallback heuristic for "i32", "i64", etc.
              }

              uint64_t result = 0;
              bool valid = true;

              if (isSigned) {
                // Evaluate using C++ SIGNED arithmetic (sdiv, srem, etc.)
                int64_t lVal = static_cast<int64_t>(lInt->getValue());
                int64_t rVal = static_cast<int64_t>(rInt->getValue());
                int64_t sResult = 0;

                switch (binOp->getOpcode()) {
                case Opcode::Add:
                  sResult = lVal + rVal;
                  break;
                case Opcode::Sub:
                  sResult = lVal - rVal;
                  break;
                case Opcode::Mul:
                  sResult = lVal * rVal;
                  break;
                case Opcode::Div:
                  if (rVal != 0)
                    sResult = lVal / rVal;
                  else
                    valid = false;
                  break;
                case Opcode::Mod:
                  if (rVal != 0)
                    sResult = lVal % rVal;
                  else
                    valid = false;
                  break;
                default:
                  valid = false;
                  break;
                }
                result = static_cast<uint64_t>(sResult);
              } else {
                // Evaluate using C++ UNSIGNED arithmetic (udiv, urem, etc.)
                uint64_t lVal = lInt->getValue();
                uint64_t rVal = rInt->getValue();

                switch (binOp->getOpcode()) {
                case Opcode::Add:
                  result = lVal + rVal;
                  break;
                case Opcode::Sub:
                  result = lVal - rVal;
                  break;
                case Opcode::Mul:
                  result = lVal * rVal;
                  break;
                case Opcode::Div:
                  if (rVal != 0)
                    result = lVal / rVal;
                  else
                    valid = false;
                  break;
                case Opcode::Mod:
                  if (rVal != 0)
                    result = lVal % rVal;
                  else
                    valid = false;
                  break;
                default:
                  valid = false;
                  break;
                }
              }

              if (valid) {
                folded = M.getOrInsertConstant<ConstantInt>(result,
                                                            binOp->getType());
              }
            }
          }
          // Floating-Point Math
          else if (auto *lFloat = llvm::dyn_cast_or_null<ConstantFloat>(lhs)) {
            if (auto *rFloat = llvm::dyn_cast_or_null<ConstantFloat>(rhs)) {
              double lVal = lFloat->getValue();
              double rVal = rFloat->getValue();
              double result = 0.0;
              bool valid = true;

              switch (binOp->getOpcode()) {
              case Opcode::Add:
              case Opcode::FAdd:
                result = lVal + rVal;
                break;
              case Opcode::Sub:
              case Opcode::FSub:
                result = lVal - rVal;
                break;
              case Opcode::Mul:
              case Opcode::FMul:
                result = lVal * rVal;
                break;
              case Opcode::Div:
              case Opcode::FDiv:
                result = lVal / rVal;
                break; // IEEE 754 natively handles x / 0.0
              default:
                valid = false;
                break;
              }
              if (valid) {
                folded = M.getOrInsertConstant<ConstantFloat>(result,
                                                              binOp->getType());
              }
            }
          }
        }

        if (folded) {
          replaceAllUsesInFunction(func, inst, folded);
          it = insts.erase(it); // Remove the redundant instruction
          changed = true;
          continue; // Don't increment iterator
        }
        ++it;
      }

      // 2. Fold Control Flow (Branches)
      if (block->getInstructions().empty())
        continue;

      MIRInst *terminator = block->getInstructions().back().get();
      if (auto *condBr = llvm::dyn_cast_or_null<CondBranchInst>(terminator)) {
        if (auto *constBool =
                llvm::dyn_cast_or_null<ConstantBool>(condBr->getCondition())) {

          MIRBlock *target = constBool->getValue() ? condBr->getTrueBlock()
                                                   : condBr->getFalseBlock();
          MIRBlock *deadTarget = constBool->getValue() ? condBr->getFalseBlock()
                                                       : condBr->getTrueBlock();

          // Rewrite to unconditional branch
          auto newBr = std::make_unique<BranchInst>(target, condBr->getLoc());
          block->getInstructionsMut().pop_back();
          block->addInstruction(std::move(newBr));

          // Clean up dead CFG edges
          if (deadTarget != target) {
            for (auto &instPtr : deadTarget->getInstructionsMut()) {
              if (auto *phi = llvm::dyn_cast_or_null<PhiInst>(instPtr.get())) {
                phi->removeIncoming(block.get());
              } else {
                break;
              }
            }
            block->removeSuccessor(deadTarget);
            deadTarget->removePredecessor(block.get());
          }
          changed = true;
        }
      }
    }
  }
  return changed;
}

} // namespace mir
} // namespace moksha
