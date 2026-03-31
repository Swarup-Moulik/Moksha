#include "moksha/AST/ASTContext.h"
#include "moksha/AST/ASTPrinter.h"
#include "moksha/Builtins/BuiltinRegistry.h"
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
#include "moksha/Ownership/BorrowChecker.h"
#include "moksha/Parser/Parser.h"
#include "moksha/Sema/SymbolTable.h"
#include "moksha/Sema/TypeChecker.h"
#include "moksha/Support/Diagnostics.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <memory>
#include <string>

// Forward-declare the entry point for HIR lowering
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

static cl::opt<bool> DumpAST("dump-ast",
                             cl::desc("Print the Abstract Syntax Tree"),
                             cl::cat(MokshaCategory));
static cl::opt<bool>
    DumpHIR("dump-hir",
            cl::desc("Print the High-Level Intermediate Representation"),
            cl::cat(MokshaCategory));

// --- ADD THE CLI FLAG FOR HIR PASSES (Used by our test suites) ---
static cl::opt<bool> RunHIRPasses(
    "run-hir-passes",
    cl::desc("Run High-Level IR analysis passes (e.g., Borrow Checker)"),
    cl::cat(MokshaCategory));

// --- CLI FLAG FOR MIR PASSES ---
static cl::opt<bool> DumpMIR("dump-mir",
                             cl::desc("Print the Mid-Level IR (MIR)"),
                             cl::cat(MokshaCategory));

static cl::opt<bool> RunMIRPasses(
    "run-mir-passes",
    cl::desc("Run Mid-Level IR analysis passes (NLL Borrow Checker, ARC)"),
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
  // Add buffer and get ID
  unsigned id = srcMgr.AddNewSourceBuffer(std::move(*fileOrErr), llvm::SMLoc());

  // 1. Setup Diagnostics FIRST (requires srcMgr)
  DiagnosticEngine diags(srcMgr);
  ASTContext astContext;
  SymbolTable symbolTable(diags);

  // Inject primitive types first so 'void' and 'string' exist
  symbolTable.addPrimitiveTypes(astContext);
  // Register 'print' and other built-ins using the types we just added
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

  llvm::outs() << "Parsing successful!\n";

  //  3. MACRO EXPANSION PASS
  MacroExpander macroExpander(astContext, diags);

  // Walk the AST to expand all macro calls
  moduleAST->accept(macroExpander);

  // 4. Setup TypeChecker FOURTH (requires context, symbols, and diags)
  TypeChecker typeChecker(astContext, symbolTable, diags);

  // 5. Run the check
  typeChecker.check(moduleAST.get());

  // Early exit if Semantic Analysis failed!
  if (typeChecker.hasErrors() || diags.hasErrors()) {
    llvm::errs() << "Semantic analysis failed with " << diags.getNumErrors()
                 << " error(s).\n";
    return 1; // Halt compilation here!
  }

  // Since we passed the error check, we know the AST is safe to dump
  if (DumpAST) {
    ASTPrinter printer(llvm::outs());
    printer.print(moduleAST.get());
  }

  // --- 6. Lower AST to HIR ---
  // This will now ONLY run if there are 0 semantic errors in the AST.
  auto hirModule = lowerASTToHIR(moduleAST.get(), astContext);

  if (DumpHIR) {
    llvm::outs() << "\n=== HIR Dump ===\n";
    hirModule->dump(llvm::outs());
    llvm::outs().flush();
    llvm::outs() << "================\n\n";
  }

  // --- 7. VERIFY NO MACROS (Pipeline Safety Barrier) ---
  if (!mir::VerifyNoMacros(hirModule.get(), diags)) {
    llvm::errs() << "Compilation halted: Unexpanded macros found in HIR.\n";
    return 1;
  }

  // --- 8. LOWER HIR TO MIR ---
  auto mirModule = mir::LowerHIRToMIR(hirModule.get(), diags);
  if (!mirModule) {
    llvm::errs() << "Failed to lower HIR to MIR.\n";
    return 1;
  }

  // --- 9. MIR PASSES ---
  if (RunMIRPasses || (!DumpAST && !DumpHIR && !DumpMIR)) {

    // A. ARC Insertion (Memory Management)
    mir::runARCInsertion(mirModule.get(), diags);

    // B. NLL Borrow Checker (Memory Safety MUST run before optimizations)
    mir::NLLBorrowChecker nllChecker(diags);
    nllChecker.checkModule(mirModule.get());

    if (diags.hasErrors()) {
      llvm::errs() << "NLL Borrow checking failed with " << diags.getNumErrors()
                   << " error(s).\n";
      return 1;
    }

    // C. Core MIR Optimizations
    mir::PassManager pm;
    pm.addPass(std::make_unique<mir::InliningPass>());
    pm.addPass(std::make_unique<mir::EscapeAnalysisPass>());
    pm.addPass(std::make_unique<mir::DropElisionPass>());
    pm.addPass(std::make_unique<mir::SROAPass>());
    pm.addPass(std::make_unique<mir::Mem2RegPass>());
    pm.addPass(std::make_unique<mir::JumpThreadingPass>());
    pm.addPass(std::make_unique<mir::ConstantFoldingPass>());
    pm.addPass(std::make_unique<mir::DeadCodeEliminationPass>());
    pm.addPass(std::make_unique<mir::SimplifyCFGPass>());

    pm.run(*mirModule);

    // D. ARC Optimization (Zero-Cost Elision)
    mir::runARCOptimization(mirModule.get(), diags);

    // E. MIR Verification (Sanity Check CFG)
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

  llvm::outs() << "Compilation successfully reached MIR phase!\n";

  return 0;
}
