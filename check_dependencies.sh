#!/bin/bash

echo "Checking dependencies for ChessGame..."

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "CMake not found. Please install CMake 3.15 or higher."
    echo "On Ubuntu/Debian: sudo apt-get install cmake"
    echo "On MacOS: brew install cmake"
    exit 1
fi
echo "CMake found."

# Check for external libraries
if [ ! -f "external/SDL3/include/SDL3/SDL.h" ]; then
    echo "SDL3 not found in external/SDL3 directory."
    echo "Please ensure you have the correct SDL3 files in external/SDL3."
    exit 1
fi

if [ ! -f "external/SDL3_image/include/SDL3_image/SDL_image.h" ]; then
    echo "SDL3_image not found in external/SDL3_image directory."
    echo "Please ensure you have the correct SDL3_image files in external/SDL3_image."
    exit 1
fi

echo "All dependencies are satisfied!"
echo "You can now build the project with:"
echo "  mkdir build"
echo "  cd build"
echo "  cmake .."
echo "  cmake --build ."