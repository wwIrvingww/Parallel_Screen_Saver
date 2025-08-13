#include "TextRender.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

// Aleatorio float
static float frand(float a, float b) { return a + (b - a) * (float(std::rand()) / float(RAND_MAX)); }

TextRender::TextRender(int N,
                       const sf::Font& font,
                       unsigned int charSize,
                       sf::Vector2u windowSize,
                       MotionMode mode,
                       float speed)
    : size_(windowSize),
      mode_(mode),
      speed_(speed),
      charSize_(charSize)
{
    std::srand(unsigned(std::time(nullptr)));
    if (mode_ == MotionMode::Rain) {
        initRain(std::max(1, N), font);
    } else {
        initParticles(std::max(1, N), font);
    }
}

sf::Color TextRender::generatePseudoRandomColor(int seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> colorChannel(50, 255);
    return sf::Color(colorChannel(rng), colorChannel(rng), colorChannel(rng));
}

void TextRender::update(float dt) {
    time_ += dt;
    switch (mode_) {
        case MotionMode::Bounce:
            for (auto& p : ps) updateBounce(p, dt);
            break;
        case MotionMode::Spiral:
            for (auto& p : ps) updateSpiral(p, dt);
            break;
        case MotionMode::Rain:
            updateRain(dt);
            break;
    }
}

void TextRender::render(sf::RenderWindow& window) {
    if (mode_ == MotionMode::Rain) {
        for (auto& d : drops)
            for (auto& g : d.glyphs) window.draw(g);
    } else {
        for (auto& p : ps) window.draw(p.text);
    }
}

void TextRender::resize(sf::Vector2u newSize) {
    size_ = newSize;
    // En rain, reconstruimos columnas para ajustarnos al nuevo ancho/alto
    if (mode_ == MotionMode::Rain) {
        // No necesitamos el font original porque sf::Text guarda referencia interna,
        // pero para reconstruir tomamos el font de cualquier glifo ya creado
        sf::Font const* f = nullptr;
        if (!drops.empty() && !drops[0].glyphs.empty()) f = drops[0].glyphs[0].getFont();
        if (f) initRain((int)std::accumulate(drops.begin(), drops.end(), 0,
                     [](int acc, const Drop& d){ return acc + (int)d.glyphs.size(); }), *f);
    }
}

// -------------------- Init para bounce/spiral --------------------
void TextRender::initParticles(int N, const sf::Font& font) {
    ps.clear(); ps.reserve(N);
    const float minR = 20.f;
    const float maxR = std::min(size_.x, size_.y) * 0.48f;

    for (int i = 0; i < N; ++i) {
        Particle p{};
        p.text.setFont(font);
        p.text.setString((std::rand() % 2) ? "1" : "0");
        p.baseSize = float(charSize_);
        p.text.setCharacterSize(unsigned(p.baseSize));
        // En modos no-rain mantenemos colores variados
        p.text.setFillColor(generatePseudoRandomColor(i));

        p.pos = { frand(0.f, float(size_.x)), frand(0.f, float(size_.y)) };

        const float ang = frand(0.f, 2.f * 3.14159265f);
        p.vel = { speed_ * std::cos(ang), speed_ * std::sin(ang) };

        p.angle     = frand(0.f, 2.f * 3.14159265f);
        p.angVel    = frand(0.6f, 1.6f) * ((std::rand() % 2) ? 1.f : -1.f);
        p.baseRadius= frand(minR, maxR);
        p.radiusAmp = frand(10.f, 60.f);
        p.z         = frand(-300.f, 300.f);
        p.zVel      = frand(-30.f, 30.f);
        p.phase     = frand(0.f, 2.f * 3.14159265f);

        p.text.setPosition(p.pos);
        ps.push_back(std::move(p));
    }
}

// -------------------- Init para lluvia Matrix --------------------
void TextRender::initRain(int approxTotalGlyphs, const sf::Font& font) {
    drops.clear();

    sf::Text probe("M", font, charSize_);
    float glyphW = probe.getLocalBounds().width;
    if (glyphW <= 0.f) glyphW = charSize_ * 0.6f;
    float spacing = std::max(14.f, charSize_ * 1.05f);

    const float cellW = std::max(8.f, glyphW * 1.1f);
    int cols = std::max(1, int(std::floor(size_.x / cellW)));

    int avgLen = std::max(6, approxTotalGlyphs / std::max(1, cols));
    drops.reserve(cols);

    for (int c = 0; c < cols; ++c) {
        Drop d{};
        d.x = (c + 0.5f) * cellW;
        d.speed = frand(speed_ * 0.6f, speed_ * 1.4f);
        d.spacing = spacing;

        int len = std::max(6, int(avgLen * frand(0.7f, 1.4f)));
        d.glyphs.clear();
        d.glyphs.reserve(len);

        const float H = float(size_.y);
        const float tail = (len - 1) * d.spacing;
        const float period = H + tail + d.spacing;

        // Reparte las columnas ya en su ciclo (lluvia “poblada” desde el inicio)
        d.headY = frand(-tail, H);

        for (int i = 0; i < len; ++i) {
            char ch = alphabet_[std::rand() % alphabet_.size()];
            sf::Text g(std::string(1, ch), font, charSize_);
            if (i == 0) {
                g.setFillColor(headColor());             // cabeza brillante
            } else {
                float t = float(i) / float(len);         // desvanecimiento cola
                unsigned char a = (unsigned char)std::clamp(255 - int(255 * t * 1.2f), 40, 255);
                g.setFillColor(neonGreen(a));
            }
            float y = d.headY - i * d.spacing;           // pos inicial (el wrap lo hace updateRain)
            g.setPosition(d.x, y);
            d.glyphs.push_back(std::move(g));
        }

        drops.push_back(std::move(d));
    }
}


// -------------------- Update: Rain --------------------
void TextRender::updateRain(float dt) {
    const float H = float(size_.y);

    for (size_t k = 0; k < drops.size(); ++k) {
        Drop& d = drops[k];

        d.headY += d.speed * dt;

        const int len = (int)d.glyphs.size();
        if (len <= 0) continue;

        const float tail = (len - 1) * d.spacing;
        const float period = H + tail + d.spacing;

        bool flicker = (std::rand() % 6 == 0);
        float wobble = std::sin(time_ * 2.f + float(k) * 0.37f) * 1.1f;

        for (int i = 0; i < len; ++i) {
            float y = d.headY - i * d.spacing;

            // Wrap continuo en el rango [-tail, H]
            float yWrapped = std::fmod(y + period, period);
            if (yWrapped < 0.f) yWrapped += period;
            y = yWrapped - tail;

            d.glyphs[i].setPosition(d.x + wobble, y);

            if (flicker && (std::rand() % 10 == 0)) {
                char ch = alphabet_[std::rand() % alphabet_.size()];
                d.glyphs[i].setString(std::string(1, ch));
            }
            if (i == 0) {
                sf::Color c = headColor();
                c.a = (unsigned char)std::clamp(200 + int(55 * std::sin(time_ * 6.f + k)), 160, 255);
                d.glyphs[i].setFillColor(c);
            }
        }

        // Si la cabeza se pasó, ajusta en un solo paso (sin while infinito)
        const float limit = H + d.spacing + tail;
        if (d.headY > limit) {
            const float over = d.headY - limit;
            const float steps = std::floor(over / period) + 1.f;
            d.headY -= steps * period;
            d.speed = frand(speed_ * 0.6f, speed_ * 1.4f);
        }
    }
}


// -------------------- Update: Bounce --------------------
void TextRender::updateBounce(Particle& p, float dt) {
    p.pos += p.vel * dt;

    const sf::FloatRect bounds = p.text.getLocalBounds();
    const float w = bounds.width, h = bounds.height;

    if (p.pos.x < 0.f) { p.pos.x = 0.f; p.vel.x = -p.vel.x; }
    if (p.pos.x + w > float(size_.x)) { p.pos.x = float(size_.x) - w; p.vel.x = -p.vel.x; }
    if (p.pos.y < 0.f) { p.pos.y = 0.f; p.vel.y = -p.vel.y; }
    if (p.pos.y + h > float(size_.y)) { p.pos.y = float(size_.y) - h; p.vel.y = -p.vel.y; }

    const float t = std::sin((p.pos.x + p.pos.y) * 0.01f);
    const float scale = 1.0f + 0.1f * t;
    p.text.setCharacterSize(unsigned(std::max(8.f, p.baseSize * scale)));

    p.text.setPosition(p.pos);
}

// -------------------- Update: Spiral --------------------
void TextRender::updateSpiral(Particle& p, float dt) {
    p.angle += p.angVel * dt;
    const float r = p.baseRadius + p.radiusAmp * std::sin(p.phase + p.angle * 0.9f);

    p.z += p.zVel * dt;
    const float Zmax = 350.f;
    if (p.z >  Zmax) { p.z =  Zmax; p.zVel = -std::abs(p.zVel); }
    if (p.z < -Zmax) { p.z = -Zmax; p.zVel =  std::abs(p.zVel); }

    const sf::Vector2f c(size_.x * 0.5f, size_.y * 0.5f);
    const float X = r * std::cos(p.angle);
    const float Y = r * std::sin(p.angle);

    const float f = 500.f;
    const float s = f / (f + p.z);
    const float x2d = c.x + X * s;
    const float y2d = c.y + Y * s + 25.f * dt;

    const float uiScale = std::clamp(s, 0.5f, 1.8f);
    p.text.setCharacterSize(unsigned(std::max(8.f, p.baseSize * uiScale)));

    sf::Color col = p.text.getFillColor();
    const float alpha = std::clamp(180.f + 70.f * (s - 1.f), 60.f, 255.f);
    col.a = static_cast<sf::Uint8>(alpha);
    p.text.setFillColor(col);

    p.text.setPosition(x2d, y2d);
}
