#include "moksha/MIR/Analysis/MIRDominance.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRInst.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace moksha {
namespace mir {

namespace {

std::vector<MIRValue *> getOperands(MIRInst *inst) {
  std::vector<MIRValue *> ops;

  if (auto *bin = llvm::dyn_cast<BinaryInst>(inst)) {
    ops.push_back(bin->getLHS());
    ops.push_back(bin->getRHS());
  } else if (auto *store = llvm::dyn_cast<StoreInst>(inst)) {
    ops.push_back(store->getValue());
    ops.push_back(store->getPointer());
  } else if (auto *load = llvm::dyn_cast<LoadInst>(inst)) {
    ops.push_back(load->getPointer());
  } else if (auto *storeW = llvm::dyn_cast<StoreWeakInst>(inst)) {
    ops.push_back(storeW->getValue());
    ops.push_back(storeW->getPointer());
  } else if (auto *loadW = llvm::dyn_cast<LoadWeakInst>(inst)) {
    ops.push_back(loadW->getPointer());
  } else if (auto *arc = llvm::dyn_cast<ARCInst>(inst)) {
    ops.push_back(arc->getObject());
  } else if (auto *cond = llvm::dyn_cast<CondBranchInst>(inst)) {
    ops.push_back(cond->getCondition());
  } else if (auto *ret = llvm::dyn_cast<ReturnInst>(inst)) {
    if (auto *val = ret->getReturnValue()) {
      ops.push_back(val);
    }
  } else if (auto *call = llvm::dyn_cast<CallInst>(inst)) {
    ops.push_back(call->getCallee());
    for (auto *arg : call->getArgs())
      ops.push_back(arg);
    // --- [NEW] Hardware & Exceptions ---
  } else if (auto *inv = llvm::dyn_cast<InvokeInst>(inst)) {
    ops.push_back(inv->getCallee());
    for (auto *arg : inv->getArgs())
      ops.push_back(arg);
  } else if (auto *res = llvm::dyn_cast<ResumeInst>(inst)) {
    ops.push_back(res->getException());
  } else if (auto *thr = llvm::dyn_cast<ThrowInst>(inst)) {
    ops.push_back(thr->getException());
  } else if (auto *ia = llvm::dyn_cast<InlineAsmInst>(inst)) {
    for (auto *arg : ia->getArgs())
      ops.push_back(arg);
  } else if (auto *makeClosure = llvm::dyn_cast<MakeClosureInst>(inst)) {
    ops.push_back(makeClosure->getFunctionPointer());
    for (auto *cap : makeClosure->getCaptures()) {
      ops.push_back(cap);
    }
  } else if (auto *spawn = llvm::dyn_cast<SpawnInst>(inst)) {
    ops.push_back(spawn->getClosure());
  } else if (auto *awaitInst = llvm::dyn_cast<AwaitInst>(inst)) {
    ops.push_back(awaitInst->getPromise());
  } else if (auto *ext = llvm::dyn_cast<ExtractValueInst>(inst)) {
    ops.push_back(ext->getAggregate());
  } else if (auto *ins = llvm::dyn_cast<InsertValueInst>(inst)) {
    ops.push_back(ins->getAggregate());
    ops.push_back(ins->getValue());
  }

  return ops;
}

void computeRPO(MIRBlock *entry, std::vector<MIRBlock *> &rpo,
                std::unordered_set<MIRBlock *> &visited) {
  if (visited.count(entry))
    return;
  visited.insert(entry);
  for (auto *succ : entry->getSuccessors()) {
    computeRPO(succ, rpo, visited);
  }
  rpo.push_back(entry);
}

} // namespace

void MIRDominance::analyze() {
  if (func->getBlocks().empty())
    return;

  MIRBlock *entry = func->getEntryBlock();
  idoms.clear();
  domTree.clear();
  dfsNumbers.clear();
  depths.clear();

  std::vector<MIRBlock *> rpoStack;
  std::unordered_set<MIRBlock *> rpoVisited;
  computeRPO(entry, rpoStack, rpoVisited);

  std::unordered_map<MIRBlock *, int> rpoIndex;
  int idx = 0;
  for (auto it = rpoStack.rbegin(); it != rpoStack.rend(); ++it) {
    rpoIndex[*it] = idx++;
  }

  idoms[entry] = entry;

  bool changed = true;
  while (changed) {
    changed = false;
    for (auto it = rpoStack.rbegin(); it != rpoStack.rend(); ++it) {
      MIRBlock *block = *it;
      if (block == entry)
        continue;

      MIRBlock *newIDom = nullptr;
      for (auto *pred : block->getPredecessors()) {
        if (idoms.count(pred)) {
          newIDom = pred;
          break;
        }
      }

      if (newIDom) {
        for (auto *pred : block->getPredecessors()) {
          if (pred != newIDom && idoms.count(pred)) {
            newIDom = intersect(pred, newIDom, rpoIndex);
          }
        }
        if (idoms[block] != newIDom) {
          idoms[block] = newIDom;
          changed = true;
        }
      }
    }
  }

  // Compute Depths
  for (auto &blockPtr : func->getBlocks()) {
    MIRBlock *curr = blockPtr.get();
    int d = 0;
    while (curr != entry) {
      if (!idoms.count(curr) || idoms[curr] == curr)
        break;
      curr = idoms[curr];
      d++;
    }
    depths[blockPtr.get()] = d;
  }

  // Build Dominance Tree
  for (auto const &[block, idom] : idoms) {
    if (block != idom)
      domTree[idom].push_back(block);
  }

  // Compute DFS numbers
  int counter = 0;
  std::unordered_set<MIRBlock *> visited;
  computeDFS(entry, counter, visited);
}

MIRBlock *MIRDominance::intersect(
    MIRBlock *b1, MIRBlock *b2,
    const std::unordered_map<MIRBlock *, int> &rpoIndex) const {
  while (b1 != b2) {
    while (rpoIndex.at(b1) > rpoIndex.at(b2))
      b1 = idoms.at(b1);
    while (rpoIndex.at(b2) > rpoIndex.at(b1))
      b2 = idoms.at(b2);
  }
  return b1;
}

void MIRDominance::computeDFS(MIRBlock *root, int &counter,
                              std::unordered_set<MIRBlock *> &visited) {
  if (visited.count(root))
    return;
  visited.insert(root);
  int start = ++counter;
  for (auto *child : domTree[root])
    computeDFS(child, counter, visited);
  dfsNumbers[root] = {start, counter};
}

bool MIRDominance::dominates(MIRBlock *a, MIRBlock *b) const {
  if (a == b)
    return true;
  auto itA = dfsNumbers.find(a);
  auto itB = dfsNumbers.find(b);
  if (itA == dfsNumbers.end() || itB == dfsNumbers.end())
    return false;
  return itA->second.first <= itB->second.first &&
         itA->second.second >= itB->second.second;
}

bool MIRDominance::verifySSA() const {
  for (auto &blockPtr : func->getBlocks()) {
    MIRBlock *useBlock = blockPtr.get();
    for (auto &instPtr : useBlock->getInstructions()) {
      MIRInst *inst = instPtr.get();
      if (auto *phi = llvm::dyn_cast<PhiInst>(inst)) {
        for (auto const &[val, pred] : phi->getIncoming()) {
          if (auto *defInst = llvm::dyn_cast<MIRInst>(val)) {
            if (!dominates(defInst->getParent(), pred))
              return false;
          }
        }
        continue;
      }
      for (MIRValue *op : getOperands(inst)) {
        if (auto *defInst = llvm::dyn_cast<MIRInst>(op)) {
          MIRBlock *defBlock = defInst->getParent();
          if (!dominates(defBlock, useBlock))
            return false;
        }
      }
    }
  }
  return true;
}

MIRBlock *MIRDominance::getIDom(MIRBlock *block) const {
  auto it = idoms.find(block);
  return (it != idoms.end()) ? it->second : nullptr;
}

const std::vector<MIRBlock *> &
MIRDominance::getChildren(MIRBlock *block) const {
  static const std::vector<MIRBlock *> empty;
  auto it = domTree.find(block);
  return (it != domTree.end()) ? it->second : empty;
}

} // namespace mir
} // namespace moksha
