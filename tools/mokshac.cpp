#include "mlir/Conversion/ReconcileUnrealizedCasts/ReconcileUnrealizedCasts.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Pass/PassManager.h"
#include "moksha/AST/ASTContext.h"
#include "moksha/AST/ASTPrinter.h"
#include "moksha/Backend/LLVM/CodeGen.h"
#include "moksha/Backend/LLVM/MLIRToLLVM.h"
#include "moksha/Backend/LLVM/TranslateToLLVM.h"
#include "moksha/Backend/MLIR/MIRToMLIR.h"
#include "moksha/Backend/MLIR/Passes/CanonicalizeARC.h"
#include "moksha/Builtins/BuiltinRegistry.h"
#include "moksha/Dialect/MokshaDialect.h"
#include "moksha/HIR/HIRModule.h"
#include "moksha/Lexer/Lexer.h"
#include "moksha/MIR/Analysis/NLLBorrowChecker.h"
#include "moksha/MIR/LowerHIRToMIR.h"
#include "moksha/MIR/MIRModule.h"
#include "moksha/MIR/MIRVerifier.h"
#include "moksha/MIR/Passes/ConstantFoldingPass.h"
#include "moksha/MIR/Passes/DeadCodeEliminationPass.h"
#include "moksha/MIR/Passes/DropElisionPass.h"
#include "moksha/MIR/Passes/EscapeAnalysisPass.h"
#include "moksha/MIR/Passes/InliningPass.h"
#include "moksha/MIR/Passes/JumpThreadingPass.h"
#include "moksha/MIR/Passes/Mem2RegPass.h"
#include "moksha/MIR/Passes/PassManager.h"
#include "moksha/MIR/Passes/SROAPass.h"
#include "moksha/MIR/Passes/SimplifyCFGPass.h"
#include "moksha/MIR/VerifyNoMacros.h"
#include "moksha/Macros/Macro.h"
#include "moksha/Ownership/ARCAnalyzer.h"
#include "moksha/Ownership/ARCInserter.h"
#include "moksha/Parser/Parser.h"
#include "moksha/Sema/SymbolTable.h"
#include "moksha/Sema/TypeChecker.h"
#include "moksha/Support/Diagnostics.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ============================================================================
// LLD Linker Forward Declarations & Helper
// ============================================================================
namespace lld {
namespace elf {
bool link(llvm::ArrayRef<const char *> args, llvm::raw_ostream &stdoutOS,
          llvm::raw_ostream &stderrOS, bool exitEarly, bool disableOutput);
}
namespace coff {
bool link(llvm::ArrayRef<const char *> args, llvm::raw_ostream &stdoutOS,
          llvm::raw_ostream &stderrOS, bool exitEarly, bool disableOutput);
}
namespace macho {
bool link(llvm::ArrayRef<const char *> args, llvm::raw_ostream &stdoutOS,
          llvm::raw_ostream &stderrOS, bool exitEarly, bool disableOutput);
}
namespace wasm {
bool link(llvm::ArrayRef<const char *> args, llvm::raw_ostream &stdoutOS,
          llvm::raw_ostream &stderrOS, bool exitEarly, bool disableOutput);
}
} // namespace lld

bool linkExecutable(const std::string &objFile, const std::string &outFile,
                    const std::string &targetTripleStr,
                    const std::string &rtLibPath) {
  llvm::Triple triple(targetTripleStr);
  std::vector<const char *> args;

  if (triple.isOSWindows()) {
    args = {"lld-link",
            objFile.c_str(),
            ("/OUT:" + outFile).c_str(),
            "-defaultlib:libcmt",
            "-defaultlib:oldnames",
            "-nologo",
            rtLibPath.c_str()};
    return lld::coff::link(args, llvm::outs(), llvm::errs(), false, false);
  } else if (triple.isOSDarwin()) {
    args = {"ld64.lld",      objFile.c_str(), "-o",
            outFile.c_str(), "-lSystem",      rtLibPath.c_str()};
    return lld::macho::link(args, llvm::outs(), llvm::errs(), false, false);
  } else if (triple.isWasm()) {
    args = {"wasm-ld",        objFile.c_str(), "-o",
            outFile.c_str(),  "--no-entry",    "--export-all",
            rtLibPath.c_str()};
    return lld::wasm::link(args, llvm::outs(), llvm::errs(), false, false);
  } else { // Linux / Baremetal / Android
    args = {"ld.lld", objFile.c_str(),  "-o", outFile.c_str(), "-lc",
            "-lm",    rtLibPath.c_str()};
    return lld::elf::link(args, llvm::outs(), llvm::errs(), false, false);
  }
}

namespace moksha {
std::unique_ptr<hir::HIRModule> lowerASTToHIR(const ModuleDecl *astModule,
                                              ASTContext &ctx);
}

using namespace moksha;
namespace cl = llvm::cl;

static cl::OptionCategory MokshaCategory("Moksha Compiler Options");

static cl::opt<std::string> InputFilename(cl::Positional,
                                          cl::desc("<input file>"),
                                          cl::Required,
                                          cl::cat(MokshaCategory));

static cl::opt<bool> DumpAST("dump-ast", cl::desc("Print the AST"),
                             cl::cat(MokshaCategory));
static cl::opt<bool> DumpHIR("dump-hir", cl::desc("Print the HIR"),
                             cl::cat(MokshaCategory));
static cl::opt<bool> RunHIRPasses("run-hir-passes", cl::desc("Run HIR passes"),
                                  cl::cat(MokshaCategory));
static cl::opt<bool> DumpMIR("dump-mir", cl::desc("Print the MIR"),
                             cl::cat(MokshaCategory));
static cl::opt<bool> RunMIRPasses("run-mir-passes", cl::desc("Run MIR passes"),
                                  cl::cat(MokshaCategory));
static cl::opt<bool> DumpMLIR("dump-mlir", cl::desc("Dump the MLIR"),
                              cl::cat(MokshaCategory));
static cl::opt<bool> DumpLLVM("dump-llvm", cl::desc("Dump the LLVM IR"),
                              cl::cat(MokshaCategory));
static cl::opt<bool> EmitObj("emit-obj",
                             cl::desc("Emit a native object file only"),
                             cl::cat(MokshaCategory));
static cl::opt<std::string> OutputFilename("o",
                                           cl::desc("Specify output filename"),
                                           cl::value_desc("filename"),
                                           cl::init("a.out"),
                                           cl::cat(MokshaCategory));
static cl::opt<std::string>
    TargetTriple("target",
                 cl::desc("Target triple (e.g. x86_64-pc-windows-msvc)"),
                 cl::init(""), cl::cat(MokshaCategory));
static cl::opt<std::string> TargetCPU("mcpu", cl::desc("Target specific CPU"),
                                      cl::init(""), cl::cat(MokshaCategory));
static cl::opt<std::string>
    TargetFeatures("mattr", cl::desc("Target specific attributes"),
                   cl::init(""), cl::cat(MokshaCategory));
static cl::opt<bool> DisableOpt("O0", cl::desc("Disable backend optimizations"),
                                cl::init(false), cl::cat(MokshaCategory));

// Allows pointing to the built runtime lib
static cl::opt<std::string> RuntimeLib("rt-lib",
                                       cl::desc("Path to libmokshaRuntime.a"),
                                       cl::init("libmokshaRuntime.a"),
                                       cl::cat(MokshaCategory));

int main(int argc, char **argv) {
  llvm::InitLLVM X(argc, argv);
  cl::HideUnrelatedOptions(MokshaCategory);
  cl::ParseCommandLineOptions(argc, argv, "Moksha Compiler\n");

  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> fileOrErr =
      llvm::MemoryBuffer::getFile(InputFilename);

  if (std::error_code ec = fileOrErr.getError()) {
    llvm::errs() << "Error opening file '" << InputFilename
                 << "': " << ec.message() << "\n";
    return 1;
  }

  llvm::SourceMgr srcMgr;
  unsigned id = srcMgr.AddNewSourceBuffer(std::move(*fileOrErr), llvm::SMLoc());

  DiagnosticEngine diags(srcMgr);
  ASTContext astContext;
  SymbolTable symbolTable(diags);

  symbolTable.addPrimitiveTypes(astContext);
  BuiltinRegistry::registerBuiltins(astContext, symbolTable);

  Lexer lexer(srcMgr.getMemoryBuffer(id)->getBuffer(), diags);
  Parser parser(lexer, astContext, srcMgr, diags);

  llvm::outs() << "Compiling " << InputFilename << "...\n";
  auto moduleAST = parser.parseModule();

  if (!moduleAST || diags.hasErrors()) {
    llvm::errs() << "Parsing failed with " << diags.getNumErrors()
                 << " error(s).\n";
    return 1;
  }

  MacroExpander macroExpander(astContext, diags);
  moduleAST->accept(macroExpander);

  TypeChecker typeChecker(astContext, symbolTable, diags);
  typeChecker.check(moduleAST.get());

  if (typeChecker.hasErrors() || diags.hasErrors()) {
    llvm::errs() << "Semantic analysis failed with " << diags.getNumErrors()
                 << " error(s).\n";
    return 1;
  }

  if (DumpAST) {
    ASTPrinter printer(llvm::outs());
    printer.print(moduleAST.get());
  }

  auto hirModule = lowerASTToHIR(moduleAST.get(), astContext);

  if (DumpHIR) {
    llvm::outs() << "\n=== HIR Dump ===\n";
    hirModule->dump(llvm::outs());
    llvm::outs() << "================\n\n";
  }

  if (!mir::VerifyNoMacros(hirModule.get(), diags)) {
    llvm::errs() << "Compilation halted: Unexpanded macros found in HIR.\n";
    return 1;
  }

  auto mirModule = mir::LowerHIRToMIR(hirModule.get(), diags);
  if (!mirModule) {
    llvm::errs() << "Failed to lower HIR to MIR.\n";
    return 1;
  }

  if (RunMIRPasses || (!DumpAST && !DumpHIR && !DumpMIR)) {
    mir::runARCInsertion(mirModule.get(), diags);
    mir::NLLBorrowChecker nllChecker(diags);
    nllChecker.checkModule(mirModule.get());

    if (diags.hasErrors()) {
      llvm::errs() << "NLL Borrow checking failed with " << diags.getNumErrors()
                   << " error(s).\n";
      return 1;
    }

    mir::PassManager pm;
    pm.addPass(std::make_unique<mir::InliningPass>());
    pm.addPass(std::make_unique<mir::EscapeAnalysisPass>());
    pm.addPass(std::make_unique<mir::DropElisionPass>());
    pm.addPass(std::make_unique<mir::SROAPass>());
    pm.addPass(std::make_unique<mir::Mem2RegPass>());
    pm.addPass(std::make_unique<mir::JumpThreadingPass>());
    pm.addPass(std::make_unique<mir::ConstantFoldingPass>());
    pm.addPass(std::make_unique<mir::SimplifyCFGPass>());
    pm.addPass(std::make_unique<mir::DeadCodeEliminationPass>());
    pm.run(*mirModule);
    mir::runARCOptimization(mirModule.get(), diags);

    mir::MIRVerifier verifier(&llvm::errs(), true);
    if (!verifier.verify(mirModule.get())) {
      llvm::errs() << "MIR Verification failed!\n";
      return 1;
    }
  }

  if (DumpMIR) {
    std::cout << "\n=== MIR Dump ===\n";
    mirModule->dump(llvm::outs());
    std::cout << "================\n\n";
  }

  mlir::MLIRContext mlirContext;
  mlirContext.disableMultithreading();
  mlirContext.getOrLoadDialect<::moksha::IR::MokshaDialect>();
  mlirContext.getOrLoadDialect<::mlir::func::FuncDialect>();
  mlirContext.getOrLoadDialect<::mlir::cf::ControlFlowDialect>();
  mlirContext.getOrLoadDialect<::mlir::LLVM::LLVMDialect>();

  auto mlirModule =
      moksha::backend::mlir::convertMIRToMLIR(*mirModule, mlirContext, diags);
  if (!mlirModule) {
    llvm::errs() << "Failed to generate MLIR!\n";
    return 1;
  }

  mlir::PassManager mlirPM(&mlirContext);
  if (mlir::failed(mlirPM.run(*mlirModule))) {
    llvm::errs() << "Failed to run MLIR passes!\n";
    return 1;
  }

  if (DumpMLIR) {
    llvm::outs() << "\n=== MLIR Dump ===\n";
    mlirModule->print(llvm::outs());
    llvm::outs() << "\n================\n\n";
  }

  // ==========================================================================
  // 11. GENERATE LLVM IR & OBJECT CODE
  // ==========================================================================

  // Always emit if DumpLLVM or regular compilation (not just emitting an
  // object)
  bool shouldCompile = !DumpAST && !DumpHIR && !DumpMIR && !DumpMLIR;

  if (DumpLLVM || EmitObj || shouldCompile) {
    mlir::PassManager llvmPM(&mlirContext);
    llvmPM.addPass(moksha::createConvertMokshaToLLVMPass());
    llvmPM.addPass(mlir::createReconcileUnrealizedCastsPass());
    if (mlir::failed(llvmPM.run(*mlirModule))) {
      llvm::errs() << "Failed to lower MLIR to LLVM Dialect!\n";
      return 1;
    }

    llvm::LLVMContext llvmCtx;
    auto llvmModule =
        moksha::translateMokshaToLLVMIR(mlirModule.get(), llvmCtx);

    if (!llvmModule) {
      llvm::errs() << "Failed to translate MLIR to LLVM IR!\n";
      return 1;
    }

    if (DumpLLVM) {
      llvm::outs() << "\n=== LLVM IR Dump ===\n";
      llvmModule->print(llvm::outs(), nullptr);
      llvm::outs() << "====================\n\n";
    }

    if (EmitObj || shouldCompile) {
      moksha::TargetConfig config;
      config.triple = TargetTriple.empty() ? llvm::sys::getDefaultTargetTriple()
                                           : TargetTriple;
      config.cpu = TargetCPU;
      config.features = TargetFeatures;
      config.optLevel = DisableOpt ? 0 : 2;

      llvm::Triple actualTriple(config.triple);

      // Determine filenames
      std::string exeFilename = OutputFilename;
      if (actualTriple.isOSWindows() &&
          !llvm::StringRef(exeFilename).ends_with(".exe") && !EmitObj) {
        exeFilename += ".exe";
      }

      // If we are just emitting an object file, output directly to
      // OutputFilename. Otherwise, emit to a temporary .o file, then link.
      std::string objFilename = EmitObj ? OutputFilename : exeFilename + ".o";

      if (!moksha::emitObjectCode(*llvmModule, objFilename, config)) {
        llvm::errs() << "Failed to emit object code!\n";
        return 1;
      }

      if (EmitObj) {
        llvm::outs() << "Successfully generated object file: " << objFilename
                     << "\n";
      } else {
        llvm::outs() << "Linking " << exeFilename << "...\n";
        if (!linkExecutable(objFilename, exeFilename, config.triple,
                            RuntimeLib)) {
          llvm::errs() << "Linker failed! Ensure '" << RuntimeLib
                       << "' is accessible.\n";
          return 1;
        }

        llvm::sys::fs::remove(objFilename);
        llvm::outs() << "Successfully generated executable: " << exeFilename
                     << "\n";
      }
    }
  }

  return 0;
}
