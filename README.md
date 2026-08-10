<p align="left">
  <img src="assets/logo.svg" alt="Moksha Language Logo" width="150" />
</p>

# Moksha

<p align="left">
  <a href="https://discord.com/channels/1510845258605527151/1510845260547227791">
    <img src="https://img.shields.io/badge/Discord-333333?style=flat-square&logo=discord&logoColor=5865F2" alt="Discord" />
  </a>
  <a href="https://youtube.com/@mokshaprogramminglanguage?si=qzSkAgmgsRRNs-jA">
    <img src="https://img.shields.io/badge/YouTube-333333?style=flat-square&logo=youtube&logoColor=FF0000" alt="YouTube" />
  </a>
  <a href="https://moksha-lang-web.onrender.com/">
    <img src="https://img.shields.io/badge/Website-333333?style=flat-square&logo=data:image/svg%2Bxml;base64,PHN2ZyB2aWV3Qm94PSIwIDAgMTAwIDEwMCIgZmlsbD0ibm9uZSIgc3Ryb2tlPSIjYjg1YzE5IiBzdHJva2Utd2lkdGg9IjYiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIgc3Ryb2tlLWxpbmVqb2luPSJyb3VuZCIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KICA8Y2lyY2xlIGN4PSIyMCIgY3k9IjgwIiByPSI1IiBmaWxsPSIjYjg1YzE5IiAvPgogIDxjaXJjbGUgY3g9IjIwIiBjeT0iMzUiIHI9IjUiIGZpbGw9IiNiODVjMTkiIC8+CiAgPGNpcmNsZSBjeD0iNTAiIGN5PSI2NSIgcj0iNSIgZmlsbD0iI2I4NWMxOSIgLz4KICA8Y2lyY2xlIGN4PSI4MCIgY3k9IjM1IiByPSI1IiBmaWxsPSIjYjg1YzE5IiAvPgoKICA8cGF0aCBkPSJNIDIwIDc1IEwgMjAgNDAiIC8+CiAgPHBhdGggZD0iTSAyMyAzOCBMIDQ3IDYyIiAvPgogIDxwYXRoIGQ9Ik0gNTMgNjIgTCA3NyAzOCIgLz4KCiAgPHBhdGggZD0iTSA4MCAzMCBMIDgwIDE1IiBzdHJva2UtZGFzaGFycmF5PSI0IDQiIC8+CiAgPHBhdGggZD0iTSA3MCAyNSBMIDgwIDE1IEwgOTAgMjUiIC8+Cjwvc3ZnPgo=" alt="Moksha Website" />
  </a>
</p>

Moksha is a statically typed systems programming language with a custom multi-stage compilation pipeline, aimed to bridge performance with flexibility and safety.

<p align="left">
  <a href="https://github.com/Swarup-Moulik/Moksha/stargazers">
    <img src="https://img.shields.io/github/stars/Swarup-Moulik/Moksha?style=flat-square&color=c9a0dc" alt="GitHub Stars" />
  </a>
  <a href="https://github.com/Swarup-Moulik/Moksha/network/members">
    <img src="https://img.shields.io/github/forks/Swarup-Moulik/Moksha?style=flat-square&color=c9a0dc" alt="GitHub Forks" />
  </a>
  
  <a href="https://github.com/Swarup-Moulik/Moksha/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/Swarup-Moulik/Moksha?style=flat-square&color=4caf50" alt="License" />
  </a>
</p>

---

> **⚠️ Disclaimer: Active Development**
> Moksha is currently in an early, active stage of development. While the core compilation pipeline is functional, features are subject to change, and you may encounter bugs or incomplete implementations.

> **⚠️ Platform Support Warning**
> Moksha has **only been tested on Windows 10 and Kali Linux**. Other platforms (macOS, Android, iOS, WebAssembly, bare-metal) are targeted by the backend, but may not work reliably and have not been validated. Build and run at your own risk on untested platforms.

## Core Features

- **Deterministic Memory Management:** Zero-cost abstractions with no Garbage Collector. Moksha utilizes Automatic Reference Counting (ARC) paired with strict Non-Lexical Lifetime (NLL) borrow checking to ensure memory safety at compile time.
- **Modern Multi-Stage Backend:** Powered by a custom MLIR dialect and the LLVM 22 infrastructure, delivering aggressive optimization passes and highly efficient machine code.
- **Built-in Fixed-Point Decimals:** First-class support for fixed-point arithmetic, guaranteeing exact precision for financial calculations and avoiding standard floating-point rounding errors.
- **Seamless C-FFI:** Native, overhead-free interoperability with existing C/C++ libraries. You can link and call external functions directly using `unsafe` blocks.
- **Write Once, Compile Anywhere:** First-class cross-compilation support for Linux, Windows, macOS, Android, iOS, and WebAssembly (WASI & Browser) straight out of the box.
- **Bare-Metal Ready:** Features a modular, lightweight runtime library (`libmoksha_rt`) designed to run in freestanding environments without OS dependencies.

## Code Example

Here is a quick look at Moksha's safe borrowing and reference mechanisms:

```moksha
    int data = 10;
    *mut int m1 = &data;

    // Reborrow from the reference itself
    *view int v = &*m1;

    int read = *v;
    println(read); // Expected: 10

    // 'v' dies here, releasing the view borrow.

    *m1 = 20; // Safe to use m1 again!
    println(data); // Expected: 20
```

---

# Architecture

```mermaid
flowchart TD
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
```

---

# Setup & Installation

To build and run Moksha, you need the MSYS2 MinGW64 environment.

---

## 1. Prerequisites

**For Linux (Debian/Ubuntu):**
Run the included setup script:
`sudo ./install_linux.sh`

**For Windows (MSYS2):**
Run the included `install_windows.sh` script, or install manually:

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

## License

Moksha is distributed under the MIT License. See `LICENSE` for more information.
