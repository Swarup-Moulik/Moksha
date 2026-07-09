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

  // 1. Check for primitives FIRST. They are never pointers.
  auto kind = val->getType()->getKind();
  if (kind == hir::TypeKind::Any || kind == hir::TypeKind::Int ||
      kind == hir::TypeKind::Float || kind == hir::TypeKind::Decimal ||
      kind == hir::TypeKind::Bool) {
    return false;
  }

  // --- [CRITICAL FIX]: ARC Managed Types are Heap-Bound, NOT Stack Pointers
  // --- If the value is managed by ARC, it is inherently safe to escape the
  // stack frame.
  if (kind == hir::TypeKind::String || kind == hir::TypeKind::Slice ||
      kind == hir::TypeKind::Closure || kind == hir::TypeKind::Map ||
      kind == hir::TypeKind::Promise || kind == hir::TypeKind::Struct) {
    return false;
  }

  if (kind == hir::TypeKind::Nullable) {
    auto *nullTy = static_cast<const hir::HIRNullableType *>(val->getType());
    auto innerKind = nullTy->getInner()->getKind();

    // [FIX]: Also exempt nullable variants of ARC types!
    if (innerKind == hir::TypeKind::String ||
        innerKind == hir::TypeKind::Slice ||
        innerKind == hir::TypeKind::Closure ||
        innerKind == hir::TypeKind::Map ||
        innerKind == hir::TypeKind::Promise ||
        innerKind == hir::TypeKind::Struct || innerKind == hir::TypeKind::Any) {
      return false;
    }
  }

  // [FIX]: Finally, if it's an explicit pointer, check if it's Shared/Owned
  if (auto *pTy = llvm::dyn_cast<hir::PointerType>(val->getType())) {
    if (pTy->getOwnership() == hir::Ownership::Shared ||
        pTy->getOwnership() == hir::Ownership::Owned) {
      return false; // ARC pointers can escape safely
    }
  }

  // Only raw Borrowed/None pointers trigger the escape analysis!
  return val->getType()->getKind() == hir::TypeKind::Pointer ||
         val->getType()->getKind() == hir::TypeKind::Reference;
}

static bool isExclusiveBorrow(MIRValue *pointerVal) {
  if (!pointerVal)
    return false;

  MIRValue *traced = pointerVal;
  while (traced) {
    if (traced->getBorrowKind() != BorrowKind::None)
      break;
    if (auto *cast = llvm::dyn_cast<CastInst>(traced)) {
      traced = cast->getValue();
    } else {
      break;
    }
  }

  BorrowKind kind = traced->getBorrowKind();

  if (kind == BorrowKind::Mut)
    return true;
  if (kind == BorrowKind::View || kind == BorrowKind::Lock)
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

static bool isMoveOnlyType(const hir::HIRType *type) {
  if (!type)
    return false;
  switch (type->getKind()) {
  case hir::TypeKind::Promise:
    return true; // Promises are strictly consumed upon await

  case hir::TypeKind::Pointer:
    if (auto *ptrTy = llvm::dyn_cast<hir::PointerType>(type)) {
      return ptrTy->getOwnership() ==
             hir::Ownership::Owned; // Unique pointers move
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
    checkFunction(func);
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
      if (inst->getName().find("cleanup_val") != std::string::npos ||
          inst->getName().find("heap.free.ptr") != std::string::npos ||
          inst->getName().find(".drop.") != std::string::npos) {
        continue;
      }
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
        if (call->getCallee()) {
          std::string calleeName = call->getCallee()->getName();
          if (calleeName == "__moksha_free" ||
              calleeName.find(".destructor_ret_void") != std::string::npos) {
            continue;
          }
        }
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
      } else if (auto *cmp = llvm::dyn_cast<CompareInst>(inst)) {
        if (cmp->getLHS())
          lastUses[cmp->getLHS()] = inst;
        if (cmp->getRHS())
          lastUses[cmp->getRHS()] = inst;
      } else if (auto *cast = llvm::dyn_cast<CastInst>(inst)) {
        if (cast->getValue())
          lastUses[cast->getValue()] = inst;
      } else if (auto *gep = llvm::dyn_cast<GetElementPtrInst>(inst)) {
        if (gep->getPointer())
          lastUses[gep->getPointer()] = inst;
      } else if (auto *ext = llvm::dyn_cast<ExtractValueInst>(inst)) {
        if (ext->getAggregate())
          lastUses[ext->getAggregate()] = inst;
      } else if (auto *arc = llvm::dyn_cast<ARCInst>(inst)) {
        if (arc->getOpcode() == Opcode::Release)
          continue;
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
      } else if (auto *atomLoad = llvm::dyn_cast<AtomicLoadInst>(inst)) {
        if (atomLoad->getPointer())
          lastUses[atomLoad->getPointer()] = inst;
      } else if (auto *atomStore = llvm::dyn_cast<AtomicStoreInst>(inst)) {
        if (atomStore->getValue())
          lastUses[atomStore->getValue()] = inst;
        if (atomStore->getPointer())
          lastUses[atomStore->getPointer()] = inst;
      } else if (auto *atomRmw = llvm::dyn_cast<AtomicRMWInst>(inst)) {
        if (atomRmw->getValue())
          lastUses[atomRmw->getValue()] = inst;
        if (atomRmw->getPointer())
          lastUses[atomRmw->getPointer()] = inst;
      } else if (auto *atomCas = llvm::dyn_cast<AtomicCmpXchgInst>(inst)) {
        if (atomCas->getPointer())
          lastUses[atomCas->getPointer()] = inst;
        if (atomCas->getExpected())
          lastUses[atomCas->getExpected()] = inst;
        if (atomCas->getDesired())
          lastUses[atomCas->getDesired()] = inst;
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
      const auto &activeLoans = activeLoansAtInst[inst];

      if (auto *cast = llvm::dyn_cast<CastInst>(inst)) {
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
        bool isActualMove = isMoveType(load);

        if (isActualMove) {
          MIRInst *consumer = nullptr;
          MIRValue *traceVal = load;
          int depth = 0;
          while (traceVal && depth++ < 10) {
            auto useIt = lastUses.find(traceVal);
            if (useIt != lastUses.end() && useIt->second) {
              consumer = const_cast<MIRInst *>(useIt->second);
              if (llvm::isa<StoreInst>(consumer) ||
                  llvm::isa<CallInst>(consumer) ||
                  llvm::isa<ReturnInst>(consumer) ||
                  llvm::isa<CompareInst>(consumer) ||
                  llvm::isa<ExtractValueInst>(consumer) ||
                  llvm::isa<GetElementPtrInst>(consumer) ||
                  consumer->getOpcode() == Opcode::Release) {
                break;
              }
              traceVal = consumer;
            } else {
              break;
            }
          }

          if (consumer) {
            if (llvm::isa<CompareInst>(consumer) ||
                llvm::isa<ExtractValueInst>(consumer) ||
                llvm::isa<GetElementPtrInst>(consumer) ||
                llvm::isa<BinaryInst>(consumer)) {
              isActualMove = false;
            } else if (auto *call = llvm::dyn_cast<CallInst>(consumer)) {
              if (call->getCallee()) {
                std::string cName = call->getCallee()->getName();
                if (cName == "print" || cName == "println" ||
                    cName.find("__moksha_") == 0) {
                  isActualMove = false;
                }
              }
            }
          }

          if (isActualMove) {
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

          bool isAggregateLoad = false;
          if (load->getType() && load->getBorrowKind() == BorrowKind::None) {
            auto kind = load->getType()->getKind();
            if (kind != hir::TypeKind::Int && kind != hir::TypeKind::Float &&
                kind != hir::TypeKind::Decimal && kind != hir::TypeKind::Bool &&
                kind != hir::TypeKind::Pointer &&
                kind != hir::TypeKind::Reference &&
                kind != hir::TypeKind::Function) {
              isAggregateLoad = true;
            }
          }

          if (isPointerType(load) || isAggregateLoad) {
            size_t n = currentLoans.size();
            std::vector<Place> ptrPlaces = resolvePlace(load->getPointer());

            // 1. Inherit existing loans
            for (size_t i = 0; i < n; ++i) {
              const Loan active = currentLoans[i];
              bool inherits = false;
              if (active.pointer == load->getPointer()) {
                inherits = true;
              } else {
                std::vector<Place> activePlaces = resolvePlace(active.pointer);
                for (const Place &p : ptrPlaces) {
                  for (const Place &ap : activePlaces) {
                    if (p.base && p.base == ap.base) {
                      bool isPrefix = true;
                      if (p.projections.size() > ap.projections.size()) {
                        isPrefix = false;
                      } else {
                        for (size_t pi = 0; pi < p.projections.size(); ++pi) {
                          if (p.projections[pi] != ap.projections[pi]) {
                            isPrefix = false;
                            break;
                          }
                        }
                      }
                      if (isPrefix) {
                        inherits = true;
                        break;
                      }
                    }
                  }
                  if (inherits)
                    break;
                }
              }
              if (inherits) {
                Loan inheritedLoan{active.borrowedPlace, load, active.isMut};
                if (std::find(currentLoans.begin(), currentLoans.end(),
                              inheritedLoan) == currentLoans.end()) {
                  currentLoans.push_back(inheritedLoan);
                }
              }
            }

            // 2. Register newly extracted capability
            if (isPointerType(load)) {
              for (const Place &src : ptrPlaces) {
                if (src.base) {
                  Loan newLoan{src, load, isExclusiveBorrow(load)};
                  if (std::find(currentLoans.begin(), currentLoans.end(),
                                newLoan) == currentLoans.end()) {
                    currentLoans.push_back(newLoan);
                  }
                }
              }
            }
          }
        } else if (auto *store = llvm::dyn_cast<StoreInst>(inst)) {
          std::vector<Place> destPlaces = resolvePlace(store->getPointer());
          std::vector<Place> srcPlaces = resolvePlace(store->getValue());

          currentLoans.erase(
              std::remove_if(currentLoans.begin(), currentLoans.end(),
                             [&](const Loan &l) {
                               if (l.pointer == nullptr) {
                                 for (const Place &dest : destPlaces) {
                                   if (l.borrowedPlace.conflictsWith(dest)) {
                                     return true;
                                   }
                                 }
                               } else if (l.pointer != LOCK_MARKER) {
                                 for (const Place &dest : destPlaces) {
                                   if (dest.base && l.pointer == dest.base) {
                                     return true;
                                   }
                                 }
                               }
                               return false;
                             }),
              currentLoans.end());

          bool isClosureEnv = false;
          if (auto *gep =
                  llvm::dyn_cast<GetElementPtrInst>(store->getPointer())) {
            if (gep->getPointer() && gep->getPointer()->getType()) {
              std::string tyName = gep->getPointer()->getType()->toString();
              if (tyName.find("Env") != std::string::npos ||
                  tyName.find("Closure") != std::string::npos ||
                  tyName.find("closure") != std::string::npos) {
                isClosureEnv = true;
              }
            }
          }

          if (!isClosureEnv) {
            bool isPtr = isPointerType(store->getValue());

            if (isPtr) {
              for (const Place &src : srcPlaces) {
                if (src.base) {
                  Loan newLoan{src, store->getPointer(),
                               isExclusiveBorrow(store->getValue())};
                  if (std::find(currentLoans.begin(), currentLoans.end(),
                                newLoan) == currentLoans.end()) {
                    currentLoans.push_back(newLoan);
                  }

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
            }

            // Loop 2: [FIX] INHERIT existing loans for ALL stores using
            // activeLoans!
            for (const Loan &active_loan : activeLoans) {
              bool inherits = false;
              if (active_loan.pointer == store->getValue()) {
                inherits = true;
              } else {
                std::vector<Place> activePlaces =
                    resolvePlace(active_loan.pointer);
                for (const Place &src : srcPlaces) {
                  for (const Place &ap : activePlaces) {
                    if (src.base && src.base == ap.base) {
                      bool isPrefix = true;
                      if (src.projections.size() > ap.projections.size()) {
                        isPrefix = false;
                      } else {
                        for (size_t pi = 0; pi < src.projections.size(); ++pi) {
                          if (src.projections[pi] != ap.projections[pi]) {
                            isPrefix = false;
                            break;
                          }
                        }
                      }
                      if (isPrefix) {
                        inherits = true;
                        break;
                      }
                    }
                  }
                  if (inherits)
                    break;
                }
              }

              if (inherits) {
                bool isValMut = isExclusiveBorrow(store->getValue());
                bool isClosure = false;

                if (store->getValue() && store->getValue()->getType()) {
                  auto kind = store->getValue()->getType()->getKind();
                  if (kind == hir::TypeKind::Closure ||
                      kind == hir::TypeKind::Function ||
                      kind == hir::TypeKind::Any) {
                    isClosure = true;
                  }
                }

                Place newBorrowedPlace = active_loan.borrowedPlace;

                bool rehomed = false;
                for (const Place &src : srcPlaces) {
                  if (active_loan.borrowedPlace.base == src.base) {
                    newBorrowedPlace.base = store->getPointer();
                    rehomed = true;
                    break;
                  }
                }

                if (!rehomed) {
                  MIRValue *tracedVal = store->getValue();
                  while (tracedVal) {
                    if (auto *loadVal = llvm::dyn_cast<LoadInst>(tracedVal)) {
                      for (const Place &src :
                           resolvePlace(loadVal->getPointer())) {
                        if (active_loan.borrowedPlace.base == src.base) {
                          newBorrowedPlace.base = store->getPointer();
                          rehomed = true;
                          break;
                        }
                      }
                      if (rehomed)
                        break;
                      tracedVal = loadVal->getPointer();
                    } else if (auto *castVal =
                                   llvm::dyn_cast<CastInst>(tracedVal)) {
                      tracedVal = castVal->getValue();
                    } else {
                      break;
                    }
                  }
                }

                Loan inheritedLoan{newBorrowedPlace, store->getPointer(),
                                   active_loan.isMut &&
                                       (isValMut || isClosure)};

                if (std::find(currentLoans.begin(), currentLoans.end(),
                              inheritedLoan) == currentLoans.end()) {
                  currentLoans.push_back(inheritedLoan);
                }
              }
            }
          }
        } else if (auto *makeClosure = llvm::dyn_cast<MakeClosureInst>(inst)) {
          bool requiresMut = false;
          bool hasRef = false;
          for (MIRValue *cap : makeClosure->getCaptures()) {
            if (!cap)
              continue;
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
                                              l.borrowedPlace.conflictsWith(
                                                  dest);
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
            std::remove_if(
                currentLoans.begin(), currentLoans.end(),
                [&](const Loan &l) {
                  if (l.pointer == nullptr || l.pointer == LOCK_MARKER)
                    return false;

                  // Evaluate the ROOT ALLOCATION, not the intermediate field
                  // pointer!
                  bool isAggregate = false;
                  if (l.borrowedPlace.base && l.borrowedPlace.base->getType()) {
                    const hir::HIRType *checkTy =
                        l.borrowedPlace.base->getType();

                    if (auto *ptrTy =
                            llvm::dyn_cast<hir::PointerType>(checkTy)) {
                      checkTy = ptrTy->getPointee();
                    } else if (auto *refTy = llvm::dyn_cast<hir::ReferenceType>(
                                   checkTy)) {
                      checkTy = refTy->getInner();
                    }

                    auto kind = checkTy->getKind();
                    if (kind == hir::TypeKind::Closure ||
                        kind == hir::TypeKind::Struct ||
                        kind == hir::TypeKind::Array ||
                        kind == hir::TypeKind::Slice ||
                        kind == hir::TypeKind::String ||
                        kind == hir::TypeKind::Any) {
                      isAggregate = true;
                    }
                  }

                  if (isAggregate && l.borrowedPlace.base) {
                    for (const Place &dest : resolvePlace(l.pointer)) {
                      if (dest.base == l.borrowedPlace.base) {
                        return false; // Safely preserve the internal borrow!
                      }
                    }
                  }

                  if ((llvm::isa<CastInst>(l.pointer) ||
                       llvm::isa<CallInst>(l.pointer)) &&
                      lastUses.find(l.pointer) == lastUses.end()) {
                    return true;
                  }

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
}

void NLLBorrowChecker::checkConflicts(MIRFunction *func) {
  auto isDerivedFrom = [&](MIRValue *child, MIRValue *parent) -> bool {
    if (!child || !parent)
      return false;
    if (child == parent)
      return true;

    if (child == LOCK_MARKER || parent == LOCK_MARKER)
      return false;

    std::queue<MIRValue *> q;
    std::unordered_set<MIRValue *> visited;

    q.push(child);
    visited.insert(child);

    while (!q.empty()) {
      MIRValue *v = q.front();
      q.pop();

      if (v == parent)
        return true;

      if (auto *inst = llvm::dyn_cast<MIRInst>(v)) {
        if (auto *cast = llvm::dyn_cast<CastInst>(inst)) {
          if (visited.insert(cast->getValue()).second)
            q.push(cast->getValue());
        } else if (auto *gep = llvm::dyn_cast<GetElementPtrInst>(inst)) {
          if (visited.insert(gep->getPointer()).second)
            q.push(gep->getPointer());
        } else if (auto *load = llvm::dyn_cast<LoadInst>(inst)) {
          if (visited.insert(load->getPointer()).second)
            q.push(load->getPointer());
        }
      }

      if (llvm::isa<AllocaInst>(v) || llvm::isa<MIRGlobal>(v) ||
          llvm::isa<MIRArgument>(v)) {
        for (auto &bPtr : func->getBlocks()) {
          for (auto &iPtr : bPtr->getInstructions()) {
            if (auto *store = llvm::dyn_cast<StoreInst>(iPtr.get())) {
              if (store->getPointer() == v && store->getValue()) {
                if (visited.insert(store->getValue()).second) {
                  q.push(store->getValue());
                }
              }
            }
          }
        }
      }
    }
    return false;
  };

  auto isSamePointer = [&](MIRValue *a, MIRValue *b) -> bool {
    auto getBase = [&](MIRValue *v) -> MIRValue * {
      while (v && v != LOCK_MARKER) {
        if (auto *inst = llvm::dyn_cast<MIRInst>(v)) {
          if (auto *cast = llvm::dyn_cast<CastInst>(inst)) {
            v = cast->getValue();
          } else if (auto *gep = llvm::dyn_cast<GetElementPtrInst>(inst)) {
            v = gep->getPointer();
          } else if (auto *load = llvm::dyn_cast<LoadInst>(inst)) {
            v = load->getPointer();
          } else {
            break;
          }
        } else {
          break;
        }
      }
      return v;
    };

    MIRValue *baseA = getBase(a);
    MIRValue *baseB = getBase(b);
    if (baseA && baseB && baseA == baseB && baseA != LOCK_MARKER &&
        (llvm::isa<AllocaInst>(baseA) || llvm::isa<MIRGlobal>(baseA) ||
         llvm::isa<MIRArgument>(baseA))) {

      bool aIsDirect = llvm::isa<GetElementPtrInst>(a);
      bool bIsDirect = llvm::isa<GetElementPtrInst>(b);

      if (aIsDirect && bIsDirect && a != b) {
        return false;
      }
      return true;
    }
    return false;
  };

  for (auto &blockPtr : func->getBlocks()) {
    for (auto &instPtr : blockPtr->getInstructions()) {
      MIRInst *inst = instPtr.get();
      if (llvm::isa<BranchInst>(inst) || llvm::isa<CondBranchInst>(inst)) {
        continue;
      }
      const auto &activeLoans = activeLoansAtInst[inst];

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

          if (llvm::isa<AllocaInst>(currPtr)) {
            return true;
          }

          if (!isPointerType(currPtr) && !isMoveType(currPtr)) {
            continue;
          }

          if (llvm::isa<AllocaInst>(currPtr)) {
            return true;
          }

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
            for (MIRValue *cap : mc->getCaptures()) {
              push(cap);
            }
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
        bool isImplicitDrop =
            (load->getName().find("cleanup_val") != std::string::npos);

        MIRInst *consumer = nullptr;

        if (!isImplicitDrop) {
          MIRValue *traceVal = load;
          int depth = 0;

          while (traceVal && depth++ < 10) {
            auto useIt = lastUses.find(traceVal);
            if (useIt != lastUses.end() && useIt->second) {
              consumer = const_cast<MIRInst *>(useIt->second);
              if (llvm::isa<StoreInst>(consumer) ||
                  llvm::isa<CallInst>(consumer) ||
                  llvm::isa<ReturnInst>(consumer) ||
                  llvm::isa<CompareInst>(consumer) ||
                  consumer->getOpcode() == Opcode::Release) {
                break;
              }
              traceVal = consumer;
            } else {
              break;
            }
          }

          if (consumer) {
            if (consumer->getOpcode() == Opcode::Release) {
              isImplicitDrop = true;
            } else if (auto *call = llvm::dyn_cast<CallInst>(consumer)) {
              if (call->getCallee()) {
                std::string calleeName = call->getCallee()->getName();
                if (calleeName.find("destructor") != std::string::npos ||
                    calleeName.find("__moksha_free") != std::string::npos) {
                  isImplicitDrop = true;
                }
              }
            }
          }
        }

        bool isMovedOrDropped = false;
        for (auto &p : resolvePlace(load->getPointer())) {
          if (!p.base)
            continue;

          for (const Loan &active : activeLoans) {
            if (active.pointer == nullptr &&
                active.borrowedPlace.conflictsWith(p)) {

              if (isImplicitDrop) {
                isMovedOrDropped = true;
                break;
              }

              diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                  << "Use of moved value. The memory was previously moved or "
                     "dropped.";
              isMovedOrDropped = true;
              break;
            }
          }
          if (isMovedOrDropped)
            break;
        }

        if (isMovedOrDropped)
          continue;

        if (!isImplicitDrop) {
          bool reportedMove = false;
          std::vector<Place> loadPlaces = resolvePlace(load->getPointer());

          for (const Loan &active : activeLoans) {
            if (active.pointer != nullptr && active.pointer != LOCK_MARKER) {

              bool isInternalBorrow = false;

              if (load->getType()) {
                auto kind = load->getType()->getKind();
                if (kind == hir::TypeKind::Int ||
                    kind == hir::TypeKind::Float ||
                    kind == hir::TypeKind::Decimal ||
                    kind == hir::TypeKind::Bool ||
                    kind == hir::TypeKind::Pointer ||
                    kind == hir::TypeKind::Reference) {
                  continue;
                }
              }

              for (const Place &p : loadPlaces) {
                if (!p.base)
                  continue;

                if (!p.projections.empty()) {
                  continue;
                }

                for (const Place &ap : resolvePlace(active.pointer)) {
                  if (ap.base == p.base &&
                      active.borrowedPlace.base == p.base) {
                    isInternalBorrow = true;
                    break;
                  }
                }
                if (isInternalBorrow)
                  break;
              }

              if (isInternalBorrow) {
                diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                    << "Unsafe relocation: Moving a value type that contains "
                       "active internal borrows invalidates those pointers.";
                reportedMove = true;
                break;
              }

              bool isAggregateLoad = false;
              if (load->getType() &&
                  load->getBorrowKind() == BorrowKind::None) {
                auto kind = load->getType()->getKind();
                if (kind != hir::TypeKind::Int &&
                    kind != hir::TypeKind::Float &&
                    kind != hir::TypeKind::Decimal &&
                    kind != hir::TypeKind::Bool &&
                    kind != hir::TypeKind::Pointer &&
                    kind != hir::TypeKind::Reference &&
                    kind != hir::TypeKind::Function) {
                  isAggregateLoad = true;
                }
              }

              if (isAggregateLoad || isMoveType(load)) {
                bool conflictFound = false;
                for (const Place &p : loadPlaces) {
                  if (active.borrowedPlace.conflictsWith(p)) {
                    conflictFound = true;
                    break;
                  }
                }

                if (conflictFound) {
                  SourceLocation errorLoc = inst->getLoc();
                  if (consumer) {
                    errorLoc = consumer->getLoc();
                  }

                  diags.report(errorLoc, DiagID::err_borrow_violation)
                      << "Cannot move value out of memory because it is "
                         "currently borrowed.";
                  reportedMove = true;
                  break;
                }
              }
            }
          }
          if (reportedMove)
            continue;
        }

        for (auto &p : resolvePlace(load->getPointer())) {
          if (!p.base)
            continue;
          bool reported = false;

          for (const Loan &active : activeLoans) {
            if (!reported && active.isMut && active.pointer != nullptr &&
                active.borrowedPlace.conflictsWith(p)) {

              bool isSamePtr = false;
              if (isDerivedFrom(load->getPointer(), active.pointer) ||
                  isSamePointer(load->getPointer(), active.pointer) ||
                  active.pointer == LOCK_MARKER) {
                isSamePtr = true;
              }
              if (!isSamePtr) {
                diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                    << "Cannot borrow memory immutably because it is "
                       "currently "
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
            MIRValue *destPtr = store->getPointer();
            while (auto *cast = llvm::dyn_cast<CastInst>(destPtr)) {
              destPtr = cast->getValue();
            }
            if (auto *gep = llvm::dyn_cast<GetElementPtrInst>(destPtr)) {
              if (gep->getPointer() && gep->getPointer()->getType()) {
                std::string tyName = gep->getPointer()->getType()->toString();
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
              bool isSamePtr = false;
              if (isDerivedFrom(store->getPointer(), active.pointer) ||
                  isSamePointer(store->getPointer(), active.pointer) ||
                  active.pointer == LOCK_MARKER) {
                isSamePtr = true;
              }
              if (!isSamePtr && active.borrowedPlace.conflictsWith(dest)) {
                diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                    << "Cannot mutate memory because it is currently "
                       "borrowed.";
                reportedMutation = true;
                break;
              }
            }
          }
          if (reportedMutation)
            break;
        }

        if (isDestLongLived && isPointerType(store->getValue())) {
          bool isAggregateCopy = false;

          auto getPointeeIfPtr = [](MIRValue *v) -> const hir::HIRType * {
            if (!v || !v->getType())
              return nullptr;
            if (auto *ptrTy =
                    llvm::dyn_cast_or_null<hir::PointerType>(v->getType())) {
              return ptrTy->getPointee();
            }
            return nullptr;
          };

          const hir::HIRType *valPointee = getPointeeIfPtr(store->getValue());
          const hir::HIRType *ptrPointee = getPointeeIfPtr(store->getPointer());

          if (valPointee && ptrPointee) {
            // If we are storing a pointer to T into a pointer to T, it's a
            // value copy of T
            if (valPointee == ptrPointee ||
                valPointee->toString() == ptrPointee->toString()) {
              auto kind = valPointee->getKind();
              if (kind == hir::TypeKind::Array ||
                  kind == hir::TypeKind::Struct ||
                  kind == hir::TypeKind::Slice || kind == hir::TypeKind::Map ||
                  kind == hir::TypeKind::Closure ||
                  kind == hir::TypeKind::Any || kind == hir::TypeKind::String) {
                isAggregateCopy = true;
              }
            }
          }

          if (!isAggregateCopy) {
            for (const Place &srcPlace : srcPlaces) {
              if (srcPlace.base) {
                if (llvm::isa<AllocaInst>(srcPlace.base)) {
                  diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                      << "Local reference escapes its scope. Cannot store a "
                         "local reference into a global variable, heap object, "
                         "or caller argument.";
                  break;
                } else if (auto *global =
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

                    bool isSamePtr = false;
                    if (isDerivedFrom(store->getValue(), active.pointer) ||
                        isSamePointer(store->getValue(), active.pointer)) {
                      isSamePtr = true;
                    }

                    bool isMutAttempt = isExclusiveBorrow(store->getValue());

                    if (!isSamePtr && (isMutAttempt || active.isMut)) {
                      if (isMutAttempt) {
                        diags.report(inst->getLoc(),
                                     DiagID::err_borrow_violation)
                            << "Cannot borrow memory mutably because it is "
                               "already "
                               "borrowed.";
                      } else {
                        diags.report(inst->getLoc(),
                                     DiagID::err_borrow_violation)
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
            if (name.find("unlock") != std::string::npos) {
              isLockOrUnlock = true;
            } else if (name.find("lock") != std::string::npos) {
              isLockOrUnlock = true;
              if (!call->getArgs().empty()) {
                for (const Place &p : resolvePlace(call->getArgs()[0])) {
                  bool deadlock = false;

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

                  if (!deadlock) {
                    for (const Loan &active : activeLoans) {
                      if (active.pointer != LOCK_MARKER &&
                          active.borrowedPlace.conflictsWith(p)) {
                        bool isSamePtr = false;
                        if (isDerivedFrom(call->getArgs()[0], active.pointer) ||
                            isSamePointer(call->getArgs()[0], active.pointer)) {
                          isSamePtr = true;
                        }
                        if (!isSamePtr && active.isMut) {
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
                    bool isSamePtr = false;
                    if (isDerivedFrom(arg, active.pointer) ||
                        isSamePointer(arg, active.pointer) ||
                        active.pointer == LOCK_MARKER) {
                      isSamePtr = true;
                    }
                    bool isMutAttempt = isExclusiveBorrow(arg);
                    if (!isSamePtr && (isMutAttempt || active.isMut)) {
                      if (isMutAttempt) {
                        diags.report(inst->getLoc(),
                                     DiagID::err_borrow_violation)
                            << "Cannot pass memory to a function mutably "
                               "because "
                               "it is already borrowed.";
                      } else {
                        diags.report(inst->getLoc(),
                                     DiagID::err_borrow_violation)
                            << "Cannot pass memory to a function immutably "
                               "because it is currently borrowed mutably.";
                      }
                      reportedAlias = true;
                      break;
                    }
                  }
                }

                if (!reportedAlias) {
                  for (const Loan &intra : intraCallLoans) {
                    if (intra.borrowedPlace.conflictsWith(src)) {
                      bool isMutAttempt = isExclusiveBorrow(arg);
                      if (isMutAttempt || intra.isMut) {
                        diags.report(inst->getLoc(),
                                     DiagID::err_borrow_violation)
                            << "Cannot alias memory in function arguments. "
                               "Exclusive mutability violated.";
                        reportedAlias = true;
                        break;
                      }
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
              continue;
            }

            MIRValue *tracedCap = cap;

            bool isMutAttempt = isExclusiveBorrow(makeClosure);

            for (const Place &src : resolvePlace(tracedCap)) {
              if (!src.base)
                continue;
              bool reported = false;

              for (const Loan &active : activeLoans) {
                if (active.pointer == nullptr &&
                    active.borrowedPlace.conflictsWith(src)) {
                  if (isRef) {
                    diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                        << "Use of moved value. The memory was previously "
                           "moved "
                           "or "
                           "dropped.";
                    reported = true;
                    break;
                  }
                }

                if (!reported && active.pointer != nullptr &&
                    active.pointer != LOCK_MARKER &&
                    active.borrowedPlace.conflictsWith(src)) {

                  bool isSamePtr = false;
                  if (isDerivedFrom(tracedCap, active.pointer) ||
                      isSamePointer(tracedCap, active.pointer)) {
                    isSamePtr = true;
                  }

                  if (!isSamePtr) {
                    if (!isRef && isMoveType(cap)) {
                      diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                          << "Cannot capture value by move/copy because it is "
                             "currently borrowed.";
                      reported = true;
                      break;
                    } else if (isMutAttempt || active.isMut) {
                      if (isMutAttempt)
                        diags.report(inst->getLoc(),
                                     DiagID::err_borrow_violation)
                            << "Cannot capture memory mutably because it is "
                               "already borrowed.";
                      else
                        diags.report(inst->getLoc(),
                                     DiagID::err_borrow_violation)
                            << "Cannot capture memory immutably because it is "
                               "currently borrowed mutably.";
                      reported = true;
                      break;
                    }
                  }
                }
              }
              if (reported)
                break;
            }
          }
        } else if (auto *spawnInst = llvm::dyn_cast<SpawnInst>(inst)) {
          if (MIRValue *closureVal = spawnInst->getClosure()) {
            if (borrowsLocalMemory(closureVal)) {
              diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                  << "Cannot borrow local variable inside a thread block. "
                     "The thread might outlive the current stack frame, "
                     "causing "
                     "a dangling pointer.";
            }
          }
        } else if (auto *retInst = llvm::dyn_cast<ReturnInst>(inst)) {
          if (MIRValue *retVal = retInst->getReturnValue()) {

            if (borrowsLocalMemory(retVal)) {
              diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                  << "Cannot return a closure or reference that captures local "
                     "stack memory.";
            }
          }
        } else if (llvm::isa<AwaitInst>(inst)) {
          bool reportedLock = false;
          for (const Loan &active : activeLoans) {
            if (active.pointer == LOCK_MARKER) {
              diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                  << "Deadlock prevention: Cannot hold a lock across an "
                     "`await` suspension point.";
              reportedLock = true;
              break;
            }
          }

          if (!reportedLock) {
            for (const Loan &active : activeLoans) {
              if (active.pointer != nullptr && active.pointer != LOCK_MARKER &&
                  active.borrowedPlace.base &&
                  llvm::isa<AllocaInst>(active.borrowedPlace.base)) {
                diags.report(inst->getLoc(), DiagID::err_borrow_violation)
                    << "Cannot hold a borrow to local stack memory across an "
                       "`await` suspension point.";
                break;
              }
            }
          }
        }
      }
    }
  }
}

std::vector<Place> NLLBorrowChecker::resolvePlace(MIRValue *val) const {
  std::vector<Place> places;

  if (!val || val == LOCK_MARKER)
    return places;

  std::queue<std::pair<MIRValue *, std::vector<uint64_t>>> worklist;
  std::unordered_set<MIRValue *> visited;

  worklist.push({val, {}});

  while (!worklist.empty()) {
    auto [currentVal, proj] = worklist.front();
    worklist.pop();

    if (!currentVal || currentVal == LOCK_MARKER)
      continue;

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

            auto getStructuralBase =
                [](MIRValue *v,
                   std::vector<uint64_t> &outIndices) -> MIRValue * {
              while (v) {
                if (auto *i = llvm::dyn_cast<MIRInst>(v)) {
                  if (auto *cast = llvm::dyn_cast<CastInst>(i)) {
                    v = cast->getValue();
                  } else if (auto *ext = llvm::dyn_cast<ExtractValueInst>(i)) {
                    v = ext->getAggregate();
                  } else if (auto *gep = llvm::dyn_cast<GetElementPtrInst>(i)) {
                    v = gep->getPointer();
                    std::vector<uint64_t> tmp;
                    for (auto *idx : gep->getIndices()) {
                      if (auto *c = llvm::dyn_cast<ConstantInt>(idx))
                        tmp.push_back(c->getValue());
                      else
                        tmp.push_back(999999);
                    }
                    outIndices.insert(outIndices.end(), tmp.rbegin(),
                                      tmp.rend());
                  } else {
                    break;
                  }
                } else {
                  break;
                }
              }
              std::reverse(outIndices.begin(), outIndices.end());
              return v;
            };

            std::vector<uint64_t> loadProj;
            MIRValue *loadBase =
                getStructuralBase(load->getPointer(), loadProj);

            for (auto &bPtr : parentFunc->getBlocks()) {
              for (auto &iPtr : bPtr->getInstructions()) {
                if (auto *store = llvm::dyn_cast<StoreInst>(iPtr.get())) {
                  bool aliases = false;
                  if (store->getPointer() == load->getPointer()) {
                    aliases = true;
                  } else {
                    std::vector<uint64_t> storeProj;
                    MIRValue *storeBase =
                        getStructuralBase(store->getPointer(), storeProj);
                    if (loadBase && storeBase && loadBase == storeBase &&
                        loadProj == storeProj) {
                      aliases = true;
                    }
                  }

                  if (aliases && store->getValue()) {
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
      if (inst->getOpcode() == Opcode::MakeClosure) {
        auto *mc = static_cast<MakeClosureInst *>(inst);
        for (auto *cap : mc->getCaptures()) {
          if (cap && (isPointerType(cap) || isMoveType(cap))) {
            worklist.push({cap, proj});
          }
        }
        continue;
      }

      if (inst->getOpcode() == Opcode::Spawn) {
        auto *spawn = static_cast<SpawnInst *>(inst);
        if (spawn->getClosure()) {
          worklist.push({spawn->getClosure(), proj});
        }
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
