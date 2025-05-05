#include "AI.h"
#include <chrono>
#include <vector>
#include <limits>
#include <algorithm>
#include <stdexcept>

AI::AI(Color aiColor, int maxDepth) : aiColor(aiColor), maxDepth(maxDepth)
{
    opponentColor = (aiColor == Color::White) ? Color::Black : Color::White;
}

int AI::getPieceValue(PieceType type)
{
    switch (type)
    {
    case PieceType::Pawn:
        return 100;
    case PieceType::Knight:
        return 300;
    case PieceType::Bishop:
        return 310;
    case PieceType::Rook:
        return 500;
    case PieceType::Queen:
        return 900;
    case PieceType::King:
        return 20000;
    default:
        return 0;
    }
}

int AI::evaluateBoard(const Board &board)
{
    int materialScore = 0;
    try
    {
        for (int r = 0; r < 8; ++r)
        {
            for (int c = 0; c < 8; ++c)
            {
                Piece *piece = board.getPiece(r, c);
                if (piece)
                {
                    int value = getPieceValue(piece->getType());
                    if (piece->getColor() == aiColor)
                    {
                        materialScore += value;
                    }
                    else
                    {
                        materialScore -= value;
                    }
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "!!! EXCEPTION inside evaluateBoard material loop: " << e.what() << std::endl;
        return 0;
    }
    catch (...)
    {
        std::cerr << "!!! UNKNOWN EXCEPTION inside evaluateBoard material loop" << std::endl;
        return 0;
    }

    Board boardCopy = board;
    Color playerWhoJustMoved = board.getCurrentTurn();
    Color playerWhoseTurnIsNext = (playerWhoJustMoved == Color::White) ? Color::Black : Color::White;
    boardCopy.setCurrentTurn(playerWhoseTurnIsNext);
    boardCopy.updateGameState();
    GameState finalState = boardCopy.getGameState();

    if (finalState == GameState::Checkmate)
    {
        int score = (playerWhoseTurnIsNext == aiColor) ? -30000 : 30000;
        return score;
    }
    else if (finalState == GameState::Stalemate)
    {
        return 0;
    }

    return materialScore;
}

std::vector<Move> AI::generateLegalMoves(const Board &board, Color color)
{
    std::vector<Move> legalMoves;
    for (int r = 0; r < 8; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            Piece *piece = board.getPiece(r, c);
            if (piece && piece->getColor() == color)
            {
                std::vector<SDL_Point> candidates = board.fastGenerateMoves(piece);

                for (const auto &pt : candidates)
                {
                    if (pt.y < 0 || pt.y >= 8 || pt.x < 0 || pt.x >= 8)
                        continue;

                    if (!piece->canMoveTo(pt.y, pt.x, board))
                        continue;

                    Board tempBoard = board;
                    if (tempBoard.movePieceForSimulation(r, c, pt.y, pt.x))
                    {
                        legalMoves.push_back({r, c, pt.y, pt.x});
                    }
                }
            }
        }
    }
    return legalMoves;
}

bool AI::applyMoveSimulation(Board &board, const Move &move)
{
    int toRow = move.toRow;
    int toCol = move.toCol;
    int fromRow = move.fromRow;
    int fromCol = move.fromCol;

    Piece *pieceToMove = board.getPiece(fromRow, fromCol);
    if (!pieceToMove)
    {
        std::cerr << " applyMoveSimulation ERROR: No piece found at start (" << fromRow << "," << fromCol << ")!" << std::endl;
        return false;
    }

    Color movingColor = pieceToMove->getColor();
    PieceType movingType = pieceToMove->getType();

    Piece *targetPiece = board.getPiece(toRow, toCol);

    if (movingType == PieceType::King && std::abs(toCol - fromCol) == 2)
    {
        int rookFromCol = (toCol > fromCol) ? 7 : 0;
        int rookToCol = (toCol > fromCol) ? toCol - 1 : toCol + 1;
        Piece *rook = board.getPiece(fromRow, rookFromCol);
        if (rook && rook->getType() == PieceType::Rook)
        {
            board.placePiece(rook, fromRow, rookToCol);
            if (board.m_squares[fromRow][rookFromCol] == rook)
            {
                board.m_squares[fromRow][rookFromCol] = nullptr;
            }
            else
            {
                board.m_squares[fromRow][rookFromCol] = nullptr;
            }
        }
        else
        {
            std::cerr << " applyMoveSimulation WARNING: Castling detected but no valid Rook found at (" << fromRow << "," << rookFromCol << ")" << std::endl;
        }
    }

    Piece *capturedPieceForDeletion = nullptr;
    int capturedPieceOriginalRow = -1, capturedPieceOriginalCol = -1;
    bool isEnPassant = (movingType == PieceType::Pawn && fromCol != toCol && !targetPiece);

    if (isEnPassant)
    {
        capturedPieceOriginalRow = fromRow;
        capturedPieceOriginalCol = toCol;
        Piece *enPassantVictim = board.getPiece(capturedPieceOriginalRow, capturedPieceOriginalCol);

        if (enPassantVictim && enPassantVictim->getType() == PieceType::Pawn)
        {
            capturedPieceForDeletion = enPassantVictim;
            if (board.m_squares[capturedPieceOriginalRow][capturedPieceOriginalCol] == enPassantVictim)
            {
                board.m_squares[capturedPieceOriginalRow][capturedPieceOriginalCol] = nullptr;
            }
            else
            {
                board.m_squares[capturedPieceOriginalRow][capturedPieceOriginalCol] = nullptr;
            }
        }
        else
        {
            std::cerr << " applyMoveSimulation ERROR: En passant indicated but no valid victim Pawn found at (" << capturedPieceOriginalRow << "," << capturedPieceOriginalCol << ")" << std::endl;
            capturedPieceForDeletion = nullptr;
        }
    }
    else if (targetPiece)
    {
        capturedPieceForDeletion = targetPiece;
        capturedPieceOriginalRow = toRow;
        capturedPieceOriginalCol = toCol;
        if (board.m_squares[toRow][toCol] == targetPiece)
        {
            board.m_squares[toRow][toCol] = nullptr;
        }
        else
        {
            board.m_squares[toRow][toCol] = nullptr;
        }
    }

    if (capturedPieceForDeletion)
    {
        delete capturedPieceForDeletion;
        capturedPieceForDeletion = nullptr;
    }

    board.placePiece(pieceToMove, toRow, toCol);

    if (board.m_squares[fromRow][fromCol] == pieceToMove)
    {
        board.m_squares[fromRow][fromCol] = nullptr;
    }
    else
    {
        if (board.m_squares[fromRow][fromCol] != nullptr)
        {
            board.m_squares[fromRow][fromCol] = nullptr;
        }
    }

    Piece *pieceOnBoardAfterMove = board.getPiece(toRow, toCol);
    if (!pieceOnBoardAfterMove)
    {
        std::cerr << " applyMoveSimulation FATAL ERROR: Piece disappeared from target square (" << toRow << "," << toCol << ") immediately after placement!" << std::endl;
        return false;
    }

    PieceType finalTypeOnSquare = pieceOnBoardAfterMove ? pieceOnBoardAfterMove->getType() : movingType;

    if (finalTypeOnSquare == PieceType::King)
    {
        board.updateKingPosition(movingColor, toRow, toCol);
    }

    if (movingType == PieceType::Pawn && std::abs(toRow - fromRow) == 2)
    {
        board.setEnPassantTarget(pieceOnBoardAfterMove, toCol);
    }
    else
    {
        board.clearEnPassantTarget();
    }

    return true;
}

Move AI::minimaxRoot(const Board &board, int depth)
{
    std::vector<Move> legalMoves = generateLegalMoves(board, aiColor);

    if (legalMoves.empty())
    {
        return {-1, -1, -1, -1};
    }

    Move bestMove = legalMoves[0];
    int maxScore = std::numeric_limits<int>::min();

    int alpha = std::numeric_limits<int>::min();
    int beta = std::numeric_limits<int>::max();

    for (const auto &move : legalMoves)
    {
        Board nextBoard = board;
        if (!applyMoveSimulation(nextBoard, move))
        {
            std::cerr << "AI WARNING: Failed to simulate legal move at root!" << std::endl;
            continue;
        }

        int score = minimax(nextBoard, depth - 1, alpha, beta, false);

        if (score > maxScore)
        {
            maxScore = score;
            bestMove = move;
            bestMove.score = score;
        }
        alpha = std::max(alpha, score);
    }

    return bestMove;
}

int AI::minimax(Board board, int depth, int alpha, int beta, bool isMaximizingPlayer)
{
    if (depth == 0)
    {
        return evaluateBoard(board);
    }

    Board boardCopy = board;
    boardCopy.updateGameState();
    GameState currentState = boardCopy.getGameState();
    if (currentState == GameState::Checkmate)
    {
        return (boardCopy.getCurrentTurn() == aiColor) ? -30000 - depth : 30000 + depth;
    }
    if (currentState == GameState::Stalemate)
    {
        return 0;
    }

    Color currentTurnColor = isMaximizingPlayer ? aiColor : opponentColor;
    std::vector<Move> legalMoves = generateLegalMoves(board, currentTurnColor);

    if (legalMoves.empty())
    {
        if (board.isInCheck(currentTurnColor))
        {
            return isMaximizingPlayer ? (-30000 - depth) : (30000 + depth);
        }
        else
        {
            return 0;
        }
    }

    if (isMaximizingPlayer)
    {
        int maxEval = std::numeric_limits<int>::min();
        for (const auto &move : legalMoves)
        {
            Board nextBoard = board;
            applyMoveSimulation(nextBoard, move);
            int eval = minimax(nextBoard, depth - 1, alpha, beta, false);
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha)
            {
                break;
            }
        }
        return maxEval;
    }
    else
    {
        int minEval = std::numeric_limits<int>::max();
        for (const auto &move : legalMoves)
        {
            Board nextBoard = board;
            applyMoveSimulation(nextBoard, move);
            int eval = minimax(nextBoard, depth - 1, alpha, beta, true);
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha)
            {
                break;
            }
        }
        return minEval;
    }
}

std::tuple<int, int, int, int> AI::getBestMove(const Board &board)
{
    try
    {
        auto startTime = std::chrono::steady_clock::now();

        Move bestMove = minimaxRoot(board, maxDepth);

        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        if (bestMove.fromRow == -1)
        {
            Board temp = board;
            temp.updateGameState();
            if (temp.getGameState() == GameState::Checkmate || temp.getGameState() == GameState::Stalemate)
            {
            }
            else
            {
                std::cerr << "AI ERROR: No moves found but game not over?" << std::endl;
            }
            return std::make_tuple(-1, -1, -1, -1);
        }

        return std::make_tuple(bestMove.fromRow, bestMove.fromCol, bestMove.toRow, bestMove.toCol);
    }
    catch (const std::exception &e)
    {
        std::cerr << "AI::getBestMove critical exception: " << e.what() << std::endl;
        return std::make_tuple(-1, -1, -1, -1);
    }
    catch (...)
    {
        std::cerr << "AI::getBestMove unknown exception" << std::endl;
        return std::make_tuple(-1, -1, -1, -1);
    }
}