#include "TextRender.h"
#include <cstdlib>
#include <ctime>

TextRender::TextRender(int N, const sf::Font& font, unsigned int charSize, sf::Vector2u windowSize)
    : size_(windowSize)
{
    std::srand(unsigned(std::time(nullptr)));
    for (int i = 0; i < N; ++i) {
        sf::Text t;
        t.setFont(font);
        char c = (std::rand() % 2) ? '1' : '0';
        t.setString(std::string(1, c));
        t.setCharacterSize(charSize);
        
        // Asignar color pseudoaleatorio basado en índice
        t.setFillColor(generatePseudoRandomColor(i));
        
        float x = std::rand() % windowSize.x;
        float y = std::rand() % windowSize.y;
        t.setPosition(x, y);
        texts.push_back(t);
    }
}

sf::Color TextRender::generatePseudoRandomColor(int seed) {
    // Generador pseudoaleatorio para colores completamente aleatorios
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> colorChannel(50, 255); // Evitar colores muy oscuros
    
    int r = colorChannel(rng);
    int g = colorChannel(rng);
    int b = colorChannel(rng);
    
    return sf::Color(r, g, b);
}

void TextRender::render(sf::RenderWindow& window) {
    for (auto& t : texts) window.draw(t);
}

void TextRender::resize(sf::Vector2u newSize) {
    size_ = newSize;
}
