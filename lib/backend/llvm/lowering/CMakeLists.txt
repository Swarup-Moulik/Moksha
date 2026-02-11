add_library(MokshaLLVMLowering
    MLIRToLLVM.cpp
)

target_link_libraries(MokshaLLVMLowering
    PUBLIC
    mokshaSettings
    MokshaMLIR  # Depends on your Dialect
    LLVMCore
)
