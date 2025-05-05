#pragma once

#include <SDL3/SDL.h>
#include <map>
#include <string>
#include "Board.h"
#include "Piece.h"
#include <future>
class Game
{
private:
    SDL_Window *window;
    SDL_Renderer *renderer;
    std::map<std::string, SDL_Texture *> pieceTextures;
    Board board;
    bool running;

    Uint32 lastFrameTime;

    bool m_promotionInProgress;
    int m_promotionRow;
    bool m_gameOver;
    int m_promotionCol;
    Color m_promotionColor;
    bool m_moveJustFinished = false;
    std::future<std::tuple<int, int, int, int>> aiFuture;
    bool aiMovePending;
    bool m_endgameMessageDisplayed = false;
public:
    Game();
    ~Game();

    bool initialize();
    void run();
    void handleEvents();
    void update(float deltaTime);
    void render();
    void cleanup();
    Board &getBoard() { return board; }
    SDL_Texture *loadTexture(const char *path) const;
    SDL_Texture *getPieceTexture(PieceType type, Color color) const;
    void startPromotion(int row, int col, Color color);
    void handlePromotion(PieceType type);
    bool isPromotionInProgress() const { return m_promotionInProgress; }
    void displayEndGameMessage();
};