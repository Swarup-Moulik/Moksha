add_mlir_dialect_library(MokshaMLIR
  dialect/MokshaDialect.cpp
  dialect/MokshaOps.cpp
  lowering/MIRToMLIR.cpp
  lowering/TypeLowering.cpp
  passes/CanonicalizeARC.cpp

  DEPENDS
  MokshaDialectIncGen

  LINK_LIBS
  mokshaSettings
  MokshaMiddle
  MokshaSupport
  MLIRIR
  MLIRFuncDialect
  MLIRControlFlowDialect
  MLIRPass
  MLIRTransforms
  MLIRRewrite
)

# --- ADD THIS BLOCK ---
target_include_directories(MokshaMLIR PRIVATE
  ${CMAKE_BINARY_DIR}/include
)
