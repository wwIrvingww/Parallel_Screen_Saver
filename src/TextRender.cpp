#include "./TextRender.h"
#include <cstdlib>
#include <ctime>

TextRender::TextRender(int N, const sf::Font& font, unsigned int charSize, sf::Vector2u windowSize)
{
    std::srand(unsigned(std::time(nullptr)));
    for (int i = 0; i < N; ++i) {
        sf::Text t;
        t.setFont(font);
        char c = (std::rand() % 2) ? '1' : '0';
        t.setString(std::string(1, c));
        t.setCharacterSize(charSize);
        t.setFillColor(sf::Color::Green);
        float x = std::rand() % windowSize.x;
        float y = std::rand() % windowSize.y;
        t.setPosition(x, y);
        texts.push_back(t);
    }
}

void TextRender::render(sf::RenderWindow& window)
{
    for (auto& t : texts)
        window.draw(t);
}
