add_library(MokshaMiddle
    hir/HIRExpr.cpp
    hir/HIRFunction.cpp
    hir/HIRGen.cpp
    hir/HIRModule.cpp
    hir/HIRStmt.cpp
    hir/HIRType.cpp
    hir/LowerASTToHIR.cpp

    mir/MIRBlock.cpp
    mir/MIRBuilder.cpp
    mir/MIRFunction.cpp
    mir/MIRInst.cpp
    mir/MIRModule.cpp
    mir/MIRVerifier.cpp

    mir/passes/ConstantFoldingPass.cpp
    mir/passes/DeadCodeEliminationPass.cpp
    mir/passes/PassManager.cpp
    mir/passes/Mem2RegPass.cpp
    mir/passes/EscapeAnalysisPass.cpp
    mir/passes/InliningPass.cpp
    mir/passes/DropElisionPass.cpp
    mir/passes/SROAPass.cpp
    mir/passes/JumpThreadingPass.cpp
    mir/passes/SimplifyCFGPass.cpp

    analysis/MIRDominance.cpp
    analysis/NLLBorrowChecker.cpp

    passes/LowerHIRToMIR.cpp
    passes/VerifyNoMacros.cpp

    ownership/ARCAnalyzer.cpp
    ownership/ARCInserter.cpp
    ownership/BorrowChecker.cpp
)

target_link_libraries(MokshaMiddle
    PUBLIC
    mokshaSettings
    MokshaSupport
    LLVMCore
    mokshaFrontend
)
