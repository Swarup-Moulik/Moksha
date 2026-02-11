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

# Public: only stable API
target_include_directories(mokshaFrontend PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

# Private: internal structure
target_include_directories(mokshaFrontend PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============================================================
# Compiler warnings
# ============================================================

if(MSVC)
  target_compile_options(mokshaFrontend PRIVATE
    /W4
  )
else()
  target_compile_options(mokshaFrontend PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
  )
endif()

# ============================================================
# Dependencies
# ============================================================

target_link_libraries(mokshaFrontend
  PUBLIC
    mokshaSettings

  PRIVATE
    LLVMSupport
    LLVMBinaryFormat
)
