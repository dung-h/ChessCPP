#ifndef AI_H
#define AI_H

#include "Board.h"
#include "Piece.h"
#include <vector>
#include <tuple>

struct Move {
    int fromRow, fromCol;
    int toRow, toCol;
    int score = 0;

    bool operator==(const Move& other) const {
        return fromRow == other.fromRow && fromCol == other.fromCol &&
               toRow == other.toRow && toCol == other.toCol;
    }
    bool operator<(const Move& other) const {
        return score > other.score;
    }
};

class AI {
public:
    AI(Color aiColor, int maxDepth);
    std::tuple<int, int, int, int> getBestMove(const Board& board);

private:
    Color aiColor;
    int maxDepth;
    Color opponentColor;

    Move minimaxRoot(const Board& board, int depth);
    int minimax(Board board, int depth, int alpha, int beta, bool isMaximizingPlayer);

    int evaluateBoard(const Board& board);
    int getPieceValue(PieceType type);

    std::vector<Move> generateLegalMoves(const Board& board, Color color);

    bool applyMoveSimulation(Board& board, const Move& move);
};

#endif