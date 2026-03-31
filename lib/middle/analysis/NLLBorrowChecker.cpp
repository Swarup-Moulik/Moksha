#include "moksha/MIR/Analysis/NLLBorrowChecker.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/MIR/MIRArgument.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRGlobal.h"
#include "moksha/MIR/MIRInst.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <cctype>
#include <queue>
#include <unordered_set>

namespace moksha {
namespace mir {

static MIRValue *LOCK_MARKER = reinterpret_cast<MIRValue *>(0x10C1C);

static bool isPointerType(MIRValue *val) {
  if (!val || !val->getType())
    return false;
  if (val->getBorrowKind() != BorrowKind::None)
    return true;

  auto kind = val->getType()->getKind();
  switch (kind) {
  case hir::TypeKind::Pointer:
  case hir::TypeKind::Reference:
  case hir::TypeKind::Closure:
    return true;
  default: {
    std::string name = val->getType()->toString();
    if (name.find("Closure") != std::string::npos ||
        name.find("closure") != std::string::npos)
      return true;
    return false;
  }
  }
}

static bool isMoveOnlyType(const hir::HIRType *type) {
  if (!type)
    return false;
  switch (type->getKind()) {
  case hir::TypeKind::Slice:
  case hir::TypeKind::String:
  case hir::TypeKind::Promise:
  case hir::TypeKind::Closure:
    return true;
  case hir::TypeKind::Pointer:
    if (auto *ptrTy = llvm::dyn_cast<hir::PointerType>(type)) {
      return ptrTy->getOwnership() == hir::Ownership::Owned;
    }
    return false;
  case hir::TypeKind::Struct:
    if (auto *structTy = llvm::dyn_cast<hir::StructType>(type)) {
      for (const hir::HIRType *fieldTy : structTy->getFields()) {
        if (isMoveOnlyType(fieldTy))
          return true;
      }
    }
    return false;
  case hir::TypeKind::Array:
    if (auto *arrTy = llvm::dyn_cast<hir::ArrayType>(type)) {
      return isMoveOnlyType(arrTy->getElementType());
    }
    return false;
  default:
    return false;
  }
}

static bool isMoveType(MIRValue *val) {
  if (!val || !val->getType())
    return false;
  if (val->getBorrowKind() != BorrowKind::None)
    return false;
  return isMoveOnlyType(val->getType());
}

static bool isExclusiveBorrow(MIRValue *pointerVal) {
  if (!pointerVal)
    return false;
  BorrowKind kind = pointerVal->getBorrowKind();
  if (kind == BorrowKind::Mut || kind == BorrowKind::Lock)
    return true;
  if (kind == BorrowKind::View)
    return false;

  if (pointerVal->getType()) {
    if (auto *ptrTy = llvm::dyn_cast<hir::PointerType>(pointerVal->getType())) {
      return ptrTy->isMut();
    } else if (auto *refTy =
                   llvm::dyn_cast<hir::ReferenceType>(pointerVal->getType())) {
      return refTy->isMut();
    }
  }
  return false;
}

bool Place::conflictsWith(const Place &other) const {
  if (base != other.base)
    return false;
  size_t minLen = std::min(projections.size(), other.projections.size());
  for (size_t i = 0; i < minLen; ++i) {
    if (projections[i] != other.projections[i])
      return false;
  }
  return true;
}

NLLBorrowChecker::NLLBorrowChecker(DiagnosticEngine &diags) : diags(diags) {}

void NLLBorrowChecker::checkModule(MIRModule *module) {
  for (auto &func : module->getFunctions())
    checkFunction(func.get());
}

void NLLBorrowChecker::checkFunction(MIRFunction *func) {
  if (func->isDeclaration())
    return;
  computeLiveness(func);
  computeDataflow(func);
  checkConflicts(func);
}

void NLLBorrowChecker::computeLiveness(MIRFunction *func) {
  lastUses.clear();
  for (auto &blockPtr : func->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructions()) {
      MIRInst *inst = instPtr.get();
      if (auto *load = llvm::dyn_cast<LoadInst>(inst)) {
        if (load->getPointer())
          lastUses[load->getPointer()] = inst;
      } else if (auto *store = llvm::dyn_cast<StoreInst>(inst)) {
        if (store->getValue())
          lastUses[store->getValue()] = inst;
        if (store->getPointer())
          lastUses[store->getPointer()] = inst;
      } else if (auto *gep = llvm::dyn_cast<GetElementPtrInst>(inst)) {
        if (gep->getPointer())
          lastUses[gep->getPointer()] = inst;
        for (auto *idx : gep->getIndices()) {
          if (idx)
            lastUses[idx] = inst;
        }
      } else if (auto *ext = llvm::dyn_cast<ExtractValueInst>(inst)) {
        if (ext->getAggregate())
          lastUses[ext->getAggregate()] = inst;
      } else if (auto *ins = llvm::dyn_cast<InsertValueInst>(inst)) {
        if (ins->getAggregate())
          lastUses[ins->getAggregate()] = inst;
        if (ins->getValue())
          lastUses[ins->getValue()] = inst;
      } else if (auto *call = llvm::dyn_cast<CallInst>(inst)) {
        if (call->getCallee() && !llvm::isa<MIRFunction>(call->getCallee()))
          lastUses[call->getCallee()] = inst;
        for (MIRValue *arg : call->getArgs()) {
          if (arg)
            lastUses[arg] = inst;
        }
      } else if (auto *ret = llvm::dyn_cast<ReturnInst>(inst)) {
        if (ret->getReturnValue())
          lastUses[ret->getReturnValue()] = inst;
      } else if (auto *condBr = llvm::dyn_cast<CondBranchInst>(inst)) {
        if (condBr->getCondition())
          lastUses[condBr->getCondition()] = inst;
      } else if (auto *phi = llvm::dyn_cast<PhiInst>(inst)) {
        for (auto const &[val, pred] : phi->getIncoming()) {
          if (val)
            lastUses[val] = inst;
        }
      } else if (auto *bin = llvm::dyn_cast<BinaryInst>(inst)) {
        if (bin->getLHS())
          lastUses[bin->getLHS()] = inst;
        if (bin->getRHS())
          lastUses[bin->getRHS()] = inst;
      } else if (auto *cast = llvm::dyn_cast<CastInst>(inst)) {
        if (cast->getValue())
          lastUses[cast->getValue()] = inst;
      } else if (auto *arc = llvm::dyn_cast<ARCInst>(inst)) {
        if (arc->getObject())
          lastUses[arc->getObject()] = inst;
      } else if (auto *await = llvm::dyn_cast<AwaitInst>(inst)) {
        if (await->getPromise())
          lastUses[await->getPromise()] = inst;
      } else if (auto *spawn = llvm::dyn_cast<SpawnInst>(inst)) {
        if (spawn->getClosure())
          lastUses[spawn->getClosure()] = inst;
      } else if (auto *makeClosure = llvm::dyn_cast<MakeClosureInst>(inst)) {
        for (MIRValue *cap : makeClosure->getCaptures()) {
          if (cap)
            lastUses[cap] = inst;
        }
      }
    }
  }
}

static bool loansEqual(const std::vector<Loan> &a, const std::vector<Loan> &b) {
  if (a.size() != b.size())
    return false;
  for (const auto &la : a) {
    if (std::find(b.begin(), b.end(), la) == b.end())
      return false;
  }
  return true;
}

std::vector<Loan> NLLBorrowChecker::mergeLoans(const std::vector<Loan> &a,
                                               const std::vector<Loan> &b) {
  std::vector<Loan> merged = a;
  for (const auto &lb : b) {
    if (std::find(merged.begin(), merged.end(), lb) == merged.end()) {
      merged.push_back(lb);
    }
  }
  return merged;
}

void NLLBorrowChecker::computeDataflow(MIRFunction *func) {
  blockIn.clear();
  blockOut.clear();
  activeLoansAtInst.clear();
  if (func->getBlocks().empty())
    return;

  std::unordered_map<MIRBlock *, std::vector<MIRBlock *>> preds;
  MIRBlock *lexicalPrev = nullptr;
  for (auto &blockPtr : func->getBlocks()) {
    MIRBlock *B = blockPtr.get();
    for (MIRBlock *p : B->getPredecessors()) {
      preds[B].push_back(p);
    }
    if (B != func->getEntryBlock() && preds[B].empty() && lexicalPrev) {
      preds[B].push_back(lexicalPrev);
    }
    lexicalPrev = B;
  }

  std::queue<MIRBlock *> worklist;
  std::unordered_set<MIRBlock *> inWorklist;

  for (auto &blockPtr : func->getBlocks()) {
    worklist.push(blockPtr.get());
    inWorklist.insert(blockPtr.get());
  }

  while (!worklist.empty()) {
    MIRBlock *B = worklist.front();
    worklist.pop();
    inWorklist.erase(B);

    std::vector<Loan> currentLoans;
    for (MIRBlock *pred : preds[B]) {
      currentLoans = mergeLoans(currentLoans, blockOut[pred]);
    }
    blockIn[B] = currentLoans;

    for (auto &instPtr : B->getInstructions()) {
      MIRInst *inst = instPtr.get();
      activeLoansAtInst[inst] = currentLoans;

      if (auto *cast = llvm::dyn_cast<CastInst>(inst)) {
        // [FIX] Forward loans through Casts
        size_t n = currentLoans.size();
        for (size_t i = 0; i < n; ++i) {
          const Loan active = currentLoans[i];
          if (active.pointer == cast->getValue()) {
            Loan inheritedLoan{active.borrowedPlace, cast, active.isMut};
            if (std::find(currentLoans.begin(), currentLoans.end(),
                          inheritedLoan) == currentLoans.end()) {
              currentLoans.push_back(inheritedLoan);
            }
          }
        }
      } else if (auto *load = llvm::dyn_cast<LoadInst>(inst)) {
        if (isMoveType(load)) {
          std::vector<Place> srcPlaces = resolvePlace(load->getPointer());
          for (const Place &src : srcPlaces) {
            if (src.base && llvm::isa<AllocaInst>(src.base)) {
              Loan moveLoan{src, nullptr, true};
              if (std::find(currentLoans.begin(), currentLoans.end(),
                            moveLoan) == currentLoans.end()) {
                currentLoans.push_back(moveLoan);
              }
            }
          }
        }

        if (isPointerType(load)) {
          size_t n = currentLoans.size();
          std::vector<Place> ptrPlaces = resolvePlace(load->getPointer());

          for (size_t i = 0; i < n; ++i) {
            const Loan active = currentLoans[i]; // Copy by value to prevent
                                                 // bad_array_new_length!

            // Check if the pointer we are loading from (or its base allocation)
            // holds a loan
            bool inherits = false;
            if (active.pointer == load->getPointer()) {
              inherits = true;
            } else {
              for (const Place &p : ptrPlaces) {
                if (p.base && p.base == active.pointer) {
                  inherits = true;
                  break;
                }
              }
            }

            // If it does, the newly loaded SSA value inherits the exact same
            // loan
            if (inherits) {
              Loan inheritedLoan{active.borrowedPlace, load, active.isMut};
              if (std::find(currentLoans.begin(), currentLoans.end(),
                            inheritedLoan) == currentLoans.end()) {
                currentLoans.push_back(inheritedLoan);
              }
            }
          }
        }
      } else if (auto *store = llvm::dyn_cast<StoreInst>(inst)) {
        // [FIX] Forward loans through Stores securely using resolvePlace
        size_t n = currentLoans.size();
        std::vector<Place> srcPlaces = resolvePlace(store->getValue());
        std::vector<Place> destPlaces = resolvePlace(store->getPointer());

        currentLoans.erase(
            std::remove_if(currentLoans.begin(), currentLoans.end(),
                           [&](const Loan &l) {
                             // Only clear move loans (where pointer == nullptr)
                             if (l.pointer == nullptr) {
                               for (const Place &dest : destPlaces) {
                                 if (l.borrowedPlace.conflictsWith(dest)) {
                                   return true; // Kill the move loan!
                                 }
                               }
                             }
                             return false;
                           }),
            currentLoans.end());

        // [FIX 1] Prevent Environment Pack StoreInsts from generating
        // independent loans
        bool isClosureEnv = false;
        if (auto *gep =
                llvm::dyn_cast<GetElementPtrInst>(store->getPointer())) {
          if (gep->getPointer() && gep->getPointer()->getType()) {
            std::string tyName = gep->getPointer()->getType()->toString();
            // Catch all variants of Env or Closure structs
            if (tyName.find("Env") != std::string::npos ||
                tyName.find("Closure") != std::string::npos ||
                tyName.find("closure") != std::string::npos) {
              isClosureEnv = true;
            }
          }
        }

        if (!isClosureEnv) {
          // [NEW FIX] Only create loans for actual pointer/alias types, not
          // primitives!
          if (isPointerType(store->getValue())) {
            for (const Place &src : srcPlaces) {
              if (src.base) {
                // 1. Register loan against the direct pointer
                Loan newLoan{src, store->getPointer(),
                             isExclusiveBorrow(store->getValue())};
                if (std::find(currentLoans.begin(), currentLoans.end(),
                              newLoan) == currentLoans.end()) {
                  currentLoans.push_back(newLoan);
                }

                // 2. Propagate the loan up to the base allocation
                for (const Place &dest : destPlaces) {
                  if (dest.base && dest.base != store->getPointer()) {
                    Loan baseLoan{src, dest.base,
                                  isExclusiveBorrow(store->getValue())};
                    if (std::find(currentLoans.begin(), currentLoans.end(),
                                  baseLoan) == currentLoans.end()) {
                      currentLoans.push_back(baseLoan);
                    }
                  }
                }
              }
            }
            for (size_t i = 0; i < n; ++i) {
              const Loan active = currentLoans[i];

              // Robustly check if the stored value aliases the active pointer
              bool inherits = false;
              if (active.pointer == store->getValue()) {
                inherits = true;
              } else {
                for (const Place &src : srcPlaces) {
                  if (src.base && src.base == active.pointer) {
                    inherits = true;
                    break;
                  }
                }
              }

              if (inherits) {
                // [FIX 2] Preserve mutability for closure loans across
                // bitcasts!
                bool isValMut = isExclusiveBorrow(store->getValue());
                bool isClosure = false;
                if (store->getValue() && store->getValue()->getType()) {
                  std::string tyName = store->getValue()->getType()->toString();
                  // Case-insensitive check ensures casted AST types retain
                  // mutability
                  if (tyName.find("Closure") != std::string::npos ||
                      tyName.find("closure") != std::string::npos)
                    isClosure = true;
                }

                Loan inheritedLoan{active.borrowedPlace, store->getPointer(),
                                   active.isMut && (isValMut || isClosure)};

                if (std::find(currentLoans.begin(), currentLoans.end(),
                              inheritedLoan) == currentLoans.end()) {
                  currentLoans.push_back(inheritedLoan);
                }

                // 2. Propagate inherited loan up to the base allocation
                for (const Place &dest : destPlaces) {
                  if (dest.base && dest.base != store->getPointer()) {
                    Loan baseLoan{active.borrowedPlace, dest.base,
                                  active.isMut && (isValMut || isClosure)};
                    if (std::find(currentLoans.begin(), currentLoans.end(),
                                  baseLoan) == currentLoans.end()) {
                      currentLoans.push_back(baseLoan);
                    }
                  }
                }
              }
            }
          }
        }
      } else if (auto *makeClosure = llvm::dyn_cast<MakeClosureInst>(inst)) {
        // [FIX] Determine the overall borrow kind of the closure based on its
        // captures
        bool requiresMut = false;
        bool hasRef = false;
        for (MIRValue *cap : makeClosure->getCaptures()) {
          if (!cap)
            continue;
          // [FIX] Evaluate the explicit tags injected by LowerHIRToMIR
          if (cap->getBorrowKind() == BorrowKind::Mut) {
            requiresMut = true;
            hasRef = true;
          } else if (cap->getBorrowKind() == BorrowKind::View) {
            hasRef = true;
          } else if (isPointerType(cap)) {
            hasRef = true;
            if (isExclusiveBorrow(cap)) {
              requiresMut = true;
            }
          }
        }
        if (requiresMut)
          makeClosure->setBorrowKind(BorrowKind::Mut);
        else if (hasRef)
          makeClosure->setBorrowKind(BorrowKind::View);
        else
          makeClosure->setBorrowKind(BorrowKind::None);

        for (MIRValue *cap : makeClosure->getCaptures()) {
          if (!cap || cap->getBorrowKind() == BorrowKind::None)
            continue;

          MIRValue *tracedCap = cap;
          while (true) {
            if (auto *cast = llvm::dyn_cast<CastInst>(tracedCap)) {
              tracedCap = cast->getValue();
            } else if (auto *load = llvm::dyn_cast<LoadInst>(tracedCap)) {
              tracedCap = load->getPointer();
            } else {
              break;
            }
          }

          std::vector<Place> srcPlaces = resolvePlace(tracedCap);
          for (const Place &src : srcPlaces) {
            if (src.base && llvm::isa<AllocaInst>(src.base)) {
              Loan newLoan{src, isMoveType(cap) ? nullptr : makeClosure,
                           isMoveType(cap)
                               ? true
                               : (cap->getBorrowKind() == BorrowKind::Mut)};
              if (std::find(currentLoans.begin(), currentLoans.end(),
                            newLoan) == currentLoans.end()) {
                currentLoans.push_back(newLoan);
              }
            }
          }
        }
      } else if (auto *call = llvm::dyn_cast<CallInst>(inst)) {
        bool isLockOrUnlock = false;
        if (call->getCallee()) {
          std::string name = call->getCallee()->getName();
          if (name == "__moksha_lock") {
            isLockOrUnlock = true;
            if (!call->getArgs().empty()) {
              for (const Place &src : resolvePlace(call->getArgs()[0])) {
                if (src.base) {
                  Loan lockLoan{src, LOCK_MARKER, true};
                  if (std::find(currentLoans.begin(), currentLoans.end(),
                                lockLoan) == currentLoans.end()) {
                    currentLoans.push_back(lockLoan);
                  }
                }
              }
            }
          } else if (name == "__moksha_unlock") {
            isLockOrUnlock = true;
            if (!call->getArgs().empty()) {
              for (const Place &dest : resolvePlace(call->getArgs()[0])) {
                currentLoans.erase(
                    std::remove_if(currentLoans.begin(), currentLoans.end(),
                                   [&](const Loan &l) {
                                     return l.pointer == LOCK_MARKER &&
                                            l.borrowedPlace.conflictsWith(dest);
                                   }),
                    currentLoans.end());
              }
            }
          }
        }

        if (!isLockOrUnlock) {
          if (isPointerType(call)) {
            for (MIRValue *arg : call->getArgs()) {
              if (isPointerType(arg)) {
                std::vector<Place> srcPlaces = resolvePlace(arg);
                for (const Place &src : srcPlaces) {
                  if (src.base && llvm::isa<AllocaInst>(src.base)) {
                    Loan newLoan{src, call, isExclusiveBorrow(call)};
                    if (std::find(currentLoans.begin(), currentLoans.end(),
                                  newLoan) == currentLoans.end()) {
                      currentLoans.push_back(newLoan);
                    }
                  }
                }
              }
            }
          }
        }
      }

      currentLoans.erase(
          std::remove_if(currentLoans.begin(), currentLoans.end(),
                         [&](const Loan &l) {
                           if (l.pointer == nullptr || l.pointer == LOCK_MARKER)
                             return false;
                           auto it = lastUses.find(l.pointer);
                           if (it != lastUses.end() && it->second == inst) {
                             return true;
                           }
                           return false;
                         }),
          currentLoans.end());
    }

    if (!loansEqual(blockOut[B], currentLoans)) {
      blockOut[B] = currentLoans;
      for (MIRBlock *succ : B->getSuccessors()) {
        if (inWorklist.find(succ) == inWorklist.end()) {
          worklist.push(succ);
          inWorklist.insert(succ);
        }
      }
    }
  }
}

void NLLBorrowChecker::checkConflicts(MIRFunction *func) {
  auto getBase1 = [](MIRValue *v) -> MIRValue * {
    // [FIX] Protect against null pointers AND the fake LOCK_MARKER address!
    if (!v || v == LOCK_MARKER)
      return v;

    while (auto *inst = llvm::dyn_cast<MIRInst>(v)) {
      if (auto *cast = llvm::dyn_cast<CastInst>(inst)) {
        v = cast->getValue();
      } else if (auto *gep = llvm::dyn_cast<GetElementPtrInst>(inst)) {
        v = gep->getPointer();
      } else if (auto *load = llvm::dyn_cast<LoadInst>(inst)) {
        v = load->getPointer();
        break; // Only strip one level of load to prevent invalid aliasing
               // across depths!
      } else {
        break;
      }
    }
    return v;
  };

  for (auto &blockPtr : func->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructions()) {
      MIRInst *inst = instPtr.get();
      if (llvm::isa<BranchInst>(inst) || llvm::isa<CondBranchInst>(inst)) {
        continue;
      }
      const auto &activeLoans = activeLoansAtInst[inst];

      // [FIX 1] Lift borrowsLocalMemory so it can be used by both Returns and
      // Spawns
      auto borrowsLocalMemory = [&](MIRValue *startPtr) -> bool {
        std::queue<MIRValue *> q;
        std::unordered_set<MIRValue *> qVisited;

        auto push = [&](MIRValue *v) {
          if (v && qVisited.insert(v).second)
            q.push(v);
        };

        push(startPtr);

        while (!q.empty()) {
          MIRValue *currPtr = q.front();
          q.pop();

          if (!isPointerType(currPtr) && !isMoveType(currPtr))
            continue;

          if (llvm::isa<AllocaInst>(currPtr))
            return true;

          if (auto *load = llvm::dyn_cast<LoadInst>(currPtr)) {
            MIRValue *srcPtr = load->getPointer();
            for (auto &bPtr : func->getBlocks()) {
              for (auto &iPtr : bPtr->getInstructions()) {
                if (auto *store = llvm::dyn_cast<StoreInst>(iPtr.get())) {
                  bool aliases = false;
                  if (store->getPointer() == srcPtr) {
                    aliases = true;
                  } else {
                    for (const Place &p : resolvePlace(store->getPointer())) {
                      if (p.base == srcPtr) {
                        aliases = true;
                        break;
                      }
                    }
                  }
                  if (aliases && store->getValue()) {
                    push(store->getValue());
                  }
                }
              }
            }
            continue;
          }

          for (const Place &p : resolvePlace(currPtr)) {
            if (p.base && p.base != currPtr)
              push(p.base);
          }

          if (auto *mc = llvm::dyn_cast<MakeClosureInst>(currPtr)) {
            for (MIRValue *cap : mc->getCaptures())
              push(cap);
          }

          for (const Loan &active : activeLoans) {
            bool holdsBorrow = false;
            if (active.pointer == currPtr) {
              holdsBorrow = true;
            } else if (active.pointer) {
              for (const Place &p : resolvePlace(active.pointer)) {
                if (p.base == currPtr) {
                  holdsBorrow = true;
                  break;
                }
              }
            }
            if (holdsBorrow && active.borrowedPlace.base) {
              push(active.borrowedPlace.base);
            }
          }
        }
        return false;
      };

      if (auto *load = llvm::dyn_cast<LoadInst>(inst)) {
        for (auto &p : resolvePlace(load->getPointer())) {
          if (!p.base)
            continue;
          bool reported = false;

          for (const Loan &active : activeLoans) {
            if (active.pointer == nullptr &&
                active.borrowedPlace.conflictsWith(p)) {
              diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                  << "Use of moved value. The memory was previously moved or "
                     "dropped.";
              reported = true;
              break;
            }

            if (!reported && active.isMut && active.pointer != nullptr &&
                active.borrowedPlace.conflictsWith(p)) {

              bool isSamePointer = false;
              if (getBase1(load->getPointer()) == getBase1(active.pointer) ||
                  load->getPointer() == active.pointer ||
                  active.pointer == LOCK_MARKER) {
                isSamePointer = true;
              }

              // [FIX] Allow reading if the pointer is an authorized reborrow
              if (!isSamePointer) {
                for (const Loan &other : activeLoans) {
                  if (other.pointer != nullptr &&
                      (other.pointer == load->getPointer() ||
                       getBase1(other.pointer) ==
                           getBase1(load->getPointer())) &&
                      other.borrowedPlace.conflictsWith(p)) {
                    isSamePointer = true;
                    break;
                  }
                }
              }

              if (!isSamePointer) {
                diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                    << "Cannot borrow memory immutably because it is currently "
                       "borrowed mutably.";
                reported = true;
                break;
              }
            }
          }
          if (reported)
            break;
        }
      } else if (auto *store = llvm::dyn_cast<StoreInst>(inst)) {
        std::vector<Place> destPlaces = resolvePlace(store->getPointer());
        std::vector<Place> srcPlaces = resolvePlace(store->getValue());

        bool reportedMutation = false;
        bool isDestLongLived = false;

        for (const Place &dest : destPlaces) {
          if (!dest.base)
            continue;
          if (!llvm::isa<AllocaInst>(dest.base)) {
            bool isClosureEnv = false;
            if (auto *gep =
                    llvm::dyn_cast<GetElementPtrInst>(store->getPointer())) {
              if (gep->getPointer() && gep->getPointer()->getType()) {
                std::string tyName = gep->getPointer()->getType()->toString();
                // Catch all variants of Env or Closure structs
                if (tyName.find("Env") != std::string::npos ||
                    tyName.find("Closure") != std::string::npos ||
                    tyName.find("closure") != std::string::npos) {
                  isClosureEnv = true;
                }
              }
            }
            if (!isClosureEnv)
              isDestLongLived = true;
          }
          for (const Loan &active : activeLoans) {
            if (active.pointer != nullptr) {
              bool isSamePointer = false;
              if (getBase1(store->getPointer()) == getBase1(active.pointer) ||
                  store->getPointer() == active.pointer) {
                isSamePointer = true;
              }
              if (active.pointer == LOCK_MARKER) {
                isSamePointer = true;
              }
              // [FIX] Allow mutation if the pointer is an authorized MUTABLE
              // reborrow
              if (!isSamePointer) {
                for (const Loan &other : activeLoans) {
                  if (other.pointer != nullptr &&
                      (other.pointer == store->getPointer() ||
                       getBase1(other.pointer) ==
                           getBase1(store->getPointer())) &&
                      other.borrowedPlace.conflictsWith(dest) && other.isMut) {
                    isSamePointer = true;
                    break;
                  }
                }
              }
              if (!isSamePointer && active.borrowedPlace.conflictsWith(dest)) {
                diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                    << "Cannot mutate memory because it is currently borrowed.";
                reportedMutation = true;
                break;
              }
            }
          }
          if (reportedMutation)
            break;
        }

        if (isDestLongLived && isPointerType(store->getValue())) {
          for (const Place &srcPlace : srcPlaces) {
            if (srcPlace.base) {
              // 1. Check for standard Local Stack escapes
              if (llvm::isa<AllocaInst>(srcPlace.base)) {
                diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                    << "Local reference escapes its scope. Cannot store a "
                       "local "
                       "reference into a global variable, heap object, or "
                       "caller "
                       "argument.";
                break;
              }
              // [FIX 2] Prevent Unsafe capabilities from leaking to the Safe
              // global scope
              else if (auto *global =
                           llvm::dyn_cast<MIRGlobal>(srcPlace.base)) {
                if (global->isConstant() &&
                    isExclusiveBorrow(store->getValue())) {
                  diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                      << "Unsafe capability leak: Cannot store a mutable "
                         "pointer aliasing immutable global memory into a "
                         "long-lived location.";
                  break;
                }
              }
            }
          }
        }

        if (isPointerType(store->getValue())) {
          // [FIX] Prevent duplicate alias errors by relying on
          // MakeClosureInst's native capture checks
          bool isStoreOfClosure = false;
          if (store->getValue() && store->getValue()->getType()) {
            std::string tyName = store->getValue()->getType()->toString();
            if (tyName.find("Closure") != std::string::npos ||
                tyName.find("closure") != std::string::npos) {
              isStoreOfClosure = true;
            }
          }

          if (!isStoreOfClosure) {
            for (const Place &src : srcPlaces) {
              if (!src.base)
                continue;
              bool reportedAlias = false;

              // [FIX 2] Prevent Lock Bypassing Aliases via Store
              bool isMutAlias =
                  store->getPointer()->getBorrowKind() == BorrowKind::Mut ||
                  store->getValue()->getBorrowKind() == BorrowKind::Mut;
              if (isMutAlias) {
                for (const Loan &active : activeLoans) {
                  if (active.pointer == LOCK_MARKER &&
                      active.borrowedPlace.conflictsWith(src)) {
                    diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                        << "Cannot alias a lock pointer as a mutable pointer.";
                    reportedAlias = true;
                    break;
                  }
                }
              }

              if (reportedAlias)
                continue;

              for (const Loan &active : activeLoans) {
                if (active.pointer == nullptr &&
                    active.borrowedPlace.conflictsWith(src)) {
                  diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                      << "Cannot borrow a moved value.";
                  reportedAlias = true;
                  break;
                }

                if (!reportedAlias && active.pointer != nullptr &&
                    active.borrowedPlace.conflictsWith(src)) {

                  bool isSamePointer = false;
                  if (getBase1(store->getValue()) == getBase1(active.pointer) ||
                      store->getValue() == active.pointer) {
                    isSamePointer = true;
                  }

                  bool isMutAttempt = isExclusiveBorrow(store->getValue());

                  // [FIX] Allow aliasing if the new pointer shares the base via
                  // an authorized loan
                  if (!isSamePointer) {
                    for (const Loan &other : activeLoans) {
                      if (other.pointer != nullptr &&
                          (other.pointer == store->getValue() ||
                           getBase1(other.pointer) ==
                               getBase1(store->getValue())) &&
                          other.borrowedPlace.conflictsWith(src)) {
                        if (!isMutAttempt || other.isMut) {
                          isSamePointer = true;
                          break;
                        }
                      }
                    }
                  }

                  // [FIX] Allow reborrowing if it shares the same base pointer
                  if (!isSamePointer && (isMutAttempt || active.isMut)) {
                    if (isMutAttempt) {
                      diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                          << "Cannot borrow memory mutably because it is "
                             "already "
                             "borrowed.";
                    } else {
                      diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                          << "Cannot borrow memory immutably because it is "
                             "currently borrowed mutably.";
                    }
                    reportedAlias = true;
                    break;
                  }
                }
              }
            }
          }
        }
      } else if (auto *call = llvm::dyn_cast<CallInst>(inst)) {
        bool isLockOrUnlock = false;
        if (call->getCallee()) {
          std::string name = call->getCallee()->getName();
          // [FIX] Use .find() to catch all variants of lock/unlock and prevent
          // them from falling through to the standard function argument checks!
          if (name.find("unlock") != std::string::npos) {
            isLockOrUnlock = true;
          } else if (name.find("lock") != std::string::npos) {
            isLockOrUnlock = true;
            if (!call->getArgs().empty()) {
              for (const Place &p : resolvePlace(call->getArgs()[0])) {
                bool deadlock = false;

                // 1. Check for Deadlocks FIRST
                for (const Loan &active : activeLoans) {
                  if (active.borrowedPlace.conflictsWith(p) &&
                      active.pointer == LOCK_MARKER) {
                    diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                        << "Deadlock prevention: Cannot lock a mutex that is "
                           "already locked in the current scope.";
                    deadlock = true;
                    break;
                  }
                }

                // 2. If no deadlock, verify we aren't locking actively aliased
                // memory
                if (!deadlock) {
                  for (const Loan &active : activeLoans) {
                    if (active.pointer != LOCK_MARKER &&
                        active.borrowedPlace.conflictsWith(p)) {
                      bool isSamePointer = false;
                      if (getBase1(call->getArgs()[0]) ==
                              getBase1(active.pointer) ||
                          call->getArgs()[0] == active.pointer) {
                        isSamePointer = true;
                      }
                      if (!isSamePointer && active.isMut) {
                        diags.report(inst->getLoc(),
                                     DiagID::err_borrow_violation)
                            << "Cannot lock memory because it is already "
                               "borrowed mutably.";
                        break;
                      }
                    }
                  }
                }
              }
            }
          }
        }

        // [RESTORED] The missing standard function argument checking block!
        if (!isLockOrUnlock) {
          std::vector<Loan> intraCallLoans;
          for (MIRValue *arg : call->getArgs()) {
            if (!isPointerType(arg))
              continue;

            for (const Place &src : resolvePlace(arg)) {
              if (!src.base)
                continue;
              bool reportedAlias = false;

              for (const Loan &active : activeLoans) {
                if (active.pointer != nullptr &&
                    active.pointer != LOCK_MARKER &&
                    active.borrowedPlace.conflictsWith(src)) {
                  bool isSamePointer = false;
                  if (getBase1(arg) == getBase1(active.pointer) ||
                      arg == active.pointer) {
                    isSamePointer = true;
                  }
                  bool isMutAttempt = isExclusiveBorrow(arg);
                  if (!isSamePointer && (isMutAttempt || active.isMut)) {
                    if (isMutAttempt) {
                      diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                          << "Cannot pass memory to a function mutably because "
                             "it is already borrowed.";
                    } else {
                      diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                          << "Cannot pass memory to a function immutably "
                             "because it is currently borrowed mutably.";
                    }
                    reportedAlias = true;
                    break;
                  }
                }
              }

              if (!reportedAlias)
                intraCallLoans.push_back({src, arg, isExclusiveBorrow(arg)});

              if (reportedAlias)
                break;
            }
          }
        }
      } else if (auto *makeClosure = llvm::dyn_cast<MakeClosureInst>(inst)) {
        for (MIRValue *cap : makeClosure->getCaptures()) {
          if (!cap)
            continue;

          bool isRef = isPointerType(cap);

          if (!isRef && !isMoveType(cap)) {
            continue; // Snapshot copy, no conflict
          }

          // Trace through implicit loads to find the true captured memory base
          MIRValue *tracedCap = cap;
          while (auto *load = llvm::dyn_cast<LoadInst>(tracedCap)) {
            tracedCap = load->getPointer();
          }

          bool isMutAttempt = isExclusiveBorrow(makeClosure);

          for (const Place &src : resolvePlace(tracedCap)) {
            if (!src.base)
              continue;
            bool reported = false;

            for (const Loan &active : activeLoans) {
              // [FIX] Detect captures of moved values (only for by-reference
              // captures)!
              if (active.pointer == nullptr &&
                  active.borrowedPlace.conflictsWith(src)) {
                if (isRef) {
                  diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                      << "Use of moved value. The memory was previously moved "
                         "or "
                         "dropped.";
                  reported = true;
                  break;
                }
              }

              if (!reported && active.pointer != nullptr &&
                  active.pointer != LOCK_MARKER &&
                  active.borrowedPlace.conflictsWith(src)) {

                bool isSamePointer = false;
                if (getBase1(tracedCap) == getBase1(active.pointer) ||
                    tracedCap == active.pointer) {
                  isSamePointer = true;
                }

                if (!isSamePointer) {
                  if (!isRef && isMoveType(cap)) {
                    diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                        << "Cannot capture value by move/copy because it is "
                           "currently borrowed.";
                    reported = true;
                    break;
                  } else if (isMutAttempt || active.isMut) {
                    if (isMutAttempt)
                      diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                          << "Cannot capture memory mutably because it is "
                             "already borrowed.";
                    else
                      diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                          << "Cannot capture memory immutably because it is "
                             "currently borrowed mutably.";
                    reported = true;
                    break;
                  }
                }
              }
              if (reported)
                break;
            }
          }
        }
      } else if (auto *spawnInst = llvm::dyn_cast<SpawnInst>(inst)) {
        if (MIRValue *closureVal = spawnInst->getClosure()) {
          if (borrowsLocalMemory(closureVal)) {
            diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                << "Cannot borrow local variable inside a thread block. "
                   "The thread might outlive the current stack frame, causing "
                   "a dangling pointer.";
          }
        }
      } else if (auto *retInst = llvm::dyn_cast<ReturnInst>(inst)) {
        if (MIRValue *retVal = retInst->getReturnValue()) {

          auto borrowsLocalMemory = [&](MIRValue *startPtr) -> bool {
            std::queue<MIRValue *> q;
            std::unordered_set<MIRValue *> qVisited;

            auto push = [&](MIRValue *v) {
              if (v && qVisited.insert(v).second)
                q.push(v);
            };

            push(startPtr);

            while (!q.empty()) {
              MIRValue *currPtr = q.front();
              q.pop();

              // Safe by-value copies of primitives do not capture stack memory!
              if (!isPointerType(currPtr) && !isMoveType(currPtr)) {
                continue;
              }

              // Direct check: is the value itself an alloca?
              if (llvm::isa<AllocaInst>(currPtr)) {
                return true;
              }

              // 1. Structurally trace backwards through Loads FIRST to prevent
              // resolvePlace from aggressively yielding the container
              // AllocaInst
              if (auto *load = llvm::dyn_cast<LoadInst>(currPtr)) {
                MIRValue *srcPtr = load->getPointer();
                for (auto &bPtr : func->getBlocks()) {
                  for (auto &iPtr : bPtr->getInstructions()) {
                    if (auto *store = llvm::dyn_cast<StoreInst>(iPtr.get())) {
                      bool aliases = false;
                      if (store->getPointer() == srcPtr) {
                        aliases = true;
                      } else {
                        for (const Place &p :
                             resolvePlace(store->getPointer())) {
                          if (p.base == srcPtr) {
                            aliases = true;
                            break;
                          }
                        }
                      }
                      if (aliases && store->getValue()) {
                        push(store->getValue());
                      }
                    }
                  }
                }
                // Skip resolvePlace for Loads!
                continue;
              }

              // 2. Unpack pointer arithmetic and casts
              for (const Place &p : resolvePlace(currPtr)) {
                if (p.base && p.base != currPtr)
                  push(p.base);
              }

              // 3. Structurally trace backwards through MakeClosure captures
              if (auto *mc = llvm::dyn_cast<MakeClosureInst>(currPtr)) {
                for (MIRValue *cap : mc->getCaptures()) {
                  push(cap);
                }
              }

              // 4. Trace through any surviving active loans
              for (const Loan &active : activeLoans) {
                bool holdsBorrow = false;
                if (active.pointer == currPtr) {
                  holdsBorrow = true;
                } else if (active.pointer) {
                  for (const Place &p : resolvePlace(active.pointer)) {
                    if (p.base == currPtr) {
                      holdsBorrow = true;
                      break;
                    }
                  }
                }
                if (holdsBorrow && active.borrowedPlace.base) {
                  push(active.borrowedPlace.base);
                }
              }
            }
            return false;
          };

          if (borrowsLocalMemory(retVal)) {
            diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                << "Cannot return a closure or reference that captures local "
                   "stack memory.";
          }
        }
      } else if (llvm::isa<AwaitInst>(inst)) {
        for (const Loan &active : activeLoans) {
          if (active.pointer != nullptr && active.borrowedPlace.base &&
              llvm::isa<AllocaInst>(active.borrowedPlace.base)) {
            if (active.pointer == LOCK_MARKER) {
              diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                  << "Deadlock prevention: Cannot hold a lock across an "
                     "`await` suspension point.";
            } else {
              diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                  << "Cannot hold a borrow to local stack memory across an "
                     "`await` suspension point.";
            }
            break;
          }
        }
      }
    }
  }
}

std::vector<Place> NLLBorrowChecker::resolvePlace(MIRValue *val) const {
  std::vector<Place> places;
  std::queue<std::pair<MIRValue *, std::vector<uint64_t>>> worklist;
  std::unordered_set<MIRValue *> visited;

  worklist.push({val, {}});

  while (!worklist.empty()) {
    auto [currentVal, proj] = worklist.front();
    worklist.pop();

    if (!visited.insert(currentVal).second)
      continue;

    if (auto *inst = llvm::dyn_cast<MIRInst>(currentVal)) {
      if (auto *cast = llvm::dyn_cast<CastInst>(inst)) {
        worklist.push({cast->getValue(), proj});
        continue;
      }
      if (auto *load = llvm::dyn_cast<LoadInst>(inst)) {
        if (!isPointerType(load)) {
          worklist.push({load->getPointer(), proj});
          continue;
        } else {
          bool foundStore = false;
          if (auto *parentFunc = load->getParent()->getParent()) {
            for (auto &bPtr : parentFunc->getBlocks()) {
              for (auto &iPtr : bPtr->getInstructions()) {
                if (auto *store = llvm::dyn_cast<StoreInst>(iPtr.get())) {
                  if (store->getPointer() == load->getPointer() &&
                      store->getValue()) {
                    worklist.push({store->getValue(), proj});
                    foundStore = true;
                  }
                }
              }
            }
          }
          if (foundStore)
            continue;
        }
      }
      if (inst->getOpcode() == Opcode::ExtractValue) {
        auto *ext = static_cast<ExtractValueInst *>(inst);
        if (ext->getIndex() == 0) {
          worklist.push({ext->getAggregate(), proj});
          continue;
        }
      }
      if (inst->getOpcode() == Opcode::GetElementPtr) {
        auto *gep = static_cast<GetElementPtrInst *>(inst);
        std::vector<uint64_t> newProj;
        for (auto *idx : gep->getIndices()) {
          if (auto *cInt = llvm::dyn_cast<ConstantInt>(idx)) {
            newProj.push_back(cInt->getValue());
          }
        }
        newProj.insert(newProj.end(), proj.begin(), proj.end());
        worklist.push({gep->getPointer(), newProj});
        continue;
      }
      if (auto *phi = llvm::dyn_cast<PhiInst>(inst)) {
        for (auto const &[incVal, block] : phi->getIncoming()) {
          worklist.push({incVal, proj});
        }
        continue;
      }
    }
    places.push_back({currentVal, proj});
  }
  return places;
}

} // namespace mir
} // namespace moksha
