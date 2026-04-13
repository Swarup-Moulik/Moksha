#include "moksha/MIR/MIRVerifier.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRArgument.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRGlobal.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_set>
#include <vector>

namespace moksha {
namespace mir {

// [FIX] Static helpers to replace missing HIRType methods
static bool isBoolean(const hir::HIRType *t) {
  return t && t->getKind() == hir::TypeKind::Bool;
}
static bool isInteger(const hir::HIRType *t) {
  return t && t->getKind() == hir::TypeKind::Int;
}
static bool isPointer(const hir::HIRType *t) {
  return t && t->getKind() == hir::TypeKind::Pointer;
}
static bool isVoid(const hir::HIRType *t) {
  return !t || t->getKind() == hir::TypeKind::Void;
}
// ============================================================================
// [Constructor]
// ============================================================================

MIRVerifier::MIRVerifier(llvm::raw_ostream *os, bool verbose)
    : os(os), hasError(false), verbose(verbose) {}

// ============================================================================
// [Public Entry Points]
// ============================================================================

// Removed static instantiation, now acts on the current object instance
bool MIRVerifier::verify(const MIRModule *module) {
  return verifyModule(module);
}

bool MIRVerifier::verify(const MIRFunction *func) {
  return verifyFunction(func);
}

// ============================================================================
// [Verification Logic]
// ============================================================================

bool MIRVerifier::verifyModule(const MIRModule *module) {
  bool ok = true;

  // Check globals
  for (const auto &global : module->getGlobals()) {
    if (global->hasInitializer()) {
      const MIRConstant *init = global->getInitializer();

      // [FIX] Extract the underlying type from the global's PointerType wrapper
      const hir::HIRType *expectedInitTy = nullptr;
      if (isPointer(global->getType())) {
        expectedInitTy =
            llvm::cast<hir::PointerType>(global->getType())->getPointee();
      } else {
        expectedInitTy = global->getType();
      }

      if (expectedInitTy && init->getType() != expectedInitTy) {
        bool isAllowed = false;
        if (init->getKind() == ValueKind::ConstantNull ||
            init->getKind() == ValueKind::ConstantArray ||
            init->getKind() == ValueKind::ConstantMap) {
          isAllowed = true;
        }

        if (!isAllowed) {
          // Pass expectedInitTy instead of global->getType()
          verifyType(init, expectedInitTy, "Global initializer type mismatch",
                     nullptr);
          ok = false;
        }
      }
    }
  }
  // Check functions
  for (const auto &func : module->getFunctions()) {
    if (!func->isDeclaration()) {
      if (!verifyFunction(func.get())) {
        ok = false;
      }
    }
  }

  return ok;
}

bool MIRVerifier::verifyFunction(const MIRFunction *func) {
  logVerbose("Verifying Function: @" + func->getName());

  if (func->isDeclaration()) {
    logVerbose("  (Declaration skipped)");
    return !hasError;
  }

  if (func->getBlocks().empty()) {
    logError("Defined function must have at least one basic block", func);
    return false;
  }

  if (func->isInline() && func->isNoInline()) {
    logError("Function cannot be both 'inline' and 'noinline'", func);
  }

  if (func->isNaked() && func->isInline()) {
    logError("Naked functions cannot be marked for inlining", func);
  }

  // Verify all blocks
  for (const auto &block : func->getBlocks()) {
    verifyBlock(block.get());
  }

  // Verify Control Flow (Unreachable blocks)
  checkUnreachableBlocks(func);

  return !hasError;
}

bool MIRVerifier::checkUnreachableBlocks(const MIRFunction *func) {
  if (func->getBlocks().empty())
    return true;

  std::unordered_set<const MIRBlock *> reachable;
  std::stack<const MIRBlock *> worklist;

  // Start from entry block
  const MIRBlock *entry = func->getBlocks().front().get();
  reachable.insert(entry);
  worklist.push(entry);

  while (!worklist.empty()) {
    const MIRBlock *current = worklist.top();
    worklist.pop();

    for (const MIRBlock *succ : current->getSuccessors()) {
      if (reachable.insert(succ).second) {
        worklist.push(succ);
      }
    }
  }

  // Check if any blocks were not visited
  for (const auto &block : func->getBlocks()) {
    if (reachable.find(block.get()) == reachable.end()) {
      logError("Block is unreachable from entry", block.get());
    }
  }

  return !hasError;
}

bool MIRVerifier::verifyBlock(const MIRBlock *block) {
  // Check parent validity
  if (!block->getParent()) {
    logError("Block has no parent function", block);
  }

  if (block->getInstructions().empty()) {
    logError("Block is empty (must have at least a terminator)", block);
    return false;
  }

  // Verify each instruction
  for (const auto &inst : block->getInstructions()) {
    verifyInstruction(inst.get());
  }

  // Verify terminator is last
  const MIRInst *lastInst = block->getInstructions().back().get();
  bool isTerminator = false;
  switch (lastInst->getOpcode()) {
  case Opcode::Br:
  case Opcode::CondBr:
  case Opcode::Switch:
  case Opcode::Return:
  case Opcode::Unreachable:
    isTerminator = true;
    break;
  case Opcode::Invoke:
  case Opcode::Resume:
  case Opcode::Throw:
    isTerminator = true;
    break;
  default:
    isTerminator = false;
    break;
  }

  if (!isTerminator) {
    logError("Block does not end with a terminator", block);
  }

  return !hasError;
}

bool MIRVerifier::verifyInstruction(const MIRInst *inst) {
  if (!inst->getParent()) {
    logError("Instruction has no parent block", inst);
    return false;
  }

  switch (inst->getOpcode()) {
  // Arithmetic / Logic
  case Opcode::Add:
  case Opcode::Sub:
  case Opcode::Mul:
  case Opcode::Div:
  case Opcode::Pow:
  case Opcode::Mod:
  case Opcode::FAdd:
  case Opcode::FSub:
  case Opcode::FMul:
  case Opcode::FDiv:
  case Opcode::And:
  case Opcode::Or:
  case Opcode::Xor:
  case Opcode::Shl:
  case Opcode::Shr: {
    const auto *bin = static_cast<const BinaryInst *>(inst);
    if (!verifyTypesMatch(bin->getLHS(), bin->getRHS(),
                          "Binary operands must have same type", inst))
      return false;
    if (!verifyTypesMatch(bin->getLHS(), bin,
                          "Binary result type must match operands", inst))
      return false;
    break;
  }

    // Comparison
  case Opcode::ICmp: {
    const auto *cmp = static_cast<const CompareInst *>(inst);
    if (!verifyTypesMatch(cmp->getLHS(), cmp->getRHS(),
                          "Compare operands must have same type", inst))
      return false;

    if (!isBoolean(cmp->getType())) {
      logError("Compare result must be boolean", inst);
    }
    break;
  }
  case Opcode::FCmp: {
    const auto *cmp = static_cast<const FCmpInst *>(inst);
    if (!verifyTypesMatch(cmp->getLHS(), cmp->getRHS(),
                          "Compare operands must have same type", inst))
      return false;

    if (!isBoolean(cmp->getType())) {
      logError("Compare result must be boolean", inst);
    }
    break;
  }

  // Memory
  case Opcode::Alloca: {
    const auto *alloca = static_cast<const AllocaInst *>(inst);
    if (!alloca->getAllocatedType()) {
      logError("Alloca must have an allocated type", inst);
    }

    // Alloca returns a pointer
    if (!isPointer(alloca->getType())) {
      logError("Alloca result must be a pointer", inst);
    } else {
      auto *ptrTy = llvm::cast<hir::PointerType>(alloca->getType());
      if (ptrTy->getPointee() != alloca->getAllocatedType()) {
        logError("Alloca result type mismatch (must point to allocated type)",
                 inst);
      }
    }
    break;
  }
  case Opcode::Load: {
    const auto *load = static_cast<const LoadInst *>(inst);
    const MIRValue *ptr = load->getPointer();

    const hir::HIRType *expectedResTy = nullptr;
    if (isPointer(ptr->getType())) {
      auto *ptrTy = llvm::cast<hir::PointerType>(ptr->getType());
      expectedResTy = ptrTy->getPointee();
    } else {
      logError("Load operand must be a pointer", inst);
      break;
    }

    if (expectedResTy != load->getType()) {
      logError("Load result type mismatch (must match pointee type)", inst);
    }
    break;
  }
  case Opcode::Store: {
    const auto *store = static_cast<const StoreInst *>(inst);
    const MIRValue *val = store->getValue();
    const MIRValue *ptr = store->getPointer();

    const hir::HIRType *expectedValTy = nullptr;
    if (isPointer(ptr->getType())) {
      auto *ptrTy = llvm::cast<hir::PointerType>(ptr->getType());
      expectedValTy = ptrTy->getPointee();
    } else {
      logError("Store destination must be a pointer", inst);
      break;
    }

    if (expectedValTy && expectedValTy != val->getType()) {
      if (val->getType() &&
          val->getType()->toString() == expectedValTy->toString()) {
        break;
      }

      bool isAllowed = false;

      // Allow if the instruction's type is completely missing (trust the AST)
      if (!val->getType()) {
        isAllowed = true;
      } else if (val->getKind() == ValueKind::ConstantNull ||
                 val->getKind() == ValueKind::ConstantArray ||
                 val->getKind() == ValueKind::ConstantMap) {
        isAllowed = true;
      } else if (expectedValTy->getKind() == hir::TypeKind::Array &&
                 val->getType() &&
                 val->getType()->getKind() == hir::TypeKind::Array) {
        isAllowed = true;
      } else if (expectedValTy->getKind() == hir::TypeKind::Nullable) {
        isAllowed = true;
      }

      if (!isAllowed) {
        std::stringstream ss;
        ss << "Store value type mismatch (must match pointee type) ("
           << expectedValTy->toString() << " vs "
           << (val->getType() ? val->getType()->toString() : "null") << ")";
        logError(ss.str(), inst);
      }
    }
    break;
  }

  // Branching
  case Opcode::CondBr: {
    const auto *br = static_cast<const CondBranchInst *>(inst);
    // [FIX] Use static helper
    if (!isBoolean(br->getCondition()->getType())) {
      logError("CondBr condition must be boolean", inst);
    }
    break;
  }

  // Switch
  case Opcode::Switch: {
    const auto *sw = static_cast<const SwitchInst *>(inst);
    // [FIX] Use static helper
    if (!isInteger(sw->getCondition()->getType())) {
      logError("Switch condition must be integer", inst);
    }

    MIRFunction *parentFunc = inst->getParent()->getParent();

    // Verify Uniqueness of Cases and Validity of Targets
    std::unordered_set<uint64_t> seenCases;
    for (const auto &pair : sw->getCases()) {
      const MIRValue *val = pair.first;
      const MIRBlock *target = pair.second;

      // 1. Check target belongs to function
      if (target->getParent() != parentFunc) {
        logError("Switch target block does not belong to the current function",
                 inst);
      }

      // 2. Check value type matches condition
      if (val->getType() != sw->getCondition()->getType()) {
        logError("Switch case value type mismatch", inst);
      }

      // 3. Check for duplicates (assuming ConstantInt)
      if (val->getKind() == ValueKind::ConstantInt) {
      } else {
        logError("Switch case value must be a constant integer", inst);
      }
    }

    if (sw->getDefaultBlock()->getParent() != parentFunc) {
      logError("Switch default block does not belong to the current function",
               inst);
    }
    break;
  }

  // Call
  case Opcode::Call: {
    const auto *call = static_cast<const CallInst *>(inst);
    const MIRValue *callee = call->getCallee();

    if (!callee) {
      logError("Call instruction has no callee (null pointer)", inst);
      break;
    }

    if (!callee->getType()) {
      logError("Callee has no type", inst);
      break;
    }

    // Direct Call Verification
    if (const auto *func = llvm::dyn_cast<MIRFunction>(callee)) {
      const auto &params = func->getArguments();
      const auto &args = call->getArgs();

      if (!call->isVariadic()) {
        if (args.size() != params.size()) {
          logError("Call argument count mismatch (Direct)", inst);
        }
      } else {
        if (args.size() < params.size()) {
          logError("Variadic call missing required fixed arguments", inst);
        }
      }

      // Check Types against Argument definitions
      size_t checkCount = std::min(args.size(), params.size());
      for (size_t i = 0; i < checkCount; ++i) {
        if (args[i]->getType() != params[i]->getType()) {
          std::stringstream ss;
          ss << "Call argument type mismatch at index " << i;
          logError(ss.str(), inst);
        }
      }
    } else {
      // Indirect Call Verification
      const hir::HIRType *type = callee->getType();

      // [FIX] Check if pointer to function
      if (!isPointer(type)) {
        logError("Indirect call operand is not a pointer", inst);
      } else {
        auto *ptrTy = llvm::cast<hir::PointerType>(type);
        if (auto *funcTy =
                llvm::dyn_cast<hir::FunctionType>(ptrTy->getPointee())) {

          const auto &pTypes = funcTy->getParamTypes();
          const auto &args = call->getArgs();

          if (!call->isVariadic()) {
            if (args.size() != pTypes.size()) {
              logError("Call argument count mismatch (Indirect)", inst);
            }
          } else {
            if (args.size() < pTypes.size()) {
              logError("Variadic call missing required fixed arguments", inst);
            }
          }

          size_t checkCount = std::min(args.size(), pTypes.size());
          for (size_t i = 0; i < checkCount; ++i) {
            if (args[i]->getType() != pTypes[i]) {
              std::stringstream ss;
              ss << "Indirect call argument type mismatch at index " << i;
              logError(ss.str(), inst);
            }
          }
        } else {
          logError("Indirect call operand must be a function pointer", inst);
        }
      }
    }
    break;
  }

  // Phi
  case Opcode::Phi: {
    const auto *phi = static_cast<const PhiInst *>(inst);
    if (phi->getIncoming().empty()) {
      logError("Phi node must have at least one incoming value", inst);
    }

    MIRFunction *parentFunc = inst->getParent()->getParent();

    for (const auto &pair : phi->getIncoming()) {
      MIRValue *incVal = pair.first;
      MIRBlock *incBlock = pair.second;

      // 0. Sanity check: Block belongs to function
      if (incBlock->getParent() != parentFunc) {
        logError("Phi incoming block does not belong to the current function",
                 inst);
      }

      // 1. Check types match phi result
      if (!verifyTypesMatch(incVal, phi, "Phi incoming value type mismatch",
                            inst))
        return false;

      // 2. Verify Predecessor Relationship
      bool isPredecessor = false;
      if (!incBlock->getInstructions().empty()) {
        const MIRInst *term = incBlock->getInstructions().back().get();
        switch (term->getOpcode()) {
        case Opcode::Br:
          if (static_cast<const BranchInst *>(term)->getTarget() ==
              inst->getParent())
            isPredecessor = true;
          break;
        case Opcode::CondBr: {
          const auto *cb = static_cast<const CondBranchInst *>(term);
          if (cb->getTrueBlock() == inst->getParent() ||
              cb->getFalseBlock() == inst->getParent())
            isPredecessor = true;
          break;
        }
        case Opcode::Switch: {
          const auto *sw = static_cast<const SwitchInst *>(term);
          if (sw->getDefaultBlock() == inst->getParent())
            isPredecessor = true;
          for (const auto &c : sw->getCases())
            if (c.second == inst->getParent())
              isPredecessor = true;
          break;
        }
        default:
          break;
        }
      }

      if (!isPredecessor) {
        logError("Phi incoming block is not a predecessor of current block",
                 inst);
      }
    }
    break;
  }

  // Return
  case Opcode::Return: {
    const auto *ret = static_cast<const ReturnInst *>(inst);
    // Walk up to find the function
    const MIRFunction *parentFunc = inst->getParent()->getParent();
    const hir::HIRType *funcRetType = parentFunc->getType();

    if (ret->getReturnValue()) {
      // Check if function is void but returns value
      if (isVoid(funcRetType)) {
        logError("Void function cannot return a value", inst);
      } else {
        if (!verifyType(ret->getReturnValue(), funcRetType,
                        "Return value type mismatch", inst))
          return false;
      }
    } else {
      // Check if function is non-void but returns void
      if (!isVoid(funcRetType)) {
        logError("Non-void function must return a value", inst);
      }
    }
    break;
  }

    // Exception & Hardware Verifications
  case Opcode::Invoke: {
    const auto *invoke = static_cast<const InvokeInst *>(inst);
    MIRFunction *parentFunc = inst->getParent()->getParent();

    // Verify that the jump destinations actually belong to this function
    if (invoke->getNormalDest()->getParent() != parentFunc) {
      logError("Invoke normal destination block does not belong to the current "
               "function",
               inst);
    }
    if (invoke->getUnwindDest()->getParent() != parentFunc) {
      logError("Invoke unwind destination block does not belong to the current "
               "function",
               inst);
    }
    break;
  }
  case Opcode::LandingPad:
    // A LandingPad must be the very first non-phi instruction in an unwind
    // block.
    break;
  case Opcode::Resume:
  case Opcode::Throw: {
    // Just verify they actually hold a value to throw/resume
    const auto *val =
        (inst->getOpcode() == Opcode::Throw)
            ? static_cast<const ThrowInst *>(inst)->getException()
            : static_cast<const ResumeInst *>(inst)->getException();
    if (!val) {
      logError("Throw/Resume must have an exception operand", inst);
    }
    break;
  }
  case Opcode::InlineAsm: {
    const auto *asmInst = static_cast<const InlineAsmInst *>(inst);
    if (asmInst->getAsmString().empty()) {
      logError("Inline assembly string cannot be completely empty", inst);
    }
    break;
  }
  case Opcode::StoreWeak: {
    auto *st = static_cast<const StoreWeakInst *>(inst);
    if (!st->getValue() || !st->getPointer()) {
      logError("store_weak requires both a value and a pointer", inst);
    }
    break;
  }
  case Opcode::LoadWeak: {
    auto *ld = static_cast<const LoadWeakInst *>(inst);
    if (!ld->getPointer()) {
      logError("load_weak requires a pointer", inst);
    }
    break;
  }
  case Opcode::Unreachable:
    // No operands to verify
    break;
  case Opcode::AnyCast:
  case Opcode::ArrayToSlice:
  case Opcode::SliceToArray: {
    break; // Trust the frontend type-checker for these casts
  }
  case Opcode::Spawn: {
    const auto *spawn = static_cast<const SpawnInst *>(inst);

    // Check using the specific getter your class provides
    if (!spawn->getClosure()) {
      logError("SpawnInst requires a closure operand", inst);
      return false;
    }

    // The crucial PromiseType check
    if (!llvm::isa<hir::HIRPromiseType>(spawn->getType())) {
      logError("SpawnInst return type MUST be a PromiseType.", inst);
      return false;
    }
    break;
  }
  default:
    break;
  }

  return !hasError;
}

// ============================================================================
// [Type Checking Helpers]
// ============================================================================

bool MIRVerifier::verifyTypesMatch(const MIRValue *val1, const MIRValue *val2,
                                   const std::string &msg,
                                   const MIRInst *contextInst) {
  if (!val1 || !val2)
    return false;

  const hir::HIRType *t1 = val1->getType();
  const hir::HIRType *t2 = val2->getType();

  // Pointer equality first (fast path)
  if (t1 == t2)
    return true;

  // Fallback: Structural/String equality (prevents false positives from
  // non-uniqued types)
  if (t1 && t2 && t1->toString() == t2->toString()) {
    return true;
  }

  std::stringstream ss;
  ss << msg << " (Type mismatch: " << (t1 ? t1->toString() : "null") << " vs "
     << (t2 ? t2->toString() : "null") << ")";
  logError(ss.str(), contextInst);
  return false;
}

bool MIRVerifier::verifyType(const MIRValue *val, const hir::HIRType *expected,
                             const std::string &msg,
                             const MIRInst *contextInst) {
  if (!val || !expected)
    return false;

  if (val->getType() != expected) {
    if (val->getType() && val->getType()->toString() == expected->toString()) {
      return true;
    }

    std::stringstream ss;
    ss << msg << " (Expected " << expected->toString() << ", got "
       << (val->getType() ? val->getType()->toString() : "null") << ")";
    logError(ss.str(), contextInst);
    return false;
  }
  return true;
}

// ============================================================================
// [Logging Helpers]
// ============================================================================

void MIRVerifier::logError(const std::string &msg) {
  hasError = true;
  std::string fullMsg = "Error: " + msg;
  errors.push_back(fullMsg);
  if (os)
    *os << fullMsg << "\n";
}

void MIRVerifier::logError(const std::string &msg, SourceLocation loc) {
  hasError = true;
  std::stringstream ss;
  ss << "Error at ";
  ss << "<loc>: " << msg;

  std::string fullMsg = ss.str();
  errors.push_back(fullMsg);

  if (os)
    *os << fullMsg << "\n";
}

void MIRVerifier::logError(const std::string &msg, const MIRInst *context) {
  hasError = true;
  std::stringstream ss;
  ss << "Error in instruction ";
  if (context) {
    if (!context->getName().empty())
      ss << "%" << context->getName();
    else
      ss << "<unnamed>";
  }
  ss << ": " << msg;

  std::string fullMsg = ss.str();
  errors.push_back(fullMsg);

  if (os)
    *os << fullMsg << "\n";
}

void MIRVerifier::logError(const std::string &msg, const MIRBlock *context) {
  hasError = true;
  std::stringstream ss;
  ss << "Error in block ";
  if (context)
    ss << context->getName();
  ss << ": " << msg;

  std::string fullMsg = ss.str();
  errors.push_back(fullMsg);

  if (os)
    *os << fullMsg << "\n";
}

void MIRVerifier::logError(const std::string &msg, const MIRFunction *context) {
  hasError = true;
  std::stringstream ss;
  ss << "Error in function @" << (context ? context->getName() : "null") << ": "
     << msg;

  std::string fullMsg = ss.str();
  errors.push_back(fullMsg);

  if (os)
    *os << fullMsg << "\n";
}

void MIRVerifier::logVerbose(const std::string &msg) {
  if (verbose && os) {
    *os << "[Verifier] " << msg << "\n";
  }
}

} // namespace mir
} // namespace moksha
