# 1. Standard inclusions
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MLIR_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
)

# 2. Set the target for Dialect, Ops, Types, and Interfaces
set(LLVM_TARGET_DEFINITIONS MokshaOps.td)

mlir_tablegen(MokshaDialect.h.inc -gen-dialect-decls -dialect=moksha)
mlir_tablegen(MokshaDialect.cpp.inc -gen-dialect-defs -dialect=moksha)
mlir_tablegen(MokshaOps.h.inc -gen-op-decls)
mlir_tablegen(MokshaOps.cpp.inc -gen-op-defs)
mlir_tablegen(MokshaOpsTypes.h.inc -gen-typedef-decls -typedefs-dialect=moksha)
mlir_tablegen(MokshaOpsTypes.cpp.inc -gen-typedef-defs -typedefs-dialect=moksha)

set(LLVM_TARGET_DEFINITIONS MokshaInterfaces.td)
mlir_tablegen(MokshaInterfaces.h.inc -gen-op-interface-decls)
mlir_tablegen(MokshaInterfaces.cpp.inc -gen-op-interface-defs)

# 3. CRITICAL FIX: Reset the definitions file for the Passes
set(LLVM_TARGET_DEFINITIONS MokshaPasses.td)
mlir_tablegen(MokshaPasses.h.inc -gen-pass-decls -name=Moksha)

# 4. Final target
add_public_tablegen_target(MokshaDialectIncGen)
