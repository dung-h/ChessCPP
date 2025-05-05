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
- Dependencies included: SDL3 and SDL3_image (Windows libraries)

## Building and Running

### Windows

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
./chess_game  # Run the executable
```

### Linux/WSL

Note: The included SDL libraries are Windows-specific. For Linux:

1. Install system SDL libraries: `sudo apt install libsdl3-dev libsdl3-image-dev`
2. Build as normal:
```bash
mkdir build && cd build
cmake ..
make
./chess_game
```

## How to Play

- **Starting**: White (you) plays first against the AI
- **Moving**: Click to select a piece, then click a highlighted square to move
- **Special Moves**: Castling, En Passant, and Pawn Promotion are supported
- **Game End**: Game automatically announces checkmate/stalemate when it occurs

## Contributing

Contributions welcome! Please submit a Pull Request.