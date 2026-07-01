@echo off
REM ARDEP GDB Toolchain Check

where arm-none-eabi-gdb >nul 2>nul
if %errorlevel% equ 0 (
    echo OK: arm-none-eabi-gdb
    exit /b 0
)

echo ERROR: No ARM GDB found.
echo Install ARM GCC toolchain from developer.arm.com
exit /b 1