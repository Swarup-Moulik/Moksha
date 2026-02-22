#include "moksha/AST/ASTContext.h"
#include "moksha/AST/ASTPrinter.h"
#include "moksha/Builtins/BuiltinRegistry.h"
#include "moksha/Lexer/Lexer.h"
#include "moksha/Macros/Macro.h"
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

  // Only dump AST if there are NO semantic errors
  if (DumpAST && !diags.hasErrors()) {
    ASTPrinter printer(llvm::outs());
    printer.print(moduleAST.get());
  }

  // 5. Final exit code check
  if (diags.hasErrors()) {
    return 1;
  }

  return 0;
}
