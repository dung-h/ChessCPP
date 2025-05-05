@echo off
echo Checking dependencies for ChessGame...

REM Check for CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo CMake not found. Please install CMake from https://cmake.org/download/
    echo You need CMake 3.15 or higher.
    exit /b 1
)
echo CMake found.

REM Check external libraries
if not exist external\SDL3\include\SDL3\SDL.h (
    echo SDL3 not found in external/SDL3 directory.
    echo Please ensure you have the correct SDL3 files in external/SDL3.
    exit /b 1
)

if not exist external\SDL3_image\include\SDL3_image\SDL_image.h (
    echo SDL3_image not found in external/SDL3_image directory.
    echo Please ensure you have the correct SDL3_image files in external/SDL3_image.
    exit /b 1
)

echo All dependencies are satisfied!
echo You can now build the project with:
echo   mkdir build
echo   cd build
echo   cmake ..
echo   cmake --build .
echo.