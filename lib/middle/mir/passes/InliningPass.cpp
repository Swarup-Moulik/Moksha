#include "moksha/MIR/Passes/InliningPass.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRArgument.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace moksha {
namespace mir {

// Helper to replace all uses of the old call with the inlined return value
static void replaceAllUsesInFunction(MIRFunction *F, MIRValue *oldVal,
                                     MIRValue *newVal) {
  for (auto &blockPtr : F->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructionsMut()) {
      instPtr->replaceOperand(oldVal, newVal);
    }
  }
}

bool InliningPass::runOnModule(MIRModule &M) {
  bool changed = false;
  for (auto &func : M.getFunctions()) {
    changed |= runOnFunction(func, M);
  }
  return changed;
}

bool InliningPass::shouldInline(MIRFunction *callee) {
  if (!callee || callee->isDeclaration())
    return false;

  if (callee->isVariadic())
    return false;

  // Do not inline async functions
  if (callee->getType()) {
    std::string retStr = callee->getType()->toString();
    if (retStr.find("promise") != std::string::npos ||
        retStr.find("Promise") != std::string::npos) {
      return false; // Abort inlining!
    }
  }

  if (callee->isNoInline() || callee->isInterrupt() || callee->isNaked()) {
    return false;
  }

  // Count instructions for heuristic limits
  size_t instCount = 0;
  for (const auto &b : callee->getBlocks()) {
    instCount += b->getInstructions().size();
  }

  return instCount < 75 || callee->isInline();
}

bool InliningPass::runOnFunction(MIRFunction *F, MIRModule &M) {
  if (F->isDeclaration())
    return false;

  bool changed = false;
  bool localChanged = true;

  // [FIX 1]: Hard cap the number of inlining passes to prevent infinite loops
  int iterationLimit = 0;

  while (localChanged && iterationLimit++ < 10) {
    localChanged = false;

    // [FIX 2]: Prevent massive function growth (Circuit breaker)
    size_t totalInsts = 0;
    for (const auto &b : F->getBlocks()) {
      totalInsts += b->getInstructions().size();
    }
    if (totalInsts > 2500)
      break;

    for (size_t bIdx = 0; bIdx < F->getBlocks().size(); ++bIdx) {
      MIRBlock *block = F->getBlocks()[bIdx].get();
      auto &insts = block->getInstructionsMut();

      for (auto it = insts.begin(); it != insts.end(); ++it) {
        if (auto *call = llvm::dyn_cast<CallInst>(it->get())) {
          if (auto *callee =
                  llvm::dyn_cast_or_null<MIRFunction>(call->getCallee())) {

            // [FIX 3]: DO NOT inline a function into itself (Self-Recursion
            // Check)
            if (callee != F && shouldInline(callee)) {
              localChanged = inlineCall(F, block, it, call, M);
              if (localChanged) {
                changed = true;
                break;
              }
            }
          }
        } else if (auto *invoke = llvm::dyn_cast<InvokeInst>(it->get())) {
          if (auto *callee =
                  llvm::dyn_cast_or_null<MIRFunction>(invoke->getCallee())) {

            // [FIX 3]: Same check for Invoke instructions
            if (callee != F && shouldInline(callee)) {
              localChanged = inlineInvoke(F, block, it, invoke, M);
              if (localChanged) {
                changed = true;
                break;
              }
            }
          }
        }
      }
      if (localChanged)
        break;
    }
  }
  if (changed)
    F->numberUnnamedValues();
  return changed;
}

// ----------------------------------------------------------------------------
// FULL BLOCK-SPLITTING INLINER
// ----------------------------------------------------------------------------
bool InliningPass::inlineCall(
    MIRFunction *caller, MIRBlock *callBlock,
    std::vector<std::unique_ptr<MIRInst>>::iterator &callIt, CallInst *call,
    MIRModule &M) {

  MIRFunction *callee = llvm::cast<MIRFunction>(call->getCallee());

  std::unordered_map<MIRValue *, MIRValue *> valueMap;
  std::unordered_map<MIRBlock *, MIRBlock *> blockMap;

  // 1. Map Arguments
  size_t argCount =
      std::min(call->getArgs().size(), callee->getRawArguments().size());
  for (size_t i = 0; i < argCount; ++i) {
    valueMap[callee->getRawArguments()[i]] = call->getArgs()[i];
  }

  // 2. Clone Blocks
  std::vector<MIRBlock *> clonedBlocks;
  for (const auto &oldBlockPtr : callee->getBlocks()) {
    std::string newName =
        caller->getUniqueName(callee->getName() + "." + oldBlockPtr->getName());
    auto newBlock = std::make_unique<MIRBlock>(newName, caller);

    blockMap[oldBlockPtr.get()] = newBlock.get();
    clonedBlocks.push_back(newBlock.get());
    caller->addBlock(std::move(newBlock));
  }

  // 3. Clone Instructions (Pass 1: Creation)
  std::vector<MIRInst *> allClonedInsts;
  std::vector<ReturnInst *> returns;

  for (const auto &oldBlockPtr : callee->getBlocks()) {
    MIRBlock *newBlock = blockMap[oldBlockPtr.get()];

    for (const auto &oldInstPtr : oldBlockPtr->getInstructions()) {
      auto clonedInst = oldInstPtr->clone();
      clonedInst->setParent(newBlock);
      clonedInst->setBorrowKind(oldInstPtr->getBorrowKind());

      valueMap[oldInstPtr.get()] = clonedInst.get();
      allClonedInsts.push_back(clonedInst.get());

      if (auto *retInst = llvm::dyn_cast<ReturnInst>(clonedInst.get())) {
        returns.push_back(retInst);
      }
      newBlock->getInstructionsMut().push_back(std::move(clonedInst));
    }
  }

  // 4. Remap Operands (Pass 2: Patching variables and branches)
  for (MIRInst *inst : allClonedInsts) {
    // Ask the instruction to replace any matching old values with the new ones
    for (auto &[oldVal, newVal] : valueMap) {
      inst->replaceOperand(oldVal, newVal);
    }
    // Do the same for block targets
    for (auto &[oldBlock, newBlock] : blockMap) {
      inst->replaceOperand(oldBlock, newBlock);
    }

    // Explicitly update Phi Nodes and Terminator block targets
    if (auto *phi = llvm::dyn_cast<PhiInst>(inst)) {
      for (auto &[val, incomingBlock] : phi->getIncomingMut()) {
        if (blockMap.count(incomingBlock)) {
          incomingBlock = blockMap[incomingBlock];
        }
      }
    } else if (auto *br = llvm::dyn_cast<BranchInst>(inst)) {
      if (blockMap.count(br->getTarget())) {
        br->setTarget(blockMap[br->getTarget()]);
      }
    } else if (auto *condBr = llvm::dyn_cast<CondBranchInst>(inst)) {
      if (blockMap.count(condBr->getTrueBlock())) {
        condBr->setTrueBlock(blockMap[condBr->getTrueBlock()]);
      }
      if (blockMap.count(condBr->getFalseBlock())) {
        condBr->setFalseBlock(blockMap[condBr->getFalseBlock()]);
      }
    } else if (auto *sw = llvm::dyn_cast<SwitchInst>(inst)) {
      if (blockMap.count(sw->getDefaultBlock())) {
        sw->setDefaultBlock(blockMap[sw->getDefaultBlock()]);
      }
      // Note: adjust the setter/getter here to match your exact SwitchInst API
      for (auto &casePair : sw->getCasesMut()) {
        if (blockMap.count(casePair.second)) {
          casePair.second = blockMap[casePair.second];
        }
      }
    } else if (auto *inv = llvm::dyn_cast<InvokeInst>(inst)) {
      if (blockMap.count(inv->getNormalDest())) {
        inv->setNormalDest(blockMap[inv->getNormalDest()]);
      }
      if (inv->getUnwindDest() && blockMap.count(inv->getUnwindDest())) {
        inv->setUnwindDest(blockMap[inv->getUnwindDest()]);
      }
    } else if (auto *throwInst = llvm::dyn_cast<ThrowInst>(inst)) {
      if (throwInst->getUnwindDest() &&
          blockMap.count(throwInst->getUnwindDest())) {
        throwInst->setUnwindDest(blockMap[throwInst->getUnwindDest()]);
      }
    }
  }

  // 4.5 Rebuild Internal CFG Edges
  for (MIRBlock *newBlock : clonedBlocks) {
    if (newBlock->getInstructions().empty())
      continue;
    MIRInst *term = newBlock->getInstructions().back().get();

    if (auto *br = llvm::dyn_cast<BranchInst>(term)) {
      newBlock->addSuccessor(br->getTarget());
      br->getTarget()->addPredecessor(newBlock);
    } else if (auto *condBr = llvm::dyn_cast<CondBranchInst>(term)) {
      newBlock->addSuccessor(condBr->getTrueBlock());
      condBr->getTrueBlock()->addPredecessor(newBlock);
      newBlock->addSuccessor(condBr->getFalseBlock());
      condBr->getFalseBlock()->addPredecessor(newBlock);
    } else if (auto *invoke = llvm::dyn_cast<InvokeInst>(term)) {
      newBlock->addSuccessor(invoke->getNormalDest());
      invoke->getNormalDest()->addPredecessor(newBlock);
      if (invoke->getUnwindDest()) {
        newBlock->addSuccessor(invoke->getUnwindDest());
        invoke->getUnwindDest()->addPredecessor(newBlock);
      }
    } else if (auto *throwInst = llvm::dyn_cast<ThrowInst>(term)) {
      if (throwInst->getUnwindDest()) {
        newBlock->addSuccessor(throwInst->getUnwindDest());
        throwInst->getUnwindDest()->addPredecessor(newBlock);
      }
    } else if (auto *switchInst = llvm::dyn_cast<SwitchInst>(term)) {
      newBlock->addSuccessor(switchInst->getDefaultBlock());
      switchInst->getDefaultBlock()->addPredecessor(newBlock);
      for (auto &casePair : switchInst->getCases()) {
        newBlock->addSuccessor(casePair.second);
        casePair.second->addPredecessor(newBlock);
      }
    }
  }

  // 5. Split the Caller Block
  auto returnBlock = std::make_unique<MIRBlock>(
      caller->getUniqueName("inline.return"), caller);
  MIRBlock *returnBlockPtr = returnBlock.get();

  auto &callBlockInsts = callBlock->getInstructionsMut();
  auto splitStart = std::next(callIt); // Everything AFTER the call instruction

  // Move remaining instructions to the new return block
  for (auto it = splitStart; it != callBlockInsts.end(); ++it) {
    (*it)->setParent(returnBlockPtr);
    returnBlockPtr->addInstruction(std::move(*it));
  }
  callBlockInsts.erase(splitStart, callBlockInsts.end());
  returnBlockPtr->getSuccessors() = callBlock->getSuccessors();
  for (MIRBlock *succ : returnBlockPtr->getSuccessors()) {
    // Update successors to point back to ReturnBlock instead of CallBlock
    auto &succPreds = succ->getPredecessors();
    std::replace(succPreds.begin(), succPreds.end(), callBlock, returnBlockPtr);

    // Patch Phis in successors
    for (auto &inst : succ->getInstructionsMut()) {
      if (auto *phi = llvm::dyn_cast<PhiInst>(inst.get())) {
        for (auto &[val, incBlock] : phi->getIncomingMut()) {
          if (incBlock == callBlock)
            incBlock = returnBlockPtr;
        }
      } else {
        break; // Phis are always at the top
      }
    }
  }
  callBlock->getSuccessors().clear();

  // 6. Connect Caller to Callee Entry
  MIRBlock *calleeEntry = blockMap[callee->getEntryBlock()];
  std::unique_ptr<MIRInst> savedCall = std::move(callBlockInsts.back());
  callBlockInsts.pop_back();
  callBlock->addInstruction(
      std::make_unique<BranchInst>(calleeEntry, call->getLoc()));
  callBlock->addSuccessor(calleeEntry);
  calleeEntry->addPredecessor(callBlock);

  // 7. Handle Returns & Connect back to the Caller Return Block
  MIRValue *returnValue = nullptr;

  if (!returns.empty()) {
    const hir::HIRType *retTy = call->getType();
    bool hasReturnValue = retTy && retTy->getKind() != hir::TypeKind::Void;

    if (hasReturnValue) {
      auto phi =
          std::make_unique<PhiInst>(retTy, "inline.ret", returns[0]->getLoc());

      for (ReturnInst *ret : returns) {
        MIRBlock *retBlock = ret->getParent();
        SourceLocation loc = ret->getLoc(); // [FIX] Extract location early

        if (ret->getReturnValue()) {
          MIRValue *retVal = ret->getReturnValue();

          if (retVal->getType() != retTy) {
            if (retVal->getType()->toString() != retTy->toString()) {
              auto castInst = std::make_unique<CastInst>(
                  Opcode::BitCast, retVal, retTy, "inline.ret.cast", loc);
              castInst->setParent(retBlock);
              retVal = castInst.get();
              auto &retInsts = retBlock->getInstructionsMut();
              retInsts.insert(retInsts.end() - 1, std::move(castInst));
            }
          }
          phi->addIncoming(retVal, retBlock);
        }

        auto &retInsts = retBlock->getInstructionsMut();
        retInsts.pop_back(); // Safely pop
        auto brInst = std::make_unique<BranchInst>(returnBlockPtr, loc);
        brInst->setParent(retBlock);
        retInsts.push_back(std::move(brInst));

        retBlock->addSuccessor(returnBlockPtr);
        returnBlockPtr->addPredecessor(retBlock);
      }
      phi->setParent(returnBlockPtr);
      returnValue = phi.get();
      returnBlockPtr->getInstructionsMut().insert(
          returnBlockPtr->getInstructionsMut().begin(), std::move(phi));

    } else {
      for (ReturnInst *ret : returns) {
        MIRBlock *retBlock = ret->getParent();
        SourceLocation loc = ret->getLoc(); // [FIX] Extract location early

        auto &retInsts = retBlock->getInstructionsMut();
        retInsts.pop_back(); // Safely pop

        auto brInst = std::make_unique<BranchInst>(returnBlockPtr, loc);
        brInst->setParent(retBlock);
        retInsts.push_back(std::move(brInst));

        retBlock->addSuccessor(returnBlockPtr);
        returnBlockPtr->addPredecessor(retBlock);
      }
    }
  }

  // Add the newly created return block to the function
  caller->addBlock(std::move(returnBlock));

  // 9. Replace uses of the call and delete it
  if (returnValue) {
    replaceAllUsesInFunction(caller, call, returnValue);
  } else if (call->getType() &&
             call->getType()->getKind() != hir::TypeKind::Void) {
    MIRValue *undef = M.getOrInsertConstant<ConstantUndef>(call->getType());
    replaceAllUsesInFunction(caller, call, undef);
  }

  return true;
}

// ----------------------------------------------------------------------------
// INVOKE INLINING (Similar block-splitting, but handles unwind dests)
// ----------------------------------------------------------------------------
bool InliningPass::inlineInvoke(
    MIRFunction *caller, MIRBlock *callBlock,
    std::vector<std::unique_ptr<MIRInst>>::iterator &callIt, InvokeInst *invoke,
    MIRModule &M) {

  MIRFunction *callee = llvm::cast<MIRFunction>(invoke->getCallee());

  std::unordered_map<MIRValue *, MIRValue *> valueMap;
  std::unordered_map<MIRBlock *, MIRBlock *> blockMap;

  // 1. Map Arguments
  size_t argCount =
      std::min(invoke->getArgs().size(), callee->getRawArguments().size());
  for (size_t i = 0; i < argCount; ++i) {
    valueMap[callee->getRawArguments()[i]] = invoke->getArgs()[i];
  }

  // 2. Clone Blocks
  std::vector<MIRBlock *> clonedBlocks;
  for (const auto &oldBlockPtr : callee->getBlocks()) {
    std::string newName =
        caller->getUniqueName(callee->getName() + "." + oldBlockPtr->getName());
    auto newBlock = std::make_unique<MIRBlock>(newName, caller);
    blockMap[oldBlockPtr.get()] = newBlock.get();
    clonedBlocks.push_back(newBlock.get());
    caller->addBlock(std::move(newBlock));
  }

  // 3. Clone Instructions
  std::vector<MIRInst *> allClonedInsts;
  std::vector<ReturnInst *> returns;
  std::vector<ThrowInst *> throws;
  std::vector<ResumeInst *> resumes;

  for (const auto &oldBlockPtr : callee->getBlocks()) {
    MIRBlock *newBlock = blockMap[oldBlockPtr.get()];
    for (const auto &oldInstPtr : oldBlockPtr->getInstructions()) {
      auto clonedInst = oldInstPtr->clone();
      clonedInst->setParent(newBlock);
      clonedInst->setBorrowKind(oldInstPtr->getBorrowKind());

      valueMap[oldInstPtr.get()] = clonedInst.get();
      allClonedInsts.push_back(clonedInst.get());

      if (auto *retInst = llvm::dyn_cast<ReturnInst>(clonedInst.get())) {
        returns.push_back(retInst);
      } else if (auto *throwInst =
                     llvm::dyn_cast<ThrowInst>(clonedInst.get())) {
        throws.push_back(throwInst);
      } else if (auto *resumeInst =
                     llvm::dyn_cast<ResumeInst>(clonedInst.get())) {
        resumes.push_back(resumeInst);
      }
      newBlock->getInstructionsMut().push_back(std::move(clonedInst));
    }
  }

  // 4. Remap Operands
  for (MIRInst *inst : allClonedInsts) {
    // Ask the instruction to replace any matching old values with the new ones
    for (auto &[oldVal, newVal] : valueMap) {
      inst->replaceOperand(oldVal, newVal);
    }
    // Do the same for block targets
    for (auto &[oldBlock, newBlock] : blockMap) {
      inst->replaceOperand(oldBlock, newBlock);
    }
    // Explicitly update Phi Nodes and Terminator block targets
    if (auto *phi = llvm::dyn_cast<PhiInst>(inst)) {
      for (auto &[val, incomingBlock] : phi->getIncomingMut()) {
        if (blockMap.count(incomingBlock)) {
          incomingBlock = blockMap[incomingBlock];
        }
      }
    } else if (auto *br = llvm::dyn_cast<BranchInst>(inst)) {
      if (blockMap.count(br->getTarget())) {
        br->setTarget(blockMap[br->getTarget()]);
      }
    } else if (auto *condBr = llvm::dyn_cast<CondBranchInst>(inst)) {
      if (blockMap.count(condBr->getTrueBlock())) {
        condBr->setTrueBlock(blockMap[condBr->getTrueBlock()]);
      }
      if (blockMap.count(condBr->getFalseBlock())) {
        condBr->setFalseBlock(blockMap[condBr->getFalseBlock()]);
      }
    } else if (auto *sw = llvm::dyn_cast<SwitchInst>(inst)) {
      if (blockMap.count(sw->getDefaultBlock())) {
        sw->setDefaultBlock(blockMap[sw->getDefaultBlock()]);
      }
      // Note: adjust the setter/getter here to match your exact SwitchInst API
      for (auto &casePair : sw->getCasesMut()) {
        if (blockMap.count(casePair.second)) {
          casePair.second = blockMap[casePair.second];
        }
      }
    } else if (auto *inv = llvm::dyn_cast<InvokeInst>(inst)) {
      if (blockMap.count(inv->getNormalDest())) {
        inv->setNormalDest(blockMap[inv->getNormalDest()]);
      }
      if (inv->getUnwindDest() && blockMap.count(inv->getUnwindDest())) {
        inv->setUnwindDest(blockMap[inv->getUnwindDest()]);
      }
    } else if (auto *throwInst = llvm::dyn_cast<ThrowInst>(inst)) {
      if (throwInst->getUnwindDest() &&
          blockMap.count(throwInst->getUnwindDest())) {
        throwInst->setUnwindDest(blockMap[throwInst->getUnwindDest()]);
      }
    }
  }

  // 4.5 Rebuild Internal CFG Edges
  for (MIRBlock *newBlock : clonedBlocks) {
    if (newBlock->getInstructions().empty())
      continue;
    MIRInst *term = newBlock->getInstructions().back().get();

    if (auto *br = llvm::dyn_cast<BranchInst>(term)) {
      newBlock->addSuccessor(br->getTarget());
      br->getTarget()->addPredecessor(newBlock);
    } else if (auto *condBr = llvm::dyn_cast<CondBranchInst>(term)) {
      newBlock->addSuccessor(condBr->getTrueBlock());
      condBr->getTrueBlock()->addPredecessor(newBlock);
      newBlock->addSuccessor(condBr->getFalseBlock());
      condBr->getFalseBlock()->addPredecessor(newBlock);
    } else if (auto *invoke = llvm::dyn_cast<InvokeInst>(term)) {
      newBlock->addSuccessor(invoke->getNormalDest());
      invoke->getNormalDest()->addPredecessor(newBlock);
      if (invoke->getUnwindDest()) {
        newBlock->addSuccessor(invoke->getUnwindDest());
        invoke->getUnwindDest()->addPredecessor(newBlock);
      }
    } else if (auto *throwInst = llvm::dyn_cast<ThrowInst>(term)) {
      if (throwInst->getUnwindDest()) {
        newBlock->addSuccessor(throwInst->getUnwindDest());
        throwInst->getUnwindDest()->addPredecessor(newBlock);
      }
    } else if (auto *switchInst = llvm::dyn_cast<SwitchInst>(term)) {
      newBlock->addSuccessor(switchInst->getDefaultBlock());
      switchInst->getDefaultBlock()->addPredecessor(newBlock);
      for (auto &casePair : switchInst->getCases()) {
        newBlock->addSuccessor(casePair.second);
        casePair.second->addPredecessor(newBlock);
      }
    }
  }

  MIRBlock *normalDest = invoke->getNormalDest();
  MIRBlock *unwindDest = invoke->getUnwindDest();

  // 5. Connect Caller to Callee Entry
  MIRBlock *calleeEntry = blockMap[callee->getEntryBlock()];
  auto &callBlockInsts = callBlock->getInstructionsMut();
  std::unique_ptr<MIRInst> savedInvoke = std::move(callBlockInsts.back());
  callBlockInsts.pop_back();
  auto brInst = std::make_unique<BranchInst>(calleeEntry, invoke->getLoc());
  brInst->setParent(callBlock);
  callBlockInsts.push_back(std::move(brInst));

  // Fix outgoing CFG
  callBlock->removeSuccessor(normalDest);
  callBlock->removeSuccessor(unwindDest);
  normalDest->removePredecessor(callBlock);
  unwindDest->removePredecessor(callBlock);

  callBlock->addSuccessor(calleeEntry);
  calleeEntry->addPredecessor(callBlock);

  // 6. Handle Returns -> Route to Normal Dest
  MIRValue *returnValue = nullptr;
  if (!returns.empty()) {
    const hir::HIRType *retTy = invoke->getType();
    if (retTy && retTy->getKind() != hir::TypeKind::Void) {
      auto phi =
          std::make_unique<PhiInst>(retTy, "invoke.ret", returns[0]->getLoc());
      for (ReturnInst *ret : returns) {
        MIRBlock *retBlock = ret->getParent();
        SourceLocation loc = ret->getLoc();
        if (ret->getReturnValue()) {
          MIRValue *retVal = ret->getReturnValue();

          if (retVal->getType() != retTy) {
            if (retVal->getType()->toString() != retTy->toString()) {
              auto castInst = std::make_unique<CastInst>(
                  Opcode::BitCast, retVal, retTy, "inline.ret.cast", loc);
              castInst->setParent(retBlock);
              retVal = castInst.get();
              auto &retInsts = retBlock->getInstructionsMut();
              // Insert the cast right before the ReturnInst
              retInsts.insert(retInsts.end() - 1, std::move(castInst));
            }
          }

          phi->addIncoming(retVal, retBlock);
        }

        auto &retInsts = retBlock->getInstructionsMut();
        retInsts.pop_back(); // Safely pop

        auto brInst = std::make_unique<BranchInst>(normalDest, loc);
        brInst->setParent(retBlock);
        retInsts.push_back(std::move(brInst));

        retBlock->addSuccessor(normalDest);
        normalDest->addPredecessor(retBlock);
      }
      phi->setParent(normalDest);
      returnValue = phi.get();
      normalDest->getInstructionsMut().insert(
          normalDest->getInstructionsMut().begin(), std::move(phi));
    } else {
      for (ReturnInst *ret : returns) {
        MIRBlock *retBlock = ret->getParent();
        SourceLocation loc = ret->getLoc();
        auto &retInsts = retBlock->getInstructionsMut();
        retInsts.pop_back();
        auto brInst = std::make_unique<BranchInst>(normalDest, loc);
        brInst->setParent(retBlock);
        retInsts.push_back(std::move(brInst));
        retBlock->addSuccessor(normalDest);
        normalDest->addPredecessor(retBlock);
      }
    }
  } else {
    auto &retInsts = normalDest->getInstructionsMut();
    if (retInsts.empty() ||
        !llvm::isa<UnreachableInst>(retInsts.back().get())) {
      auto unreachInst = std::make_unique<UnreachableInst>(invoke->getLoc());
      unreachInst->setParent(normalDest);
      retInsts.insert(retInsts.begin(), std::move(unreachInst));
    }
  }

  // 7. Handle Throws & Resumes -> Route to Unwind Dest
  std::vector<MIRBlock *> newUnwindSources;

  for (ThrowInst *throwInst : throws) {
    // [FIX 2A]: Only hijack throws that DON'T have a local cleanup block!
    if (throwInst->getUnwindDest() == nullptr) {
      MIRBlock *throwBlock = throwInst->getParent();
      throwInst->setUnwindDest(unwindDest);
      throwBlock->addSuccessor(unwindDest);
      unwindDest->addPredecessor(throwBlock);
    }
  }

  MIRBlock *phiPatchBlock = unwindDest;

  if (!resumes.empty()) {
    MIRBlock *actualCleanupBlock = unwindDest;
    PhiInst *exPhi = nullptr;

    // [FIX 2B]: LLVM strictly forbids branching into a block that begins with a
    // LandingPadInst. If the unwindDest has a landing pad, we MUST split the
    // block and branch AFTER it!
    if (!unwindDest->getInstructions().empty() &&
        llvm::isa<LandingPadInst>(
            unwindDest->getInstructions().front().get())) {

      auto newBlock = std::make_unique<MIRBlock>(
          caller->getUniqueName("inline.cleanup"), caller);
      actualCleanupBlock = newBlock.get();

      auto &unwindInsts = unwindDest->getInstructionsMut();

      // [CRITICAL MISSING LINE ADDED HERE]:
      auto *callerLpad = unwindInsts.front().get();

      // Move all instructions AFTER the landing pad to actualCleanupBlock
      auto splitIt = unwindInsts.begin() + 1;
      for (auto it = splitIt; it != unwindInsts.end(); ++it) {
        (*it)->setParent(actualCleanupBlock);
        actualCleanupBlock->addInstruction(std::move(*it));
      }
      unwindInsts.erase(splitIt, unwindInsts.end());

      // Insert the physical branch terminator to connect the split blocks!
      auto brFallback = std::make_unique<BranchInst>(
          actualCleanupBlock, unwindInsts.front()->getLoc());
      brFallback->setParent(unwindDest);
      unwindInsts.push_back(std::move(brFallback));

      // Update CFG Topologies
      actualCleanupBlock->getSuccessors() = unwindDest->getSuccessors();
      for (MIRBlock *succ : actualCleanupBlock->getSuccessors()) {
        auto &preds = succ->getPredecessors();
        std::replace(preds.begin(), preds.end(), unwindDest,
                     actualCleanupBlock);

        // Patch Phis in successors
        for (auto &inst : succ->getInstructionsMut()) {
          if (auto *phi = llvm::dyn_cast<PhiInst>(inst.get())) {
            for (auto &[val, incBlock] : phi->getIncomingMut()) {
              if (incBlock == unwindDest)
                incBlock = actualCleanupBlock;
            }
          } else
            break;
        }
      }
      unwindDest->getSuccessors().clear();
      unwindDest->addSuccessor(actualCleanupBlock);
      actualCleanupBlock->addPredecessor(unwindDest);

      // --- NEW: Exception Threading Fix ---
      // Create a Phi node to merge the caller's landing pad and the callee's
      // resumes
      auto phiNode = std::make_unique<PhiInst>(callerLpad->getType(), "ex.phi",
                                               callerLpad->getLoc());
      phiNode->addIncoming(callerLpad, unwindDest);
      exPhi = phiNode.get();

      // Replace uses of the old LandingPad with the new Phi in the cleanup
      // block
      for (auto &inst : actualCleanupBlock->getInstructionsMut()) {
        inst->replaceOperand(callerLpad, exPhi);
      }

      // Insert the Phi at the very top of the cleanup block
      exPhi->setParent(actualCleanupBlock);
      actualCleanupBlock->getInstructionsMut().insert(
          actualCleanupBlock->getInstructionsMut().begin(), std::move(phiNode));

      caller->addBlock(std::move(newBlock));
    }

    phiPatchBlock = actualCleanupBlock;

    // Route resumes safely around the landing pad!
    for (ResumeInst *resumeInst : resumes) {
      MIRBlock *resumeBlock = resumeInst->getParent();

      // [FIX] Extract all data BEFORE destroying the instruction
      SourceLocation loc = resumeInst->getLoc();
      MIRValue *exVal = resumeInst->getException();

      auto &insts = resumeBlock->getInstructionsMut();
      insts.pop_back(); // Now it's safe to pop!

      auto brInst = std::make_unique<BranchInst>(actualCleanupBlock, loc);
      brInst->setParent(resumeBlock);
      insts.push_back(std::move(brInst));

      resumeBlock->addSuccessor(actualCleanupBlock);
      actualCleanupBlock->addPredecessor(resumeBlock);
      newUnwindSources.push_back(resumeBlock);

      // Feed the callee's exception into the Phi if we created one
      if (exPhi) {
        exPhi->addIncoming(exVal,
                           resumeBlock); // Using the safely extracted value
      }
    }
  }

  if (returnValue) {
    replaceAllUsesInFunction(caller, invoke, returnValue);
  } else if (invoke->getType() &&
             invoke->getType()->getKind() != hir::TypeKind::Void) {
    MIRValue *undef = M.getOrInsertConstant<ConstantUndef>(invoke->getType());
    replaceAllUsesInFunction(caller, invoke, undef);
  }

  // 8. Patch existing Phis in normalDest
  for (auto &inst : normalDest->getInstructionsMut()) {
    if (auto *phi = llvm::dyn_cast<PhiInst>(inst.get())) {
      if (phi == returnValue)
        continue;

      MIRValue *valFromCallBlock = nullptr;
      for (auto &[val, incBlock] : phi->getIncoming()) {
        if (incBlock == callBlock) {
          valFromCallBlock = val;
          break;
        }
      }

      if (valFromCallBlock) {
        phi->removeIncoming(callBlock);
        for (ReturnInst *ret : returns) {
          phi->addIncoming(valFromCallBlock, ret->getParent());
        }
      }
    } else {
      break;
    }
  }

  // 9. Patch existing Phis in phiPatchBlock
  for (auto &inst : phiPatchBlock->getInstructionsMut()) {
    if (auto *phi = llvm::dyn_cast<PhiInst>(inst.get())) {
      MIRValue *valFromCallBlock = nullptr;
      for (auto &[val, incBlock] : phi->getIncoming()) {
        if (incBlock == callBlock) {
          valFromCallBlock = val;
          break;
        }
      }

      if (valFromCallBlock) {
        phi->removeIncoming(callBlock);
        // [FIX 2C]: Wire the Phis to the new sources (Throws AND Resumes)
        for (MIRBlock *srcBlock : newUnwindSources) {
          phi->addIncoming(valFromCallBlock, srcBlock);
        }
      }
    } else {
      break;
    }
  }

  return true;
}

} // namespace mir
} // namespace moksha
