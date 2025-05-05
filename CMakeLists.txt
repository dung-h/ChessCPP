cmake_minimum_required(VERSION 3.15)

project(ChessGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Set the proper SDL3 paths using relative paths
set(SDL3_PATH "${CMAKE_SOURCE_DIR}/external/SDL3")
set(SDL3_INCLUDE_DIR "${SDL3_PATH}/include")
set(SDL3_LIB_DIR "${SDL3_PATH}/lib/x64")

# Add SDL3_image paths separately
set(SDL3_IMAGE_PATH "${CMAKE_SOURCE_DIR}/external/SDL3_image")
set(SDL3_IMAGE_INCLUDE_DIR "${SDL3_IMAGE_PATH}/include")
set(SDL3_IMAGE_LIB_DIR "${SDL3_IMAGE_PATH}/lib/x64")

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${SDL3_INCLUDE_DIR})
include_directories(${SDL3_IMAGE_INCLUDE_DIR})

# Add library directories
link_directories(${SDL3_LIB_DIR})
link_directories(${SDL3_IMAGE_LIB_DIR})

# Copy DLL files to build directory
file(COPY "${SDL3_PATH}/lib/x64/SDL3.dll" DESTINATION ${CMAKE_BINARY_DIR})
file(COPY "${SDL3_IMAGE_PATH}/lib/x64/SDL3_image.dll" DESTINATION ${CMAKE_BINARY_DIR})
file(COPY "${CMAKE_SOURCE_DIR}/images" DESTINATION ${CMAKE_BINARY_DIR})

# Add all source files
file(GLOB SOURCES "src/AI.cpp" "src/Board.cpp" "src/Game.cpp" 
               "src/Piece.cpp" "src/Zobrist.cpp" "src/main.cpp")

# Add executable
add_executable(chess_game ${SOURCES})

# Link the SDL3 libraries with correct paths
target_link_libraries(chess_game PRIVATE 
    "${SDL3_LIB_DIR}/SDL3.lib"
    "${SDL3_IMAGE_LIB_DIR}/SDL3_image.lib")