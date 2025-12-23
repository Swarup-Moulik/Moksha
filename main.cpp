#include "moksha/Lexer.hpp"
#include "moksha/Parser.hpp"
#include "moksha/ASTPrinter.hpp"
#include "moksha/SemanticAnalyzer.hpp"
#include "moksha/CodeGenerator.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/IR/LegacyPassManager.h>

// Helper to sanitize input strings
std::string sanitize(std::string input)
{
    for (char &c : input)
    {
        if (c == (char)-96 || c == (char)0xA0)
        {
            c = ' ';
        }
    }
    return input;
}

// Function to read file content into a string
std::string readFile(const std::string &filepath)
{
    std::ifstream t(filepath);
    if (!t.is_open())
    {
        std::cerr << "Error: Could not open file '" << filepath << "'" << std::endl;
        exit(1);
    }
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

void emitObjectFile(llvm::Module *module, const std::string &filename, bool freestanding)
{
    // 1. Initialize LLVM Targets

    // Native target initialization is not needed for cross-compilation
    // llvm::InitializeNativeTarget();
    // llvm::InitializeNativeTargetAsmPrinter();
    // llvm::InitializeNativeTargetAsmParser();

    // Initialize all targets for broader compatibility
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    // 2. Select Target (generic x86-64 ELF for kernels)
    std::string targetTriple;
    if (freestanding)
    {
        targetTriple = "i386-pc-none-elf"; // FIX: Assign to the variable
        module->setTargetTriple(llvm::Triple(targetTriple));
    }
    else
    {
        // Automatically detect the host (Windows, Linux, Mac, etc.)
        targetTriple = llvm::sys::getDefaultTargetTriple();
    }

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

    if (!target)
    {
        std::cerr << "Error looking up target: " << error << std::endl;
        return;
    }

    // 3. Configure Target Machine
    std::string cpu = "generic";
    std::string features = "";
    llvm::TargetOptions opt;
    auto rm = std::optional<llvm::Reloc::Model>();
    llvm::Triple triple(targetTriple);
    auto targetMachine = target->createTargetMachine(
        triple, cpu, features, opt, rm,
        std::nullopt, llvm::CodeGenOptLevel::Aggressive // This enables O3 optimizations
    );

    module->setTargetTriple(triple);
    module->setDataLayout(targetMachine->createDataLayout());

    // 4. Write Output
    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);

    if (ec)
    {
        std::cerr << "Could not open file: " << ec.message() << std::endl;
        return;
    }

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::ObjectFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType))
    {
        std::cerr << "TargetMachine can't emit a file of this type" << std::endl;
        return;
    }

    pass.run(*module);
    dest.flush();

    std::cout << "[Moksha] Successfully generated object file: " << filename << std::endl;
}

int main(int argc, char *argv[])
{
    std::string source_code;
    std::string inputFilename;
    bool compileMode = false;
    bool freestandingMode = false;
    bool dumpAst = false;

    // --- LOGIC: Check for Arguments ---
    // Usage: ./moksha <file.mox> [-c]
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            std::string arg = argv[i];
            if (arg == "-c")
            {
                compileMode = true;
            }
            else if (arg == "--freestanding")
            {
                freestandingMode = true;
            }
            else if (arg == "--dump-ast" || arg == "-d")
            {
                dumpAst = true;
            }
            else
            {
                inputFilename = arg;
            }
        }

        if (!inputFilename.empty())
        {
            std::cout << "[Moksha] Processing file: " << inputFilename << std::endl;
            source_code = readFile(inputFilename);
        }
        else
        {
            std::cerr << "Error: No input file provided." << std::endl;
            return 1;
        }
    }
    else
    {
        std::cout << "[Moksha] No input file. Running internal demo..." << std::endl;
        source_code = R"MOKSHA( ... )MOKSHA";
    }

    // 1. Sanitize
    source_code = sanitize(source_code);

    // 2. Lexing
    Moksha::Lexer lexer(source_code);

    // 3. Parsing
    Moksha::Parser parser(lexer);
    auto program = parser.parseProgram();

    if (parser.hasError())
    {
        std::cerr << ">> Compilation Aborted due to Parse Errors." << std::endl;
        return 1;
    }

    if (dumpAst)
    {
        Moksha::ASTPrinter printer;
        std::cout << "==== AST DUMP ====" << std::endl;
        std::cout << printer.print(program) << std::endl;
        std::cout << "==================" << std::endl;
    }

    // 4. Semantic Analysis
    Moksha::SemanticAnalyzer analyzer;
    if (!analyzer.analyze(program))
    {
        std::cerr << ">> Compilation Aborted due to Semantic Errors." << std::endl;
        return 1;
    }

    // 5. Code Generation
    Moksha::CodeGenerator codegen;
    codegen.isFreestanding = freestandingMode;
    codegen.generate(program);

    if (llvm::verifyModule(*codegen.getModule(), &llvm::errs()))
    {
        std::cerr << "Error: Module verification failed! IR is malformed." << std::endl;
        return 1;
    }

    if (compileMode)
    {
        // --- PHASE 1: PREPARE PATHS ---

        // 1. Extract clean base name
        std::string baseName = inputFilename;
        size_t lastSlash = baseName.find_last_of("/\\");
        if (lastSlash != std::string::npos)
        {
            baseName = baseName.substr(lastSlash + 1);
        }
        size_t lastDot = baseName.find_last_of(".");
        if (lastDot != std::string::npos)
        {
            baseName = baseName.substr(0, lastDot);
        }

        // 2. Define Output Paths
        std::string irFilename = "llvmIR/" + baseName + ".ll";
        std::string outputFilename = "output/" + baseName + ".o";
        std::string execName = "executable/" + baseName;
#ifdef _WIN32
        execName += ".exe";
#endif

        // --- PHASE 2: SAVE LLVM IR (.ll) ---
        // This writes the intermediate code to the llvmIR folder
        std::error_code ec;
        llvm::raw_fd_ostream dest_ir(irFilename, ec, llvm::sys::fs::OF_None);
        if (ec)
        {
            std::cerr << "Error: Could not open file '" << irFilename << "': " << ec.message() << std::endl;
            return 1;
        }
        codegen.getModule()->print(dest_ir, nullptr);
        dest_ir.flush();
        std::cout << "[Moksha] Generated IR: " << irFilename << std::endl;

        // --- PHASE 3: EMIT OBJECT FILE (.o) ---
        emitObjectFile(codegen.getModule(), outputFilename, freestandingMode);

        // --- PHASE 4: LINKING (.exe) ---
        std::string cmd;
        if (freestandingMode)
        {
            // Use ld.lld for easier cross-compilation
            cmd = "ld.lld -m elf_i386 -T linker.ld -o kernel.tmp " + outputFilename + " output/kernel_runtime.o";
            // Note: You would need to compile kernel_runtime.cpp to an .o first
            cmd += " && objcopy -O binary kernel.tmp kernel.bin";
        }
        else
        {
            cmd = "g++ " + outputFilename + " runtime/runtime.cpp -o " + execName;
            cmd += " && strip " + execName;  // only for production builds
        }

        if (!cmd.empty())
        {
            std::cout << "[Moksha] Linking: " << cmd << std::endl;
            int result = system(cmd.c_str());
            if (result != 0)
            {
                std::cerr << "Linking failed." << std::endl;
                return 1;
            }
        }
    }
    else
    {
        // Default: Print IR to stderr (for debugging/piping)
        codegen.getModule()->print(llvm::errs(), nullptr);
    }

    return 0;
}