#!/bin/bash
# ARDEP GDB Toolchain Check

if command -v gdb-multiarch >/dev/null 2>&1; then
    echo "OK: gdb-multiarch"
    exit 0
elif command -v arm-none-eabi-gdb >/dev/null 2>&1; then
    echo "OK: arm-none-eabi-gdb"
    exit 0
else
    echo "ERROR: No ARM GDB found."
    echo "Install: sudo apt install gdb-multiarch"
    exit 1
fi