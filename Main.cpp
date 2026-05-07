// ============================================================
//  main.cpp  –  Entry point, SFML window & game loop
// ============================================================
//
//  Controls:
//    Left-click      → select / move piece
//    U               → undo last move
//    R               → restart game
//    Escape          → quit
//
// ============================================================
#include <SFML/Graphics.hpp>
#include "Board.h"

int main()
{
    const int BOARD_PIXELS = 640;   // 8 * 80
    const int WIN_HEIGHT   = 680;   // board + 40px status bar

    // ---- Create window ----
    sf::RenderWindow window(
        sf::VideoMode(BOARD_PIXELS, WIN_HEIGHT),
        "Chess  –  SFML",
        sf::Style::Titlebar | sf::Style::Close);

    window.setFramerateLimit(60);

    // ---- Load font ----
    sf::Font font;
    // Try common system font locations
    if (!font.loadFromFile("assets/font.ttf"))
    if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
    if (!font.loadFromFile("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"))
    if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf"))
    {
        // Font not found: the game still works, text may not render
    }

    // ---- Create board ----
    Board* board = new Board();

    // ---- Game loop ----
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            // ---- Window close ----
            if (event.type == sf::Event::Closed)
                window.close();

            // ---- Key press ----
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape)
                    window.close();

                // Undo move
                if (event.key.code == sf::Keyboard::U)
                    board->handleUndoKey();

                // Restart
                if (event.key.code == sf::Keyboard::R)
                {
                    delete board;
                    board = new Board();
                }
            }

            // ---- Mouse click ----
            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left)
            {
                int mx = event.mouseButton.x;
                int my = event.mouseButton.y;

                if (board->needsPromotion())
                    board->handlePromoClick(mx, my);
                else
                    board->handleClick(mx, my);
            }
        }

        // ---- Render ----
        window.clear(sf::Color(20, 20, 20));
        board->draw(window, font);
        window.display();
    }

    delete board;
    return 0;
}