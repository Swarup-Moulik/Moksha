#include "moksha/Backend/LLVM/CodeGen.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {
/** @brief Emits the object code from the given LLVM module. */
bool emitObjectCode(llvm::Module &llvmModule, const std::string &outputFilename,
                    const TargetConfig &config) {
  std::error_code errorCode;
  llvm::raw_fd_ostream dest(outputFilename, errorCode, llvm::sys::fs::OF_None);

  if (errorCode) {
    llvm::errs() << "Could not open file: " << errorCode.message() << "\n";
    return false;
  }
  llvmModule.print(dest, nullptr);
  dest.flush();
  return true;
}
} // namespace moksha
