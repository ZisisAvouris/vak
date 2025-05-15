@echo off
setlocal enabledelayedexpansion

echo.
cmake -Wno-dev -S . -B build/ -G "Visual Studio 17 2022" -A x64 -DCMAKE_CONFIGURATION_TYPES=RelWithDebInfo
if %errorlevel% neq 0 (
    echo Failed to generate Project Files.
    exit /b %errorlevel%
)

cmake --build build/ --config RelWithDebInfo
if %errorlevel% neq 0 (
    echo Build Failed.
    exit /b %errorlevel%
)

endlocal
