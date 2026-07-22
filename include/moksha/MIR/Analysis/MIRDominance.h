#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace moksha {
namespace mir {

class MIRBlock;
class MIRFunction;
class MIRValue;

/** @brief Identifies control-flow dependencies where execution of one block
 * guarantees the prior execution of another. */
class MIRDominance {
public:
  explicit MIRDominance(MIRFunction *func) : func(func) {}
  void analyze();
  bool dominates(MIRBlock *a, MIRBlock *b) const;
  bool dominates(MIRValue *def, MIRBlock *useBlock) const;
  MIRBlock *getIDom(MIRBlock *block) const;
  const std::vector<MIRBlock *> &getChildren(MIRBlock *block) const;
  bool verifySSA() const;

private:
  MIRFunction *func;

  std::unordered_map<MIRBlock *, int> depths;
  std::unordered_map<MIRBlock *, MIRBlock *> idoms;
  std::unordered_map<MIRBlock *, std::vector<MIRBlock *>> domTree;
  std::unordered_map<MIRBlock *, std::pair<int, int>> dfsNumbers;
  void computeDFS(MIRBlock *root, int &counter,
                  std::unordered_set<MIRBlock *> &visited);
  MIRBlock *
  intersect(MIRBlock *b1, MIRBlock *b2,
            const std::unordered_map<MIRBlock *, int> &rpoIndex) const;
};

} // namespace mir
} // namespace moksha
