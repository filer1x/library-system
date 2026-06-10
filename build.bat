@echo off
chcp 65001 >nul
cd /d "%~dp0"

where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo CMake was not found on this PC.
    echo Install CMake and a C compiler, or open the project in an IDE with CMake support.
    echo.
    pause
    exit /b 1
)

rem Delete stale CMakeCache so a fresh configure always runs
if exist build\CMakeCache.txt del /q build\CMakeCache.txt

cmake -S . -B build
if %errorlevel% neq 0 goto build_error

cmake --build build
if %errorlevel% neq 0 goto build_error

echo.
echo Build successful.
echo Run the program with run.bat
pause
exit /b 0

:build_error
echo.
echo Build failed.
pause
exit /b 1
