@echo off
setlocal

REM ============================================
REM Mavish Game - Windows Build Script
REM ============================================

REM Check for VCPKG_ROOT environment variable
if "%VCPKG_ROOT%"=="" (
    echo ERROR: VCPKG_ROOT environment variable is not set.
    echo.
    echo Please install vcpkg and set VCPKG_ROOT:
    echo   1. git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
    echo   2. cd C:\vcpkg ^&^& bootstrap-vcpkg.bat
    echo   3. setx VCPKG_ROOT "C:\vcpkg"
    echo   4. Restart your terminal
    echo.
    exit /b 1
)

REM Parse arguments
set BUILD_TYPE=Release
set CLEAN_BUILD=0
set RUN_AFTER=0

:parse_args
if "%~1"=="" goto :done_args
if /i "%~1"=="debug" set BUILD_TYPE=Debug
if /i "%~1"=="release" set BUILD_TYPE=Release
if /i "%~1"=="clean" set CLEAN_BUILD=1
if /i "%~1"=="run" set RUN_AFTER=1
shift
goto :parse_args
:done_args

REM Clean build directory if requested
if %CLEAN_BUILD%==1 (
    echo Cleaning build directory...
    if exist build rmdir /s /q build
)

REM Create build directory
if not exist build mkdir build

REM Configure with CMake
echo.
echo Configuring CMake (%BUILD_TYPE%)...
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%

if errorlevel 1 (
    echo CMake configuration failed!
    exit /b 1
)

REM Build
echo.
echo Building...
cmake --build build --config %BUILD_TYPE%

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo.
echo ============================================
echo Build successful!
echo Executable: build\%BUILD_TYPE%\MavishGame.exe
echo ============================================

if %RUN_AFTER%==1 (
    echo.
    echo Starting game...
    start "" "build\%BUILD_TYPE%\MavishGame.exe"
)

endlocal
