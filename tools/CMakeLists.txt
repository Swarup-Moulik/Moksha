# Build the modular optimizer tool
add_executable(moksha-opt moksha-opt.cpp)

target_link_libraries(moksha-opt
    PRIVATE
    mokshaSettings
    MokshaMLIR
    MLIRIR
    MLIRInitAllDialects
    MLIRInitAllPasses
    MLIRTools
)

# Build the main compiler driver
add_executable(mokshac mokshac.cpp)

# Force usage of 'main' entry point on Windows/MinGW
if(MINGW OR WIN32)
    target_link_options(mokshac PRIVATE -mconsole)
endif()

# [FIX] Link against the Frontend and LLVM libraries
target_link_libraries(mokshac
    PRIVATE
    mokshaSettings
    mokshaFrontend  # Contains Lexer, Parser, AST
    LLVMSupport     # Contains MemoryBuffer, SourceMgr, CommandLine
    LLVMCore
)
