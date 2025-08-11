#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class TextRender {
public:
    TextRender(int N, const sf::Font& font, unsigned int charSize, sf::Vector2u windowSize);
    void render(sf::RenderWindow& window);
    void resize(sf::Vector2u newSize);

private:
    std::vector<sf::Text> texts;
    sf::Vector2u size_;
};
