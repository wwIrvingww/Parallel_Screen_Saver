// src/TextRender.cpp
#include "TextRender.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>

#ifdef _OPENMP
  #include <omp.h>
#endif

// -------------------- util --------------------
static float frand(float a, float b) { return a + (b - a) * (float(std::rand()) / float(RAND_MAX)); }

// -------------------- ctor --------------------
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
        initRain(std::max(1, N), font);
        int dashCount = std::clamp(int(std::round(std::sqrt(float(std::max(1, N))) / 3.f)), 4, 7);
        initDashes(font, dashCount);
    } else if (mode_ == MotionMode::Nebula) {
        initNebula(std::max(1, N), font);

        // OBJ centrado en pantalla
        model_ = std::make_unique<ObjModel>();
        modelEnabled_ = model_->loadFromOBJ("assets/models/center.obj");
        if (!modelEnabled_) {
            std::cerr << "[OBJ] No se pudo cargar assets/models/center.obj\n";
        }

        // Estado inicial: o rota, o deriva en diagonal
        if (std::rand() % 2 == 0) {
            modelCtrl_.mode = ModelCtrl::Mode::RotateY;
            modelCtrl_.yawVelDeg = frand(40.f, 120.f) * ((std::rand()%2)? 1.f : -1.f);
            modelCtrl_.timer = frand(2.5f, 5.0f);
        } else {
            modelCtrl_.mode = ModelCtrl::Mode::Drift;
            float speedPix = frand(60.f, 160.f);
            float sx = (std::rand()%2 ? 1.f : -1.f);
            float sy = (std::rand()%2 ? 1.f : -1.f);
            float inv = 1.0f / std::sqrt(2.f);
            modelCtrl_.driftVel = { sx * speedPix * inv, sy * speedPix * inv };
            modelCtrl_.timer = frand(2.0f, 4.0f);
        }
        modelCtrl_.offset = {0.f, 0.f};
        modelCtrl_.returning = false;

    } else {
        initParticles(std::max(1, N), font);
    }
}

// -------------------- helpers --------------------
sf::Color TextRender::generatePseudoRandomColor(int seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> colorChannel(50, 255);
    return sf::Color(colorChannel(rng), colorChannel(rng), colorChannel(rng));
}

// -------------------- update/render/resize --------------------
void TextRender::update(float dt) {
    time_ += dt;
    if (mode_ == MotionMode::Rain) {
        updateRain(dt);
        updateDashes(dt);
    } else if (mode_ == MotionMode::Bounce) {
        #pragma omp parallel for schedule(static)
        for (auto& p : ps) updateBounce(p, dt);
    } else if (mode_ == MotionMode::Spiral) {
        #pragma omp parallel for schedule(static)
        for (auto& p : ps) updateSpiral(p, dt);
    } else { // Nebula
        #pragma omp parallel for schedule(static)
        for (auto& p : ps) updateNebula(p, dt);
        updateModel(dt);
    }
}

void TextRender::render(sf::RenderWindow& window) {
    if (mode_ == MotionMode::Rain) {
        for (auto& d : drops)
            for (auto& g : d.glyphs) window.draw(g);
        for (auto& L : dashes)
            for (auto& dot : L.dots) window.draw(dot);
    } else if (mode_ == MotionMode::Nebula) {
    for (auto& p : ps) window.draw(p.text);
    if (modelEnabled_ && model_) {
        // centro desplazado por el offset animado
        sf::Vector2f centerPx(
            float(size_.x) * 0.5f + modelCtrl_.offset.x,
            float(size_.y) * 0.5f + modelCtrl_.offset.y
        );

        // escala del modelo
        float scalePx = 0.40f * std::min(float(size_.x), float(size_.y));

        // convertir grados -> radianes
        float angleRad = modelCtrl_.yawDeg * 0.01745329252f; // pi/180

        // NUEVA llamada (reemplaza a drawProjectedYawAndOffset)
        model_->drawProjected(window, centerPx, scalePx, angleRad,
                              sf::Color(220, 220, 220, 235));
    }
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
            for (int k = 0; k < (int)drops.size(); ++k) totalGlyphs += (int)drops[k].glyphs.size();
            initRain(totalGlyphs, *f);
            initDashes(*f, std::max(4, (int)dashes.size()));
        }
    }
}

// -------------------- control del modelo (Nebula) --------------------
void TextRender::updateModel(float dt) {
    if (!modelEnabled_ || !model_) return;

    modelCtrl_.timer -= dt;

    if (modelCtrl_.mode == ModelCtrl::Mode::RotateY) {
        modelCtrl_.yawDeg += modelCtrl_.yawVelDeg * dt;

        // decaimiento exponencial del offset hacia el centro
        float k = 2.0f;
        modelCtrl_.offset.x *= std::exp(-k * dt);
        modelCtrl_.offset.y *= std::exp(-k * dt);

        if (modelCtrl_.timer <= 0.f) {
            // Cambiar a drift
            modelCtrl_.mode = ModelCtrl::Mode::Drift;
            float speedPix = frand(60.f, 160.f);
            float sx = (std::rand()%2 ? 1.f : -1.f);
            float sy = (std::rand()%2 ? 1.f : -1.f);
            float inv = 1.0f / std::sqrt(2.f);
            modelCtrl_.driftVel = { sx * speedPix * inv, sy * speedPix * inv };
            modelCtrl_.timer = frand(2.0f, 4.0f);
            modelCtrl_.returning = false;
        }
    } else { // Drift
        if (!modelCtrl_.returning) {
            modelCtrl_.offset += modelCtrl_.driftVel * dt;
            if (modelCtrl_.timer <= 0.f) modelCtrl_.returning = true;
        } else {
            // regreso suave al centro
            sf::Vector2f toCenter = {-modelCtrl_.offset.x, -modelCtrl_.offset.y};
            float len = std::sqrt(toCenter.x*toCenter.x + toCenter.y*toCenter.y);
            float retSpeed = 180.f; // px/s
            if (len > 1e-3f) {
                toCenter.x /= len; toCenter.y /= len;
                modelCtrl_.offset.x += toCenter.x * retSpeed * dt;
                modelCtrl_.offset.y += toCenter.y * retSpeed * dt;
            }
            if (std::abs(modelCtrl_.offset.x) < 2.f && std::abs(modelCtrl_.offset.y) < 2.f) {
                modelCtrl_.offset = {0.f, 0.f};
                modelCtrl_.returning = false;
                modelCtrl_.mode = ModelCtrl::Mode::RotateY;
                modelCtrl_.yawVelDeg = frand(40.f, 120.f) * ((std::rand()%2)? 1.f : -1.f);
                modelCtrl_.timer = frand(2.5f, 5.0f);
            }
        }
    }
}

// -------------------- init partículas (Bounce/Spiral) --------------------
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

        // Spiral
        p.angle     = frand(0.f, 2.f * 3.14159265f);
        p.angVel    = frand(0.6f, 1.6f) * ((std::rand() % 2) ? 1.f : -1.f);
        p.baseRadius= frand(minR, maxR);
        p.radiusAmp = frand(10.f, 60.f);
        p.z         = frand(-300.f, 300.f);
        p.zVel      = frand(-30.f, 30.f);
        p.phase     = frand(0.f, 2.f * 3.14159265f);

        // Nebula
        p.spinDeg   = frand(0.f, 360.f);
        p.spinVelDeg= frand(60.f, 220.f) * ((std::rand() % 2) ? 1.f : -1.f);
        p.scale     = frand(0.8f, 1.2f);
        p.scaleVel  = frand(-0.35f, 0.35f);
        p.alpha     = frand(140.f, 230.f);
        p.alphaVel  = frand(-25.f, 25.f);
        p.noiseSeed = frand(0.f, 1000.f);

        p.text.setPosition(p.pos);
        ps.push_back(std::move(p));
    }
}

// -------------------- init Nebula --------------------
void TextRender::initNebula(int N, const sf::Font& font) {
    ps.clear(); ps.reserve(N);
    const float densityScale = std::clamp(300.f / float(std::max(200, N)), 0.35f, 1.0f);

    for (int i = 0; i < N; ++i) {
        Particle p{};
        p.text.setFont(font);
        char ch = alphabet_[std::rand() % alphabet_.size()];
        p.text.setString(std::string(1, ch));

        p.baseSize = float(charSize_) * densityScale;
        p.text.setCharacterSize(unsigned(p.baseSize));

        float t01 = frand(0.f, 1.f);
        p.text.setFillColor(nebulaColor(t01, 200.f));

        p.pos = { frand(0.f, float(size_.x)), frand(0.f, float(size_.y)) };

        const float ang = frand(0.f, 6.2831853f);
        const float vmag = std::max(30.f, speed_) * frand(0.5f, 1.0f) * 0.5f;
        p.vel = { vmag * std::cos(ang), vmag * std::sin(ang) };

        // Spiral (no usado aquí)
        p.angle = 0.f; p.angVel = 0.0f; p.baseRadius = 0.f; p.radiusAmp = 0.f;
        p.z = 0.f; p.zVel = 0.f; p.phase = 0.f;

        // Nebula
        p.spinDeg    = frand(0.f, 360.f);
        p.spinVelDeg = frand(80.f, 260.f) * ((std::rand() % 2) ? 1.f : -1.f);
        p.scale      = frand(0.75f, 1.25f);
        p.scaleVel   = frand(-0.25f, 0.25f);
        p.alpha      = frand(120.f, 240.f);
        p.alphaVel   = frand(-35.f, 35.f);
        p.noiseSeed  = frand(0.f, 1000.f);

        p.text.setPosition(p.pos);
        ps.push_back(std::move(p));
    }
}

// -------------------- campo de flujo + color Nebula --------------------
sf::Vector2f TextRender::nebulaFlowField(const sf::Vector2f& p, float seed, float t) const {
    (void)p;
    float nx = std::sin(0.08f * t + seed * 0.71f);
    float ny = std::cos(0.11f * t + seed * 1.31f);
    return { 12.f * nx, 12.f * ny };
}

sf::Color TextRender::nebulaColor(float t01, float alpha) const {
    t01 = std::clamp(t01, 0.f, 1.f);
    sf::Color a(80,   0,   0);
    sf::Color b(180,  30,  30);
    sf::Color c(255,  60,  60);
    sf::Color d(255, 160,  60);

    auto lerp = [](const sf::Color& u, const sf::Color& v, float t){
        return sf::Color(
            (sf::Uint8)std::round(u.r + (v.r - u.r) * t),
            (sf::Uint8)std::round(u.g + (v.g - u.g) * t),
            (sf::Uint8)std::round(u.b + (v.b - u.b) * t),
            255
        );
    };

    sf::Color m;
    if (t01 < 0.33f)      m = lerp(a, b, t01 / 0.33f);
    else if (t01 < 0.66f) m = lerp(b, c, (t01 - 0.33f) / 0.33f);
    else                  m = lerp(c, d, (t01 - 0.66f) / 0.34f);

    m.a = (sf::Uint8)std::clamp((int)std::round(alpha), 0, 255);
    return m;
}

// -------------------- update Nebula (partículas) --------------------
void TextRender::updateNebula(Particle& p, float dt) {
    sf::Vector2f flow = nebulaFlowField(p.pos, p.noiseSeed, time_);
    p.vel.x += flow.x * dt;
    p.vel.y += flow.y * dt;

    float vmax = std::max(40.f, speed_) * 0.5f;
    float vlen = std::sqrt(p.vel.x*p.vel.x + p.vel.y*p.vel.y);
    if (vlen > vmax) { p.vel.x *= vmax/vlen; p.vel.y *= vmax/vlen; }

    p.pos += p.vel * dt;

    p.spinDeg += p.spinVelDeg * dt;
    p.scale += p.scaleVel * dt;
    if (p.scale < 0.7f) { p.scale = 0.7f; p.scaleVel = std::abs(p.scaleVel); }
    if (p.scale > 1.4f) { p.scale = 1.4f; p.scaleVel = -std::abs(p.scaleVel); }

    p.alpha += p.alphaVel * dt;
    if (p.alpha < 90.f)  { p.alpha = 90.f;  p.alphaVel = std::abs(p.alphaVel); }
    if (p.alpha > 255.f) { p.alpha = 255.f; p.alphaVel = -std::abs(p.alphaVel); }

    float t01 = 0.5f + 0.5f * std::sin(0.7f * time_ + p.noiseSeed * 0.9f);
    sf::Color col = nebulaColor(t01, p.alpha);
    p.text.setFillColor(col);

    sf::FloatRect lb = p.text.getLocalBounds();
    p.text.setOrigin(lb.left + lb.width * 0.5f, lb.top + lb.height * 0.5f);
    p.text.setPosition(p.pos);
    p.text.setRotation(p.spinDeg);
    p.text.setScale(p.scale, p.scale);
}

// -------------------- lluvia Matrix --------------------
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

        d.headY = frand(-tail, H);

        for (int i = 0; i < len; ++i) {
            char ch = alphabet_[std::rand() % alphabet_.size()];
            sf::Text g(std::string(1, ch), font, charSize_);
            if (i == 0) g.setFillColor(headColor());
            else {
                float t = float(i) / float(len);
                unsigned char a = (unsigned char)std::clamp(255 - int(255 * t * 1.2f), 40, 255);
                g.setFillColor(neonGreen(a));
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

    #pragma omp parallel for schedule(static)
    for (int k = 0; k < (int)drops.size(); ++k) {
        drops[k].headY += std::max(80.f, speed_) * dt;
    }

    int maxLen = 0;
    #pragma omp parallel for reduction(max:maxLen) schedule(static)
    for (int k = 0; k < (int)drops.size(); ++k) {
        int len = (int)drops[k].glyphs.size();
        if (len > maxLen) maxLen = len;
    }

    #pragma omp parallel for collapse(2) schedule(static)
    for (int k = 0; k < (int)drops.size(); ++k) {
        for (int i = 0; i < maxLen; ++i) {
            if (i >= (int)drops[k].glyphs.size()) continue;
            Drop& d = drops[k];

            const int len = (int)d.glyphs.size();
            const float tail   = (len - 1) * d.spacing;
            const float period = H + tail + d.spacing;

            float y = d.headY - i * d.spacing;
            float yWrapped = std::fmod(y + period, period);
            if (yWrapped < 0.f) yWrapped += period;
            y = yWrapped - tail;

            bool flickCol = (((unsigned)k*73856093u ^ (unsigned)frame*19349663u) & 7u) == 0u;
            bool flickGly = (((unsigned)i*83492791u ^ (unsigned)frame*2971215073u) % 10) == 0u;
            if (flickCol && flickGly) {
                char ch = alphabet_[(k + i + frame) % (int)alphabet_.size()];
                d.glyphs[i].setString(std::string(1, ch));
            }

            if (i == 0) {
                sf::Color c = headColor();
                c.a = (unsigned char)std::clamp(200 + int(55 * std::sin(time_ * 6.f + k)), 160, 255);
                d.glyphs[i].setFillColor(c);
            }

            d.glyphs[i].setPosition(d.x, y);
        }
    }

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

// -------------------- líneas punteadas --------------------
void TextRender::initDashes(const sf::Font& font, int count) {
    dashes.clear();
    dashes.reserve(count);

    unsigned int dotCharSize = std::max(10u, charSize_ / 2);

    sf::Text probe(".", font, dotCharSize);
    float dotW = probe.getLocalBounds().width;
    if (dotW <= 0.f) dotW = dotCharSize * 0.45f;

    for (int k = 0; k < count; ++k) {
        DashLine L{};
        L.dotWidth = dotW;
        L.spacing  = std::max(8.f, dotW * 1.8f);

        float frac     = frand(0.12f, 0.28f);
        float targetW  = std::max(120.f, float(size_.x) * frac);
        int   nDots    = std::max(5, int(std::round(targetW / L.spacing)));
        float totalW   = (nDots - 1) * L.spacing + L.dotWidth;

        L.y = frand(dotCharSize * 1.2f, float(size_.y) - dotCharSize * 1.8f);
        float travel = std::max(50.f, float(size_.x) - totalW);
        float T      = frand(2.5f, 5.0f);
        float vxMag  = travel / T;
        L.vx         = ((std::rand() % 2) ? vxMag : -vxMag);

        L.xLeft = frand(0.f, float(size_.x) - totalW);

        L.dots.reserve(nDots);
        for (int i = 0; i < nDots; ++i) {
            sf::Text dot(".", font, dotCharSize);
            sf::Color c = neonGreen(220);
            if (i % 2 == 1) c.a = 180;
            dot.setFillColor(c);
            dot.setPosition(L.xLeft + i * L.spacing, L.y);
            L.dots.push_back(std::move(dot));
        }

        dashes.push_back(std::move(L));
    }
}

void TextRender::updateDashes(float dt) {
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

    int maxDots = 0;
    #pragma omp parallel for reduction(max:maxDots) schedule(static)
    for (int li = 0; li < (int)dashes.size(); ++li) {
        int n = (int)dashes[li].dots.size();
        if (n > maxDots) maxDots = n;
    }

    #pragma omp parallel for collapse(2) schedule(static)
    for (int li = 0; li < (int)dashes.size(); ++li) {
        for (int i = 0; i < maxDots; ++i) {
            if (i >= (int)dashes[li].dots.size()) continue;
            DashLine& L = dashes[li];
            L.dots[i].setPosition(L.xLeft + i * L.spacing, L.y);
        }
    }
}

// -------------------- bounce/spiral --------------------
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
