@echo off
echo ======================================
echo       FintechApp Build Script
echo ======================================

echo.
echo Compiling resources...
windres resource.rc -O coff -o resource.res
if %errorlevel% neq 0 (
    echo Resource compilation FAILED!
    pause
    exit /b 1
)

echo.
echo Compiling source files...
g++ -I. -std=c++17 -mwindows ^
main.cpp ^
bank.cpp ^
resource.res ^
-o FintechApp.exe ^
-lcomctl32 -lgdi32 -luser32 -lbcrypt ^
-static -static-libgcc -static-libstdc++

if %errorlevel% neq 0 (
    echo.
    echo Build FAILED
) else (
    echo.
    echo Build SUCCESSFUL
)

pause