#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <string>
#include <mutex>

// Mantenemos estos enums para ser compatibles con tu main actual
enum class MotionMode { Bounce, Spiral, Rain };
enum class Palette    { Mono, Neon, Rainbow }; // ignorado en Rain (volvemos a verde clásico)

class TextRender {
public:
    TextRender(int N,
               const sf::Font& font,
               unsigned int charSize,
               sf::Vector2u windowSize,
               MotionMode mode = MotionMode::Rain,
               float speed = 160.f,
               Palette palette = Palette::Mono);

    void update(float dt);
    void render(sf::RenderWindow& window);
    void resize(sf::Vector2u newSize);

private:
    // --------- Partículas (para otros modos; las dejamos tal cual) ---------
    struct Particle {
        sf::Text text;
        sf::Vector2f pos;
        sf::Vector2f vel;
        float baseSize;
        float angle, angVel, baseRadius, radiusAmp;
        float z, zVel, phase;
    };
    std::vector<Particle> ps;

    // --------- Lluvia Matrix (columna con cabeza + cola) ---------
    struct Drop {
        std::vector<sf::Text> glyphs; // cabeza + cola
        float x;                      // X de la columna
        float headY;                  // Y de la cabeza
        float spacing;                // separación vertical
    };
    std::vector<Drop> drops;

    // --------- Líneas punteadas (". . . .") que rebotan ---------
    struct DashLine {
        std::vector<sf::Text> dots;   // cada punto "."
        float xLeft;                  // posición X del primer punto
        float y;                      // altura de la línea
        float vx;                     // velocidad horizontal (px/s), con rebote
        float spacing;                // separación entre puntos
        float dotWidth;               // ancho del carácter "."
    };
    std::vector<DashLine> dashes;

    // --------- Estado general ---------
    sf::Vector2u size_;
    MotionMode mode_;
    float speed_;
    unsigned int charSize_;
    float time_ = 0.f;
    std::string alphabet_ = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    Palette palette_; // ignorado en Rain (usamos verde clásico)
    
    // Thread-safe random number generation
    mutable std::mutex rng_mutex_;
    mutable std::mt19937 rng_;

    // Helpers color Matrix
    sf::Color neonGreen(unsigned char a = 255) const { return sf::Color(0, 255, 70, a); }
    sf::Color headColor() const { return sf::Color(230, 255, 230); } // cabeza casi blanca

    // Thread-safe random number generation
    float threadSafeRand(float min, float max) const;
    int threadSafeRandInt(int min, int max) const;

    // Generador (para otros modos)
    sf::Color generatePseudoRandomColor(int seed);

    // Inicializaciones
    void initParticles(int N, const sf::Font& font);
    void initRain(int approxTotalGlyphs, const sf::Font& font);
    void initDashes(const sf::Font& font, int count); // NUEVO

    // Actualizaciones
    void updateBounce(Particle& p, float dt);
    void updateSpiral(Particle& p, float dt);
    void updateRain(float dt);
    void updateDashes(float dt); // NUEVO
};
