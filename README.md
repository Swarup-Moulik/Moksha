# Moksha

Moksha is a statically typed systems programming language with a custom
multi-stage compilation pipeline:

AST → HIR → MIR → MLIR → LLVM

## Architecture

Frontend:
- Lexer
- Parser
- Semantic analysis
- Generic resolution
- AST → HIR lowering

Middle-end:
- HIR representation
- Ownership analysis (ARC)
- HIR → MIR lowering
- MIR verification & dominance analysis

Backend:
- Custom MLIR dialect
- MIR → MLIR lowering
- MLIR → LLVM lowering

Runtime:
- ARC runtime
- Strings
- Arrays
- Panic handling

## Build

```bash
cmake -B build -G Ninja
cmake --build build
