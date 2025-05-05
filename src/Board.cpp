#include "Board.h"
#include "Game.h"
#include <unordered_map>
#include <vector>

#include <future>
#include <unordered_set>
#include <chrono>
#include <mutex>
#include <sstream>
#include <string>
static std::mutex g_validMovesMutex;

#include <future>
#include <atomic>
#include "Piece.h"

#include "Zobrist.h"

std::atomic<bool> g_validMovesReady(false);
std::atomic<bool> g_validMovesComputed(false);

inline bool operator==(const SDL_Point &a, const SDL_Point &b)
{
    return a.x == b.x && a.y == b.y;
}

Piece* findPieceInCopy(const std::vector<std::vector<Piece*>>& squares, const Piece* originalPiece) {
    if (!originalPiece) return nullptr;
    int r = originalPiece->getRow();
    int c = originalPiece->getCol();
    if (r >= 0 && r < 8 && c >= 0 && c < 8) {

        return squares[r][c];
    }
    return nullptr;
}

Board::Board(const Board& other) :
    m_currentTurn(other.m_currentTurn),
    m_gameState(other.m_gameState),
    m_selectedRow(other.m_selectedRow),
    m_selectedCol(other.m_selectedCol),
    m_pieceSelected(other.m_pieceSelected),
    m_whiteKingRow(other.m_whiteKingRow),
    m_whiteKingCol(other.m_whiteKingCol),
    m_blackKingRow(other.m_blackKingRow),
    m_blackKingCol(other.m_blackKingCol),
    m_lastMovedPawn(nullptr),
    m_enPassantCol(other.m_enPassantCol),
    m_movingPiece(nullptr),
    m_animProgress(0.0f),
    m_animating(false),
    m_moveFromRow(other.m_moveFromRow),
    m_moveFromCol(other.m_moveFromCol),
    m_moveToRow(other.m_moveToRow),
    m_moveToCol(other.m_moveToCol),
    m_capturedPiece(nullptr)

{
    m_squares.resize(8, std::vector<Piece*>(8, nullptr));


    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (other.m_squares[r][c] != nullptr) {
                const Piece* originalPiece = other.m_squares[r][c];

                switch (originalPiece->getType()) {
                    case PieceType::King:   m_squares[r][c] = new King(originalPiece->getColor(), r, c); break;
                    case PieceType::Queen:  m_squares[r][c] = new Queen(originalPiece->getColor(), r, c); break;
                    case PieceType::Rook:   m_squares[r][c] = new Rook(originalPiece->getColor(), r, c); break;
                    case PieceType::Bishop: m_squares[r][c] = new Bishop(originalPiece->getColor(), r, c); break;
                    case PieceType::Knight: m_squares[r][c] = new Knight(originalPiece->getColor(), r, c); break;
                    case PieceType::Pawn:   m_squares[r][c] = new Pawn(originalPiece->getColor(), r, c); break;

                }

                if (originalPiece->hasMoved()) {
                   if(m_squares[r][c]) m_squares[r][c]->setPosition(r, c);
                }
            } else {
                m_squares[r][c] = nullptr;
            }
        }
    }


    if (other.m_lastMovedPawn && other.m_enPassantCol != -1) {

        int pawnRow = other.m_lastMovedPawn->getRow();
        int pawnCol = other.m_lastMovedPawn->getCol();
        if (pawnRow >= 0 && pawnRow < 8 && pawnCol >= 0 && pawnCol < 8 && m_squares[pawnRow][pawnCol]) {

             if (m_squares[pawnRow][pawnCol]->getType() == PieceType::Pawn &&
                 m_squares[pawnRow][pawnCol]->getColor() == other.m_lastMovedPawn->getColor())
             {
                  m_lastMovedPawn = m_squares[pawnRow][pawnCol];
             } else {

                 m_lastMovedPawn = nullptr;
                 m_enPassantCol = -1;
             }
        } else {

             m_lastMovedPawn = nullptr;
             m_enPassantCol = -1;
        }
    } else {

        m_lastMovedPawn = nullptr;

        if (m_lastMovedPawn == nullptr) {
            m_enPassantCol = -1;
        }
    }
}


Board& Board::operator=(const Board& other) {
    if (this == &other) {
        return *this;
    }


    for (auto& row : m_squares) {
        for (auto piece : row) {
            delete piece;
        }
        row.assign(8, nullptr);
    }


    m_currentTurn = other.m_currentTurn;
    m_gameState = other.m_gameState;
    m_selectedRow = other.m_selectedRow;
    m_selectedCol = other.m_selectedCol;
    m_pieceSelected = other.m_pieceSelected;
    m_whiteKingRow = other.m_whiteKingRow;
    m_whiteKingCol = other.m_whiteKingCol;
    m_blackKingRow = other.m_blackKingRow;
    m_blackKingCol = other.m_blackKingCol;
    m_enPassantCol = other.m_enPassantCol;
    m_animProgress = 0.0f;
    m_animating = false;
    m_moveFromRow = other.m_moveFromRow;
    m_moveFromCol = other.m_moveFromCol;
    m_moveToRow = other.m_moveToRow;
    m_moveToCol = other.m_moveToCol;
    m_capturedPiece = nullptr;
    m_lastMovedPawn = nullptr;
    m_movingPiece = nullptr;
    m_validMoves.clear();
    m_candidateMoves.clear();
    m_legalMoveCache.clear();



    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (other.m_squares[r][c] != nullptr) {
                 const Piece* originalPiece = other.m_squares[r][c];
                switch (originalPiece->getType()) {
                    case PieceType::King:   m_squares[r][c] = new King(originalPiece->getColor(), r, c); break;
                    case PieceType::Queen:  m_squares[r][c] = new Queen(originalPiece->getColor(), r, c); break;
                    case PieceType::Rook:   m_squares[r][c] = new Rook(originalPiece->getColor(), r, c); break;
                    case PieceType::Bishop: m_squares[r][c] = new Bishop(originalPiece->getColor(), r, c); break;
                    case PieceType::Knight: m_squares[r][c] = new Knight(originalPiece->getColor(), r, c); break;
                    case PieceType::Pawn:   m_squares[r][c] = new Pawn(originalPiece->getColor(), r, c); break;
                }
                 if (originalPiece->hasMoved()) {
                     if(m_squares[r][c]) m_squares[r][c]->setPosition(r, c);
                 }
            } else {
                m_squares[r][c] = nullptr;
            }
        }
    }

    if (other.m_lastMovedPawn && other.m_enPassantCol != -1) {
        int pawnRow = other.m_lastMovedPawn->getRow();
        int pawnCol = other.m_lastMovedPawn->getCol();
        if (pawnRow >= 0 && pawnRow < 8 && pawnCol >= 0 && pawnCol < 8 && m_squares[pawnRow][pawnCol]) {
            if (m_squares[pawnRow][pawnCol]->getType() == PieceType::Pawn &&
                 m_squares[pawnRow][pawnCol]->getColor() == other.m_lastMovedPawn->getColor())
             {
                  m_lastMovedPawn = m_squares[pawnRow][pawnCol];
             } else {

                 m_lastMovedPawn = nullptr;
                 m_enPassantCol = -1;
             }
        } else {
             m_lastMovedPawn = nullptr;
             m_enPassantCol = -1;
        }
    } else {
        m_lastMovedPawn = nullptr;
        if (m_lastMovedPawn == nullptr) {
            m_enPassantCol = -1;
        }
    }
    return *this;
}

uint64_t Board::getZobristKey() const {
    uint64_t key = 0;

    for (int row = 0; row < 8; row++){
        for (int col = 0; col < 8; col++){
            Piece* piece = getPiece(row, col);
            if (piece) {
                int square = row * 8 + col;
                int colorIndex = (piece->getColor() == Color::White) ? 0 : 1;
                int typeIndex;
                switch(piece->getType()){
                    case PieceType::King:   typeIndex = 0; break;
                    case PieceType::Queen:  typeIndex = 1; break;
                    case PieceType::Rook:   typeIndex = 2; break;
                    case PieceType::Bishop: typeIndex = 3; break;
                    case PieceType::Knight: typeIndex = 4; break;
                    case PieceType::Pawn:   typeIndex = 5; break;
                }
                key ^= zobristTable[colorIndex][typeIndex][square];
            }
        }
    }

    if(m_currentTurn == Color::White)
        key ^= zobristSide;
    return key;
}

bool Board::isValidMovesComputed() const
{
    return g_validMovesComputed.load();
}


std::string Board::getPositionKey() const {
    std::stringstream ss;
    for (int row = 0; row < 8; row++){
        for (int col = 0; col < 8; col++){
            Piece* piece = getPiece(row, col);
            if (piece) {
                ss << static_cast<int>(piece->getType())
                   << (piece->getColor() == Color::White ? "W" : "B");
            } else {
                ss << "0";
            }
            ss << ",";
        }
    }
    return ss.str();
}

Board::Board() : m_currentTurn(Color::White), m_gameState(GameState::Active),
                 m_selectedRow(-1), m_selectedCol(-1), m_pieceSelected(false),
                 m_whiteKingRow(0), m_whiteKingCol(4),
                 m_blackKingRow(7), m_blackKingCol(4),
                 m_lastMovedPawn(nullptr), m_enPassantCol(-1),
                 m_movingPiece(nullptr), m_animProgress(0.0f), m_animating(false)
{
    m_candidateMoves = {};

    m_squares.resize(8);
    for (auto &row : m_squares)
    {
        row.resize(8, nullptr);
    }
}

Board::~Board()
{

    for (auto &row : m_squares)
    {
        for (auto piece : row)
        {
            delete piece;
        }
    }
}

void Board::initialize()
{

    for (auto &row : m_squares)
    {
        for (auto &piece : row)
        {
            delete piece;
            piece = nullptr;
        }
    }


    m_squares[0][0] = new Rook(Color::White, 0, 0);
    m_squares[0][1] = new Knight(Color::White, 0, 1);
    m_squares[0][2] = new Bishop(Color::White, 0, 2);
    m_squares[0][3] = new Queen(Color::White, 0, 3);
    m_squares[0][4] = new King(Color::White, 0, 4);
    m_squares[0][5] = new Bishop(Color::White, 0, 5);
    m_squares[0][6] = new Knight(Color::White, 0, 6);
    m_squares[0][7] = new Rook(Color::White, 0, 7);

    for (int col = 0; col < 8; col++)
    {
        m_squares[1][col] = new Pawn(Color::White, 1, col);
    }


    m_squares[7][0] = new Rook(Color::Black, 7, 0);
    m_squares[7][1] = new Knight(Color::Black, 7, 1);
    m_squares[7][2] = new Bishop(Color::Black, 7, 2);
    m_squares[7][3] = new Queen(Color::Black, 7, 3);
    m_squares[7][4] = new King(Color::Black, 7, 4);
    m_squares[7][5] = new Bishop(Color::Black, 7, 5);
    m_squares[7][6] = new Knight(Color::Black, 7, 6);
    m_squares[7][7] = new Rook(Color::Black, 7, 7);

    for (int col = 0; col < 8; col++)
    {
        m_squares[6][col] = new Pawn(Color::Black, 6, col);
    }


    m_currentTurn = Color::White;
    m_gameState = GameState::Active;
    m_selectedRow = -1;
    m_selectedCol = -1;
    m_pieceSelected = false;
    m_validMoves.clear();
    m_lastMovedPawn = nullptr;
    m_enPassantCol = -1;
    m_animating = false;


    m_whiteKingRow = 0;
    m_whiteKingCol = 4;
    m_blackKingRow = 7;
    m_blackKingCol = 4;
}

void Board::updateLegalMoveCache(Color color)
{

    m_legalMoveCache.clear();


    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            Piece* piece = getPiece(row, col);
            if(piece && piece->getColor() == color)
            {

                m_legalMoveCache[piece] = fastGenerateMoves(piece);
            }
        }
    }
}

void Board::calculateValidMovesAsync(int row, int col)
{
    Piece* piece = getPiece(row, col);
    if (!piece)
        return;

    std::future<void> task = std::async(std::launch::async, [this, row, col, piece]() {
        auto start = std::chrono::high_resolution_clock::now();


        std::vector<SDL_Point> candidates = fastGenerateMoves(piece);
        auto afterCandidates = std::chrono::high_resolution_clock::now();



        std::vector<SDL_Point> validMoves;
        bool inCheck = isInCheck(m_currentTurn);
        auto afterInCheck = std::chrono::high_resolution_clock::now();



        std::vector<std::tuple<int, int, int, int>> pins = getPins(m_currentTurn);
        std::vector<std::tuple<int, int, int, int>> checks = getChecks(m_currentTurn);
        auto afterPinsChecks = std::chrono::high_resolution_clock::now();



        int kingRow = (m_currentTurn == Color::White) ? m_whiteKingRow : m_blackKingRow;
        int kingCol = (m_currentTurn == Color::White) ? m_whiteKingCol : m_blackKingCol;

        if (inCheck)
        {
            if (checks.size() == 1)
            {
                auto [checkRow, checkCol, dr, dc] = checks[0];
                std::vector<SDL_Point> validSquares;
                Piece* checker = getPiece(checkRow, checkCol);
                if (checker->getType() != PieceType::Knight)
                {
                    for (int i = 1; i < 8; i++)
                    {
                        int r = kingRow + dr * i;
                        int c = kingCol + dc * i;
                        if (r == checkRow && c == checkCol)
                            break;
                        if (r >= 0 && r < 8 && c >= 0 && c < 8)
                            validSquares.push_back({c, r});
                    }
                }
                validSquares.push_back({checkCol, checkRow});

                for (const auto& move : candidates)
                {
                    if (piece->getType() == PieceType::King)
                    {
                        if (movePieceForSimulation(row, col, move.y, move.x))
                            validMoves.push_back(move);
                    }
                    else if (std::find(validSquares.begin(), validSquares.end(), move) != validSquares.end())
                    {
                        if (movePieceForSimulation(row, col, move.y, move.x))
                            validMoves.push_back(move);
                    }
                }
            }
            else
            {

                if (piece->getType() == PieceType::King)
                {
                    for (const auto& move : candidates)
                    {
                        if (movePieceForSimulation(row, col, move.y, move.x))
                            validMoves.push_back(move);
                    }
                }
            }
        }
        else
        {
            bool pinned = false;
            std::pair<int, int> pinDir;
            for (const auto& pin : pins)
            {
                if (std::get<0>(pin) == row && std::get<1>(pin) == col)
                {
                    pinned = true;
                    pinDir = {std::get<2>(pin), std::get<3>(pin)};
                    break;
                }
            }

            int simCount = 0;
            for (const auto& move : candidates)
            {
                if (!pinned || alignsWithPin(row, col, move.y, move.x, pinDir))
                {
                    if (piece->canMoveTo(move.y, move.x, *this) && movePieceForSimulation(row, col, move.y, move.x))
                        validMoves.push_back(move);
                    simCount++;
                }
            }
            auto end = std::chrono::high_resolution_clock::now();

        }

        std::lock_guard<std::mutex> lock(g_validMovesMutex);
        m_validMoves = validMoves;
        g_validMovesComputed = true;

    });
}

std::vector<SDL_Point> Board::fastGenerateMoves(Piece* piece) const {
    std::vector<SDL_Point> moves;
    int fromRow = piece->getRow();
    int fromCol = piece->getCol();
    Color color = piece->getColor();
    PieceType type = piece->getType();


    if (type == PieceType::King) {

        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                if (dr == 0 && dc == 0)
                    continue;
                int newRow = fromRow + dr;
                int newCol = fromCol + dc;
                if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {

                    Piece* destPiece = getPiece(newRow, newCol);
                    if (!destPiece || destPiece->getColor() != color) {
                        moves.push_back({newCol, newRow});
                    }
                }
            }
        }


        if (!piece->hasMoved() && !isInCheck(color)) {

            if (!getPiece(fromRow, fromCol + 1) && !getPiece(fromRow, fromCol + 2)) {
                Piece* rook = getPiece(fromRow, 7);
                if (rook && rook->getType() == PieceType::Rook && !rook->hasMoved()) {
                    if (!isSquareUnderAttack(fromRow, fromCol + 1, color == Color::White ? Color::Black : Color::White)) {
                        moves.push_back({fromCol + 2, fromRow});
                    }
                }
            }

            if (!getPiece(fromRow, fromCol - 1) && !getPiece(fromRow, fromCol - 2) && !getPiece(fromRow, fromCol - 3)) {
                Piece* rook = getPiece(fromRow, 0);
                if (rook && rook->getType() == PieceType::Rook && !rook->hasMoved()) {
                    if (!isSquareUnderAttack(fromRow, fromCol - 1, color == Color::White ? Color::Black : Color::White)) {
                        moves.push_back({fromCol - 2, fromRow});
                    }
                }
            }
        }
    }
    else if (type == PieceType::Pawn) {
        int direction = (color == Color::White) ? 1 : -1;
        int oneStep = fromRow + direction;
        if (oneStep >= 0 && oneStep < 8) {

            if (!getPiece(oneStep, fromCol))
                moves.push_back({fromCol, oneStep});

            if (!piece->hasMoved()) {
                int twoStep = fromRow + 2 * direction;
                if (twoStep >= 0 && twoStep < 8 && !getPiece(oneStep, fromCol) && !getPiece(twoStep, fromCol))
                    moves.push_back({fromCol, twoStep});
            }

            for (int dc = -1; dc <= 1; dc += 2) {
                int newCol = fromCol + dc;
                if (newCol >= 0 && newCol < 8)
                    moves.push_back({newCol, oneStep});
            }
        }
    }
    else if (type == PieceType::Bishop || type == PieceType::Rook || type == PieceType::Queen) {
        std::vector<std::pair<int, int>> directions;
        if (type == PieceType::Bishop || type == PieceType::Queen) {
            directions.push_back({1, 1});
            directions.push_back({-1, 1});
            directions.push_back({-1, -1});
            directions.push_back({1, -1});
        }
        if (type == PieceType::Rook || type == PieceType::Queen) {
            directions.push_back({0, 1});
            directions.push_back({1, 0});
            directions.push_back({0, -1});
            directions.push_back({-1, 0});
        }
        for (auto& dir : directions) {
            int r = fromRow, c = fromCol;
            while (true) {
                r += dir.second;
                c += dir.first;
                if (r < 0 || r >= 8 || c < 0 || c >= 8)
                    break;
                moves.push_back({c, r});
                if (getPiece(r, c))
                    break;
            }
        }
    }
    else if (type == PieceType::Knight) {

        const std::pair<int, int> offsets[8] = {{2, 1}, {1, 2}, {-1, 2}, {-2, 1},
                                                {-2, -1}, {-1, -2}, {1, -2}, {2, -1}};
        for (const auto& [dc, dr] : offsets) {
            int newRow = fromRow + dr;
            int newCol = fromCol + dc;
            if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                moves.push_back({newCol, newRow});
            }
        }
    }
    return moves;
}



bool Board::hasLegalMoves(Color color) const {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece* piece = getPiece(r, c);
            if (piece && piece->getColor() == color) {

                std::vector<SDL_Point> candidates = fastGenerateMoves(piece);


                for (const auto& move : candidates) {


                    Board simulationBoard = *this;


                     if (move.y >= 0 && move.y < 8 && move.x >= 0 && move.x < 8 &&
                         piece->canMoveTo(move.y, move.x, *this))
                     {

                         if (simulationBoard.movePieceForSimulation(r, c, move.y, move.x)) {

                             return true;
                         }
                     }
                }
            }
        }
    }

    return false;
}


bool Board::isBlockingCheck(int row, int col, int kingRow, int kingCol,
                           int attackerRow, int attackerCol) const {

    int dr = attackerRow - kingRow;
    int dc = attackerCol - kingCol;


    if (dr != 0) dr /= abs(dr);
    if (dc != 0) dc /= abs(dc);


    int r = kingRow + dr;
    int c = kingCol + dc;
    while (r != attackerRow || c != attackerCol) {
        if (r == row && c == col) return true;
        r += dr;
        c += dc;
    }
    return false;
}
Piece *Board::getPiece(int row, int col) const
{
    if (row >= 0 && row < 8 && col >= 0 && col < 8)
    {
        return m_squares[row][col];
    }
    return nullptr;
}

bool Board::movePiece(int fromRow, int fromCol, int toRow, int toCol)
{

    if (m_animating)
        return false;

    Piece *piece = getPiece(fromRow, fromCol);
    if (!piece || piece->getColor() != m_currentTurn)
    {

        return false;
    }



    m_lastMovedPawn = nullptr;
    m_enPassantCol = -1;


    if (!piece->canMoveTo(toRow, toCol, *this))
    {
        return false;
    }


    if (piece->getType() == PieceType::King && std::abs(toCol - fromCol) == 2)
    {

        if (toCol > fromCol)
        {

            Piece *rook = getPiece(fromRow, 7);
            m_squares[fromRow][fromCol + 1] = rook;
            m_squares[fromRow][7] = nullptr;
            if (rook)
            {
                rook->setPosition(fromRow, fromCol + 1);
            }
        }

        else
        {

            Piece *rook = getPiece(fromRow, 0);
            m_squares[fromRow][fromCol - 1] = rook;
            m_squares[fromRow][0] = nullptr;
            if (rook)
            {
                rook->setPosition(fromRow, fromCol - 1);
            }
        }
    }


    Piece *capturedPiece = nullptr;


    if (piece->getType() == PieceType::Pawn && fromCol != toCol && !getPiece(toRow, toCol))
    {

        capturedPiece = getPiece(fromRow, toCol);
        m_squares[fromRow][toCol] = nullptr;

    }
    else
    {

        capturedPiece = getPiece(toRow, toCol);
        if (capturedPiece)
        {


            switch (capturedPiece->getType())
            {
            case PieceType::Pawn:

                break;
            case PieceType::Knight:

                break;
            case PieceType::Bishop:

                break;
            case PieceType::Rook:

                break;
            case PieceType::Queen:

                break;
            case PieceType::King:

                break;
            }

        }
    }


    if (piece->getType() == PieceType::Pawn && std::abs(toRow - fromRow) == 2)
    {
        m_lastMovedPawn = piece;
        m_enPassantCol = toCol;

    }


    m_movingPiece = piece;
    m_animStartX = fromCol * SQUARE_SIZE;
    m_animStartY = (7 - fromRow) * SQUARE_SIZE;
    m_animEndX = toCol * SQUARE_SIZE;
    m_animEndY = (7 - toRow) * SQUARE_SIZE;
    m_animProgress = 0.0f;
    m_animating = true;


    m_moveFromRow = fromRow;
    m_moveFromCol = fromCol;
    m_moveToRow = toRow;
    m_moveToCol = toCol;
    m_capturedPiece = this->getPiece(toRow, toCol);

    return true;
}

void Board::handleClick(int x, int y)
{
    if (m_gameState == GameState::Checkmate || m_gameState == GameState::Stalemate || m_animating)
        return;

    int row, col;
    screenToBoard(x, y, row, col);
    if (row < 0 || row >= 8 || col < 0 || col >= 8)
        return;

    Piece* clickedPiece = getPiece(row, col);

    if (!m_pieceSelected)
    {
        if (clickedPiece && clickedPiece->getColor() == m_currentTurn)
        {
            m_selectedRow = row;
            m_selectedCol = col;
            m_pieceSelected = true;
            m_validMoves.clear();
            g_validMovesComputed = false;

            m_candidateMoves = fastGenerateMoves(clickedPiece);

            calculateValidMovesAsync(row, col);
        }
    }
    else
    {

        if (g_validMovesComputed)
        {
            std::lock_guard<std::mutex> lock(g_validMovesMutex);
            for (const auto& move : m_validMoves)
            {
                if (move.x == col && move.y == row)
                {
                    if (movePiece(m_selectedRow, m_selectedCol, row, col))
                    {

                    }
                    break;
                }
            }
            m_pieceSelected = false;
            m_selectedRow = -1;
            m_selectedCol = -1;
            m_validMoves.clear();
            m_candidateMoves.clear();
            g_validMovesComputed = false;
        }
        else if (clickedPiece && clickedPiece->getColor() == m_currentTurn)
        {

            m_selectedRow = row;
            m_selectedCol = col;
            m_validMoves.clear();
            m_candidateMoves = fastGenerateMoves(clickedPiece);
            g_validMovesComputed = false;
            calculateValidMovesAsync(row, col);
        }
        else
        {

            m_pieceSelected = false;
            m_selectedRow = -1;
            m_selectedCol = -1;
            m_validMoves.clear();
            m_candidateMoves.clear();
            g_validMovesComputed = false;
        }
    }
}

void Board::calculateValidMoves(int row, int col)
{
    try {

        m_validMoves.clear();

        Piece* piece = getPiece(row, col);
        if (!piece)
        {

            return;
        }


        std::vector<SDL_Point> candidates = generateCandidateMoves(piece);
        for (const auto &move : candidates)
        {
            int toCol = move.x;
            int toRow = move.y;
            try {

                if (piece->canMoveTo(toRow, toCol, *this))
                {

                    Board simulation = *this;
                    if (simulation.movePieceForSimulation(row, col, toRow, toCol))
                    {
                        m_validMoves.push_back(move);

                    }
                }
            }
            catch (const std::exception &e) {

            }
            catch (...) {

            }
        }

    }
    catch (const std::exception &e) {

    }
    catch (...) {

    }
}

const std::vector<SDL_Point> Board::knightMoves[64] = {

    {{1, 2}, {2, 1}},
    {{2, 1}, {1, 2}, {-1, 2}},
    {{1, 2}, {-1, 2}, {2, 1}},
    {{1, 2}, {-1, 2}, {2, 1}},
    {{1, 2}, {-1, 2}, {2, 1}},
    {{1, 2}, {-1, 2}, {2, 1}},
    {{1, 2}, {-1, 2}, {-2, 1}},
    {{1, 2}, {-2, 1}},


    {{1, 2}, {2, 1}, {2, -1}},
    {{2, 1}, {1, 2}, {-1, 2}, {2, -1}, {-2, 1}},
    {{1, 2}, {-1, 2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, 2}, {-1, 2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, 2}, {-1, 2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, 2}, {-1, 2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, 2}, {-1, 2}, {-2, 1}, {2, -1}},
    {{1, 2}, {-2, 1}, {2, -1}},


    {{2, 1}, {1, -2}, {2, -1}},
    {{2, 1}, {1, -2}, {-1, -2}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {-2, 1}, {2, -1}},
    {{1, -2}, {-2, 1}, {2, -1}},


    {{2, 1}, {1, -2}, {2, -1}},
    {{2, 1}, {1, -2}, {-1, -2}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {-2, 1}, {2, -1}},
    {{1, -2}, {-2, 1}, {2, -1}},


    {{2, 1}, {1, -2}, {2, -1}},
    {{2, 1}, {1, -2}, {-1, -2}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {-2, 1}, {2, -1}},
    {{1, -2}, {-2, 1}, {2, -1}},


    {{2, 1}, {1, -2}, {2, -1}},
    {{2, 1}, {1, -2}, {-1, -2}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, -2}, {-1, -2}, {-2, 1}, {2, -1}},
    {{1, -2}, {-2, 1}, {2, -1}},


    {{2, 1}, {1, 2}, {2, -1}},
    {{2, 1}, {1, 2}, {-1, 2}, {2, -1}, {-2, 1}},
    {{1, 2}, {-1, 2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, 2}, {-1, 2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, 2}, {-1, 2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, 2}, {-1, 2}, {2, 1}, {2, -1}, {-2, 1}},
    {{1, 2}, {-1, 2}, {-2, 1}, {2, -1}},
    {{1, 2}, {-2, 1}, {2, -1}},


    {{2, 1}, {1, -2}},
    {{2, 1}, {1, -2}, {-1, -2}},
    {{1, -2}, {-1, -2}, {2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}},
    {{1, -2}, {-1, -2}, {2, 1}},
    {{1, -2}, {-1, -2}, {-2, 1}},
    {{1, -2}, {-2, 1}},
};
void Board::render(SDL_Renderer* renderer, const Game& game) const
{
    const int BOARD_SIZE = 600;
    const int SQUARE_SIZE = BOARD_SIZE / 8;


    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if ((row + col) % 2 == 0) {
                SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255);
            }
            SDL_FRect rect = {(float)(col * SQUARE_SIZE), (float)((7 - row) * SQUARE_SIZE),
                              (float)SQUARE_SIZE, (float)SQUARE_SIZE};
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    if (m_pieceSelected)
    {
        SDL_SetRenderDrawColor(renderer, 186, 202, 43, 128);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_FRect rect = {(float)(m_selectedCol * SQUARE_SIZE), (float)((7 - m_selectedRow) * SQUARE_SIZE),
                          (float)SQUARE_SIZE, (float)SQUARE_SIZE};
        SDL_RenderFillRect(renderer, &rect);

        Piece* piece = getPiece(m_selectedRow, m_selectedCol);
        if (piece)
        {

            if (!g_validMovesComputed)
            {
                SDL_SetRenderDrawColor(renderer, 255, 165, 0, 96);
                for (const auto& move : m_candidateMoves)
                {
                    SDL_FRect moveRect = {(float)(move.x * SQUARE_SIZE), (float)((7 - move.y) * SQUARE_SIZE),
                                          (float)SQUARE_SIZE, (float)SQUARE_SIZE};
                    SDL_RenderFillRect(renderer, &moveRect);
                }
            }

            if (g_validMovesComputed)
            {
                std::lock_guard<std::mutex> lock(g_validMovesMutex);
                SDL_SetRenderDrawColor(renderer, 186, 202, 43, 96);
                for (const auto& move : m_validMoves)
                {
                    SDL_FRect moveRect = {(float)(move.x * SQUARE_SIZE), (float)((7 - move.y) * SQUARE_SIZE),
                                          (float)SQUARE_SIZE, (float)SQUARE_SIZE};
                    SDL_RenderFillRect(renderer, &moveRect);
                }
            }
        }
    }

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            Piece* piece = m_squares[row][col];
            if (piece && (!m_animating || piece != m_movingPiece)) {
                piece->render(renderer, game, SQUARE_SIZE);
            }
        }
    }

    if (m_animating && m_movingPiece) {
        float progress = std::max(0.0f, std::min(m_animProgress, 1.0f));
        float x = m_animStartX + (m_animEndX - m_animStartX) * progress;
        float y = m_animStartY + (m_animEndY - m_animStartY) * progress;
        SDL_Texture* texture = game.getPieceTexture(m_movingPiece->getType(), m_movingPiece->getColor());
        if (texture) {
            SDL_FRect destRect = {x, y, (float)SQUARE_SIZE, (float)SQUARE_SIZE};
            if (x >= 0 && y >= 0 && x < BOARD_SIZE && y < BOARD_SIZE) {
                SDL_RenderTexture(renderer, texture, nullptr, &destRect);
            }
        }
    }
}

void Board::updateAnimation(float deltaTime)
{
    if (!m_animating || !m_movingPiece)
        return;


    float cappedDelta = std::min(deltaTime, 0.05f);


    float prevProgress = m_animProgress;


    m_animProgress += cappedDelta * 2.5f;


    if (int(prevProgress * 10) != int(m_animProgress * 10)) {

    }

    if (m_animProgress >= 1.0f)
    {


        try {
            completeMoveAfterAnimation();
        }
        catch (const std::exception& e) {

        }

        m_animProgress = 0.0f;
        m_animating = false;
        m_movingPiece = nullptr;


    }
}

void Board::completeMoveAfterAnimation()
{
    try {
        int fromRow = m_moveFromRow;
        int fromCol = m_moveFromCol;
        int toRow   = m_moveToRow;
        int toCol   = m_moveToCol;


        Piece* piece = m_movingPiece;
        if (!piece) {

            return;
        }



        if (piece->getType() == PieceType::King && std::abs(toCol - fromCol) == 2)
        {

            if (toCol > fromCol) {
                Piece* rook = getPiece(fromRow, 7);
                if (rook) {

                    m_squares[fromRow][toCol - 1] = rook;
                    m_squares[fromRow][7] = nullptr;
                    rook->setPosition(fromRow, toCol - 1);

                }
            }

            else {
                Piece* rook = getPiece(fromRow, 0);
                if (rook) {

                    m_squares[fromRow][toCol + 1] = rook;
                    m_squares[fromRow][0] = nullptr;
                    rook->setPosition(fromRow, toCol + 1);

                }
            }
        }



        piece->setPosition(toRow, toCol);



        if(m_capturedPiece) ;


        if (m_capturedPiece) {


            delete m_capturedPiece;
            m_capturedPiece = nullptr;
        }


        m_squares[toRow][toCol] = piece;
        m_squares[fromRow][fromCol] = nullptr;



        if(m_squares[toRow][toCol]) {



        }

        if (piece->getType() == PieceType::King) {
            if (piece->getColor() == Color::White) {
                m_whiteKingRow = toRow;
                m_whiteKingCol = toCol;
            } else {
                m_blackKingRow = toRow;
                m_blackKingCol = toCol;
            }
        }



        if (piece->getType() == PieceType::Pawn) {
            if ((piece->getColor() == Color::White && toRow == 7) ||
                (piece->getColor() == Color::Black && toRow == 0))
            {
                Color pawnColor = piece->getColor();
                delete piece;
                Piece* promoted = new Queen(pawnColor, toRow, toCol);
                m_squares[toRow][toCol] = promoted;

            }
        }


        m_selectedRow = -1;
        m_selectedCol = -1;
        m_pieceSelected = false;
        m_validMoves.clear();
        g_validMovesComputed = false;


        Color oldTurn = m_currentTurn;
        m_currentTurn = (m_currentTurn == Color::White) ? Color::Black : Color::White;



        updateLegalMoveCache(Color::White);
        updateLegalMoveCache(Color::Black);
        checkForPromotion();
        updateGameState();


    }
    catch (const std::exception& e) {

    }
    catch (...) {

    }
}



bool Board::isInCheck(Color color) const
{

    int kingRow, kingCol;
    if (color == Color::White)
    {
        kingRow = m_whiteKingRow;
        kingCol = m_whiteKingCol;
    }
    else
    {
        kingRow = m_blackKingRow;
        kingCol = m_blackKingCol;
    }

    return isSquareUnderAttack(kingRow, kingCol, (color == Color::White) ? Color::Black : Color::White);
}

bool Board::isSquareUnderAttack(int row, int col, Color attackingColor) const
{
    auto start = std::chrono::high_resolution_clock::now();


    std::vector<std::pair<int, int>> directions = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (size_t j = 0; j < directions.size(); j++)
    {
        auto [dr, dc] = directions[j];
        for (int i = 1; i < 8; i++)
        {
            int r = row + dr * i;
            int c = col + dc * i;
            if (r < 0 || r >= 8 || c < 0 || c >= 8)
                break;
            Piece* piece = getPiece(r, c);
            if (piece)
            {
                if (piece->getColor() == attackingColor)
                {
                    if ((j < 4 && piece->getType() == PieceType::Rook) ||
                        (j >= 4 && piece->getType() == PieceType::Bishop) ||
                        piece->getType() == PieceType::Queen ||
                        (i == 1 && piece->getType() == PieceType::King))
                        return true;
                }
                break;
            }
        }
    }


    std::vector<std::pair<int, int>> knightMoves = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}};
    for (const auto& [dr, dc] : knightMoves)
    {
        int r = row + dr;
        int c = col + dc;
        if (r >= 0 && r < 8 && c >= 0 && c < 8)
        {
            Piece* piece = getPiece(r, c);
            if (piece && piece->getColor() == attackingColor && piece->getType() == PieceType::Knight)
                return true;
        }
    }


    int pawnDir = (attackingColor == Color::White) ? -1 : 1;
    for (int dc : {-1, 1})
    {
        int r = row + pawnDir;
        int c = col + dc;
        if (r >= 0 && r < 8 && c >= 0 && c < 8)
        {
            Piece* piece = getPiece(r, c);
            if (piece && piece->getColor() == attackingColor && piece->getType() == PieceType::Pawn)
                return true;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    return false;
}


std::vector<SDL_Point> Board::generateCandidateMoves(Piece* piece) const {
    std::vector<SDL_Point> moves;
    int fromRow = piece->getRow();
    int fromCol = piece->getCol();
    Color color = piece->getColor();
    PieceType type = piece->getType();

    if(type == PieceType::Pawn) {
        int direction = (color == Color::White) ? 1 : -1;
        int oneStep = fromRow + direction;

        if(getPiece(oneStep, fromCol) == nullptr) {
            moves.push_back({fromCol, oneStep});

            if(!piece->hasMoved()) {
                int twoStep = fromRow + 2 * direction;
                if(getPiece(oneStep, fromCol) == nullptr && getPiece(twoStep, fromCol) == nullptr)
                    moves.push_back({fromCol, twoStep});
            }
        }

        for (int dc = -1; dc <= 1; dc += 2) {
            int capCol = fromCol + dc;
            if(capCol >= 0 && capCol < 8) {
                moves.push_back({capCol, oneStep});
            }
        }

    }
    else if(type == PieceType::Knight) {
        const int offsets[8][2] = { {2,1}, {1,2}, {-1,2}, {-2,1},
                                    {-2,-1}, {-1,-2}, {1,-2}, {2,-1} };
        for (auto& offset : offsets) {
            int r = fromRow + offset[1];
            int c = fromCol + offset[0];
            if(r >= 0 && r < 8 && c >= 0 && c < 8) {
                moves.push_back({c, r});
            }
        }
    }

    else if(type == PieceType::Bishop || type == PieceType::Rook || type == PieceType::Queen) {
        std::vector<std::pair<int,int>> directions;
        if(type == PieceType::Bishop || type == PieceType::Queen) {
            directions.push_back({1,1});
            directions.push_back({-1,1});
            directions.push_back({-1,-1});
            directions.push_back({1,-1});
        }
        if(type == PieceType::Rook || type == PieceType::Queen) {
            directions.push_back({0,1});
            directions.push_back({1,0});
            directions.push_back({0,-1});
            directions.push_back({-1,0});
        }
        for(auto& dir : directions) {
            int r = fromRow, c = fromCol;
            while(true) {
                r += dir.second;
                c += dir.first;
                if(r < 0 || r >= 8 || c < 0 || c >= 8)
                    break;
                moves.push_back({c, r});

                if(getPiece(r,c) != nullptr)
                    break;
            }
        }
    }

    else if(type == PieceType::King) {
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if(dr == 0 && dc == 0)
                    continue;
                int r = fromRow + dr;
                int c = fromCol + dc;
                if(r >= 0 && r < 8 && c >= 0 && c < 8)
                    moves.push_back({c, r});
            }
        }
    }
    return moves;
}
bool Board::isCheckmate() const {

    return isInCheck(m_currentTurn) && !hasLegalMoves(m_currentTurn);
}

bool Board::isStalemate() const {

    return !isInCheck(m_currentTurn) && !hasLegalMoves(m_currentTurn);
}

bool Board::movePieceForSimulation(int fromRow, int fromCol, int toRow, int toCol)
{
    Piece* piece = getPiece(fromRow, fromCol);
    if (!piece) {

        return false;
    }
    Color movingColor = piece->getColor();


    Piece* captured = getPiece(toRow, toCol);
    int oldWhiteKingRow = m_whiteKingRow, oldWhiteKingCol = m_whiteKingCol;
    int oldBlackKingRow = m_blackKingRow, oldBlackKingCol = m_blackKingCol;
    Piece* oldLastMovedPawn = m_lastMovedPawn;
    int oldEnPassantCol = m_enPassantCol;
    bool pieceHadMoved = piece->hasMoved();


    Piece* enPassantVictim = nullptr;
    int epVictimRow = -1, epVictimCol = -1;


    if (piece->getType() == PieceType::Pawn && fromCol != toCol && !captured) {
        epVictimRow = fromRow;
        epVictimCol = toCol;
        enPassantVictim = m_squares[epVictimRow][epVictimCol];
        if (!enPassantVictim || enPassantVictim->getType() != PieceType::Pawn) {

             enPassantVictim = nullptr;
        } else {
             m_squares[epVictimRow][epVictimCol] = nullptr;
        }
    }


    m_squares[toRow][toCol] = piece;
    m_squares[fromRow][fromCol] = nullptr;



    piece->setPosition(toRow, toCol);


    if (piece->getType() == PieceType::King) {
        updateKingPosition(movingColor, toRow, toCol);
    }

    if (piece->getType() == PieceType::Pawn && std::abs(toRow - fromRow) == 2) {
        setEnPassantTarget(piece, toCol);
    } else {
        clearEnPassantTarget();
    }


    bool isMoveLegal = !isInCheck(movingColor);


    piece->m_row = fromRow;
    piece->m_col = fromCol;
    piece->m_hasMoved = pieceHadMoved;


    m_squares[fromRow][fromCol] = piece;
    m_squares[toRow][toCol] = captured;
    if (enPassantVictim) {
        m_squares[epVictimRow][epVictimCol] = enPassantVictim;
    }


    m_whiteKingRow = oldWhiteKingRow; m_whiteKingCol = oldWhiteKingCol;
    m_blackKingRow = oldBlackKingRow; m_blackKingCol = oldBlackKingCol;
    m_lastMovedPawn = oldLastMovedPawn;
    m_enPassantCol = oldEnPassantCol;

    return isMoveLegal;
}












void Board::updateGameState()
{
    try {
        bool inCheck = isInCheck(m_currentTurn);


        updateLegalMoveCache(m_currentTurn);
        bool hasLegalMovesCached = hasLegalMoves(m_currentTurn);

        if (inCheck) {
            if (!hasLegalMovesCached) {
                m_gameState = GameState::Checkmate;
                std::string winner = (m_currentTurn == Color::White) ? "Black" : "White";

            } else {
                m_gameState = GameState::Check;

            }
        } else {
            if (!hasLegalMovesCached) {
                m_gameState = GameState::Stalemate;

            } else {
                m_gameState = GameState::Active;
            }
        }
    } catch (const std::exception& e) {

        m_gameState = GameState::Active;
    } catch (...) {

        m_gameState = GameState::Active;
    }
}

void Board::screenToBoard(int screenX, int screenY, int &boardRow, int &boardCol) const
{
    const int BOARD_SIZE = 600;
    const int SQUARE_SIZE = BOARD_SIZE / 8;


    boardCol = screenX / SQUARE_SIZE;
    boardRow = 7 - (screenY / SQUARE_SIZE);
}


void Board::placePiece(Piece *piece, int row, int col)
{

    if (row < 0 || row >= 8 || col < 0 || col >= 8) {

        return;
    }

    Piece* existingPiece = m_squares[row][col];
    PieceType incomingType = piece ? piece->getType() : PieceType::King;
    Color incomingColor = piece ? piece->getColor() : Color::White;





    if(piece) { ; }
    else { ; }
    ;


    if (existingPiece != nullptr) {


        if (existingPiece == piece) {

        } else {

            delete existingPiece;
            m_squares[row][col] = nullptr;
        }
    } else {

    }


    m_squares[row][col] = piece;


    if (piece)
    {

        if (piece->getRow() != row || piece->getCol() != col) {

            piece->setPosition(row, col);
        } else {



             if (!piece->hasMoved()) {

                  piece->m_hasMoved = true;
             }
        }
    } else {

    }

}

void Board::setSquare(int r, int c, Piece* p) {
    if (r >= 0 && r < 8 && c >= 0 && c < 8) {


        m_squares[r][c] = p;
    } else {

    }
}


Piece* Board::getLastMovedPawn() const {
    return m_lastMovedPawn;
}

int Board::getEnPassantCol() const {
    return m_enPassantCol;
}


void Board::checkForPromotion()
{


    for (int col = 0; col < 8; ++col) {
        Piece* whitePawn = getPiece(7, col);
        if (whitePawn && whitePawn->getType() == PieceType::Pawn && whitePawn->getColor() == Color::White) {
            delete whitePawn;
            placePiece(new Queen(Color::White, 7, col), 7, col);

        }
        Piece* blackPawn = getPiece(0, col);
        if (blackPawn && blackPawn->getType() == PieceType::Pawn && blackPawn->getColor() == Color::Black) {
            delete blackPawn;
            placePiece(new Queen(Color::Black, 0, col), 0, col);

        }
    }
}


std::vector<std::tuple<int, int, int, int>> Board::getPins(Color color) const {
    std::vector<std::tuple<int, int, int, int>> pins;
    int kingRow = (color == Color::White) ? m_whiteKingRow : m_blackKingRow;
    int kingCol = (color == Color::White) ? m_whiteKingCol : m_blackKingCol;

    std::vector<std::pair<int, int>> directions = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (const auto& [dr, dc] : directions) {
        int possiblePinRow = -1, possiblePinCol = -1;
        for (int i = 1; i < 8; i++) {
            int r = kingRow + dr * i;
            int c = kingCol + dc * i;
            if (r < 0 || r >= 8 || c < 0 || c >= 8)
                break;
            Piece* piece = getPiece(r, c);
            if (piece) {
                if (piece->getColor() == color && piece->getType() != PieceType::King) {
                    if (possiblePinRow == -1) {
                        possiblePinRow = r;
                        possiblePinCol = c;
                    } else {
                        break;
                    }
                } else if (piece->getColor() != color) {
                    if (possiblePinRow != -1 && (
                        (dr == 0 || dc == 0) && piece->getType() == PieceType::Rook ||
                        (dr != 0 && dc != 0) && piece->getType() == PieceType::Bishop ||
                        piece->getType() == PieceType::Queen)) {
                        pins.push_back({possiblePinRow, possiblePinCol, dr, dc});
                    }
                    break;
                }
            }
        }
    }
    return pins;
}

std::vector<std::tuple<int, int, int, int>> Board::getChecks(Color color) const {
    std::vector<std::tuple<int, int, int, int>> checks;
    int kingRow = (color == Color::White) ? m_whiteKingRow : m_blackKingRow;
    int kingCol = (color == Color::White) ? m_whiteKingCol : m_blackKingCol;


    std::vector<std::pair<int, int>> directions = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (const auto& [dr, dc] : directions) {
        for (int i = 1; i < 8; i++) {
            int r = kingRow + dr * i;
            int c = kingCol + dc * i;
            if (r < 0 || r >= 8 || c < 0 || c >= 8)
                break;
            Piece* piece = getPiece(r, c);
            if (piece && piece->getColor() != color) {
                if (((dr == 0 || dc == 0) && piece->getType() == PieceType::Rook) ||
                    (dr != 0 && dc != 0 && piece->getType() == PieceType::Bishop) ||
                    piece->getType() == PieceType::Queen ||
                    (i == 1 && piece->getType() == PieceType::King)) {
                    checks.push_back({r, c, dr, dc});
                }
                break;
            }
        }
    }


    std::vector<std::pair<int, int>> knightMoves = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}};
    for (const auto& [dr, dc] : knightMoves) {
        int r = kingRow + dr;
        int c = kingCol + dc;
        if (r >= 0 && r < 8 && c >= 0 && c < 8) {
            Piece* piece = getPiece(r, c);
            if (piece && piece->getColor() != color && piece->getType() == PieceType::Knight) {
                checks.push_back({r, c, dr, dc});
            }
        }
    }

    return checks;
}

bool Board::alignsWithPin(int fromRow, int fromCol, int toRow, int toCol, std::pair<int, int> pinDir) const {
    int dr = toRow - fromRow;
    int dc = toCol - fromCol;
    return (dr == pinDir.first && dc == pinDir.second) || (dr == -pinDir.first && dc == -pinDir.second);
}



void Board::updateKingPosition(Color color, int row, int col) {
    if (color == Color::White) {
        m_whiteKingRow = row;
        m_whiteKingCol = col;
    } else {
        m_blackKingRow = row;
        m_blackKingCol = col;
    }
}

void Board::setEnPassantTarget(Piece* pawn, int col) {
    m_lastMovedPawn = pawn;
    m_enPassantCol = col;
}

void Board::clearEnPassantTarget() {
    m_lastMovedPawn = nullptr;
    m_enPassantCol = -1;
}