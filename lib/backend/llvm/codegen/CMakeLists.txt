# --- [CRITICAL FIX] Use monolithic DLL to match MLIR dependencies ---
set(llvm_codegen_libs LLVM)

add_library(MokshaLLVMCodeGen
    CodeGen.cpp
)

target_include_directories(MokshaLLVMCodeGen PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(MokshaLLVMCodeGen
    PUBLIC
    mokshaSettings
    ${llvm_codegen_libs}
)
