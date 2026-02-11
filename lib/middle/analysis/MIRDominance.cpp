#include "moksha/MIR/Analysis/MIRDominance.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRInst.h"
#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace moksha {
namespace mir {

namespace {

std::vector<MIRBlock *> getSuccessors(MIRBlock *block) {
  std::vector<MIRBlock *> succs;
  if (!block || block->getInstructions().empty())
    return succs;

  MIRInst *term = block->getInstructions().back().get();
  if (auto *br = dynamic_cast<BranchInst *>(term)) {
    succs.push_back(br->getTarget());
  } else if (auto *cbr = dynamic_cast<CondBranchInst *>(term)) {
    // [FIX] Use correct accessor names
    succs.push_back(cbr->getTrueBlock());
    succs.push_back(cbr->getFalseBlock());
  }
  return succs;
}

std::vector<MIRValue *> getOperands(MIRInst *inst) {
  std::vector<MIRValue *> ops;

  if (auto *bin = dynamic_cast<BinaryInst *>(inst)) {
    ops.push_back(bin->getLHS());
    ops.push_back(bin->getRHS());
  } else if (auto *store = dynamic_cast<StoreInst *>(inst)) {
    ops.push_back(store->getValue());
    ops.push_back(store->getPointer());
  } else if (auto *load = dynamic_cast<LoadInst *>(inst)) {
    ops.push_back(load->getPointer());

    // [FIX 1] Use getObject() for ARCInst
  } else if (auto *arc = dynamic_cast<ARCInst *>(inst)) {
    ops.push_back(arc->getObject());

  } else if (auto *cond = dynamic_cast<CondBranchInst *>(inst)) {
    ops.push_back(cond->getCondition());

    // [FIX 2] Use getReturnValue() for ReturnInst
  } else if (auto *ret = dynamic_cast<ReturnInst *>(inst)) {
    if (auto *val = ret->getReturnValue()) {
      ops.push_back(val);
    }
  }

  return ops;
}
} // namespace

void computeRPO(MIRBlock *entry, std::vector<MIRBlock *> &rpo,
                std::unordered_set<MIRBlock *> &visited) {
  if (visited.count(entry))
    return;
  visited.insert(entry);
  for (auto *succ : getSuccessors(entry)) {
    computeRPO(succ, rpo, visited);
  }
  rpo.push_back(entry);
}

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
      if (auto *phi = dynamic_cast<PhiInst *>(inst)) {
        for (auto const &[val, pred] : phi->getIncoming()) {
          if (auto *defInst = dynamic_cast<MIRInst *>(val)) {
            if (!dominates(defInst->getParent(), pred))
              return false;
          }
        }
        continue;
      }
      for (MIRValue *op : getOperands(inst)) {
        if (auto *defInst = dynamic_cast<MIRInst *>(op)) {
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
