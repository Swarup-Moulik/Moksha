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
#include <array>
#include <cstdio>
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
static cl::opt<std::string>
    Sanitize("fsanitize", cl::desc("Enable a sanitizer (e.g., address)"),
             cl::value_desc("sanitizer"), cl::init(""),
             cl::cat(MokshaCategory));
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

  std::string osName;
  std::string baremetalFolder =
      ""; // Used to target your specific build folders

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

    // Map the specific architectures to the folders you generated
    switch (triple.getArch()) {
    case llvm::Triple::aarch64:
      baremetalFolder = "build_baremetal_aarch64";
      break;
    case llvm::Triple::arm:
      baremetalFolder = "build_baremetal_stm32";
      break;
    case llvm::Triple::riscv32:
      baremetalFolder = "build_baremetal_riscv32";
      break;
    case llvm::Triple::riscv64:
      baremetalFolder = "build_baremetal_riscv64";
      break;
    case llvm::Triple::x86:
      baremetalFolder = "build_baremetal_i686";
      break;
    case llvm::Triple::x86_64:
      // Differentiate UEFI vs BIOS based on the environment field in the triple
      if (triple.getEnvironment() == llvm::Triple::MSVC) {
        baremetalFolder = "build_baremetal_uefi";
      } else {
        baremetalFolder = "build_baremetal_x86";
      }
      break;
    default:
      break;
    }
  } else {
    osName = "posix_common";
  }

  std::string libFilename = "libmoksha_rt_" + osName + ".a";
  std::string exePath = llvm::sys::fs::getMainExecutable(
      argv0, (void *)(intptr_t)resolveRuntimeLibrary);
  llvm::StringRef binDir = llvm::sys::path::parent_path(exePath);

  std::vector<std::string> searchDirs = {"../runtime", "../lib",
                                         "../../runtime", "."};

  // If this is a bare-metal build, inject the specific arch folder into the
  // search paths
  if (!baremetalFolder.empty()) {
    // 1. Matches C:\Moksha\build_baremetal_<arch> (Your new deployment layout)
    searchDirs.insert(searchDirs.begin(), "../" + baremetalFolder);

    // 2. Matches C:\dev\moksha\moksha\runtime\build_baremetal_<arch> (Your dev
    // layout)
    searchDirs.insert(searchDirs.begin(), "../runtime/" + baremetalFolder);

    // 3. Fallback for deeply nested build folders
    searchDirs.insert(searchDirs.begin(), "../../runtime/" + baremetalFolder);
  }

  for (const auto &dir : searchDirs) {
    llvm::SmallString<256> libPath = binDir;
    llvm::sys::path::append(libPath, dir, libFilename);
    if (llvm::sys::fs::exists(libPath)) {
      return std::string(libPath.str());
    }
  }

  // Fallback to default path for clear error messages
  llvm::SmallString<256> fallback = binDir;
  llvm::sys::path::append(fallback, "..", "runtime", baremetalFolder,
                          libFilename);
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

// Asks Clang (or GCC for ARM) for the exact path to its compiler-rt/libgcc
// builtins
std::string getBuiltinsLibraryPath(const std::string &compilerBinary,
                                   const llvm::Triple &triple,
                                   const std::string &cpu) {
  std::string cmd;

  // Bulletproof check using LLVM's internal architecture enum instead of
  // strings
  if (triple.getArch() == llvm::Triple::arm ||
      triple.getArch() == llvm::Triple::thumb) {
    std::string cpuFlag = cpu.empty() ? "cortex-m3" : cpu;
    cmd = "arm-none-eabi-gcc -mthumb -mcpu=" + cpuFlag +
          " -print-libgcc-file-name";
  } else {
    cmd = compilerBinary + " --target=" + triple.str() +
          " -print-libgcc-file-name";
  }

  std::array<char, 256> buffer;
  std::string result;

#ifdef _WIN32
  std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"),
                                                 _pclose);
#else
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
                                                pclose);
#endif

  if (!pipe)
    return "";
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }

  // Trim trailing newlines
  if (!result.empty() && result.back() == '\n')
    result.pop_back();
  if (!result.empty() && result.back() == '\r')
    result.pop_back();

  if (!llvm::sys::fs::exists(result)) {
    return "";
  }

  return result;
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

  // PYTHON-STYLE MODULE RESOLUTION PATHS
  std::vector<std::string> searchPaths;

  // 1. Current file's directory (Highest Priority)
  searchPaths.push_back(llvm::sys::path::parent_path(InputFilename).str());

  // 2. Environment Variable: MOKSHA_PATH
  if (const char *envPath = std::getenv("MOKSHA_PATH")) {
    llvm::SmallVector<llvm::StringRef, 4> paths;
    // Splits by ':' on Linux/Mac, and ';' on Windows
    llvm::StringRef(envPath).split(paths, llvm::sys::EnvPathSeparator);
    for (auto p : paths) {
      searchPaths.push_back(p.str());
    }
  }

  // 3. Standard Library (Relative to mokshac executable)
  std::string exePath =
      llvm::sys::fs::getMainExecutable(argv[0], (void *)(intptr_t)main);
  llvm::StringRef binDir = llvm::sys::path::parent_path(exePath);

  llvm::SmallString<256> stdlibPath1 = binDir;
  llvm::sys::path::append(stdlibPath1, "..", "stdlib");
  searchPaths.push_back(stdlibPath1.str().str());

  llvm::SmallString<256> stdlibPath2 = binDir;
  llvm::sys::path::append(stdlibPath2, "..", "..", "stdlib");
  searchPaths.push_back(stdlibPath2.str().str());

  // 2. Define the callback to parse imported files
  typeChecker.loadModuleCallback =
      [&](const std::string &modName) -> moksha::ModuleDecl * {
    std::string resolvedPath = "";

    // Support nested modules (e.g. import "std/collections/table")
    std::string relativePath = modName + ".mox";

    // Loop through our search paths to find the file
    for (const auto &dir : searchPaths) {
      llvm::SmallString<256> fullPath = llvm::StringRef(dir);
      llvm::sys::path::append(fullPath, relativePath);

      if (llvm::sys::fs::exists(fullPath)) {
        resolvedPath = fullPath.str().str();
        break; // Found it!
      }
    }

    // If it's not found in ANY path, fail out
    if (resolvedPath.empty()) {
      return nullptr;
    }

    // If it's already in the cache, just return it
    if (moduleCache.count(resolvedPath)) {
      return moduleCache[resolvedPath].get();
    }

    // Open the file using LLVM's Memory Buffer
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> bufOrErr =
        llvm::MemoryBuffer::getFile(resolvedPath);
    if (std::error_code ec = bufOrErr.getError()) {
      return nullptr;
    }

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
    moduleCache[resolvedPath] = std::move(importedAST);
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
          if (auto *fnDecl = llvm::dyn_cast_or_null<FunctionDecl>(decl.get())) {
            if (fnDecl->getName() == "main") {
              continue; // Skip this declaration
            }
          }
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

    moksha::TargetConfig config;
    config.triple = TargetTriple.empty() ? llvm::sys::getDefaultTargetTriple()
                                         : TargetTriple;
    config.features = TargetFeatures;
    config.optLevel = DisableOpt ? 0 : 2;

    llvm::Triple actualTriple(config.triple);

    if (TargetCPU.empty()) {
      if (actualTriple.getArch() == llvm::Triple::aarch64) {
        config.cpu = "cortex-a53";
      } else if (actualTriple.getArch() == llvm::Triple::riscv32) {
        config.cpu = "generic-rv32";
        // Enable hardware Multiply/Divide (M) and Double Float (D)
        if (config.features.empty())
          config.features = "+m,+d";
      } else if (actualTriple.getArch() == llvm::Triple::riscv64) {
        config.cpu = "generic-rv64";
        // Enable hardware Multiply/Divide (M) and Double Float (D)
        if (config.features.empty())
          config.features = "+m,+d";
      } else if (actualTriple.getArch() == llvm::Triple::arm) {
        config.cpu = "cortex-m3";
      } else {
        config.cpu = "";
      }
    } else {
      config.cpu = TargetCPU;
    }

    llvm::LLVMContext llvmCtx;
    auto llvmModule =
        moksha::translateMokshaToLLVMIR(mlirModule.get(), llvmCtx, config);

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
        std::string optLevel = DisableOpt ? "-O0" : "-O2";
        std::string optCmd = optBinary + " " + optLevel + " " + llFilename +
                             " -S -o " + llFilename;
        int optResult = std::system(optCmd.c_str());

        if (optResult != 0) {
          llvm::errs() << "Fatal: '" << optBinary
                       << "' failed to optimize the LLVM IR.\n";
          return 1;
        }

        std::string extraFilesStr = "";
        for (const auto &file : ExtraLinkFiles) {
          extraFilesStr += " " + file;
        }

        std::string compilerBase = "clang++";
        if (actualTriple.isWasm() &&
            actualTriple.getOS() != llvm::Triple::WASI) {
          compilerBase = "emcc";
        }

        std::string compilerBinary = findLLVMTool(compilerBase);
        std::string asanFlags = (Sanitize == "address")
                                    ? " -g -fsanitize=address -static-libasan"
                                    : "";

        // ===================================================================
        // STEP 1: Compile LLVM IR (.ll) to Native Object File (.o)
        // ===================================================================
        std::string objFilename = exeFilename + ".o";
        std::string compileCmd =
            compilerBinary + " -c " + optLevel + asanFlags +
            " -fno-exceptions -fno-rtti -Wno-override-module " + llFilename +
            " -o " + objFilename;

        if (!TargetTriple.empty() && compilerBase == "clang++") {
          if (actualTriple.getArch() == llvm::Triple::x86_64 &&
              actualTriple.getEnvironment() == llvm::Triple::MSVC) {
            // Force Windows triple during object generation to ensure PE/COFF
            // format instead of ELF
            compileCmd += " --target=x86_64-unknown-windows-msvc";
          } else {
            compileCmd += " --target=" + TargetTriple;
          }
        }

        // Target Specific Feature Flags for Object Compilation
        if (actualTriple.getArch() == llvm::Triple::riscv64) {
          compileCmd += " -march=rv64gc -mabi=lp64d -mcmodel=medany";
        } else if (actualTriple.getArch() == llvm::Triple::riscv32) {
          compileCmd += " -march=rv32gc -mabi=ilp32d -mcmodel=medany";
        } else if (actualTriple.getArch() == llvm::Triple::x86) {
          compileCmd += " -m32 -mno-sse -mno-mmx";
        } else if (actualTriple.getArch() == llvm::Triple::x86_64) {
          if (actualTriple.getEnvironment() == llvm::Triple::MSVC) {
            // Use specific UEFI architecture flags
            compileCmd += " -m64 -mcmodel=large -mno-red-zone -mno-sse "
                          "-mno-mmx -fshort-wchar -ffreestanding "
                          "-fno-unwind-tables -fno-asynchronous-unwind-tables";
          } else {
            // Standard PC BIOS baremetal
            compileCmd +=
                " -m64 -mcmodel=kernel -mno-red-zone -mno-sse -mno-mmx";
          }
        } else if (actualTriple.getArch() == llvm::Triple::arm) {
          compileCmd += " -mthumb"; // Force thumb instructions for Cortex-M
        }

        // Dynamically forward the CPU flag if the user provided one
        if (!TargetCPU.empty()) {
          compileCmd += " -mcpu=" + TargetCPU;
        }

        int compileResult = std::system(compileCmd.c_str());
        if (compileResult != 0) {
          llvm::errs() << "Clang failed to compile LLVM IR to object file.\n";
          return 1;
        }

        // ===================================================================
        // STEP 2: Direct LLD Link Execution (No GCC/g++ Wrappers)
        // ===================================================================
        if (actualTriple.getOS() == llvm::Triple::UnknownOS) {
          std::string linkCmd = "";

          if (actualTriple.getArch() == llvm::Triple::x86_64 &&
              actualTriple.getEnvironment() == llvm::Triple::MSVC) {
            // UEFI Target: Use PE/COFF linker (lld-link) instead of ELF
            // (ld.lld)
            std::string lldBinary = findLLVMTool("lld-link");

            linkCmd = lldBinary + " /OUT:" + exeFilename + " " + objFilename +
                      " " + resolvedRtLib + extraFilesStr +
                      " /NOLOGO /NODEFAULTLIB /SUBSYSTEM:EFI_APPLICATION "
                      "/ENTRY:efi_main";
          } else {
            // Standard ELF Baremetal Target
            std::string lldBinary = findLLVMTool("ld.lld");
            std::string builtinsLib = getBuiltinsLibraryPath(
                compilerBinary, actualTriple, config.cpu);

            linkCmd = lldBinary + " -o " + exeFilename + " " + objFilename +
                      " " + resolvedRtLib + " " + builtinsLib + " " +
                      extraFilesStr + " -static";

            // Inject ELF Target Machine Emulation
            if (actualTriple.getArch() == llvm::Triple::x86) {
              linkCmd += " -m elf_i386";
            } else if (actualTriple.getArch() == llvm::Triple::x86_64) {
              linkCmd += " -m elf_x86_64";
            } else if (actualTriple.getArch() == llvm::Triple::riscv64) {
              linkCmd += " -m elf64lriscv";
            } else if (actualTriple.getArch() == llvm::Triple::riscv32) {
              linkCmd += " -m elf32lriscv";
            } else if (actualTriple.getArch() == llvm::Triple::aarch64) {
              linkCmd += " -m aarch64elf";
            } else if (actualTriple.getArch() == llvm::Triple::arm) {
              linkCmd += " -m armelf";
            }
          }

          // Resolve Linker Scripts & Base Load Address
          std::string linkerScript = "";
          std::string baseAddr = "";

          if (actualTriple.getArch() == llvm::Triple::arm) {
            if (config.cpu == "cortex-m3" || config.cpu == "cortex-m4") {
              linkerScript = "src/sys/baremetal/platform/stm32/linker.ld";
              baseAddr = "0x08000000";
            } else {
              linkerScript = "src/sys/baremetal/platform/qemu_virt/linker.ld";
              baseAddr = "0x40000000";
            }
          } else if (actualTriple.getArch() == llvm::Triple::aarch64) {
            linkerScript = "src/sys/baremetal/platform/qemu_virt/linker.ld";
            baseAddr = "0x40000000";
          } else if (actualTriple.isRISCV()) {
            linkerScript = "src/sys/baremetal/platform/qemu_virt/linker.ld";
            baseAddr = "0x80200000";
          } else if (actualTriple.getArch() == llvm::Triple::x86) {
            linkerScript = "src/sys/baremetal/platform/pc_bios_i686/linker.ld";
            baseAddr = "0x100000";
          } else if (actualTriple.getArch() == llvm::Triple::x86_64) {
            if (actualTriple.getEnvironment() == llvm::Triple::MSVC) {
              linkerScript = "";
            } else {
              linkerScript =
                  "src/sys/baremetal/platform/pc_bios_x86_64/linker.ld";
              baseAddr = "0x200000";
            }
          }

          if (!linkerScript.empty()) {
            std::string exePath = llvm::sys::fs::getMainExecutable(
                argv[0], (void *)(intptr_t)main);
            llvm::StringRef binDir = llvm::sys::path::parent_path(exePath);

            llvm::SmallString<256> ldPath1 = binDir;
            llvm::sys::path::append(ldPath1, "..", "runtime", linkerScript);

            llvm::SmallString<256> ldPath2 = binDir;
            llvm::sys::path::append(ldPath2, "..", "..", "runtime",
                                    linkerScript);

            if (llvm::sys::fs::exists(ldPath1)) {
              linkCmd += " --defsym=__BASE_ADDR=" + baseAddr;
              linkCmd += " -T \"" + std::string(ldPath1.c_str()) + "\"";
            } else if (llvm::sys::fs::exists(ldPath2)) {
              linkCmd += " --defsym=__BASE_ADDR=" + baseAddr;
              linkCmd += " -T \"" + std::string(ldPath2.c_str()) + "\"";
            } else {
              llvm::errs()
                  << "[FATAL] Could not find linker script at path:\n  -> "
                  << ldPath1.c_str() << "\n";
              return 1;
            }
          }

          int linkResult = std::system(linkCmd.c_str());
          if (linkResult != 0) {
            llvm::errs()
                << "ld.lld failed to link the bare-metal kernel binary.\n";
            return 1;
          }

          if (actualTriple.getArch() == llvm::Triple::x86_64 &&
              actualTriple.getEnvironment() != llvm::Triple::MSVC) {
            std::string objcopyBinary = findLLVMTool("llvm-objcopy");
            std::string convertCmd = objcopyBinary + " -O elf32-i386 " +
                                     exeFilename + " " + exeFilename;
            int convertResult = std::system(convertCmd.c_str());
            if (convertResult != 0) {
              llvm::errs() << "Failed to convert x86_64 kernel ELF header to "
                              "elf32-i386.\n";
              return 1;
            }
          }
        } else {
          // Standard OS Hosted Linking (Linux, Windows, Darwin, WASM)
          std::string cmd = compilerBinary + " " + optLevel + asanFlags +
                            " -fno-exceptions -fno-rtti -Wno-override-module " +
                            objFilename + extraFilesStr + " " + resolvedRtLib;

          for (const auto &path : LibraryPaths)
            cmd += " -L" + path;
          for (const auto &lib : Libraries)
            cmd += " -l" + lib;

          if (!TargetTriple.empty() && compilerBase == "clang++") {
            cmd += " --target=" + TargetTriple;
          }

          if (actualTriple.isOSLinux() || actualTriple.isAndroid()) {
            cmd += " -pthread -lm";
          }

          cmd += " -o " + exeFilename;

          int result = std::system(cmd.c_str());
          if (result != 0) {
            llvm::errs() << "Clang failed to link the hosted executable.\n";
            return 1;
          }
        }

        // Clean up temporary intermediate files (.ll and .o)
        llvm::sys::fs::remove(llFilename);
        llvm::sys::fs::remove(objFilename);

        llvm::outs() << "Successfully generated executable: " << exeFilename
                     << "\n";
      }
    }
  }

  return 0;
}
