# Safely handle MSYS2/Dynamic LLVM vs Standard Static LLVM
if(LLVM_LINK_LLVM_DYLIB OR LLVM_BUILD_LLVM_DYLIB)
    # Use the monolithic dynamic library (MSYS2 default)
    set(llvm_codegen_libs LLVM)
else()
    # Use individual static components (Linux/Mac default)
    llvm_map_components_to_libnames(llvm_codegen_libs
        Core
        Target
        CodeGen
        MC
        Support
        TargetParser
        AsmPrinter
        AsmParser
        ${LLVM_TARGETS_TO_BUILD}
    )
endif()

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
