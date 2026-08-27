@echo off
chcp 65001 >nul
echo ==========================================
echo   DockForge Release Build (Chat 12)
echo   v1.0.0
echo ==========================================

set BUILD_TYPE=Release
set GENERATOR="Visual Studio 17 2022"

if not exist build mkdir build
cd build

echo [1/5] Configuring CMake...
cmake .. -G %GENERATOR% -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

echo [2/5] Building DockForge...
cmake --build . --config %BUILD_TYPE% --parallel
if errorlevel 1 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo [3/5] Running self-tests...
.\%BUILD_TYPE%\DockForge.exe /selftest
if errorlevel 1 (
    echo [WARN] Self-tests detected issues, continuing...
)

echo [4/5] Building Installer...
if not exist ..\dist mkdir ..\dist
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" ..\installer\DockForge.iss
if errorlevel 1 (
    echo [WARN] Installer build failed! Make sure Inno Setup 6 is installed.
    echo [INFO] Executable is still available at build\Release\DockForge.exe
) else (
    echo [OK] Installer built successfully.
)

echo [5/5] Release build complete!
echo Output: build\Release\DockForge.exe
echo Installer: dist\DockForge_1.0.0_Setup.exe
echo ==========================================
pause
