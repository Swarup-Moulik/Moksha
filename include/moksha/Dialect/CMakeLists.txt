# include/moksha/Dialect/CMakeLists.txt

# 1. Set the source .td file
set(LLVM_TARGET_DEFINITIONS MokshaOps.td)

# 2. Tell TableGen where to find .td files
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MLIR_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
)

# 3. Generate the .inc files
# Since this file is in 'include/moksha/Dialect', the output
# will effectively be in 'build/include/moksha/Dialect'
mlir_tablegen(MokshaDialect.h.inc -gen-dialect-decls -dialect=moksha)
mlir_tablegen(MokshaDialect.cpp.inc -gen-dialect-defs -dialect=moksha)
mlir_tablegen(MokshaOps.h.inc -gen-op-decls)
mlir_tablegen(MokshaOps.cpp.inc -gen-op-defs)
mlir_tablegen(MokshaOpsTypes.h.inc -gen-typedef-decls -typedefs-dialect=moksha)
mlir_tablegen(MokshaOpsTypes.cpp.inc -gen-typedef-defs -typedefs-dialect=moksha)

# 4. Expose the target so the backend library can depend on it
add_public_tablegen_target(MokshaDialectIncGen)
