#include "moksha/AST/ASTContext.h"
#include "moksha/AST/ASTPrinter.h"
#include "moksha/Lexer/Lexer.h"
#include "moksha/Parser/Parser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <memory>

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
  // [FIX] Add buffer and get ID
  unsigned id = srcMgr.AddNewSourceBuffer(std::move(*fileOrErr), llvm::SMLoc());

  ASTContext astContext;

  // [FIX] Lexer takes StringRef, not SourceMgr
  Lexer lexer(srcMgr.getMemoryBuffer(id)->getBuffer());

  // [FIX] Parser takes Context and SourceMgr
  Parser parser(lexer, astContext, srcMgr);

  std::cout << "Compiling " << InputFilename << "...\n";
  auto moduleAST = parser.parseModule();

  if (!moduleAST) {
    llvm::errs() << "Parsing failed.\n";
    return 1;
  }

  std::cout << "Parsing successful!\n";

  if (DumpAST) {
    // [FIX] ASTPrinter needs llvm::raw_ostream
    ASTPrinter printer(llvm::outs());
    printer.print(moduleAST.get());
  }

  return 0;
}
