#include "moksha/Backend/LLVM/CodeGen.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {
bool emitObjectCode(llvm::Module &llvmModule, const std::string &outputFilename,
                    const TargetConfig &config) {
  std::error_code errorCode;
  llvm::raw_fd_ostream dest(outputFilename, errorCode, llvm::sys::fs::OF_None);

  if (errorCode) {
    llvm::errs() << "Could not open file: " << errorCode.message() << "\n";
    return false;
  }

  // Simply dump the IR. Clang will run the coroutine passes for us.
  llvmModule.print(dest, nullptr);
  dest.flush();
  return true;
}
} // namespace moksha
