#include <iostream>
#include "Game.h"
#include <SDL3_image/SDL_image.h>

int main(int argc, char* argv[])
{

    Game game;
    
   
    if (!game.initialize()) {
        std::cerr << "Failed to initialize game!" << std::endl;
        return 1;
    }
    
    game.run();

  
    std::cout << "Game finished. Press Enter to exit..." << std::endl;
    return 0;
}