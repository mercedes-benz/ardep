#!/bin/bash
# ARDEP Toolchain Validation Script
# Checks for compatible GDB required by Black Magic Probe

echo "================================================================================"
echo "ARDEP Toolchain Validation"
echo "================================================================================"
echo "Checking for compatible GDB..."
echo ""

if command -v gdb-multiarch >/dev/null 2>&1; then
    GDB_VERSION=$(gdb-multiarch --version | head -n1)
    echo "[OK] gdb-multiarch found: $GDB_VERSION"
    exit 0
elif command -v arm-none-eabi-gdb >/dev/null 2>&1; then
    GDB_VERSION=$(arm-none-eabi-gdb --version | head -n1)
    echo "[OK] arm-none-eabi-gdb found: $GDB_VERSION"
    exit 0
else
    echo "[FAIL] No compatible GDB found"
    echo ""
    echo "Install one of the following:"
    echo "  Ubuntu/Debian: sudo apt install gdb-multiarch"
    echo "  Arch Linux:    sudo pacman -S arm-none-eabi-gdb"
    echo "  macOS:         brew install armmbed/formulae/arm-none-eabi-gcc"
    echo "  Windows:       Download from https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm"
    echo ""
    echo "After installation, re-run this script to verify."
    exit 1
fi