#pragma once

#include "SDLIncludes.h"
#include <string>
#include <vector>

class Board;
class Game;

enum class PieceType {
    King, Queen, Rook, Bishop, Knight, Pawn
};

enum class Color {
    White, Black
};

class Piece {
    friend class Board; 
protected:
    PieceType m_type;
    Color m_color;
    int m_row;
    int m_col;
    bool m_hasMoved;

    bool isValidPosition(int row, int col) const;
    bool isPathClear(int toRow, int toCol, const Board& board) const;
    bool canCapture(int row, int col, const Board& board) const;

public:
    Piece(PieceType type, Color color, int row, int col);
    virtual ~Piece() = default;
    
    virtual bool canMoveTo(int toRow, int toCol, const Board& board) const = 0;
    
    PieceType getType() const;
    Color getColor() const;
    int getRow() const;
    int getCol() const;
    bool hasMoved() const;
    void setPosition(int row, int col);
    virtual void render(SDL_Renderer* renderer, const Game& game, int squareSize) const;
    // For rendering
    void setPositionWithoutMovedFlag(int row, int col) {
        m_row = row;
        m_col = col;
        
    }
  
};

class King : public Piece {
public:
    King(Color color, int row, int col);
    bool canMoveTo(int toRow, int toCol, const Board& board) const override;
};

class Queen : public Piece {
public:
    Queen(Color color, int row, int col);
    bool canMoveTo(int toRow, int toCol, const Board& board) const override;
};

class Rook : public Piece {
public:
    Rook(Color color, int row, int col);
    bool canMoveTo(int toRow, int toCol, const Board& board) const override;
};

class Bishop : public Piece {
public:
    Bishop(Color color, int row, int col);
    bool canMoveTo(int toRow, int toCol, const Board& board) const override;
};

class Knight : public Piece {
public:
    Knight(Color color, int row, int col);
    bool canMoveTo(int toRow, int toCol, const Board& board) const override;
};

class Pawn : public Piece {
public:
    Pawn(Color color, int row, int col);
    bool canMoveTo(int toRow, int toCol, const Board& board) const override;
};
