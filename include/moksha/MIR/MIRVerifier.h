#pragma once

#include "moksha/Support/SourceLocation.h"
#include "llvm/Support/raw_ostream.h"
#include <iosfwd>
#include <string>
#include <vector>

namespace moksha {

// Forward declare HIRType in the correct namespace (outside mir)
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

/** @brief Verifies the MIR module and functions for errors. */
class MIRVerifier {
public:
  explicit MIRVerifier(llvm::raw_ostream *os = nullptr, bool verbose = false);
  [[nodiscard]] bool verify(const MIRModule *module);
  [[nodiscard]] bool verify(const MIRFunction *func);
  const std::vector<std::string> &getErrors() const { return errors; }
  bool hasFailed() const { return hasError; }

private:
  bool verifyModule(const MIRModule *module);
  bool verifyFunction(const MIRFunction *func);
  bool verifyBlock(const MIRBlock *block);
  bool verifyInstruction(const MIRInst *inst);
  bool checkUnreachableBlocks(const MIRFunction *func);
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

  llvm::raw_ostream *os;
  bool hasError;
  bool verbose;
  std::vector<std::string> errors;
};

} // namespace mir
} // namespace moksha
