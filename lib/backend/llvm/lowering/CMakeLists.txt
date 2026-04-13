add_library(MokshaLLVMLowering
    MLIRToLLVM.cpp
)

target_include_directories(MokshaLLVMLowering PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(MokshaLLVMLowering
    PUBLIC
    mokshaSettings
    MokshaMLIR
    MLIRLLVMDialect
    MLIRLLVMCommonConversion
    MLIRPass
    MLIRTransforms
    MLIRFuncToLLVM
    MLIRControlFlowToLLVM
)
