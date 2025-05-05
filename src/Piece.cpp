#include "Piece.h"
#include "Board.h"
#include <cmath>
#include "Game.h"
#include <iostream>


Piece::Piece(PieceType type, Color color, int row, int col)
    : m_type(type), m_color(color), m_row(row), m_col(col), m_hasMoved(false)
{

}

PieceType Piece::getType() const
{
    return m_type;
}

Color Piece::getColor() const
{
    return m_color;
}

int Piece::getRow() const
{
    return m_row;
}

int Piece::getCol() const
{
    return m_col;
}

bool Piece::isValidPosition(int row, int col) const
{
    return row >= 0 && row < 8 && col >= 0 && col < 8;
}

void Piece::setPosition(int row, int col)
{
    m_row = row;
    m_col = col;
    m_hasMoved = true;
}

bool Piece::hasMoved() const
{
    return m_hasMoved;
}


bool Piece::isPathClear(int toRow, int toCol, const Board &board) const
{
    int rowDir = (toRow > m_row) ? 1 : (toRow < m_row) ? -1 : 0;
    int colDir = (toCol > m_col) ? 1 : (toCol < m_col) ? -1 : 0;

    int row = m_row + rowDir;
    int col = m_col + colDir;

    while (row != toRow || col != toCol)
    {
        if (board.getPiece(row, col) != nullptr)
        {
            return false;
        }
        row += rowDir;
        col += colDir;
    }

    return true;
}


bool Piece::canCapture(int row, int col, const Board &board) const
{
    Piece *piece = board.getPiece(row, col);
    return piece && piece->getColor() != m_color;
}

void Piece::render(SDL_Renderer* renderer, const Game& game, int squareSize) const
{
    SDL_Texture* texture = game.getPieceTexture(m_type, m_color);
    if (texture) {
        SDL_FRect destRect = {
            (float)(m_col * squareSize),
            (float)((7 - m_row) * squareSize),
            (float)squareSize,
            (float)squareSize
        };
        SDL_RenderTexture(renderer, texture, nullptr, &destRect);
    }
}


King::King(Color color, int row, int col) : Piece(PieceType::King, color, row, col) {}

bool King::canMoveTo(int toRow, int toCol, const Board &board) const
{

    int rowDiff = std::abs(toRow - m_row);
    int colDiff = std::abs(toCol - m_col);


    if (rowDiff <= 1 && colDiff <= 1)
    {
        if (board.getPiece(toRow, toCol) == nullptr || canCapture(toRow, toCol, board))
        {
            return true;
        }
    }


    if (!m_hasMoved && m_row == toRow && std::abs(toCol - m_col) == 2)
    {

        if (board.isSquareUnderAttack(m_row, m_col, (m_color == Color::White) ? Color::Black : Color::White))
        {
            return false;
        }


        if (toCol > m_col)
        {
            Piece *rook = board.getPiece(m_row, 7);
            if (rook && rook->getType() == PieceType::Rook && !rook->hasMoved())
            {

                if (board.getPiece(m_row, m_col + 1) == nullptr &&
                    board.getPiece(m_row, m_col + 2) == nullptr)
                {

                    if (!board.isSquareUnderAttack(m_row, m_col + 1, (m_color == Color::White) ? Color::Black : Color::White))
                    {
                        return true;
                    }
                }
            }
        }

        else
        {
            Piece *rook = board.getPiece(m_row, 0);
            if (rook && rook->getType() == PieceType::Rook && !rook->hasMoved())
            {

                if (board.getPiece(m_row, m_col - 1) == nullptr &&
                    board.getPiece(m_row, m_col - 2) == nullptr &&
                    board.getPiece(m_row, m_col - 3) == nullptr)
                {

                    if (!board.isSquareUnderAttack(m_row, m_col - 1, (m_color == Color::White) ? Color::Black : Color::White))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}


Queen::Queen(Color color, int row, int col) : Piece(PieceType::Queen, color, row, col) {}

bool Queen::canMoveTo(int toRow, int toCol, const Board &board) const
{
    int rowDiff = std::abs(toRow - m_row);
    int colDiff = std::abs(toCol - m_col);


    if ((rowDiff == 0 || colDiff == 0) || (rowDiff == colDiff))
    {
        if (isPathClear(toRow, toCol, board))
        {
            if (board.getPiece(toRow, toCol) == nullptr || canCapture(toRow, toCol, board))
            {
                return true;
            }
        }
    }
    return false;
}


Rook::Rook(Color color, int row, int col) : Piece(PieceType::Rook, color, row, col) {}

bool Rook::canMoveTo(int toRow, int toCol, const Board &board) const
{

    if (m_row == toRow || m_col == toCol)
    {
        if (isPathClear(toRow, toCol, board))
        {
            if (board.getPiece(toRow, toCol) == nullptr || canCapture(toRow, toCol, board))
            {
                return true;
            }
        }
    }
    return false;
}


Bishop::Bishop(Color color, int row, int col) : Piece(PieceType::Bishop, color, row, col) {}

bool Bishop::canMoveTo(int toRow, int toCol, const Board &board) const
{
    int rowDiff = std::abs(toRow - m_row);
    int colDiff = std::abs(toCol - m_col);


    if (rowDiff == colDiff)
    {
        if (isPathClear(toRow, toCol, board))
        {
            if (board.getPiece(toRow, toCol) == nullptr || canCapture(toRow, toCol, board))
            {
                return true;
            }
        }
    }
    return false;
}


Knight::Knight(Color color, int row, int col) : Piece(PieceType::Knight, color, row, col) {}

bool Knight::canMoveTo(int toRow, int toCol, const Board &board) const
{
    int rowDiff = std::abs(toRow - m_row);
    int colDiff = std::abs(toCol - m_col);


    if ((rowDiff == 2 && colDiff == 1) || (rowDiff == 1 && colDiff == 2))
    {
        if (board.getPiece(toRow, toCol) == nullptr || canCapture(toRow, toCol, board))
        {
            return true;
        }
    }
    return false;
}


Pawn::Pawn(Color color, int row, int col) : Piece(PieceType::Pawn, color, row, col) {}



bool Pawn::canMoveTo(int toRow, int toCol, const Board &board) const
{
    try {

        if (toRow < 0 || toRow > 7 || toCol < 0 || toCol > 7) {
            return false;
        }

        int direction = (m_color == Color::White) ? 1 : -1;
        int rowDiff = toRow - m_row;
        int colDiff = std::abs(toCol - m_col);

        Piece* targetPiece = board.getPiece(toRow, toCol);




        if ((m_color == Color::White && rowDiff <= 0) ||
            (m_color == Color::Black && rowDiff >= 0)) {

            return false;
        }


        if (colDiff == 0)
        {

            if (std::abs(rowDiff) == 1) {

                if (targetPiece != nullptr) {
                    return false;
                }

                return true;
            }


            if (!m_hasMoved && std::abs(rowDiff) == 2) {

                int middleRow = m_row + direction;


                if (board.getPiece(middleRow, m_col) != nullptr ||
                    targetPiece != nullptr) {
                    return false;
                }


                return true;
            }
        }

        else if (colDiff == 1 && std::abs(rowDiff) == 1) {

            if (targetPiece && targetPiece->getColor() != m_color) {

                return true;
            }


            if (m_row == (m_color == Color::White ? 4 : 3)) {
                Piece* lastPawn = board.getLastMovedPawn();
                int enPassantCol = board.getEnPassantCol();

                if (lastPawn && lastPawn->getType() == PieceType::Pawn &&
                    lastPawn->getColor() != m_color && enPassantCol == toCol) {

                    return true;
                }
            }
        }

        return false;
    }
    catch (const std::exception& e) {

        return false;
    }
    catch (...) {

        return false;
    }
}