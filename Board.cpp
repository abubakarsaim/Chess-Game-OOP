// ============================================================
//  Board.cpp  –  Full chess board implementation
//  Covers: movement, castling, en-passant, promotion,
//          check/checkmate/stalemate detection, undo
// ============================================================
#include "Board.h"
#include <cmath>
#include <cstring>
#include <cstdio>   // sprintf

// ---- Color constants for the board squares ----
static const sf::Color LIGHT_SQ (240, 217, 181);
static const sf::Color DARK_SQ  (181, 136,  99);
static const sf::Color SEL_COLOR(100, 200, 100, 160);   // selected piece
static const sf::Color MOV_COLOR(100, 200, 100, 100);   // valid move dot
static const sf::Color CHK_COLOR(220,  50,  50, 180);   // king in check

// ============================================================
//  Constructor
// ============================================================
Board::Board()
{
    // Clear grid
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            grid[r][c] = nullptr;

    enPassantCol     = -1;
    enPassantRow     = -1;
    currentTurn      = WHITE;
    pieceSelected    = false;
    selectedRow      = -1;
    selectedCol      = -1;
    gameOver         = false;
    isCheck          = false;
    isCheckmate      = false;
    isStalemate      = false;
    historySize      = 0;
    awaitingPromotion= false;
    promoRow = promoCol = 0;
    promoColor       = WHITE;

    loadTextures();
    initBoard();
}

// ============================================================
//  Destructor  –  free all allocated pieces
// ============================================================
Board::~Board()
{
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            delete grid[r][c];
}

// ============================================================
//  loadTextures
//  Loads PNG files named like "wP.png", "bQ.png" etc.
//  Place the 12 PNG files in the same folder as the executable.
// ============================================================
void Board::loadTextures()
{
    // File naming convention: w/b + P/R/N/B/Q/K
    const char* colorPrefix[2] = { "w", "b" };
    const char* typeSuffix[6]  = { "P","R","N","B","Q","K" };

    for (int col = 0; col < 2; col++)
    {
        for (int t = 0; t < 6; t++)
        {
            char filename[32];
            sprintf(filename, "assets/%s%s.png", colorPrefix[col], typeSuffix[t]);

            if (!textures[col][t].loadFromFile(filename))
            {
                // If textures not found we still run (pieces show as colored squares)
            }
            else
            {
                sprites[col][t].setTexture(textures[col][t]);
                // Scale to fit inside a square
                sf::Vector2u sz = textures[col][t].getSize();
                float scaleX = (float)(SQUARE_SIZE - 8) / sz.x;
                float scaleY = (float)(SQUARE_SIZE - 8) / sz.y;
                sprites[col][t].setScale(scaleX, scaleY);
            }
        }
    }
}

// ============================================================
//  initBoard  –  Place all 32 pieces in starting positions
// ============================================================
void Board::initBoard()
{
    // ---- Black back rank (row 0) ----
    grid[0][0] = new Rook  (BLACK);
    grid[0][1] = new Knight(BLACK);
    grid[0][2] = new Bishop(BLACK);
    grid[0][3] = new Queen (BLACK);
    grid[0][4] = new King  (BLACK);
    grid[0][5] = new Bishop(BLACK);
    grid[0][6] = new Knight(BLACK);
    grid[0][7] = new Rook  (BLACK);

    // ---- Black pawns (row 1) ----
    for (int c = 0; c < 8; c++)
        grid[1][c] = new Pawn(BLACK);

    // ---- White pawns (row 6) ----
    for (int c = 0; c < 8; c++)
        grid[6][c] = new Pawn(WHITE);

    // ---- White back rank (row 7) ----
    grid[7][0] = new Rook  (WHITE);
    grid[7][1] = new Knight(WHITE);
    grid[7][2] = new Bishop(WHITE);
    grid[7][3] = new Queen (WHITE);
    grid[7][4] = new King  (WHITE);
    grid[7][5] = new Bishop(WHITE);
    grid[7][6] = new Knight(WHITE);
    grid[7][7] = new Rook  (WHITE);
}

// ============================================================
//  findKing
// ============================================================
void Board::findKing(Color c, int &row, int &col) const
{
    for (int r = 0; r < 8; r++)
        for (int cc = 0; cc < 8; cc++)
            if (grid[r][cc] &&
                grid[r][cc]->getType()  == KING &&
                grid[r][cc]->getColor() == c)
            {
                row = r; col = cc; return;
            }
    row = col = -1; // should never happen
}

// ============================================================
//  isSquareAttacked
//  Returns true if square (r,c) is attacked by any piece of
//  color byColor.
// ============================================================
bool Board::isSquareAttacked(int r, int c, Color byColor) const
{
    for (int fr = 0; fr < 8; fr++)
        for (int fc = 0; fc < 8; fc++)
        {
            Piece* p = grid[fr][fc];
            if (!p || p->getColor() != byColor) continue;

            // For pawn: attack squares differ from move squares
            if (p->getType() == PAWN)
            {
                int dir = (byColor == WHITE) ? -1 : 1;
                if (fr + dir == r && (fc - 1 == c || fc + 1 == c))
                    return true;
                continue;
            }

            if (p->isValidMove(fr, fc, r, c, grid))
                return true;
        }
    return false;
}

// ============================================================
//  kingInCheck
// ============================================================
bool Board::kingInCheck(Color kingColor) const
{
    int kr, kc;
    findKing(kingColor, kr, kc);
    if (kr == -1) return false;
    Color enemy = (kingColor == WHITE) ? BLACK : WHITE;
    return isSquareAttacked(kr, kc, enemy);
}

// ============================================================
//  isCastlingMove
//  Checks all castling conditions:
//   1. King has not moved
//   2. Rook has not moved
//   3. Squares between are empty
//   4. King is not in check
//   5. King does not pass through or land on an attacked square
// ============================================================
bool Board::isCastlingMove(int fromR, int fromC,
                           int toR,   int toC) const
{
    Piece* king = grid[fromR][fromC];
    if (!king || king->getType() != KING) return false;
    if (king->getHasMoved()) return false;
    if (fromR != toR) return false;

    int dc = toC - fromC;
    if (dc != 2 && dc != -2) return false;

    // King-side (dc==+2) or queen-side (dc==-2)
    int rookCol = (dc == 2) ? 7 : 0;
    Piece* rook = grid[fromR][rookCol];
    if (!rook || rook->getType() != ROOK) return false;
    if (rook->getHasMoved()) return false;
    if (rook->getColor() != king->getColor()) return false;

    // Squares between king and rook must be empty
    int minC = (rookCol < fromC) ? rookCol + 1 : fromC + 1;
    int maxC = (rookCol < fromC) ? fromC - 1 : rookCol - 1;
    for (int c = minC; c <= maxC; c++)
        if (grid[fromR][c] != nullptr) return false;

    // King must not be in check right now
    Color kColor = king->getColor();
    Color enemy  = (kColor == WHITE) ? BLACK : WHITE;
    if (isSquareAttacked(fromR, fromC, enemy)) return false;

    // King must not pass through an attacked square
    int step = (dc == 2) ? 1 : -1;
    if (isSquareAttacked(fromR, fromC + step,   enemy)) return false;
    if (isSquareAttacked(fromR, fromC + 2*step, enemy)) return false;

    return true;
}

// ============================================================
//  isEnPassantMove
// ============================================================
bool Board::isEnPassantMove(int fromR, int fromC,
                            int toR,   int toC) const
{
    Piece* p = grid[fromR][fromC];
    if (!p || p->getType() != PAWN) return false;
    if (enPassantCol == -1) return false;

    int dir = (p->getColor() == WHITE) ? -1 : 1;
    // The pawn must move diagonally to the en-passant column
    return (toR   == fromR + dir  &&
            toC   == enPassantCol &&
            fromR == enPassantRow); // must be on the same row as the captured pawn
}

// ============================================================
//  isLegalMove
//  Full legality check: piece rules + not leaving king in check
// ============================================================
bool Board::isLegalMove(int fromR, int fromC,
                        int toR,   int toC) const
{
    if (!Piece::inBounds(fromR,fromC) ||
        !Piece::inBounds(toR,  toC )) return false;

    Piece* p = grid[fromR][fromC];
    if (!p) return false;
    if (p->getColor() != currentTurn) return false;

    // Can't capture own piece
    if (grid[toR][toC] &&
        grid[toR][toC]->getColor() == currentTurn) return false;

    // ---- Castling special path ----
    bool castling = isCastlingMove(fromR, fromC, toR, toC);
    if (p->getType() == KING && abs(toC - fromC) == 2 && !castling)
        return false;

    // ---- En-passant special path ----
    bool enPassant = isEnPassantMove(fromR, fromC, toR, toC);

    if (!castling && !enPassant)
    {
        if (!p->isValidMove(fromR, fromC, toR, toC, grid))
            return false;
    }

    // ---- Simulate move and check if own king ends up in check ----
    // We temporarily alter the board, then restore it.
    // (Casting const away for simulation – logical const maintained)
    Board* self = const_cast<Board*>(this);

    Piece* savedTo   = self->grid[toR][toC];
    Piece* savedFrom = self->grid[fromR][fromC];
    Piece* epCapture = nullptr;

    self->grid[toR][toC]     = self->grid[fromR][fromC];
    self->grid[fromR][fromC] = nullptr;

    // Handle en-passant captured pawn removal
    if (enPassant)
    {
        epCapture = self->grid[enPassantRow][enPassantCol];
        self->grid[enPassantRow][enPassantCol] = nullptr;
    }

    // Handle castling rook move during simulation
    int castleRookFromC = -1, castleRookToC = -1;
    if (castling)
    {
        int dc = toC - fromC;
        castleRookFromC = (dc == 2) ? 7 : 0;
        castleRookToC   = (dc == 2) ? 5 : 3;
        self->grid[fromR][castleRookToC]   = self->grid[fromR][castleRookFromC];
        self->grid[fromR][castleRookFromC] = nullptr;
    }

    bool inCheck = self->kingInCheck(savedFrom->getColor());

    // ---- Restore board ----
    self->grid[fromR][fromC] = savedFrom;
    self->grid[toR][toC]     = savedTo;

    if (enPassant && epCapture)
        self->grid[enPassantRow][enPassantCol] = epCapture;

    if (castling)
    {
        self->grid[fromR][castleRookFromC]  = self->grid[fromR][castleRookToC];
        self->grid[fromR][castleRookToC]    = nullptr;
    }

    return !inCheck;
}

// ============================================================
//  hasLegalMoves  –  used for checkmate / stalemate detection
// ============================================================
bool Board::hasLegalMoves(Color forColor)
{
    Color savedTurn = currentTurn;
    currentTurn = forColor;

    for (int fr = 0; fr < 8; fr++)
        for (int fc = 0; fc < 8; fc++)
        {
            if (!grid[fr][fc]) continue;
            if (grid[fr][fc]->getColor() != forColor) continue;

            for (int tr = 0; tr < 8; tr++)
                for (int tc = 0; tc < 8; tc++)
                    if (isLegalMove(fr, fc, tr, tc))
                    {
                        currentTurn = savedTurn;
                        return true;
                    }
        }

    currentTurn = savedTurn;
    return false;
}

// ============================================================
//  executeMove  –  apply a validated move to the board
// ============================================================
void Board::executeMove(int fromR, int fromC, int toR, int toC)
{
    Piece* moving = grid[fromR][fromC];

    // ---- Record history for undo ----
    MoveRecord rec;
    rec.fromR            = fromR;
    rec.fromC            = fromC;
    rec.toR              = toR;
    rec.toC              = toC;
    rec.capturedPiece    = grid[toR][toC];
    rec.wasEnPassant     = false;
    rec.wasCastle        = false;
    rec.wasPromotion     = false;
    rec.promotedFrom     = PAWN;
    rec.pieceHadMoved    = moving->getHasMoved();
    rec.rookHadMoved     = false;
    rec.prevEnPassantCol = enPassantCol;
    rec.prevEnPassantRow = enPassantRow;

    // ---- En-passant capture ----
    bool ep = isEnPassantMove(fromR, fromC, toR, toC);
    if (ep)
    {
        rec.wasEnPassant  = true;
        rec.capturedPiece = grid[enPassantRow][enPassantCol];
        delete grid[enPassantRow][enPassantCol];  // actually remove captured pawn
        grid[enPassantRow][enPassantCol] = nullptr;
    }

    // ---- Reset en-passant ----
    enPassantCol = -1;
    enPassantRow = -1;

    // ---- Pawn two-square advance → set en-passant target ----
    if (moving->getType() == PAWN && abs(toR - fromR) == 2)
    {
        enPassantCol = toC;
        enPassantRow = toR;  // row of the pawn itself
    }

    // ---- Castling: move the rook ----
    bool castle = isCastlingMove(fromR, fromC, toR, toC);
    if (castle)
    {
        rec.wasCastle = true;
        int dc = toC - fromC;
        int rookFromC = (dc == 2) ? 7 : 0;
        int rookToC   = (dc == 2) ? 5 : 3;
        Piece* rook   = grid[fromR][rookFromC];
        rec.rookHadMoved = rook->getHasMoved();
        grid[fromR][rookToC]   = rook;
        grid[fromR][rookFromC] = nullptr;
        rook->setHasMoved(true);
    }

    // ---- Delete whatever was on destination (normal capture) ----
    if (!ep && grid[toR][toC])
    {
        delete grid[toR][toC];
        rec.capturedPiece = nullptr; // already deleted, don't double-free on undo
    }

    // ---- Move the piece ----
    grid[toR][toC]     = moving;
    grid[fromR][fromC] = nullptr;
    moving->setHasMoved(true);

    // ---- Pawn promotion ----
    if (moving->getType() == PAWN &&
        (toR == 0 || toR == 7))
    {
        rec.wasPromotion = true;
        // We flag awaiting promotion; actual piece swap happens after dialog
        awaitingPromotion = true;
        promoRow          = toR;
        promoCol          = toC;
        promoColor        = moving->getColor();
    }

    // ---- Save history ----
    if (historySize < MAX_HISTORY)
    {
        history[historySize] = rec;
        historySize++;
    }
}

// ============================================================
//  promoteAndPlace  –  replace pawn with chosen piece
// ============================================================
void Board::promoteAndPlace(PromotionChoice choice)
{
    if (!awaitingPromotion) return;

    Piece* oldPawn = grid[promoRow][promoCol];
    delete oldPawn;

    Piece* newPiece = nullptr;
    switch (choice)
    {
        case PROMO_QUEEN:  newPiece = new Queen (promoColor); break;
        case PROMO_ROOK:   newPiece = new Rook  (promoColor); break;
        case PROMO_BISHOP: newPiece = new Bishop(promoColor); break;
        case PROMO_KNIGHT: newPiece = new Knight(promoColor); break;
        default:           newPiece = new Queen (promoColor); break;
    }
    newPiece->setHasMoved(true);
    grid[promoRow][promoCol] = newPiece;

    awaitingPromotion = false;

    // Update game state after promotion
    currentTurn = (currentTurn == WHITE) ? BLACK : WHITE;
    isCheck     = kingInCheck(currentTurn);

    if (!hasLegalMoves(currentTurn))
    {
        gameOver = true;
        if (isCheck) isCheckmate = true;
        else         isStalemate = true;
    }
}

// ============================================================
//  undoLastMove
// ============================================================
void Board::undoLastMove()
{
    if (historySize == 0 || gameOver) return;

    historySize--;
    MoveRecord& rec = history[historySize];

    Piece* moving = grid[rec.toR][rec.toC];

    // Undo promotion: swap back to pawn
    if (rec.wasPromotion)
    {
        delete moving;
        moving = new Pawn(promoColor);  // restore pawn
        grid[rec.toR][rec.toC] = moving;
    }

    // Move piece back
    moving->setHasMoved(rec.pieceHadMoved);
    grid[rec.fromR][rec.fromC] = moving;
    grid[rec.toR  ][rec.toC  ] = nullptr;

    // Restore captured piece (normal capture)
    if (rec.capturedPiece)
        grid[rec.toR][rec.toC] = rec.capturedPiece;

    // Undo en-passant
    if (rec.wasEnPassant)
    {
        // Re-create the captured pawn
        Color capColor = (moving->getColor() == WHITE) ? BLACK : WHITE;
        grid[enPassantRow][enPassantCol] = new Pawn(capColor);
        grid[enPassantRow][enPassantCol]->setHasMoved(true);
    }

    // Undo castling rook
    if (rec.wasCastle)
    {
        int dc = rec.toC - rec.fromC;
        int rookCurC  = (dc == 2) ? 5 : 3;
        int rookOrigC = (dc == 2) ? 7 : 0;
        Piece* rook   = grid[rec.fromR][rookCurC];
        if (rook) rook->setHasMoved(rec.rookHadMoved);
        grid[rec.fromR][rookOrigC] = rook;
        grid[rec.fromR][rookCurC ] = nullptr;
    }

    // Restore en-passant state
    enPassantCol = rec.prevEnPassantCol;
    enPassantRow = rec.prevEnPassantRow;

    // Flip turn back
    currentTurn = (currentTurn == WHITE) ? BLACK : WHITE;
    isCheck     = kingInCheck(currentTurn);
    gameOver    = false;
    isCheckmate = false;
    isStalemate = false;
    pieceSelected = false;
}

// ============================================================
//  handleClick  –  called with pixel coordinates from main
// ============================================================
void Board::handleClick(int pixelX, int pixelY)
{
    if (gameOver || awaitingPromotion) return;

    int col = pixelX / SQUARE_SIZE;
    int row = pixelY / SQUARE_SIZE;
    if (!Piece::inBounds(row, col)) return;

    // ---- If a piece is already selected ----
    if (pieceSelected)
    {
        // Clicking same square: deselect
        if (row == selectedRow && col == selectedCol)
        {
            pieceSelected = false;
            return;
        }

        // Try to move
        if (isLegalMove(selectedRow, selectedCol, row, col))
        {
            executeMove(selectedRow, selectedCol, row, col);
            pieceSelected = false;

            if (!awaitingPromotion)
            {
                // Switch turn and check game state
                currentTurn = (currentTurn == WHITE) ? BLACK : WHITE;
                isCheck     = kingInCheck(currentTurn);

                if (!hasLegalMoves(currentTurn))
                {
                    gameOver = true;
                    if (isCheck) isCheckmate = true;
                    else         isStalemate = true;
                }
            }
            return;
        }

        // Clicked another own piece: select it instead
        if (grid[row][col] &&
            grid[row][col]->getColor() == currentTurn)
        {
            selectedRow = row;
            selectedCol = col;
            return;
        }

        // Invalid move: deselect
        pieceSelected = false;
        return;
    }

    // ---- No piece selected yet ----
    if (grid[row][col] &&
        grid[row][col]->getColor() == currentTurn)
    {
        pieceSelected = true;
        selectedRow   = row;
        selectedCol   = col;
    }
}

// ============================================================
//  handlePromoClick  –  called when awaiting promotion
// ============================================================
void Board::handlePromoClick(int pixelX, int pixelY)
{
    if (!awaitingPromotion) return;

    // Dialog drawn at centre of screen
    // Buttons: Q  R  B  N  each SQUARE_SIZE wide
    int dialogX = (8 * SQUARE_SIZE - 4 * SQUARE_SIZE) / 2;
    int dialogY = (8 * SQUARE_SIZE - SQUARE_SIZE) / 2;

    if (pixelY < dialogY || pixelY > dialogY + SQUARE_SIZE) return;
    if (pixelX < dialogX || pixelX > dialogX + 4*SQUARE_SIZE) return;

    int idx = (pixelX - dialogX) / SQUARE_SIZE;
    PromotionChoice choices[4] = {PROMO_QUEEN, PROMO_ROOK,
                                   PROMO_BISHOP, PROMO_KNIGHT};
    if (idx >= 0 && idx < 4)
        promoteAndPlace(choices[idx]);
}

// ============================================================
//  handleUndoKey
// ============================================================
void Board::handleUndoKey()
{
    undoLastMove();
}

// ============================================================
//  drawHighlights
// ============================================================
void Board::drawHighlights(sf::RenderWindow& window) const
{
    if (!pieceSelected) return;

    sf::RectangleShape sq(sf::Vector2f(SQUARE_SIZE, SQUARE_SIZE));

    // Highlight selected square
    sq.setFillColor(SEL_COLOR);
    sq.setPosition(selectedCol * SQUARE_SIZE, selectedRow * SQUARE_SIZE);
    window.draw(sq);

    // Highlight king if in check
    if (isCheck)
    {
        int kr, kc;
        const_cast<Board*>(this)->findKing(currentTurn, kr, kc);
        sq.setFillColor(CHK_COLOR);
        sq.setPosition(kc * SQUARE_SIZE, kr * SQUARE_SIZE);
        window.draw(sq);
    }

    // Show valid move targets as small dots
    sf::CircleShape dot(12.f);
    dot.setFillColor(sf::Color(50, 150, 50, 180));

    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (const_cast<Board*>(this)->isLegalMove(
                    selectedRow, selectedCol, r, c))
            {
                float px = c * SQUARE_SIZE + SQUARE_SIZE/2.f - 12.f;
                float py = r * SQUARE_SIZE + SQUARE_SIZE/2.f - 12.f;
                dot.setPosition(px, py);
                window.draw(dot);
            }
}

// ============================================================
//  drawPromoDialog
// ============================================================
void Board::drawPromoDialog(sf::RenderWindow& window,
                             sf::Font& font) const
{
    int dW  = 4 * SQUARE_SIZE;
    int dH  = SQUARE_SIZE + 40;
    int dX  = (8 * SQUARE_SIZE - dW) / 2;
    int dY  = (8 * SQUARE_SIZE - dH) / 2;

    // Background
    sf::RectangleShape bg(sf::Vector2f(dW, dH));
    bg.setFillColor(sf::Color(30,30,30,230));
    bg.setPosition(dX, dY);
    window.draw(bg);

    // Title
    sf::Text title;
    title.setFont(font);
    title.setString("Promote pawn:");
    title.setCharacterSize(18);
    title.setFillColor(sf::Color::White);
    title.setPosition(dX + 10, dY + 5);
    window.draw(title);

    // Four choice buttons
    const char* labels[4] = {"Queen","Rook","Bishop","Knight"};
    for (int i = 0; i < 4; i++)
    {
        sf::RectangleShape btn(sf::Vector2f(SQUARE_SIZE - 4, SQUARE_SIZE - 4));
        btn.setFillColor(sf::Color(80, 80, 200));
        btn.setPosition(dX + i * SQUARE_SIZE + 2, dY + 32);
        window.draw(btn);

        sf::Text lbl;
        lbl.setFont(font);
        lbl.setString(labels[i]);
        lbl.setCharacterSize(13);
        lbl.setFillColor(sf::Color::White);
        lbl.setPosition(dX + i * SQUARE_SIZE + 4, dY + 38);
        window.draw(lbl);
    }
}

// ============================================================
//  draw  –  main render function
// ============================================================
void Board::draw(sf::RenderWindow& window, sf::Font& font)
{
    // ---- 1. Draw the board squares ----
    sf::RectangleShape sq(sf::Vector2f(SQUARE_SIZE, SQUARE_SIZE));
    for (int r = 0; r < 8; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            bool isLight = (r + c) % 2 == 0;
            sq.setFillColor(isLight ? LIGHT_SQ : DARK_SQ);
            sq.setPosition(c * SQUARE_SIZE, r * SQUARE_SIZE);
            window.draw(sq);
        }
    }

    // ---- 2. Draw check highlight ----
    if (isCheck && !pieceSelected)
    {
        int kr, kc;
        findKing(currentTurn, kr, kc);
        sq.setFillColor(CHK_COLOR);
        sq.setPosition(kc * SQUARE_SIZE, kr * SQUARE_SIZE);
        window.draw(sq);
    }

    // ---- 3. Draw selection + move highlights ----
    drawHighlights(window);

    // ---- 4. Draw rank/file labels ----
    sf::Text label;
    label.setFont(font);
    label.setCharacterSize(12);
    for (int i = 0; i < 8; i++)
    {
        // Rank numbers (1-8)
        char rankChar[2] = { (char)('8' - i), '\0' };
        label.setString(rankChar);
        label.setFillColor((i % 2 == 0) ? DARK_SQ : LIGHT_SQ);
        label.setPosition(2, i * SQUARE_SIZE + 2);
        window.draw(label);

        // File letters (a-h)
        char fileChar[2] = { (char)('a' + i), '\0' };
        label.setString(fileChar);
        label.setFillColor((i % 2 == 0) ? LIGHT_SQ : DARK_SQ);
        label.setPosition(i * SQUARE_SIZE + SQUARE_SIZE - 14,
                           8 * SQUARE_SIZE - 16);
        window.draw(label);
    }

    // ---- 5. Draw pieces ----
    for (int r = 0; r < 8; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            Piece* p = grid[r][c];
            if (!p) continue;

            int colIdx  = (p->getColor() == WHITE) ? 0 : 1;
            int typeIdx = (int)p->getType();  // PAWN=0 … KING=5

            sf::Sprite& sp = sprites[colIdx][typeIdx];

            // Fallback: if texture not loaded, draw a colored square
            if (textures[colIdx][typeIdx].getSize().x == 0)
            {
                sf::RectangleShape fallback(sf::Vector2f(SQUARE_SIZE-10, SQUARE_SIZE-10));
                fallback.setFillColor(
                    (p->getColor() == WHITE) ? sf::Color(255,255,255,220)
                                             : sf::Color(50,50,50,220));
                fallback.setOutlineColor(sf::Color::Black);
                fallback.setOutlineThickness(2);
                fallback.setPosition(c*SQUARE_SIZE+5, r*SQUARE_SIZE+5);
                window.draw(fallback);

                // Draw type letter
                const char* typeLetters[6] = {"P","R","N","B","Q","K"};
                sf::Text tl;
                tl.setFont(font);
                tl.setString(typeLetters[typeIdx]);
                tl.setCharacterSize(28);
                tl.setFillColor(
                    (p->getColor() == WHITE) ? sf::Color::Black
                                             : sf::Color::White);
                tl.setPosition(c*SQUARE_SIZE+24, r*SQUARE_SIZE+18);
                window.draw(tl);
            }
            else
            {
                sp.setPosition(c * SQUARE_SIZE + 4.f,
                               r * SQUARE_SIZE + 4.f);
                window.draw(sp);
            }
        }
    }

    // ---- 6. Status bar below the board ----
    sf::RectangleShape statusBar(sf::Vector2f(8*SQUARE_SIZE, 40));
    statusBar.setFillColor(sf::Color(30,30,30));
    statusBar.setPosition(0, 8*SQUARE_SIZE);
    window.draw(statusBar);

    sf::Text status;
    status.setFont(font);
    status.setCharacterSize(18);
    status.setFillColor(sf::Color::White);

    if (isCheckmate)
    {
        const char* winner = (currentTurn == WHITE) ? "Black" : "White";
        char msg[64];
        sprintf(msg, "Checkmate! %s wins! (U=Undo)", winner);
        status.setString(msg);
    }
    else if (isStalemate)
    {
        status.setString("Stalemate! It's a Draw!");
    }
    else if (isCheck)
    {
        const char* t = (currentTurn == WHITE) ? "White" : "Black";
        char msg[64];
        sprintf(msg, "%s is in CHECK! (U=Undo)", t);
        status.setFillColor(sf::Color(255, 100, 100));
        status.setString(msg);
    }
    else
    {
        const char* t = (currentTurn == WHITE) ? "White" : "Black";
        char msg[64];
        sprintf(msg, "%s's turn  (U=Undo)", t);
        status.setString(msg);
    }
    status.setPosition(8, 8*SQUARE_SIZE + 10);
    window.draw(status);

    // ---- 7. Promotion dialog ----
    if (awaitingPromotion)
        drawPromoDialog(window, font);
}