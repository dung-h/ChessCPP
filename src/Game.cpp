#include "Game.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <future>
#include "Board.h"
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
    window = SDL_CreateWindow("Chess", 800, 800, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        
        exit(1);
    }
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        
        exit(1);
    }
    
}

Game::~Game()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::displayEndGameMessage()
{
    if (m_gameOver && !m_endgameMessageDisplayed)
    {
        if (board.isCheckmate())
        {
            std::string winner = (board.getCurrentTurn() == Color::White) ? "Black" : "White";
            std::cout << "Game over: Checkmate! " << winner << " wins!" << std::endl;
        }
        else if (board.isStalemate())
        {
            std::cout << "Game over: Stalemate! Game is a draw." << std::endl;
        }
        m_endgameMessageDisplayed = true;
        
        
        SDL_Delay(3000); 
        running = false; 
    }
}

void Game::render()
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    board.render(renderer, *this);

    
    if (m_gameOver)
    {
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        SDL_FRect fullscreen = {0.0f, 0.0f, (float)w, (float)h};
        SDL_RenderFillRect(renderer, &fullscreen);

        
        
    }

    SDL_RenderPresent(renderer);
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
                if (m_gameOver)
                {
                    
                    running = false;
                    
                }
                else if (board.isAnimationDone() && !m_gameOver)
                {
                    int x = event.button.x;
                    int y = event.button.y;
                    
                    board.handleClick(x, y);
                }
            }
        }

        float deltaTime = 1.0f / 60.0f;
        try
        {
            update(deltaTime);
            
            
            if (m_gameOver)
            {
                displayEndGameMessage();
            }
        }
        catch (const std::exception &e)
        {
            
        }
        render();

        Uint32 frameTime = SDL_GetTicks() - lastFrameTime;
        if (frameTime < 16)
        {
            SDL_Delay(16 - frameTime);
        }
        lastFrameTime = SDL_GetTicks();
    }
}

void Game::update(float deltaTime)
{
    board.updateAnimation(deltaTime);
    if (m_moveJustFinished)
    {
        
        m_moveJustFinished = false;
        
        if (board.isCheckmate() || board.isStalemate())
        {
            m_gameOver = true;
            
        }
        return;
    }

    if (board.getCurrentTurn() == Color::Black && board.isAnimationDone() && !m_promotionInProgress && !m_gameOver)
    {
        static AI ai(Color::Black, 2);
        
        try
        {
            auto [fromRow, fromCol, toRow, toCol] = ai.getBestMove(board);
            if (fromRow != -1)
            {
                if (fromRow == -1 || fromCol == -1 || toRow == -1 || toCol == -1)
                {
                    
                    m_gameOver = true;
                    return;
                }
                
                board.movePiece(fromRow, fromCol, toRow, toCol);
                m_moveJustFinished = true;
            }
            else
            {
                
                m_gameOver = true; 
            }
        }
        catch (const std::exception &e)
        {
            
            m_gameOver = true; 
        }
    }
}

bool Game::initialize()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        
        return false;
    }

    

    window = SDL_CreateWindow("Chess Game", 600, 600, 0);
    if (!window)
    {
        
        return false;
    }

    
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        
        return false;
    }

    
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

    
    for (const auto &[key, texture] : pieceTextures)
    {
        if (!texture)
        {
            
            return false;
        }
    }

    
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
        
        return nullptr;
    }

    
    SDL_SetSurfaceColorKey(surface, true, SDL_MapSurfaceRGB(surface, 255, 0, 255));

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface); 

    if (!texture)
    {
        
    }

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