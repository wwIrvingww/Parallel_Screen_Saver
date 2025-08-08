#include <SFML/Graphics.hpp>
#include "TextRender.h"  
#include <iostream>

int main(int argc, char** argv) {
    // N, ancho, alto
    int N = (argc > 1) ? std::atoi(argv[1]) : 100;
    unsigned int width  = (argc > 2) ? std::atoi(argv[2]) : 800;
    unsigned int height = (argc > 3) ? std::atoi(argv[3]) : 600;

    sf::RenderWindow window({width, height}, "Matrix N caracteres");

    sf::Font font;
    if (!font.loadFromFile("assets/fonts/Matrix-MZ4P.ttf")) {
        std::cerr << "Error al cargar fuente en assets/fonts/Matrix-MZ4P.ttf\n";
        return -1;
    }

    TextRender renderer(N, font, 24, window.getSize());

    // Bucle principal
    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed)
                window.close();
        }

        window.clear(sf::Color::Black);
        renderer.render(window);
        window.display();
    }

    return 0;
}
