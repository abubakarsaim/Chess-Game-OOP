#pragma once

// ============================================================
//  Board.h  –  The chess board: state, rules, rendering
// ============================================================
#include <SFML/Graphics.hpp>
#include "Piece.h"

// ---- Promotion dialog state ----
enum PromotionChoice { PROMO_NONE, PROMO_QUEEN, PROMO_ROOK,
                       PROMO_BISHOP, PROMO_KNIGHT };

// ---- Simple move record (for move history / undo) ----
struct MoveRecord
{
    int  fromR, fromC, toR, toC;
    Piece* capturedPiece;          // nullptr if no capture
    bool   wasEnPassant;
    bool   wasCastle;
    bool   wasPromotion;
    PieceType promotedFrom;        // PAWN before promotion
    bool   pieceHadMoved;          // hasMoved flag before this move
    bool   rookHadMoved;           // for castling undo
    int    prevEnPassantCol;       // en-passant column before move
    int    prevEnPassantRow;       // en-passant row before move
};

// ============================================================
//  Board
// ============================================================
class Board
{
    // ---- 2-D array of piece pointers (nullptr = empty) ----
    Piece* grid[8][8];

    // ---- Textures for all 12 piece sprites ----
    sf::Texture textures[2][6]; // [color][pieceType]
    sf::Sprite  sprites [2][6];

    // ---- En-passant tracking ----
    // After a pawn moves two squares, we record the column
    // and the row of the pawn that can be captured en passant.
    int enPassantCol; // column of the pawn that just moved 2 squares (-1 = none)
    int enPassantRow; // row   of that pawn

    // ---- Turn tracking ----
    Color currentTurn; // WHITE or BLACK

    // ---- Selection state ----
    bool  pieceSelected;
    int   selectedRow, selectedCol;

    // ---- Game-status flags ----
    bool gameOver;
    bool isCheck;
    bool isCheckmate;
    bool isStalemate;

    // ---- Move history (for undo, max 200 half-moves) ----
    static const int MAX_HISTORY = 200;
    MoveRecord history[MAX_HISTORY];
    int        historySize;

    // ---- Promotion state ----
    bool  awaitingPromotion;
    int   promoRow, promoCol;
    Color promoColor;

    // ---- UI constants ----
    static const int SQUARE_SIZE = 80;

    // ---- Internal helpers ----
    void initBoard();
    void loadTextures();

    // Rule helpers
    bool isSquareAttacked(int r, int c, Color byColor) const;
    bool kingInCheck(Color kingColor) const;
    bool hasLegalMoves(Color forColor);

    bool isLegalMove(int fromR, int fromC, int toR, int toC) const;
    bool isCastlingMove(int fromR, int fromC, int toR, int toC) const;
    bool isEnPassantMove(int fromR, int fromC, int toR, int toC) const;

    // Execution helpers
    void executeMove(int fromR, int fromC, int toR, int toC);
    void undoLastMove();

    // Promotion
    void promoteAndPlace(PromotionChoice choice);

    // Find king position
    void findKing(Color c, int &row, int &col) const;

    // Highlight helpers
    void drawHighlights(sf::RenderWindow& window) const;
    void drawPromoDialog(sf::RenderWindow& window, sf::Font& font) const;

public:
    Board();
    ~Board();

    // ---- Main entry points called from main.cpp ----
    void handleClick(int pixelX, int pixelY);
    void handlePromoClick(int pixelX, int pixelY);
    void handleUndoKey();

    void draw(sf::RenderWindow& window, sf::Font& font);

    // ---- State queries ----
    bool isGameOver()      const { return gameOver;      }
    bool isInCheck()       const { return isCheck;       }
    bool isCheckmated()    const { return isCheckmate;   }
    bool isStalemated()    const { return isStalemate;   }
    bool needsPromotion()  const { return awaitingPromotion; }
    Color getTurn()        const { return currentTurn;   }
};