cmake_minimum_required(VERSION 3.20)

# ============================================================
# Project
# ============================================================
project(moksha
  VERSION 0.1
  DESCRIPTION "Moksha Programming Language Compiler"
  LANGUAGES CXX C
)

# ============================================================
# Global build settings
# ============================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Useful for clangd / IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================
# LLVM / MLIR discovery
# ============================================================
# Do NOT hard-pin versions; assert minimum instead
find_package(LLVM REQUIRED CONFIG)
find_package(MLIR REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Found MLIR ${MLIR_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")

if(LLVM_PACKAGE_VERSION VERSION_LESS 17)
  message(FATAL_ERROR "LLVM >= 17 is required")
endif()

# ============================================================
# CMake module paths
# ============================================================
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${LLVM_CMAKE_DIR}"
)

# Some MLIR installs don't export MLIR_CMAKE_DIR
if(NOT MLIR_CMAKE_DIR)
  set(MLIR_CMAKE_DIR "${MLIR_DIR}")
endif()
list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")

# ============================================================
# LLVM / MLIR helper modules
# ============================================================
include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(HandleLLVMOptions)

add_subdirectory(include/moksha/Dialect)

# ============================================================
# Project-wide interface target
# ============================================================
# All Moksha libraries/tools should link against this
add_library(mokshaSettings INTERFACE)

target_include_directories(mokshaSettings INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

# Let LLVM/MLIR targets propagate their own flags & definitions
# (do NOT manually inject LLVM_INCLUDE_DIRS or LLVM_DEFINITIONS)

# ============================================================
# Warnings (strict but sane)
# ============================================================
if(MSVC)
  target_compile_options(mokshaSettings INTERFACE /W4)
else()
  target_compile_options(mokshaSettings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
  )
endif()

# ============================================================
# Subprojects (dependency order matters)
# ============================================================
add_subdirectory(lib)
add_subdirectory(runtime)
add_subdirectory(tools)

# ============================================================
# Testing (optional, enable later)
# ============================================================
# enable_testing()
# add_subdirectory(tests)
