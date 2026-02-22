# ============================================================
# Moksha Frontend Library
# ============================================================

add_library(mokshaFrontend STATIC
  # --- Lexer ---
  lexer/Lexer.cpp

  # --- Parser ---
  parser/Parser.cpp

  # --- AST ---
  ast/ASTContext.cpp
  ast/ASTPrinter.cpp
  ast/Type.cpp
  ast/Expr.cpp
  ast/Stmt.cpp

  # --- Builtins ---
  builtins/BuiltinRegistry.cpp

  # --- Macros ---
  macros/MacroExpander.cpp

  # --- Semantic Analysis ---
  sema/GenericResolver.cpp
  sema/SymbolTable.cpp
  sema/TypeChecker.cpp
)

# ============================================================
# Include directories
# ============================================================

target_include_directories(mokshaFrontend PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

target_include_directories(mokshaFrontend PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============================================================
# Compiler warnings
# ============================================================

if(MSVC)
  target_compile_options(mokshaFrontend PRIVATE /W4)
else()
  target_compile_options(mokshaFrontend PRIVATE
    -Wall -Wextra -Wpedantic -Wno-unused-parameter
  )
endif()

# ============================================================
# Dependencies (MERGED AND CORRECTED)
# ============================================================

target_link_libraries(mokshaFrontend
  PUBLIC
    mokshaSettings
  PRIVATE
    MokshaSupport    # This provides the DiagnosticEngine/Builder implementation
    LLVMSupport
    LLVMBinaryFormat
)
