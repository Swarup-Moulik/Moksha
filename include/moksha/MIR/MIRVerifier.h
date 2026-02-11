#pragma once

#include "moksha/Support/SourceLocation.h"
#include <iosfwd>
#include <string>
#include <vector>

namespace moksha {

// [FIX] Forward declare HIRType in the correct namespace (outside mir)
namespace hir {
class HIRType;
}

namespace mir {

// Forward declarations
class MIRModule;
class MIRFunction;
class MIRBlock;
class MIRInst;
class MIRValue;

class MIRVerifier {
public:
  // Check a module for errors.
  [[nodiscard]] static bool verify(const MIRModule *module,
                                   std::ostream *os = nullptr,
                                   bool verbose = false);

  // Check a single function for errors.
  [[nodiscard]] static bool verify(const MIRFunction *func,
                                   std::ostream *os = nullptr,
                                   bool verbose = false);

  const std::vector<std::string> &getErrors() const { return errors; }

private:
  explicit MIRVerifier(std::ostream *os, bool verbose);

  bool verifyModule(const MIRModule *module);
  bool verifyFunction(const MIRFunction *func);
  bool verifyBlock(const MIRBlock *block);
  bool verifyInstruction(const MIRInst *inst);

  bool checkUnreachableBlocks(const MIRFunction *func);

  // [FIX] Use hir::HIRType fully qualified or ensure namespace visibility
  bool verifyTypesMatch(const MIRValue *val1, const MIRValue *val2,
                        const std::string &msg,
                        const MIRInst *contextInst = nullptr);
  bool verifyType(const MIRValue *val, const hir::HIRType *expected,
                  const std::string &msg, const MIRInst *contextInst = nullptr);

  void logError(const std::string &msg);
  void logError(const std::string &msg, SourceLocation loc);
  void logError(const std::string &msg, const MIRInst *context);
  void logError(const std::string &msg, const MIRBlock *context);
  void logError(const std::string &msg, const MIRFunction *context);

  void logVerbose(const std::string &msg);

  std::ostream *os;
  bool hasError;
  bool verbose;
  std::vector<std::string> errors;
};

} // namespace mir
} // namespace moksha
