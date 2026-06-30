@echo off
REM ARDEP Toolchain Validation Script (Windows)
REM Checks for compatible GDB required by Black Magic Probe

echo ================================================================================
echo ARDEP Toolchain Validation
echo ================================================================================
echo Checking for compatible GDB...
echo.

where gdb-multiarch >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] gdb-multiarch found
    exit /b 0
)

where arm-none-eabi-gdb >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] arm-none-eabi-gdb found
    exit /b 0
)

echo [FAIL] No compatible GDB found
echo.
echo Install ARM GCC toolchain from:
echo https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
echo.
exit /b 1