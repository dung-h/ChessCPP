#include "Game.h"
#include <iostream>
#include "SDLIncludes.h"
#include <future>
#include "Board.h"
#include <cmath>
#include "AI.h"
#include <chrono>


#include "Game.h"
#include <iostream>

Game::Game() : lastFrameTime(SDL_GetTicks()), m_moveJustFinished(false),
               m_promotionInProgress(false), m_gameOver(false)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        
        exit(1);
    }
}

Game::~Game()
{
    // Clean up UI textures
    if (m_titleTexture) SDL_DestroyTexture(m_titleTexture);
    if (m_stockfishButtonTexture) SDL_DestroyTexture(m_stockfishButtonTexture);
    if (m_minimaxButtonTexture) SDL_DestroyTexture(m_minimaxButtonTexture);
    if (m_exitButtonTexture) SDL_DestroyTexture(m_exitButtonTexture);
    if (m_returnToMenuTexture) SDL_DestroyTexture(m_returnToMenuTexture);
    if (m_whiteWinsTexture) SDL_DestroyTexture(m_whiteWinsTexture);
    if (m_blackWinsTexture) SDL_DestroyTexture(m_blackWinsTexture);
    if (m_drawTexture) SDL_DestroyTexture(m_drawTexture);
    

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::displayEndGameMessage()
{   
    // std::cout << "Enter displayEndGameMessage\n";
    if (m_gameOver && !m_endgameMessageDisplayed)
    {
        // Store the time when game over was triggered
        m_gameOverStartTime = SDL_GetTicks();
        
        if (board.isCheckmate())
        {
            Color losingColor = board.getCurrentTurn();
            std::string winner = (losingColor == Color::White) ? "Black" : "White";
            std::cerr << "Game over: Checkmate! " << winner << " wins!" << std::endl;
            
            // Store the losing king's position
            m_showLosingKing = true;
            if (losingColor == Color::White) {
                // White is in checkmate
                m_losingKingRow = board.getWhiteKingRow();
                m_losingKingCol = board.getWhiteKingCol();
                std::cerr << "White king at: " << m_losingKingRow << "," << m_losingKingCol << std::endl;
            } else {
                // Black is in checkmate
                m_losingKingRow = board.getBlackKingRow();
                m_losingKingCol = board.getBlackKingCol();
                std::cerr << "Black king at: " << m_losingKingRow << "," << m_losingKingCol << std::endl;
            }
        }
        else if (board.isStalemate())
        {
            std::cout << "Game over: Stalemate! Game is a draw." << std::endl;
            m_showLosingKing = false;
        }
        
        m_endgameMessageDisplayed = true;
    }
}

void Game::run()
{
    running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                int x = event.button.x;
                int y = event.button.y;
                
                // Handle clicks based on current game state
                if (m_gameState == UIState::MainMenu) {
                    handleMainMenuClick(x, y);
                }
                else if (m_gameState == UIState::Playing) {
                    if (m_gameOver) { 
                        // Handle game over click
                        int w, h;
                        SDL_GetWindowSize(window, &w, &h);
                        int boxHeight = 150;
                        int btnWidth = 100;
                        int btnHeight = 40;
                        SDL_FRect button = {
                            (float)(w - btnWidth) / 2,
                            (float)(h + boxHeight/2 - btnHeight - 20) / 2,
                            (float)btnWidth,
                            (float)btnHeight
                        };
                        
                        if (x >= button.x && x <= button.x + button.w &&
                            y >= button.y && y <= button.y + button.h) {
                            // Return to main menu when game ends
                            m_gameState = UIState::MainMenu;
                            m_gameOver = false;
                            m_endgameMessageDisplayed = false;
                        }
                    }
                    else if (board.isAnimationDone() && !m_gameOver) {
                        board.handleClick(x, y);
                    }
                }
            }
        }

        // Update and render based on current game state
        float deltaTime = 1.0f / 60.0f;
        
        try {
            if (m_gameState == UIState::MainMenu) {
                renderMainMenu();
            }
            else if (m_gameState == UIState::Playing) {
                update(deltaTime);
                
                if (m_gameOver) {
                    displayEndGameMessage();
                }
                render();
            }
        }
        catch (const std::exception &e) {
            std::cerr << "Exception in game loop: " << e.what() << std::endl;
        }

        // Cap the frame rate
        Uint32 frameTime = SDL_GetTicks() - lastFrameTime;
        if (frameTime < 16) {
            SDL_Delay(16 - frameTime);
        }
        lastFrameTime = SDL_GetTicks();
    }
}

void Game::render()
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    
    // Draw the board and pieces
    board.render(renderer, *this);

    // If game is over and we have a losing king, highlight it with a flashing red square
    if (m_showLosingKing && m_losingKingRow >= 0 && m_losingKingCol >= 0) {
        const int SQUARE_SIZE = 75; // Make sure this matches your board's square size
        
        // Make the square flash by changing alpha based on time
        Uint8 alpha = 128 + (Uint8)(127 * sin((SDL_GetTicks() - m_gameOverStartTime) / 300.0));
        
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, alpha);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        SDL_FRect kingRect = {
            (float)(m_losingKingCol * SQUARE_SIZE),
            (float)((7 - m_losingKingRow) * SQUARE_SIZE), // Convert from board to screen coordinates
            (float)SQUARE_SIZE,
            (float)SQUARE_SIZE
        };
        
        SDL_RenderFillRect(renderer, &kingRect);
    }

    // Show thinking indicator when Stockfish is calculating
    if (stockfishMovePending) {
        // Draw a semi-transparent overlay for the thinking indicator
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 64);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        // Draw at the bottom of the screen
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        SDL_FRect indicatorRect = {10.0f, h - 40.0f, 120.0f, 30.0f};
        SDL_RenderFillRect(renderer, &indicatorRect);
        
        // Simple animation of dots
        int numDots = (SDL_GetTicks() / 500) % 4; // Changes every 500ms
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        
        for (int i = 0; i < numDots; i++) {
            SDL_FRect dot = {20.0f + i * 20.0f, h - 25.0f, 10.0f, 10.0f};
            SDL_RenderFillRect(renderer, &dot);
        }
    }
    
     
    if (m_gameOver)
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        SDL_FRect fullscreen = {0.0f, 0.0f, (float)w, (float)h};
        SDL_RenderFillRect(renderer, &fullscreen);
        
        // Draw message box
        int boxWidth = 300;
        int boxHeight = 150;
        SDL_FRect messageBox = {
            (float)(w - boxWidth) / 2, 
            (float)(h - boxHeight) / 2, 
            (float)boxWidth, 
            (float)boxHeight
        };
        
        // Message box background
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 240);
        SDL_RenderFillRect(renderer, &messageBox);
        
        // Message box border
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderRect(renderer, &messageBox);
        
        // Draw appropriate game over message
        SDL_Texture* messageTexture = nullptr;
        if (board.isCheckmate()) {
            Color losingColor = board.getCurrentTurn();
            if (losingColor == Color::White) {
                messageTexture = m_blackWinsTexture;
            } else {
                messageTexture = m_whiteWinsTexture;
            }
        } else {
            messageTexture = m_drawTexture; // Stalemate
        }
        
        if (messageTexture) {
            SDL_FRect messageRect = {
                (float)(w - 200) / 2,
                (float)(h - boxHeight/2 - 40),
                200.0f,
                40.0f
            };
            SDL_RenderTexture(renderer, messageTexture, nullptr, &messageRect);
        }
        
        // Draw return to menu button
        int btnWidth = 100;
        int btnHeight = 40;
        button = {
            (float)(w - btnWidth) / 2,
            (float)(h + boxHeight/2 - btnHeight - 20) / 2,
            (float)btnWidth,
            (float)btnHeight
        };
        
        // Button with hover effect
        float mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        bool hovered = (mouseX >= button.x && mouseX <= button.x + button.w &&
                        mouseY >= button.y && mouseY <= button.y + button.h);
        
        // Draw a glow effect behind the button if hovered
        if (hovered) {
            SDL_FRect hoverRect = button;
            hoverRect.x -= 2;
            hoverRect.y -= 2;
            hoverRect.w += 4;
            hoverRect.h += 4;
            
            SDL_SetRenderDrawColor(renderer, 100, 150, 255, 128);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(renderer, &hoverRect);
        }
        
        // Draw the return to menu button
        SDL_RenderTexture(renderer, m_returnToMenuTexture, nullptr, &button);
    }
    

    SDL_RenderPresent(renderer);
}

void Game::update(float deltaTime)
{
    board.updateAnimation(deltaTime);
    
    if (!m_gameOver && board.isAnimationDone() && (board.isCheckmate() || board.isStalemate()))
    {
        m_gameOver = true;
        displayEndGameMessage();
        return;
    }
    
    if (m_moveJustFinished)
    {
        m_moveJustFinished = false;
        return;
    }
        // In the update method where Stockfish is used, modify the timeout check
    if (board.getCurrentTurn() == Color::Black && board.isAnimationDone() && !m_promotionInProgress && !m_gameOver)
    {
        if (stockfishInitInProgress) {
            // Don't make a move yet, wait for initialization to complete
            return;
        }
    
        try
        {
            if (useStockfish && stockfishInitialized) {
                std::string currentPosition = boardToFEN(board);
    
                // If we've already started analyzing this position, check if result is ready
                if (stockfishMovePending) {
                    // Check if the future is ready without blocking
                    auto status = stockfishFuture.wait_for(std::chrono::milliseconds(100));
                    
                    // Add timeout check for Stockfish calculation
                    static Uint32 stockfishStartTime = SDL_GetTicks();
                    if (status == std::future_status::ready) {
                        try {
                            // Move calculation finished, get the result
                            lastAnalyzedMove = stockfishFuture.get();
                            lastAnalyzedPosition = currentPosition;
                            stockfishMovePending = false;
                            stockfishStartTime = 0;
                            
                            // Make the move
                            auto [fromRow, fromCol, toRow, toCol] = lastAnalyzedMove;
                            
                            if (fromRow != -1 && fromCol != -1 && toRow != -1 && toCol != -1) {
                                board.movePiece(fromRow, fromCol, toRow, toCol);
                                m_moveJustFinished = true;
                            } else {
                                m_gameOver = true;
                                displayEndGameMessage();
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "Exception getting Stockfish move: " << e.what() << std::endl;
                            stockfishMovePending = false;
                            
                            // Fall back to built-in AI
                            std::cout << "Falling back to built-in AI due to Stockfish error" << std::endl;
                            AI ai(Color::Black, 2);
                            auto move = ai.getBestMove(board);
                            auto [fromRow, fromCol, toRow, toCol] = move;
                            
                            if (fromRow != -1) {
                                board.movePiece(fromRow, fromCol, toRow, toCol);
                                m_moveJustFinished = true;
                            } else {
                                m_gameOver = true;
                                displayEndGameMessage();
                            }
                        }
                    } else if (SDL_GetTicks() - stockfishStartTime > 5000) { // 5 second timeout (reduced from 30s)
                        // Stockfish is taking too long or crashed - use quick fallback
                        std::cerr << "Stockfish calculation timeout, falling back to built-in AI" << std::endl;
                        stockfishMovePending = false;
                        
                        // Kill the hanging Stockfish process if possible
                        try {
                            stockfish.stopEngine();// Try to stop Stockfish gracefully
                        } catch (...) {
                            // Ignore errors when trying to stop Stockfish
                        }
                        
                        // Use the built-in AI instead
                        AI ai(Color::Black, 2);
                        auto move = ai.getBestMove(board);
                        auto [fromRow, fromCol, toRow, toCol] = move;
                        
                        if (fromRow != -1) {
                            board.movePiece(fromRow, fromCol, toRow, toCol);
                            m_moveJustFinished = true;
                        } else {
                            m_gameOver = true;
                            displayEndGameMessage();
                        }
                    }
                    // Still calculating and within timeout, don't do anything
                    return;
                }
                
                // If this is a new position and no calculation is pending, start a new one
                if (currentPosition != lastAnalyzedPosition && !stockfishMovePending) {
                    // Start async calculation
                    stockfishMovePending = true;
                    static Uint32 stockfishStartTime = SDL_GetTicks();
                    try {
                        stockfishFuture = std::async(std::launch::async, [this, currentPosition]() {
                            if (!stockfish.ensureEngineRunning()) {
                                std::cerr << "Failed to ensure Stockfish is running, falling back to built-in AI" << std::endl;
                                // Fall back to built-in AI
                                AI ai(Color::Black, 2);
                                return ai.getBestMove(board);
                            }
                            return stockfish.getBestMove(board, 1000);
                        });
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to start Stockfish calculation: " << e.what() << std::endl;
                        stockfishMovePending = false;
                        useStockfish = false;  // Disable Stockfish after failure
                        
                        // Use built-in AI immediately
                        AI ai(Color::Black, 2);
                        auto move = ai.getBestMove(board);
                        auto [fromRow, fromCol, toRow, toCol] = move;
                        if (fromRow != -1) {
                            board.movePiece(fromRow, fromCol, toRow, toCol);
                            m_moveJustFinished = true;
                        }
                    }
                    return;
                }
                
                // Use cached result if position hasn't changed
                if (currentPosition == lastAnalyzedPosition && !stockfishMovePending) {
                    auto [fromRow, fromCol, toRow, toCol] = lastAnalyzedMove;
                    if (fromRow != -1 && fromCol != -1 && toRow != -1 && toCol != -1) {
                        board.movePiece(fromRow, fromCol, toRow, toCol);
                        m_moveJustFinished = true;
                    } else {
                        m_gameOver = true;
                        displayEndGameMessage();
                    }
                }
            } else {
                // Use original AI
                static AI ai(Color::Black, 2);
                auto move = ai.getBestMove(board);
                
                auto [fromRow, fromCol, toRow, toCol] = move;
                
                if (fromRow != -1) {
                    board.movePiece(fromRow, fromCol, toRow, toCol);
                    m_moveJustFinished = true;
                } else {
                    m_gameOver = true;
                    displayEndGameMessage();
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "AI move error: " << e.what() << std::endl;
            m_gameOver = true;
            displayEndGameMessage();
        }
    }
}

void Game::renderMainMenu()
{
    // Clear background with a chess-themed gradient
    SDL_SetRenderDrawColor(renderer, 50, 50, 80, 255);
    SDL_RenderClear(renderer);
    
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    
    // Draw title
    int titleWidth = 300; 
    int titleHeight = 60;
    SDL_FRect titleRect = {(float)(w - titleWidth) / 2, 80.0f, (float)titleWidth, (float)titleHeight};
    SDL_RenderTexture(renderer, m_titleTexture, nullptr, &titleRect);
    
    // Draw menu buttons
    int btnWidth = 250;
    int btnHeight = 50;
    int btnSpacing = 20;
    int startY = 200;
    
    // Define button rectangles
    menuButtons[0] = {
        (float)(w - btnWidth) / 2, 
        (float)startY, 
        (float)btnWidth, 
        (float)btnHeight
    };
    
    menuButtons[1] = {
        (float)(w - btnWidth) / 2, 
        (float)(startY + btnHeight + btnSpacing), 
        (float)btnWidth, 
        (float)btnHeight
    };
    
    menuButtons[2] = {
        (float)(w - btnWidth) / 2, 
        (float)(startY + (btnHeight + btnSpacing) * 2), 
        (float)btnWidth, 
        (float)btnHeight
    };
    
    // Get mouse position for hover effect
    float mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    
    // Render each button with hover effect
    SDL_Texture* buttonTextures[3] = {
        m_stockfishButtonTexture, 
        m_minimaxButtonTexture, 
        m_exitButtonTexture
    };
    
    for (int i = 0; i < 3; i++) {
        bool hovered = (mouseX >= menuButtons[i].x && mouseX <= menuButtons[i].x + menuButtons[i].w &&
                      mouseY >= menuButtons[i].y && mouseY <= menuButtons[i].y + menuButtons[i].h);
        
        // Draw button background slightly larger if hovered
        if (hovered) {
            // Create a slightly larger rectangle for hover effect
            SDL_FRect hoverRect = menuButtons[i];
            hoverRect.x -= 2;
            hoverRect.y -= 2;
            hoverRect.w += 4;
            hoverRect.h += 4;
            
            // Draw a glow effect behind the button
            SDL_SetRenderDrawColor(renderer, 100, 150, 255, 128);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(renderer, &hoverRect);
        }
        
        // Draw the button image
        SDL_RenderTexture(renderer, buttonTextures[i], nullptr, &menuButtons[i]);
    }
    
    SDL_RenderPresent(renderer);
}

void Game::handleMainMenuClick(int x, int y)
{
    // Check if any button was clicked
    for (int i = 0; i < 3; i++) {
        if (x >= menuButtons[i].x && x <= menuButtons[i].x + menuButtons[i].w &&
            y >= menuButtons[i].y && y <= menuButtons[i].y + menuButtons[i].h) {
            
            switch (i) {
                case 0: // Play with Stockfish
                    startGameWithStockfish();
                    break;
                    
                case 1: // Play with Minimax AI
                    startGameWithMinimaxAI();
                    break;
                    
                case 2: // Exit
                    running = false;
                    break;
            }
            break;
        }
    }
}

void Game::startGameWithStockfish()
{
    m_gameState = UIState::Playing;
    useStockfish = true;
    stockfishInitInProgress = true;
    
    #ifdef _WIN32
    const std::string stockfishPath = "stockfish.exe";
    #else
    const std::string stockfishPath = "./stockfish";
    #endif
    
    stockfishInitThread = std::thread([this, stockfishPath]() {
        try {
            if (stockfish.initialize(stockfishPath)) {
                std::cout << "Successfully initialized Stockfish!" << std::endl;
                useStockfish = true;
                stockfishInitialized = true;
            } else {
                std::cout << "Failed to initialize Stockfish, falling back to built-in AI" << std::endl;
                useStockfish = false;
                stockfishInitialized = false;
            }
        } catch (const std::exception& e) {
            std::cout << "Exception during Stockfish initialization: " << e.what() << std::endl;
            useStockfish = false;
            stockfishInitialized = false;
        } catch (...) {
            std::cout << "Unknown exception during Stockfish initialization" << std::endl;
            useStockfish = false;
            stockfishInitialized = false;
        }
        stockfishInitInProgress = false;
    });
    
    stockfishInitThread.detach();
    board.initialize();
}

void Game::startGameWithMinimaxAI()
{
    m_gameState = UIState::Playing;
    useStockfish = false;
    stockfishInitialized = false;
    stockfishInitInProgress = false;
    board.initialize();
}
bool Game::initialize()
{
    // Set up initial game state to show menu
    m_gameState = UIState::MainMenu;
    useStockfish = false;
    stockfishInitInProgress = false;
    stockfishInitialized = false;


    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Chess Game", 600, 600, 0);
    if (!window)
    {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    m_titleTexture = loadTexture("images/Title.bmp");
    m_stockfishButtonTexture = loadTexture("images/PlayvsStockfish(Strong).bmp");
    m_minimaxButtonTexture = loadTexture("images/PlayvsMinimax(Medium).bmp");
    m_exitButtonTexture = loadTexture("images/ExitGame.bmp");
    m_returnToMenuTexture = loadTexture("images/ReturnToMenu.bmp");
    m_whiteWinsTexture = loadTexture("images/Whitewin.bmp");
    m_blackWinsTexture = loadTexture("images/Blackwin.bmp");
    m_drawTexture = loadTexture("images/Draw.bmp");
    
   
    if (!m_titleTexture || !m_stockfishButtonTexture || !m_minimaxButtonTexture || 
        !m_exitButtonTexture || !m_returnToMenuTexture || !m_whiteWinsTexture || 
        !m_blackWinsTexture || !m_drawTexture) {
        std::cerr << "Failed to load UI textures!" << std::endl;
        return false;
    }
    // Load piece textures
    pieceTextures["wK"] = loadTexture("images/wK.bmp");
    pieceTextures["wQ"] = loadTexture("images/wQ.bmp");
    pieceTextures["wB"] = loadTexture("images/wB.bmp");
    pieceTextures["wN"] = loadTexture("images/wN.bmp");
    pieceTextures["wR"] = loadTexture("images/wR.bmp");
    pieceTextures["wp"] = loadTexture("images/wp.bmp");

    pieceTextures["bK"] = loadTexture("images/bK.bmp");
    pieceTextures["bQ"] = loadTexture("images/bQ.bmp");
    pieceTextures["bB"] = loadTexture("images/bB.bmp");
    pieceTextures["bN"] = loadTexture("images/bN.bmp");
    pieceTextures["bR"] = loadTexture("images/bR.bmp");
    pieceTextures["bp"] = loadTexture("images/bp.bmp");

    // Check if all textures loaded successfully
    for (const auto &[key, texture] : pieceTextures)
    {
        if (!texture)
        {
            std::cerr << "Failed to load texture: " << key << std::endl;
            return false;
        }
    }

    // Initialize the chess board
    board.initialize();
    
    running = true;
    lastFrameTime = SDL_GetTicks();
    return true;
}

SDL_Texture *Game::loadTexture(const char *path) const
{
    SDL_Surface *surface = SDL_LoadBMP(path);
    if (!surface)
    {
        std::cerr << "Failed to load image: " << path << std::endl;
        return nullptr;
    }

    SDL_SetSurfaceColorKey(surface, true, SDL_MapSurfaceRGB(surface, 255, 0, 255));
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface); 

    return texture;
}
SDL_Texture *Game::getPieceTexture(PieceType type, Color color) const
{
    std::string key;

    
    key += (color == Color::White) ? 'w' : 'b';

    
    switch (type)
    {
    case PieceType::King:
        key += "K";
        break;
    case PieceType::Queen:
        key += "Q";
        break;
    case PieceType::Bishop:
        key += "B";
        break;
    case PieceType::Knight:
        key += "N";
        break;
    case PieceType::Rook:
        key += "R";
        break;
    case PieceType::Pawn:
        key += "p";
        break;
    }

    auto it = pieceTextures.find(key);
    if (it != pieceTextures.end())
    {
        return it->second;
    }

    return nullptr;
}

void Game::cleanup()
{
    
    for (auto &[key, texture] : pieceTextures)
    {
        if (texture)
        {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }

    pieceTextures.clear();

    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    
    SDL_Quit();
}

void Game::handleEvents()
{
    
    static bool lastAnimatingState = false;
    bool currentlyAnimating = !board.isAnimationDone();

    if (lastAnimatingState != currentlyAnimating)
    {
        
        lastAnimatingState = currentlyAnimating;
    }

    
    if (currentlyAnimating)
    { 
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
                
            }
        }
        return;
    }

    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            running = false;
            
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                
                board.handleClick(event.button.x, event.button.y);
            }
            break;
        }
    }
}

void Game::startPromotion(int row, int col, Color color)
{
    m_promotionInProgress = true;
    m_promotionRow = row;
    m_promotionCol = col;
    m_promotionColor = color;
}

void Game::handlePromotion(PieceType type)
{
    
    Piece *newPiece = nullptr;
    switch (type)
    {
    case PieceType::Queen:
        newPiece = new Queen(m_promotionColor, m_promotionRow, m_promotionCol);
        break;
    case PieceType::Rook:
        newPiece = new Rook(m_promotionColor, m_promotionRow, m_promotionCol);
        break;
    case PieceType::Bishop:
        newPiece = new Bishop(m_promotionColor, m_promotionRow, m_promotionCol);
        break;
    case PieceType::Knight:
        newPiece = new Knight(m_promotionColor, m_promotionRow, m_promotionCol);
        break;
    default:
        newPiece = new Queen(m_promotionColor, m_promotionRow, m_promotionCol);
        break;
    }

    
    delete board.getPiece(m_promotionRow, m_promotionCol);
    board.placePiece(newPiece, m_promotionRow, m_promotionCol);

    m_promotionInProgress = false;
}