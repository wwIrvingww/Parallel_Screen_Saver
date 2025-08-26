#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <string>

// Ahora incluye Nebula
enum class MotionMode { Bounce, Spiral, Rain, Nebula };
enum class Palette    { Mono, Neon, Rainbow }; // ignorado en Rain

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
    // --------- Partículas (Bounce/Spiral/Nebula) ---------
    struct Particle {
        sf::Text text;
        sf::Vector2f pos;
        sf::Vector2f vel;
        float baseSize;

        // Usados por Spiral
        float angle, angVel, baseRadius, radiusAmp;
        float z, zVel, phase;

        // Usados por Nebula
        float spinDeg;     // rotacion propia
        float spinVelDeg;  // vel de giro
        float scale;       // escala actual (pulsacion)
        float scaleVel;    // vel de cambio de escala
        float alpha;       // 0..255
        float alphaVel;    // vel de alpha
        float noiseSeed;   // semilla para campo de flujo
    };
    std::vector<Particle> ps;

    // --------- Lluvia Matrix ---------
    struct Drop {
        std::vector<sf::Text> glyphs;
        float x;
        float headY;
        float spacing;
    };
    std::vector<Drop> drops;

    // --------- Lineas punteadas ---------
    struct DashLine {
        std::vector<sf::Text> dots;
        float xLeft;
        float y;
        float vx;
        float spacing;
        float dotWidth;
    };
    std::vector<DashLine> dashes;

    // --------- Estado general ---------
    sf::Vector2u size_;
    MotionMode mode_;
    float speed_;
    unsigned int charSize_;
    float time_ = 0.f;
    std::string alphabet_ = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    Palette palette_;

    // Helpers color Matrix
    sf::Color neonGreen(unsigned char a = 255) const { return sf::Color(0, 255, 70, a); }
    sf::Color headColor() const { return sf::Color(230, 255, 230); }

    // Colores pseudoaleatorios
    sf::Color generatePseudoRandomColor(int seed);

    // Inicializaciones
    void initParticles(int N, const sf::Font& font);         // Bounce/Spiral base
    void initNebula(int N, const sf::Font& font);            // NUEVO
    void initRain(int approxTotalGlyphs, const sf::Font& font);
    void initDashes(const sf::Font& font, int count);

    // Actualizaciones
    void updateBounce(Particle& p, float dt);
    void updateSpiral(Particle& p, float dt);
    void updateNebula(Particle& p, float dt);                 // NUEVO
    void updateRain(float dt);
    void updateDashes(float dt);

    // Utilidad Nebula
    sf::Vector2f nebulaFlowField(const sf::Vector2f& p, float seed, float t) const;
    sf::Color nebulaColor(float t01, float alpha) const;      // gradiente
};