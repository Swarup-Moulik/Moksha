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
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/InitializePasses.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
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

static cl::list<std::string>
    ExtraLinkFiles(cl::Positional,
                   cl::desc("[<extra C/C++/obj files to link>...]"),
                   cl::ZeroOrMore, cl::cat(MokshaCategory));

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
static cl::list<std::string>
    Libraries("l",
              cl::desc("Libraries to link against (e.g., curl for -lcurl)"),
              cl::ZeroOrMore, cl::Prefix, cl::cat(MokshaCategory));
static cl::list<std::string>
    LibraryPaths("L", cl::desc("Library search paths (e.g., /usr/local/lib)"),
                 cl::ZeroOrMore, cl::Prefix, cl::cat(MokshaCategory));

// Allows pointing to the built runtime lib
static cl::opt<std::string>
    RuntimeLib("rt-lib",
               cl::desc("Override path to the Moksha runtime library"),
               cl::init(""), cl::cat(MokshaCategory));

std::string resolveRuntimeLibrary(const std::string &explicitPath,
                                  const llvm::Triple &triple,
                                  const char *argv0) {
  // 1. Explicit override via -rt-lib=...
  if (!explicitPath.empty()) {
    return explicitPath;
  }

  // 2. Map LLVM Triple OS to our CMake Target Strings
  std::string osName;
  if (triple.isOSWindows()) {
    osName = "windows";
  } else if (triple.isOSDarwin()) {
    osName = "darwin";
  } else if (triple.isOSLinux() || triple.isAndroid()) {
    osName = "linux";
  } else if (triple.getOS() == llvm::Triple::WASI) {
    osName = "wasi";
  } else if (triple.isWasm()) {
    osName = "wasm_web";
  } else if (triple.getOS() == llvm::Triple::UnknownOS) {
    osName = "baremetal";
  } else {
    osName = "posix_common";
  }

  std::string libFilename = "libmoksha_rt_" + osName + ".a";
  std::string exePath = llvm::sys::fs::getMainExecutable(
      argv0, (void *)(intptr_t)resolveRuntimeLibrary);
  llvm::StringRef binDir = llvm::sys::path::parent_path(exePath);

  // --- [FIX] Search multiple common CMake output directories ---
  const char *searchDirs[] = {
      "../runtime",    // Default target directory
      "../lib",        // Standard CMake CMAKE_ARCHIVE_OUTPUT_DIRECTORY
      "../../runtime", // Fallback source directory
      "."              // Current directory
  };

  for (const char *dir : searchDirs) {
    llvm::SmallString<256> libPath = binDir;
    llvm::sys::path::append(libPath, dir, libFilename);
    if (llvm::sys::fs::exists(libPath)) {
      return std::string(libPath.str());
    }
  }

  // Fallback to default path to give a clear error message if not found
  llvm::SmallString<256> fallback = binDir;
  llvm::sys::path::append(fallback, "..", "runtime", libFilename);
  return std::string(fallback.str());
}

std::string findLLVMTool(const std::string &baseName) {
  // 1. Try to find the version we compiled against first (LLVM 22)
  std::string versionedName = baseName + "-22";
  if (auto path = llvm::sys::findProgramByName(versionedName)) {
    return *path;
  }

  // 2. If targeting WASM, check for emcc
  if (baseName == "emcc") {
    if (auto path = llvm::sys::findProgramByName("emcc")) {
      return *path;
    }
    if (auto path =
            llvm::sys::findProgramByName("emcc.bat")) { // Windows fallback
      return *path;
    }
  }

  // 3. Fallback to the standard un-versioned base name
  if (auto path = llvm::sys::findProgramByName(baseName)) {
    return *path;
  }

  // 4. If all else fails, just return the base name and let the shell try
  return baseName;
}

int main(int argc, char **argv) {
  llvm::InitLLVM X(argc, argv);

  // 1. INITIALIZE NATIVE TARGETS FIRST!
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  // 2. INITIALIZE GLOBAL PASS REGISTRY SECOND!
  llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
  llvm::initializeCore(Registry);
  llvm::initializeCodeGen(Registry);
  llvm::initializeTarget(Registry);
  llvm::initializeTransformUtils(Registry);
  llvm::initializeScalarOpts(Registry);
  llvm::initializeAnalysis(Registry);
  llvm::initializeIPO(Registry);
  llvm::initializeInstCombine(Registry);
  llvm::initializeTargetTransformInfoWrapperPassPass(Registry);
  llvm::initializeTargetLibraryInfoWrapperPassPass(Registry);
  llvm::initializeTargetPassConfigPass(Registry);
  llvm::initializeMachineModuleInfoWrapperPassPass(Registry);
  llvm::initializeGlobalISel(Registry);

  // Legacy pass names specifically required by TargetPassConfig
  llvm::initializeLoopStrengthReducePass(Registry);
  llvm::initializeLowerIntrinsicsPass(Registry);
  llvm::initializeUnreachableBlockElimLegacyPassPass(Registry);
  llvm::initializeUnreachableMachineBlockElimLegacyPass(Registry);

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

  // 1. Create a cache to hold ownership of the imported ASTs
  std::unordered_map<std::string, std::unique_ptr<moksha::ModuleDecl>>
      moduleCache;

  TypeChecker typeChecker(astContext, symbolTable, diags);

  // Extract the base name of the input file and mark it as loaded
  llvm::StringRef mainModName = llvm::sys::path::stem(InputFilename);
  typeChecker.markModuleAsLoaded(mainModName.str());

  // 2. Define the callback to parse imported files
  typeChecker.loadModuleCallback =
      [&](const std::string &modName) -> moksha::ModuleDecl * {
    llvm::SmallString<256> parentDir =
        llvm::sys::path::parent_path(InputFilename);
    llvm::SmallString<256> modulePath = parentDir;
    llvm::sys::path::append(modulePath, modName + ".mox");

    std::string path = modulePath.str().str();

    // If it's already in the cache, just return it
    if (moduleCache.count(path)) {
      return moduleCache[path].get();
    }

    // Open the file using LLVM's Memory Buffer
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> bufOrErr =
        llvm::MemoryBuffer::getFile(path);
    if (std::error_code ec = bufOrErr.getError()) {
      return nullptr; // Returning nullptr tells Sema to emit an error
    }

    // Add the new file to your existing SourceMgr so diagnostics point to the
    // right file!
    unsigned bufID =
        srcMgr.AddNewSourceBuffer(std::move(*bufOrErr), llvm::SMLoc());

    // Lex and Parse the imported file
    moksha::Lexer importLexer(srcMgr.getMemoryBuffer(bufID)->getBuffer(),
                              diags);
    moksha::Parser importParser(importLexer, astContext, srcMgr, diags);

    std::unique_ptr<moksha::ModuleDecl> importedAST =
        importParser.parseModule();
    if (!importedAST || diags.hasErrors()) {
      return nullptr;
    }

    // Run Macro expansion on the imported file
    importedAST->accept(macroExpander);

    // Store in cache to keep memory alive, and return the raw pointer to Sema
    moksha::ModuleDecl *rawPtr = importedAST.get();
    moduleCache[path] = std::move(importedAST);
    return rawPtr;
  };

  // 3. Trigger semantic analysis
  typeChecker.check(moduleAST.get());

  if (typeChecker.hasErrors() || diags.hasErrors()) {
    llvm::errs() << "Semantic analysis failed with " << diags.getNumErrors()
                 << " error(s).\n";
    return 1;
  }

  // WHOLE-PROGRAM AST MERGE
  for (auto &pair : moduleCache) {
    if (pair.second) {
      for (auto &decl : pair.second->getDeclsMut()) {
        if (decl) {
          moduleAST->addDeclaration(std::move(decl));
        }
      }
    }
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

      // --- [FIX] Emit a .ll file instead of an object file ---
      std::string llFilename = EmitObj ? OutputFilename : exeFilename + ".ll";

      if (!moksha::emitObjectCode(*llvmModule, llFilename, config)) {
        llvm::errs() << "Failed to emit LLVM IR code!\n";
        return 1;
      }

      if (EmitObj) {
        llvm::outs() << "Successfully generated LLVM IR file: " << llFilename
                     << "\n";
      } else {
        llvm::outs() << "Compiling " << exeFilename << " via clang...\n";

        std::string resolvedRtLib =
            resolveRuntimeLibrary(RuntimeLib, actualTriple, argv[0]);

        if (!llvm::sys::fs::exists(resolvedRtLib)) {
          llvm::errs() << "Fatal: Could not find runtime library at: "
                       << resolvedRtLib << "\n";
          return 1;
        }

        llvm::outs() << "Running LLVM middle-end optimizers...\n";

        // Dynamically find the correct optimizer binary
        std::string optBinary = findLLVMTool("opt");
        std::string optCmd =
            optBinary + " -O2 " + llFilename + " -S -o " + llFilename;
        int optResult = std::system(optCmd.c_str());

        if (optResult != 0) {
          llvm::errs() << "Fatal: '" << optBinary
                       << "' failed to optimize the LLVM IR.\n";
          return 1;
        }

        // Shell out to clang.exe to do the heavy lifting
        std::string extraFilesStr = "";
        for (const auto &file : ExtraLinkFiles) {
          extraFilesStr += " " + file;
        }

        // 1. Determine the correct compiler driver
        std::string compilerBase = "clang++";
        if (actualTriple.isWasm() &&
            actualTriple.getOS() != llvm::Triple::WASI) {
          compilerBase = "emcc";
        }

        // Dynamically find the correct compiler binary
        std::string compilerBinary = findLLVMTool(compilerBase);

        // 2. Build the base command
        std::string cmd = compilerBinary + " -O2 -Wno-override-module " +
                          llFilename + extraFilesStr + " " + resolvedRtLib;

        // Inject custom library paths (-L)
        for (const auto &path : LibraryPaths) {
          cmd += " -L" + path;
        }

        // Inject custom libraries (-l)
        for (const auto &lib : Libraries) {
          cmd += " -l" + lib;
        }

        // 3. Pass the target triple explicitly so Clang cross-compiles
        // correctly
        if (!TargetTriple.empty() && compilerBase == "clang++") {
          cmd += " --target=" + TargetTriple;
        }

        // 4. Inject OS-Specific Linker Flags
        if (actualTriple.isOSLinux() || actualTriple.isAndroid()) {
          cmd += " -pthread -lm"; // Required for Linux threading and math
        }

        cmd += " -o " + exeFilename;

        int result = std::system(cmd.c_str());

        if (result != 0) {
          llvm::errs() << "Clang failed to compile the generated LLVM IR.\n";
          return 1;
        }

        // Clean up the temporary .ll file
        llvm::sys::fs::remove(llFilename);
        llvm::outs() << "Successfully generated executable: " << exeFilename
                     << "\n";
      }
    }
  }

  return 0;
}
