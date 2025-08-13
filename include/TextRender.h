#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <string>

enum class MotionMode { Bounce, Spiral, Rain };

class TextRender {
public:
    TextRender(int N,
               const sf::Font& font,
               unsigned int charSize,
               sf::Vector2u windowSize,
               MotionMode mode = MotionMode::Rain,
               float speed = 160.f);

    // Animación por frame (dt en segundos)
    void update(float dt);

    // Dibujo
    void render(sf::RenderWindow& window);

    // Resize de ventana
    void resize(sf::Vector2u newSize);

private:
    // --------- Partículas (para bounce/spiral) ---------
    struct Particle {
        sf::Text text;
        sf::Vector2f pos;
        sf::Vector2f vel;
        float baseSize;
        float angle, angVel, baseRadius, radiusAmp;
        float z, zVel, phase;
    };
    std::vector<Particle> ps;

    // --------- Lluvia Matrix (catarata) ---------
    struct Drop {
        std::vector<sf::Text> glyphs; // cabeza + cola
        float x;                      // columna X
        float headY;                  // Y de la cabeza
        float speed;                  // px/s
        float spacing;                // separación vertical
    };
    std::vector<Drop> drops;

    // --------- Estado general ---------
    sf::Vector2u size_;
    MotionMode mode_;
    float speed_;
    unsigned int charSize_;
    float time_ = 0.f;               // para animaciones suaves
    std::string alphabet_ = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    // Helpers de color
    sf::Color neonGreen(unsigned char alpha = 255) const { return sf::Color(0, 255, 70, alpha); }
    sf::Color headColor() const { return sf::Color(230, 255, 230); } // cabeza casi blanca

    // Tu generador anterior (lo mantenemos para otros modos)
    sf::Color generatePseudoRandomColor(int seed);

    // Actualizadores por modo
    void updateBounce(Particle& p, float dt);
    void updateSpiral(Particle& p, float dt);
    void updateRain(float dt);

    // Construcción por modo
    void initParticles(int N, const sf::Font& font);
    void initRain(int approxTotalGlyphs, const sf::Font& font);
};
