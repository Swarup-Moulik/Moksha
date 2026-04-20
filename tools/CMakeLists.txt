# ==============================================================================
# 1. The Optimizer Tool (moksha-opt)
# ==============================================================================
add_executable(moksha-opt moksha-opt.cpp)

target_link_libraries(moksha-opt
    PRIVATE
    mokshaSettings
    MokshaMLIR
    MLIROptLib
    MLIRLLVMDialect
    MLIRFuncDialect
    MLIRControlFlowDialect
    MLIRIR
    MLIRSupport
)

# ==============================================================================
# 2. The Main Compiler Driver (mokshac)
# ==============================================================================
add_executable(mokshac mokshac.cpp)

# Force usage of 'main' entry point on Windows/MinGW
if(MINGW OR WIN32)
   target_link_options(mokshac PRIVATE -mconsole -Wl,--allow-multiple-definition)
endif()

# Now we safely link the libraries to the defined target
target_link_libraries(mokshac
    PRIVATE
    mokshaSettings
    mokshaFrontend
    MokshaMiddle
    MokshaSupport

    # --- LLVM Backend Pipeline ---
    MokshaLLVMLowering
    MokshaLLVMTranslation
    MokshaLLVMCodeGen


    # --- LLD LINKER LIBRARIES ---
    lldELF
    lldCOFF
    lldMachO
    lldWasm
    lldMinGW
    lldCommon

    # --- MLIR LIBRARIES ---
    MokshaMLIR
    MLIRIR
    MLIRFuncDialect
    MLIRControlFlowDialect
    MLIRLLVMDialect
    MLIRPass
    MLIRSupport
    MLIRTransforms
    MLIRRewrite
    MLIRTargetLLVMIRExport
    MLIRLLVMToLLVMIRTranslation
    MLIRReconcileUnrealizedCasts
)
