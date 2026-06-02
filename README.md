<p align="left">
  <img src="logo.svg" alt="Moksha Language Logo" width="150" />
</p>

# Moksha

Moksha is a statically typed systems programming language with a custom multi-stage compilation pipeline, aimed to
bridge the performance with flexibility and safety.

---

## Code Example

Here is a quick look at Moksha's safe borrowing and reference mechanisms:

````moksha
    int data = 10;
    *mut int m1 = &data;

    // Reborrow from the reference itself
    *view int v = &*m1;

    int read = *v;
    println(read); // Expected: 10

    // 'v' dies here, releasing the view borrow.

    *m1 = 20; // Safe to use m1 again!
    println(data); // Expected: 20
````

---

# Architecture

graph TD
    Source[(Source Code)] --> Frontend

    subgraph Frontend [1. Frontend]
        direction TB
        L[Lexer] --> P[Parser]
        P --> SA[Semantic Analysis]
        SA --> GR[Generic Resolution]
        GR --> AST[AST → HIR Lowering]
    end

    Frontend --> MiddleEnd

    subgraph MiddleEnd [2. Middle-end]
        direction TB
        HIR[HIR Representation] --> ARC[Ownership Analysis ARC]
        ARC --> NLL[Non-Lexical Lifetime Borrow Checking]
        NLL --> H2M[HIR → MIR Lowering]
        H2M --> MV[MIR Verification & Dominance Analysis]
    end

    MiddleEnd --> Backend

    subgraph Backend [3. Backend]
        direction TB
        MLIR[Custom MLIR Dialect] --> M2M[MIR → MLIR Lowering]
        M2M --> LLVM[MLIR → LLVM Lowering]
    end

    Backend --> Exec[(Executable)]

    subgraph RuntimeEnvironment [Runtime Components]
        direction LR
        RT_ARC[ARC Runtime]
        RT_STR[Strings]
        RT_ARR[Arrays]
        RT_PANIC[Panic Handling]
    end

    RuntimeEnvironment -. Links to .-> Exec

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
````

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
cp build/tools/moksha-opt.exe /c/Moksha/bin/
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

---

# Cross Compilation Targets

Moksha supports LLVM target triples for cross-platform compilation.

Example:

```bash
mokshac cross_compile.mox -target x86_64-pc-linux-gnu -o cross_compile_linux
```

## Supported Target Flags

| Platform              | Target Flag                      |
| --------------------- | -------------------------------- |
| Linux (x86_64)        | `-target x86_64-pc-linux-gnu`    |
| Android (ARM64)       | `-target aarch64-linux-android`  |
| macOS (Apple Silicon) | `-target arm64-apple-darwin`     |
| iOS (ARM64)           | `-target aarch64-apple-ios`      |
| Windows (MSVC x86_64) | `-target x86_64-pc-windows-msvc` |
| WebAssembly WASI      | `-target wasm32-wasi`            |
| WebAssembly Browser   | `-target wasm32-unknown-unknown` |

## Example Commands

```bash
# Linux
mokshac cross_compile.mox -target x86_64-pc-linux-gnu -o cross_compile_linux

# Android
mokshac cross_compile.mox -target aarch64-linux-android -o cross_compile_android

# macOS
mokshac cross_compile.mox -target arm64-apple-darwin -o cross_compile_mac

# iOS
mokshac cross_compile.mox -target aarch64-apple-ios -o cross_compile_ios

# Windows
mokshac cross_compile.mox -target x86_64-pc-windows-msvc -o cross_compile_windows

# WASI
mokshac cross_compile.mox -target wasm32-wasi -o cross_compile_wasm.wasm

# Browser WebAssembly
mokshac cross_compile.mox -target wasm32-unknown-unknown -o index.html
```
