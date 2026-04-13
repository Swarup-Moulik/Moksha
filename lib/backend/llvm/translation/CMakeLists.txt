add_library(MokshaLLVMTranslation
    TranslateToLLVM.cpp
)

target_include_directories(MokshaLLVMTranslation PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(MokshaLLVMTranslation
    PUBLIC
    mokshaSettings
    MLIRTargetLLVMIRExport
    MLIRLLVMToLLVMIRTranslation
    MLIRBuiltinToLLVMIRTranslation
)
