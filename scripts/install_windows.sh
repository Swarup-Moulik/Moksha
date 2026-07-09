#!/bin/bash

echo "========================================"
echo " Setting up Moksha for Windows (MSYS2)  "
echo "========================================"

# Check if running inside MSYS2
if [ ! -n "$MSYSTEM" ]; then
    echo "Error: This script must be run from inside the MSYS2 MinGW64 terminal."
    echo "Please download MSYS2 from https://www.msys2.org/ if you haven't already."
    exit 1
fi

echo "Updating MSYS2 package databases..."
pacman -Syu --noconfirm

echo "Installing LLVM, Clang, MLIR, CMake, and Ninja..."
pacman -S --noconfirm \
    mingw-w64-x86_64-llvm \
    mingw-w64-x86_64-clang \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-ninja \
    mingw-w64-x86_64-toolchain

echo "========================================"
echo " Installation Complete!                 "
echo "========================================"
echo "You can now build Moksha using CMake and Ninja."