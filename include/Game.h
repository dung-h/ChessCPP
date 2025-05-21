#pragma once

#include "SDLIncludes.h"
#include <map>
#include <string>
#include "Board.h"
#include "Piece.h"
#include "StockFish.h"
#include <future>
#include <thread>
#include <atomic>

// Renamed from GameState to UIState to avoid conflict with Board.h
enum class UIState {
    MainMenu,
    Playing
};

class Game
{
private:
    SDL_Window *window;
    SDL_Renderer *renderer;
    std::map<std::string, SDL_Texture *> pieceTextures;
    Board board;
    bool running;
    std::thread stockfishInitThread;
    std::atomic<bool> stockfishInitInProgress;
    std::atomic<bool> stockfishInitialized;
    Uint32 lastFrameTime;
    std::string lastAnalyzedPosition;
    std::tuple<int, int, int, int> lastAnalyzedMove;
    bool m_promotionInProgress;
    int m_promotionRow;
    bool m_gameOver;
    int m_promotionCol;
    Color m_promotionColor;
    bool m_moveJustFinished = false;
    std::future<std::tuple<int, int, int, int>> aiFuture;
    bool aiMovePending;
    bool m_endgameMessageDisplayed = false;
    bool m_showLosingKing = false;
    int m_losingKingRow = -1;
    int m_losingKingCol = -1;
    Uint32 m_gameOverStartTime = 0;
    // Add these for async Stockfish
    std::future<std::tuple<int, int, int, int>> stockfishFuture;
    bool stockfishMovePending = false;
    SDL_Texture* thinkingIndicator = nullptr;
    SDL_FRect button;
    StockfishConnector stockfish;
    bool useStockfish = true;
    UIState m_gameState; // Changed from GameState to UIState
    SDL_FRect menuButtons[3]; // Array to hold menu buttons
    SDL_Texture* m_titleTexture = nullptr;
    SDL_Texture* m_stockfishButtonTexture = nullptr;
    SDL_Texture* m_minimaxButtonTexture = nullptr;
    SDL_Texture* m_exitButtonTexture = nullptr;
    SDL_Texture* m_returnToMenuTexture = nullptr;
    SDL_Texture* m_whiteWinsTexture = nullptr;
    SDL_Texture* m_blackWinsTexture = nullptr;
    SDL_Texture* m_drawTexture = nullptr;
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
    void renderMainMenu();
    void handleMainMenuClick(int x, int y);
    void startGameWithStockfish();
    void startGameWithMinimaxAI();
};