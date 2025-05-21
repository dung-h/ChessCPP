cmake_minimum_required(VERSION 3.15)

project(ChessGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Include project headers for all platforms
include_directories(${PROJECT_SOURCE_DIR}/include)

# Platform-specific configuration
if(WIN32)
    # Windows-specific setup
    set(SDL3_PATH "${CMAKE_SOURCE_DIR}/external/SDL3")
    set(SDL3_INCLUDE_DIR "${SDL3_PATH}/include")
    set(SDL3_LIB_DIR "${SDL3_PATH}/lib/x64")
    
    set(SDL3_IMAGE_PATH "${CMAKE_SOURCE_DIR}/external/SDL3_image")
    set(SDL3_IMAGE_INCLUDE_DIR "${SDL3_IMAGE_PATH}/include")
    set(SDL3_IMAGE_LIB_DIR "${SDL3_IMAGE_PATH}/lib/x64")
    
    # Include directories (Windows)
    include_directories(${SDL3_INCLUDE_DIR})
    include_directories(${SDL3_IMAGE_INCLUDE_DIR})
    
    # Add library directories (Windows only)
    link_directories(${SDL3_LIB_DIR})
    link_directories(${SDL3_IMAGE_LIB_DIR})
    
    # Copy DLL files to build directory
    file(COPY "stockfish.exe" DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY "${SDL3_LIB_DIR}/SDL3.dll" DESTINATION ${CMAKE_BINARY_DIR})
    
    # Copy SDL3_image.dll and dependencies
    file(COPY "${SDL3_IMAGE_LIB_DIR}/SDL3_image.dll" DESTINATION ${CMAKE_BINARY_DIR})
    
    # Copy any additional required DLLs (like libpng, zlib, etc.)
    file(GLOB SDL_DEPENDENCY_DLLS "${SDL3_IMAGE_LIB_DIR}/*.dll")
    foreach(DLL_FILE ${SDL_DEPENDENCY_DLLS})
        if(NOT DLL_FILE MATCHES "SDL3_image.dll")
            file(COPY ${DLL_FILE} DESTINATION ${CMAKE_BINARY_DIR})
        endif()
    endforeach()
else()
    # Add Linux-specific definitions
    add_compile_definitions(USE_SDL2)
    
    # Find required packages for Linux
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
    
    # Include the SDL2 directories
    include_directories(${SDL2_INCLUDE_DIRS})
    include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
    
    # Copy the stockfish binary
    file(COPY "stockfish" DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Copy images folder to build directory (for all platforms)
file(COPY "${CMAKE_SOURCE_DIR}/images" DESTINATION ${CMAKE_BINARY_DIR})

# Add all source files
file(GLOB SOURCE_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

# Create executable
add_executable(chess_game ${SOURCE_FILES})

# Link libraries
if(WIN32)
    target_link_libraries(chess_game SDL3 SDL3_image)
else()
    target_link_libraries(chess_game ${SDL2_LIBRARIES} SDL2_image)
endif()