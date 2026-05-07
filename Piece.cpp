// ============================================================
//  Piece.cpp  –  Move-validation logic for every piece
// ============================================================
#include "Piece.h"

// ------------------------------------------------------------
//  Base class constructor
// ------------------------------------------------------------
Piece::Piece(Color c, PieceType t)
    : color(c), type(t), hasMoved(false)
{}

// ============================================================
//  PAWN
// ============================================================
//  NOTE: En-passant is handled in Board::isLegalMove() because
//        it needs knowledge about the previous move.  Here we
//        only cover normal forward moves and diagonal captures.
// ============================================================
bool Pawn::isValidMove(int fromR, int fromC,
                       int toR,   int toC,
                       Piece* board[8][8]) const
{
    // Direction: WHITE moves up (decreasing row), BLACK moves down
    int dir = (color == WHITE) ? -1 : 1;

    int dr = toR - fromR;
    int dc = toC - fromC;

    // ---- One square forward (must be empty) ----
    if (dc == 0 && dr == dir)
    {
        return (board[toR][toC] == nullptr);
    }

    // ---- Two squares forward from starting rank ----
    int startRow = (color == WHITE) ? 6 : 1;
    if (dc == 0 && dr == 2 * dir && fromR == startRow)
    {
        // Both squares must be empty
        return (board[fromR + dir][fromC] == nullptr &&
                board[toR][toC]          == nullptr);
    }

    // ---- Diagonal capture ----
    if (abs(dc) == 1 && dr == dir)
    {
        // Normal capture: destination occupied by enemy
        if (board[toR][toC] != nullptr &&
            board[toR][toC]->getColor() != color)
        {
            return true;
        }
        // En-passant is checked separately in Board
    }

    return false;
}

// ============================================================
//  ROOK  –  horizontal or vertical, no jumping
// ============================================================
bool Rook::isValidMove(int fromR, int fromC,
                       int toR,   int toC,
                       Piece* board[8][8]) const
{
    // Must move along a rank OR a file
    if (fromR != toR && fromC != toC) return false;

    // Check that the path is clear (excluding destination)
    int dr = (toR == fromR) ? 0 : (toR > fromR ? 1 : -1);
    int dc = (toC == fromC) ? 0 : (toC > fromC ? 1 : -1);

    int r = fromR + dr;
    int c = fromC + dc;
    while (r != toR || c != toC)
    {
        if (board[r][c] != nullptr) return false; // blocked
        r += dr;
        c += dc;
    }

    // Destination: empty or enemy
    return (board[toR][toC] == nullptr ||
            board[toR][toC]->getColor() != color);
}

// ============================================================
//  KNIGHT  –  L-shape, can jump over pieces
// ============================================================
bool Knight::isValidMove(int fromR, int fromC,
                         int toR,   int toC,
                         Piece* board[8][8]) const
{
    int dr = abs(toR - fromR);
    int dc = abs(toC - fromC);

    bool isL = (dr == 2 && dc == 1) || (dr == 1 && dc == 2);
    if (!isL) return false;

    // Destination: empty or enemy
    return (board[toR][toC] == nullptr ||
            board[toR][toC]->getColor() != color);
}

// ============================================================
//  BISHOP  –  diagonal only, no jumping
// ============================================================
bool Bishop::isValidMove(int fromR, int fromC,
                         int toR,   int toC,
                         Piece* board[8][8]) const
{
    int dr = abs(toR - fromR);
    int dc = abs(toC - fromC);

    if (dr != dc || dr == 0) return false; // not diagonal

    // Step direction
    int stepR = (toR > fromR) ? 1 : -1;
    int stepC = (toC > fromC) ? 1 : -1;

    int r = fromR + stepR;
    int c = fromC + stepC;
    while (r != toR || c != toC)
    {
        if (board[r][c] != nullptr) return false;
        r += stepR;
        c += stepC;
    }

    return (board[toR][toC] == nullptr ||
            board[toR][toC]->getColor() != color);
}

// ============================================================
//  QUEEN  –  rook + bishop combined
// ============================================================
bool Queen::isValidMove(int fromR, int fromC,
                        int toR,   int toC,
                        Piece* board[8][8]) const
{
    int dr = abs(toR - fromR);
    int dc = abs(toC - fromC);

    bool straightLine = (fromR == toR || fromC == toC);
    bool diagonal     = (dr == dc && dr > 0);

    if (!straightLine && !diagonal) return false;

    // Reuse rook/bishop path-check logic inline
    int stepR = (toR == fromR) ? 0 : (toR > fromR ? 1 : -1);
    int stepC = (toC == fromC) ? 0 : (toC > fromC ? 1 : -1);

    int r = fromR + stepR;
    int c = fromC + stepC;
    while (r != toR || c != toC)
    {
        if (board[r][c] != nullptr) return false;
        r += stepR;
        c += stepC;
    }

    return (board[toR][toC] == nullptr ||
            board[toR][toC]->getColor() != color);
}

// ============================================================
//  KING  –  one square any direction (castling in Board)
// ============================================================
bool King::isValidMove(int fromR, int fromC,
                       int toR,   int toC,
                       Piece* board[8][8]) const
{
    int dr = abs(toR - fromR);
    int dc = abs(toC - fromC);

    // Normal king move: one step in any direction
    if (dr <= 1 && dc <= 1 && (dr + dc > 0))
    {
        return (board[toR][toC] == nullptr ||
                board[toR][toC]->getColor() != color);
    }

    // Two-square horizontal move is castling – validated in Board
    if (dr == 0 && dc == 2) return true;

    return false;
}