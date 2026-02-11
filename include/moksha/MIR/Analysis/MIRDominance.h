#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace moksha {
namespace mir {

class MIRBlock;
class MIRFunction;
class MIRValue;

/// \brief Computes and stores dominance relationships for a MIR Function.
///
/// This analysis is essential for verifying SSA (Static Single Assignment)
/// invariants: specifically, that every definition must dominate its uses.
class MIRDominance {
public:
  explicit MIRDominance(MIRFunction *func) : func(func) {}

  /// \brief Computes the dominator tree using the Lengauer-Tarjan algorithm
  /// or a simpler iterative bit-vector approach.
  void analyze();

  /// \brief Checks if block A dominates block B.
  /// \note A block always dominates itself.
  bool dominates(MIRBlock *a, MIRBlock *b) const;

  /// \brief Checks if the definition of a value dominates its use at a
  /// specific instruction location.
  bool dominates(MIRValue *def, MIRBlock *useBlock) const;

  /// \brief Returns the immediate dominator (IDom) of a block.
  MIRBlock *getIDom(MIRBlock *block) const;

  /// \brief Returns the set of blocks dominated by this block (Dominance Tree
  /// children).
  const std::vector<MIRBlock *> &getChildren(MIRBlock *block) const;

  /// \brief Validates that all value uses in the function satisfy SSA dominance
  /// rules.
  /// \return true if the function is valid SSA.
  bool verifySSA() const;

private:
  MIRFunction *func;

  std::unordered_map<MIRBlock *, int> depths;

  /// Mapping from a block to its immediate dominator.
  std::unordered_map<MIRBlock *, MIRBlock *> idoms;

  /// Mapping from a block to the list of blocks it immediately dominates.
  std::unordered_map<MIRBlock *, std::vector<MIRBlock *>> domTree;

  /// Pre-order and Post-order numbers for fast dominance queries (O(1)).
  std::unordered_map<MIRBlock *, std::pair<int, int>> dfsNumbers;

  void computeDFS(MIRBlock *root, int &counter,
                  std::unordered_set<MIRBlock *> &visited);

  MIRBlock *
  intersect(MIRBlock *b1, MIRBlock *b2,
            const std::unordered_map<MIRBlock *, int> &rpoIndex) const;
};

} // namespace mir
} // namespace moksha
