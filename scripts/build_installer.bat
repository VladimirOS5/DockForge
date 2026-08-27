@echo off
chcp 65001 >nul
echo ==========================================
echo   DockForge Build Script (Chat 11)
echo ==========================================

set BUILD_TYPE=Release
set GENERATOR="Visual Studio 17 2022"

if not exist build mkdir build
cd build

echo [1/4] Configuring CMake...
cmake .. -G %GENERATOR% -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo [2/4] Building DockForge...
cmake --build . --config %BUILD_TYPE% --parallel
if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo [3/4] Building Installer...
if not exist ..\dist mkdir ..\dist
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" ..\installer\DockForge.iss
if errorlevel 1 (
    echo Installer build failed! Make sure Inno Setup 6 is installed.
    pause
    exit /b 1
)

echo [4/4] Done!
echo Output: dist\DockForge_1.0.0_Setup.exe
echo ==========================================
pause
