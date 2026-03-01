#include "moksha/AST/ASTContext.h"
#include "moksha/AST/ASTPrinter.h"
#include "moksha/Builtins/BuiltinRegistry.h"
#include "moksha/HIR/HIRModule.h"
#include "moksha/Lexer/Lexer.h"
#include "moksha/Macros/Macro.h"
#include "moksha/Parser/Parser.h"
#include "moksha/Sema/SymbolTable.h"
#include "moksha/Sema/TypeChecker.h"
#include "moksha/Support/Diagnostics.h"
// --- ADD THE BORROW CHECKER HEADER ---
#include "moksha/Ownership/BorrowChecker.h"

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

  // --- 7. HIR SEMANTIC ANALYSIS (BORROW CHECKING) ---
  // We run this if specifically requested by the test suite, OR if we
  // are doing a standard compilation (i.e. not just dumping the AST/HIR).
  if (RunHIRPasses || (!DumpAST && !DumpHIR)) {
    llvm::outs() << "Running Borrow Checker...\n";
    ownership::BorrowChecker borrowChecker(diags);
    borrowChecker.checkModule(*hirModule);

    if (diags.hasErrors()) {
      llvm::errs() << "Borrow checking failed with " << diags.getNumErrors()
                   << " error(s).\n";
      return 1; // Halt compilation if memory safety is violated!
    }
    llvm::outs() << "Borrow Check passed! Memory is safe.\n";
  }

  return 0;
}
