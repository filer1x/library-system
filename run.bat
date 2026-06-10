@echo off
chcp 65001 >nul
cd /d "%~dp0"

if exist build\Debug\LibraryApp.exe (
    build\Debug\LibraryApp.exe
    echo.
    pause
    exit /b 0
)

if exist build\LibraryApp.exe (
    build\LibraryApp.exe
    echo.
    pause
    exit /b 0
)

if not exist LibraryApp.exe (
    call build.bat
)

if exist LibraryApp.exe (
    LibraryApp.exe
    echo.
    pause
)
