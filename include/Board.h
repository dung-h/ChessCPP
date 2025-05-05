#pragma once
#include <algorithm>
#include <vector>
#include <memory>
#include <SDL3/SDL.h>
#include "Piece.h"
#include <iostream>
#include <tuple>
#include <unordered_map>

enum class GameState {
    Active,
    Check,
    Checkmate,
    Stalemate
};
extern std::atomic<bool> g_validMovesComputed;
class Board {
    friend class AI;
private:
    static const int SQUARE_SIZE = 75;
    std::vector<std::vector<Piece*>> m_squares;
    Color m_currentTurn;
    GameState m_gameState;
    std::unordered_map<Piece*, std::vector<SDL_Point>> m_legalMoveCache;
    int m_selectedRow;
    int m_selectedCol;
    bool m_pieceSelected;
    std::vector<SDL_Point> m_validMoves;

    int m_whiteKingRow;
    int m_whiteKingCol;
    int m_blackKingRow;
    int m_blackKingCol;

    Piece* m_lastMovedPawn;
    int m_enPassantCol;

    Piece* m_movingPiece;
    float m_animStartX;
    float m_animStartY;
    float m_animEndX;
    float m_animEndY;
    float m_animProgress;
    bool m_animating;
    int m_moveFromRow, m_moveFromCol;
    int m_moveToRow, m_moveToCol;
    Piece* m_capturedPiece;
    static const std::vector<SDL_Point> knightMoves[64];
public:
    bool isInCheck(Color color) const;
    Board(const Board& other);
    Board& operator=(const Board& other);
    bool hasLegalMoves(Color color) const;
    void updateGameState();
    void calculateValidMoves(int row, int col);
    bool movePieceForSimulation(int fromRow, int fromCol, int toRow, int toCol);
    void updateLegalMoveCache(Color color);

    std::vector<SDL_Point> fastGenerateMoves(Piece* piece) const;

    std::vector<SDL_Point> m_candidateMoves;
    Board();
    ~Board();
    std::string getPositionKey() const;
    void initialize();
    Piece* getPiece(int row, int col) const;
    bool movePiece(int fromRow, int fromCol, int toRow, int toCol);
    void handleClick(int x, int y);
    void render(SDL_Renderer* renderer, const Game& game) const;
    void updateAnimation(float deltaTime);
    bool isPieceSelected() const { return m_pieceSelected; }
    Color getCurrentTurn() const { return m_currentTurn; }
    GameState getGameState() const { return m_gameState; }
    bool isAnimating() const { return m_animating; }
    void completeMoveAfterAnimation();
    bool isSquareUnderAttack(int row, int col, Color attackingColor) const;
    void checkForPromotion();
    void calculateValidMovesAsync(int row, int col);
    bool isAnimationDone() const {

        static float lastProgress = 0.0f;
        static bool lastAnimState = false;

        if (m_animating != lastAnimState ||
            (m_animating && std::abs(m_animProgress - lastProgress) > 0.05f)) {

            lastProgress = m_animProgress;
            lastAnimState = m_animating;
        }

        return !m_animating;
    }
    std::vector<SDL_Point> generateCandidateMoves(Piece* piece) const;
    void screenToBoard(int screenX, int screenY, int& boardRow, int& boardCol) const;
    void placePiece(Piece* piece, int row, int col);
    Piece* getLastMovedPawn() const;
    int getEnPassantCol() const;
    bool isValidMovesComputed() const;
    std::vector<std::tuple<int, int, int, int>> getPins(Color color) const;
    std::vector<std::tuple<int, int, int, int>> getChecks(Color color) const;
    bool alignsWithPin(int fromRow, int fromCol, int toRow, int toCol, std::pair<int, int> pinDir) const;
    uint64_t getZobristKey() const;
    bool isCheckmate() const;
    bool isStalemate() const;
    bool isBlockingCheck(int row, int col, int kingRow, int kingCol,int attackerRow, int attackerCol) const;
    void updateKingPosition(Color color, int row, int col);
    void setEnPassantTarget(Piece* pawn, int col);
    void clearEnPassantTarget();
    void setCurrentTurn(Color turn){
        m_currentTurn = turn;
    }
    void printForDebug() const {
        std::cout << "    ";
        for(int c=0; c<8; ++c) std::cout << c << " ";
        std::cout << std::endl;
        std::cout << "    -----------------" << std::endl;
        for (int r = 7; r >= 0; --r) {
            std::cout << r << " | ";
            for (int c = 0; c < 8; ++c) {
                Piece* p = m_squares[r][c];
                if (!p) {
                    std::cout << ". ";
                } else {
                    char typeChar;
                    switch (p->getType()) {
                        case PieceType::Pawn: typeChar = 'p'; break;
                        case PieceType::Knight: typeChar = 'n'; break;
                        case PieceType::Bishop: typeChar = 'b'; break;
                        case PieceType::Rook: typeChar = 'r'; break;
                        case PieceType::Queen: typeChar = 'q'; break;
                        case PieceType::King: typeChar = 'k'; break;
                        default: typeChar = '?'; break;
                    }
                    std::cout << (p->getColor() == Color::White ? (char)toupper(typeChar) : typeChar) << " ";
                }
            }
            std::cout << "|" << std::endl;
        }
          std::cout << "    -----------------" << std::endl;
          std::cout << "    Turn: " << (m_currentTurn == Color::White ? "White" : "Black") << std::endl;
          std::cout << "    EP Col: " << m_enPassantCol << std::endl;
          std::cout << "    WK: (" << m_whiteKingRow << "," << m_whiteKingCol << ") BK: (" << m_blackKingRow << "," << m_blackKingCol << ")" << std::endl;

    }
    void setSquare(int row, int col, Piece* piece);
};