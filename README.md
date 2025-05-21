# Chess Game

A chess game implementation using SDL3 with AI opponent capabilities.

## Features

- Complete chess rules implementation including special moves
- AI opponent with minimax algorithm
- Graphical user interface built with SDL3
- Check and checkmate detection

## Requirements

- C++17 compatible compiler
- CMake 3.15 or higher
- SDL3 and SDL3_image libraries (installation instructions below)

## Building and Running

### Windows

#### Pre-requisites
1. Install [CMake](https://cmake.org/download/) (3.15 or higher)
2. Install a C++ compiler (Visual Studio, MinGW, etc.)

#### Building
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

#### Running
Before running the game, make sure the SDL DLLs are in the same directory as your executable:
1. Copy `SDL3.dll` and `SDL3_image.dll` from `external/SDL3/bin/` and `external/SDL3_image/bin/` to your build directory
2. Run the game: `.\chess_game.exe`

#### Troubleshooting
If the game builds but doesn't run:
- Ensure SDL DLLs are correctly copied to the build directory
- Try running the executable from command prompt to see any error messages

### Linux

#### Pre-requisites
Install the required development libraries:

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential cmake libsdl2-dev libsdl2-image-dev
```

**Fedora:**
```bash
sudo dnf install cmake gcc-c++ SDL2-devel SDL2_image-devel
```

**Arch Linux:**
```bash
sudo pacman -S cmake gcc sdl2 sdl2_image
```

#### Building
```bash
mkdir build && cd build
cmake -DUSE_SDL2=ON ..  # Note: Linux uses SDL2
make
```

#### Running
```bash
./chess_game
```

### macOS

#### Pre-requisites
1. Install [Homebrew](https://brew.sh/)
2. Install dependencies:
```bash
brew install cmake sdl2 sdl2_image
```

#### Building
```bash
mkdir build && cd build
cmake -DUSE_SDL2=ON ..  # Note: macOS uses SDL2
make
```

#### Running
```bash
./chess_game
```

## How to Play

- **Starting**: White (you) plays first against the AI
- **Moving**: Click to select a piece, then click a highlighted square to move
- **Special Moves**: Castling, En Passant, and Pawn Promotion are supported
- **Game End**: Game automatically announces checkmate/stalemate when it occurs

## Contributing

Contributions welcome! Please submit a Pull Request.