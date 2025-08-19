#include "TextRender.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <numeric> // std::accumulate

#ifdef _OPENMP
  #include <omp.h>
#endif

// Aleatorio float
static float frand(float a, float b) { return a + (b - a) * (float(std::rand()) / float(RAND_MAX)); }

TextRender::TextRender(int N,
                       const sf::Font& font,
                       unsigned int charSize,
                       sf::Vector2u windowSize,
                       MotionMode mode,
                       float speed,
                       Palette palette)
    : size_(windowSize),
      mode_(mode),
      speed_(speed),
      charSize_(charSize),
      palette_(palette)
{
    std::srand(unsigned(std::time(nullptr)));
    if (mode_ == MotionMode::Rain) {
        // Lluvia Matrix clásica (verde, infinita)
        initRain(std::max(1, N), font);
        // Líneas punteadas que rebotan — 4..7 líneas, velocidades aleatorias
        int dashCount = std::clamp(int(std::round(std::sqrt(float(std::max(1, N))) / 3.f)), 4, 7);
        initDashes(font, dashCount);
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
    if (mode_ == MotionMode::Rain) {
        updateRain(dt);
        updateDashes(dt);
    } else if (mode_ == MotionMode::Bounce) {
        #pragma omp parallel for schedule(static)
        for (auto& p : ps) updateBounce(p, dt);
    } else { // Spiral
        #pragma omp parallel for schedule(static)
        for (auto& p : ps) updateSpiral(p, dt);
    }
}

void TextRender::render(sf::RenderWindow& window) {
    if (mode_ == MotionMode::Rain) {
        for (auto& d : drops)
            for (auto& g : d.glyphs) window.draw(g);
        for (auto& L : dashes)
            for (auto& dot : L.dots) window.draw(dot);
    } else {
        for (auto& p : ps) window.draw(p.text);
    }
}

void TextRender::resize(sf::Vector2u newSize) {
    size_ = newSize;
    if (mode_ == MotionMode::Rain) {
        const sf::Font* f = nullptr;
        if (!drops.empty() && !drops[0].glyphs.empty()) f = drops[0].glyphs[0].getFont();
        if (!f && !dashes.empty() && !dashes[0].dots.empty()) f = dashes[0].dots[0].getFont();
        if (f) {
            int totalGlyphs = 0;
            #pragma omp parallel for reduction(+:totalGlyphs) schedule(static)
            for (int k = 0; k < (int)drops.size(); ++k) {
                totalGlyphs += (int)drops[k].glyphs.size();
            }

            initRain(totalGlyphs, *f);
            initDashes(*f, std::max(4, (int)dashes.size()));
        }
    }
}


// -------------------- Init: bounce/spiral (sin cambios “Matrix”) --------------------
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

// -------------------- Init: lluvia Matrix (clásica, verde, infinita) --------------------
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
        d.spacing = spacing;

        int len = std::max(6, int(avgLen * frand(0.7f, 1.4f)));
        d.glyphs.clear();
        d.glyphs.reserve(len);

        const float H = float(size_.y);
        const float tail = (len - 1) * d.spacing;

        // cabeza en un punto del ciclo [-tail, H]
        d.headY = frand(-tail, H);

        for (int i = 0; i < len; ++i) {
            char ch = alphabet_[std::rand() % alphabet_.size()];
            sf::Text g(std::string(1, ch), font, charSize_);
            if (i == 0) {
                g.setFillColor(headColor()); // cabeza
            } else {
                float t = float(i) / float(len);
                unsigned char a = (unsigned char)std::clamp(255 - int(255 * t * 1.2f), 40, 255);
                g.setFillColor(neonGreen(a)); // cola verde
            }
            float y = d.headY - i * d.spacing;
            g.setPosition(d.x, y);
            d.glyphs.push_back(std::move(g));
        }

        drops.push_back(std::move(d));
    }
}

void TextRender::updateRain(float dt) {
    const float H = float(size_.y);
    const int frame = int(time_ * 60.0f);

    // 1) Avanza la cabeza de cada columna (independiente por drop)
    #pragma omp parallel for schedule(static)
    for (int k = 0; k < (int)drops.size(); ++k) {
        drops[k].headY += std::max(80.f, speed_) * dt;
    }

    // 2) Tamaño rectangular para collapse(2): max longitud de cola
    int maxLen = 0;
    #pragma omp parallel for reduction(max:maxLen) schedule(static)
    for (int k = 0; k < (int)drops.size(); ++k) {
        int len = (int)drops[k].glyphs.size();
        if (len > maxLen) maxLen = len;
    }

    // 3) Posicionar TODOS los glifos con collapse(2); guardamos los "huecos"
    #pragma omp parallel for collapse(2) schedule(static)
    for (int k = 0; k < (int)drops.size(); ++k) {
        for (int i = 0; i < maxLen; ++i) {
            if (i >= (int)drops[k].glyphs.size()) continue; // fuera del tamaño real
            Drop& d = drops[k];

            const int len = (int)d.glyphs.size();
            const float tail   = (len - 1) * d.spacing;
            const float period = H + tail + d.spacing;

            // y conceptual + wrap continuo
            float y = d.headY - i * d.spacing;
            float yWrapped = std::fmod(y + period, period);
            if (yWrapped < 0.f) yWrapped += period;
            y = yWrapped - tail;

            // "flicker" determinista (evita RNG compartido en paralelo)
            bool flickCol = (((unsigned)k*73856093u ^ (unsigned)frame*19349663u) & 7u) == 0u;   // ~1/8
            bool flickGly = (((unsigned)i*83492791u ^ (unsigned)frame*2971215073u) % 10) == 0u; // ~1/10
            if (flickCol && flickGly) {
                char ch = alphabet_[(k + i + frame) % (int)alphabet_.size()];
                d.glyphs[i].setString(std::string(1, ch));
            }

            // pulso leve en cabeza
            if (i == 0) {
                sf::Color c = headColor();
                c.a = (unsigned char)std::clamp(200 + int(55 * std::sin(time_ * 6.f + k)), 160, 255);
                d.glyphs[i].setFillColor(c);
            }

            d.glyphs[i].setPosition(d.x, y);
        }
    }

    // 4) Ajuste de headY si se pasó varios periodos (por drop)
    #pragma omp parallel for schedule(static)
    for (int k = 0; k < (int)drops.size(); ++k) {
        Drop& d = drops[k];
        const int len = (int)d.glyphs.size();
        if (len <= 0) continue;

        const float tail   = (len - 1) * d.spacing;
        const float period = H + tail + d.spacing;
        const float limit  = H + tail + d.spacing;

        if (d.headY > limit) {
            float over  = d.headY - limit;
            float steps = std::floor(over / period) + 1.f;
            d.headY -= steps * period;
        }
    }
}


// -------------------- Init: líneas punteadas --------------------
void TextRender::initDashes(const sf::Font& font, int count) {
    dashes.clear();
    dashes.reserve(count);

    // Puntos más pequeños que los dígitos para que se sientan "líneas"
    unsigned int dotCharSize = std::max(10u, charSize_ / 2);

    sf::Text probe(".", font, dotCharSize);
    float dotW = probe.getLocalBounds().width;
    if (dotW <= 0.f) dotW = dotCharSize * 0.45f;

    for (int k = 0; k < count; ++k) {
        DashLine L{};
        L.dotWidth = dotW;
        L.spacing  = std::max(8.f, dotW * 1.8f);

        // Largo objetivo del segmento: 12%–28% del ancho (¡no toda la pantalla!)
        float frac     = frand(0.12f, 0.28f);
        float targetW  = std::max(120.f, float(size_.x) * frac);
        int   nDots    = std::max(5, int(std::round(targetW / L.spacing)));
        float totalW   = (nDots - 1) * L.spacing + L.dotWidth;

        // Altura y tiempo objetivo para cruzar de lado a lado
        L.y = frand(dotCharSize * 1.2f, float(size_.y) - dotCharSize * 1.8f);
        float travel = std::max(50.f, float(size_.x) - totalW); // recorrido real
        float T      = frand(2.5f, 5.0f);                       // segundos por cruce
        float vxMag  = travel / T;                              // px/s
        L.vx         = ((std::rand() % 2) ? vxMag : -vxMag);    // dirección aleatoria

        // Posición inicial dentro del rango útil
        L.xLeft = frand(0.f, float(size_.x) - totalW);

        // Construye los puntos
        L.dots.reserve(nDots);
        for (int i = 0; i < nDots; ++i) {
            sf::Text dot(".", font, dotCharSize);
            sf::Color c = neonGreen(220);
            if (i % 2 == 1) c.a = 180; // alterna un poco el alpha para más "dashy"
            dot.setFillColor(c);
            dot.setPosition(L.xLeft + i * L.spacing, L.y);
            L.dots.push_back(std::move(dot));
        }

        dashes.push_back(std::move(L));
    }
}

void TextRender::updateDashes(float dt) {
    // 1) Avanza cada línea y rebota (independiente)
    #pragma omp parallel for schedule(static)
    for (int li = 0; li < (int)dashes.size(); ++li) {
        DashLine& L = dashes[li];

        L.xLeft += L.vx * dt;

        const int n = (int)L.dots.size();
        if (n == 0) continue;

        float totalW    = (n - 1) * L.spacing + L.dotWidth;
        float leftBound  = 0.f;
        float rightBound = std::max(0.f, float(size_.x) - totalW);

        if (L.xLeft < leftBound)  { L.xLeft = leftBound;  L.vx =  std::fabs(L.vx); }
        if (L.xLeft > rightBound) { L.xLeft = rightBound; L.vx = -std::fabs(L.vx); }
    }

    // 2) Rectángulo para collapse: máximo número de puntos en una línea
    int maxDots = 0;
    #pragma omp parallel for reduction(max:maxDots) schedule(static)
    for (int li = 0; li < (int)dashes.size(); ++li) {
        int n = (int)dashes[li].dots.size();
        if (n > maxDots) maxDots = n;
    }

    // 3) Posiciona todos los puntos con collapse(2)
    #pragma omp parallel for collapse(2) schedule(static)
    for (int li = 0; li < (int)dashes.size(); ++li) {
        for (int i = 0; i < maxDots; ++i) {
            if (i >= (int)dashes[li].dots.size()) continue;
            DashLine& L = dashes[li];
            L.dots[i].setPosition(L.xLeft + i * L.spacing, L.y);
        }
    }
}



// -------------------- Update: Bounce (otros modos) --------------------
void TextRender::updateBounce(Particle& p, float dt) {
    #pragma omp critical
    {
        p.pos += p.vel * dt;
    }

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

// -------------------- Update: Spiral (otros modos) --------------------
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
