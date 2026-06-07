#!/bin/bash

echo "========================================"
echo " Setting up Moksha for Linux            "
echo "========================================"

# Ensure script is run with root privileges for apt
if [ "$EUID" -ne 0 ]; then
  echo "Please run this script with sudo:"
  echo "sudo ./install_linux.sh"
  exit 1
fi

echo "1. Installing base build tools (CMake, Ninja, standard libraries)..."
apt-get update
apt-get install -y \
    cmake \
    ninja-build \
    build-essential \
    wget \
    lsb-release \
    software-properties-common \
    gnupg

echo "2. Fetching the official LLVM installation script..."
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh

echo "3. Installing LLVM 22, Clang 22, and MLIR 22..."
# The 'all' argument ensures MLIR and other sub-projects are included
./llvm.sh 22 all

# Clean up the downloaded script
rm llvm.sh

echo "========================================"
echo " Installation Complete!                 "
echo "========================================"
echo "Note: The LLVM tools are installed with version suffixes (e.g., clang-22, mlir-opt-22)."