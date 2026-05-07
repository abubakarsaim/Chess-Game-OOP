#pragma once

// ============================================================
//  Piece.h  –  Base class and all Chess Piece declarations
// ============================================================

// Colour of each piece
enum Color { WHITE, BLACK, NONE };

// Type identifiers (useful for promotion, rendering, etc.)
enum PieceType { PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING, EMPTY };

// ============================================================
//  Base Class: Piece
// ============================================================
class Piece
{
protected:
    Color     color;    // WHITE or BLACK
    PieceType type;     // PAWN, ROOK … KING
    bool      hasMoved; // needed for castling & en-passant

public:
    // Constructor / Destructor
    Piece(Color c, PieceType t);
    virtual ~Piece() {}

    // ---- Accessors ----
    Color     getColor()    const { return color; }
    PieceType getType()     const { return type;  }
    bool      getHasMoved() const { return hasMoved; }
    void      setHasMoved(bool v) { hasMoved = v; }

    // ---- Pure virtual: each piece validates its own moves ----
    // Returns true if moving from (fromR,fromC) to (toR,toC) is
    // geometrically legal FOR THIS PIECE (ignores blocking/check).
    virtual bool isValidMove(int fromR, int fromC,
                             int toR,   int toC,
                             Piece* board[8][8]) const = 0;

    // ---- Helper: is square (r,c) inside the board? ----
    static bool inBounds(int r, int c)
    {
        return r >= 0 && r < 8 && c >= 0 && c < 8;
    }
};

// ============================================================
//  Derived Class: Pawn
// ============================================================
class Pawn : public Piece
{
public:
    Pawn(Color c) : Piece(c, PAWN) {}

    // Special: en-passant target column (-1 = none)
    // Stored on the board, not here, but we need to read it
    // so we pass it via the board pointer trick in Board.
    bool isValidMove(int fromR, int fromC,
                     int toR,   int toC,
                     Piece* board[8][8]) const override;
};

// ============================================================
//  Derived Class: Rook
// ============================================================
class Rook : public Piece
{
public:
    Rook(Color c) : Piece(c, ROOK) {}

    bool isValidMove(int fromR, int fromC,
                     int toR,   int toC,
                     Piece* board[8][8]) const override;
};

// ============================================================
//  Derived Class: Knight
// ============================================================
class Knight : public Piece
{
public:
    Knight(Color c) : Piece(c, KNIGHT) {}

    bool isValidMove(int fromR, int fromC,
                     int toR,   int toC,
                     Piece* board[8][8]) const override;
};

// ============================================================
//  Derived Class: Bishop
// ============================================================
class Bishop : public Piece
{
public:
    Bishop(Color c) : Piece(c, BISHOP) {}

    bool isValidMove(int fromR, int fromC,
                     int toR,   int toC,
                     Piece* board[8][8]) const override;
};

// ============================================================
//  Derived Class: Queen
// ============================================================
class Queen : public Piece
{
public:
    Queen(Color c) : Piece(c, QUEEN) {}

    bool isValidMove(int fromR, int fromC,
                     int toR,   int toC,
                     Piece* board[8][8]) const override;
};

// ============================================================
//  Derived Class: King
// ============================================================
class King : public Piece
{
public:
    King(Color c) : Piece(c, KING) {}

    bool isValidMove(int fromR, int fromC,
                     int toR,   int toC,
                     Piece* board[8][8]) const override;
};