# Moksha

Moksha is a statically typed systems programming language with a custom multi-stage compilation pipeline, aimed to 
bridge the performance with flexibility and safety.

---

# Architecture

## Frontend

- Lexer
- Parser
- Semantic analysis
- Generic resolution
- AST → HIR lowering

## Middle-end

- HIR representation
- Ownership analysis (ARC)
- Non Lexical Lifetime Borrow Checking
- HIR → MIR lowering
- MIR verification & dominance analysis

## Backend

- Custom MLIR dialect
- MIR → MLIR lowering
- MLIR → LLVM lowering

## Runtime

- ARC runtime
- Strings
- Arrays
- Panic handling

---

# Setup & Installation

To build and run Moksha, you need the MSYS2 MinGW64 environment.

---

## 1. Prerequisites

Open your MSYS2 MinGW 64-bit terminal and install the necessary toolchain:

```bash
# Update package database
pacman -Syu

# Install GCC, Clang, LLVM, MLIR, CMake, and Ninja
pacman -S --needed mingw-w64-x86_64-toolchain \
  mingw-w64-x86_64-llvm \
  mingw-w64-x86_64-mlir \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja
```

---

## 2. Configure Environment Path

To run `mokshac` from terminal of any IDE, add the Moksha binary directory to your `PATH` of msys2 mingw64 terminal.

### Create the destination directory

```bash
mkdir -p /c/Moksha/bin /c/Moksha/runtime
```

### Open your bash configuration

```bash
nano ~/.bashrc
```

### Add the following line to the end of the file

```bash
export PATH="$PATH:/c/Moksha/bin"
```

### Apply the changes

```bash
source ~/.bashrc
```

---

## 3. Build & Install

Build the compiler from scratch and copy the artifacts to your path:

```bash
# Configure and Build
cmake -B build -G Ninja
cmake --build build

# Install to system path
cp build/tools/mokshac.exe /c/Moksha/bin/
cp build/runtime/libmoksha_rt_windows.a /c/Moksha/runtime/
```

---

## 4. Verify Installation

Open a new terminal and verify the installation:

```bash
which mokshac
```

Expected output:

```bash
/c/Moksha/bin/mokshac.exe
```

---

# Usage

Compile a Moksha source file:

```bash
mokshac array_builtins.mox -o array_builtins
./array_builtins
```
