#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class TextRender {
    public:
        // N = número de caracteres,
        TextRender(int N, const sf::Font& font, unsigned int charSize, sf::Vector2u windowSize);
        void render(sf::RenderWindow& window);
    private:
        std::vector<sf::Text> texts;
};
