@echo off
REM ARDEP Black Magic Probe Detection

for %%D in (\\.\COM3 \\.\COM4 \\.\COM5 \\.\COM6 \\.\COM7 \\.\COM8 \\.\COM9 \\.\COM10 \\.\COM11 \\.\COM12 \\.\COM13 \\.\COM14 \\.\COM15 \\.\COM16) do (
    mode %%D: >nul 2>nul
    if %errorlevel% equ 0 (
        echo OK: BMP at %%D
        exit /b 0
    )
)

echo ERROR: No Black Magic Probe detected.
echo Check USB connection and driver installation.
exit /b 1