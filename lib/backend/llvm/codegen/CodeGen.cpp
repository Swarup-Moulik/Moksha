#include "moksha/Backend/LLVM/CodeGen.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Transforms/Coroutines/CoroCleanup.h"
#include "llvm/Transforms/Coroutines/CoroEarly.h"
#include "llvm/Transforms/Coroutines/CoroElide.h"
#include "llvm/Transforms/Coroutines/CoroSplit.h"

namespace moksha {

bool emitObjectCode(llvm::Module &llvmModule, const std::string &outputFilename,
                    const TargetConfig &config) {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  // --- FIX 1: Use std::string for lookup, but llvm::Triple for configuration
  // ---
  std::string targetTripleStr = config.triple.empty()
                                    ? llvm::sys::getDefaultTargetTriple()
                                    : config.triple;

  llvm::Triple targetTriple(targetTripleStr);
  llvmModule.setTargetTriple(targetTriple); // Now passes the Triple object!

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
  if (!target) {
    llvm::errs() << "Target lookup failed: " << error << "\n";
    return false;
  }

  std::string cpu = config.cpu.empty() ? "generic" : config.cpu;
  std::string features = config.features;
  llvm::TargetOptions opt;
  auto rm = std::optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);

  llvm::CodeGenOptLevel llvmOptLevel;
  switch (config.optLevel) {
  case 0:
    llvmOptLevel = llvm::CodeGenOptLevel::None;
    break;
  case 1:
    llvmOptLevel = llvm::CodeGenOptLevel::Less;
    break;
  case 3:
    llvmOptLevel = llvm::CodeGenOptLevel::Aggressive;
    break;
  case 2:
  default:
    llvmOptLevel = llvm::CodeGenOptLevel::Default;
    break;
  }

  // --- FIX 2: Pass the Triple object, not the string ---
  auto targetMachine = target->createTargetMachine(
      targetTriple, cpu, features, opt, rm, std::nullopt, llvmOptLevel);

  if (!targetMachine) {
    llvm::errs() << "Could not allocate target machine.\n";
    return false;
  }

  llvmModule.setDataLayout(targetMachine->createDataLayout());

  std::error_code errorCode;
  llvm::raw_fd_ostream dest(outputFilename, errorCode, llvm::sys::fs::OF_None);
  if (errorCode) {
    llvm::errs() << "Could not open file: " << errorCode.message() << "\n";
    return false;
  }

  // Run the Middle-End optimization passes to split the coroutines
  llvm::LoopAnalysisManager lam;
  llvm::FunctionAnalysisManager fam;
  llvm::CGSCCAnalysisManager cgam;
  llvm::ModuleAnalysisManager mam;

  llvm::PassBuilder pb(targetMachine);
  pb.registerModuleAnalyses(mam);
  pb.registerCGSCCAnalyses(cgam);
  pb.registerFunctionAnalyses(fam);
  pb.registerLoopAnalyses(lam);
  pb.crossRegisterProxies(lam, fam, cgam, mam);

  llvm::ModulePassManager mpm =
      pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
  mpm.run(llvmModule, mam);

  llvm::legacy::PassManager pass;
  auto fileType = llvm::CodeGenFileType::ObjectFile;

  if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
    llvm::errs() << "TargetMachine can't emit a file of this type.\n";
    return false;
  }

  pass.run(llvmModule);
  dest.flush();

  return true;
}

} // namespace moksha
