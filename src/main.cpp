#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <regex>
#include <cstdlib>
#include <filesystem>
#include "TextRender.h"

// -------------------- CLI --------------------
enum class MotionMode; // ya viene de TextRender.h, pero evitamos warnings

struct CliOptions {
    int nChars = 200;
    int width  = 800;
    int height = 600;
    bool forceSequential = false;
    int threads = 0;
    MotionMode mode = MotionMode::Rain; // rain por defecto
    float speed = 160.f;                // usado por rain/bounce
};

static void print_usage(const char* prog) {
    std::cout
        << "Uso: " << prog << " [opciones] [N] [ANCHOxALTO]\n\n"
        << "Posicionales:\n"
        << "  N               Numero de caracteres (defecto: 200)\n"
        << "  ANCHOxALTO      Resolucion, p.ej. 1024x768 (defecto: 800x600)\n\n"
        << "Opciones:\n"
        << "  --seq                 Fuerza modo secuencial\n"
        << "  --threads K           Sugerir K hilos (para paralelo futuro)\n"
        << "  --mode rain|bounce|spiral   Modo de movimiento (defecto: rain)\n"
        << "  --speed V             Velocidad base (px/s, defecto: 160)\n"
        << "  -h, --help            Ayuda\n\n"
        << "Ejemplos:\n"
        << "  " << prog << "\n"
        << "  " << prog << " 150 1024x768 --mode rain --speed 220\n"
        << "  " << prog << " --mode bounce\n";
}

static bool parse_resolution(const std::string& s, int& w, int& h) {
    std::regex re(R"(^\s*(\d+)\s*[xX]\s*(\d+)\s*$)");
    std::smatch m;
    if (std::regex_match(s, m, re)) {
        w = std::stoi(m[1]);
        h = std::stoi(m[2]);
        return (w >= 160 && h >= 120 && w <= 10000 && h <= 10000);
    }
    return false;
}

static bool file_exists(const std::string& path) {
    namespace fs = std::filesystem;
    std::error_code ec;
    return fs::exists(path, ec) && fs::is_regular_file(path, ec);
}

static bool parse_cli(int argc, char** argv, CliOptions& opts) {
    // ayuda
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        }
    }
    // opciones
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--seq") { opts.forceSequential = true; continue; }
        if (a == "--threads") {
            if (i + 1 >= argc) { std::cerr << "Error: --threads K\n"; return false; }
            int k = std::atoi(argv[++i]);
            if (k <= 0 || k > 1024) { std::cerr << "Error: --threads 1..1024\n"; return false; }
            opts.threads = k; continue;
        }
        if (a == "--mode") {
            if (i + 1 >= argc) { std::cerr << "Error: --mode requiere valor.\n"; return false; }
            std::string m = argv[++i];
            if      (m == "rain")   opts.mode = MotionMode::Rain;
            else if (m == "bounce") opts.mode = MotionMode::Bounce;
            else if (m == "spiral") opts.mode = MotionMode::Spiral;
            else { std::cerr << "Error: --mode {rain|bounce|spiral}\n"; return false; }
            continue;
        }
        if (a == "--speed") {
            if (i + 1 >= argc) { std::cerr << "Error: --speed V\n"; return false; }
            float v = std::atof(argv[++i]);
            if (v <= 0.f || v > 5000.f) { std::cerr << "Error: --speed 1..5000\n"; return false; }
            opts.speed = v; continue;
        }
    }
    // posicionales: N y ANCHOxALTO (en cualquier orden)
    int pos = 0;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--seq" || a == "--threads" || a == "--mode" || a == "--speed" || a == "-h" || a == "--help") {
            if (a == "--threads" || a == "--mode" || a == "--speed") ++i; // saltar valor
            continue;
        }
        if (pos < 2) {
            int w, h;
            if (parse_resolution(a, w, h)) { opts.width = w; opts.height = h; }
            else {
                int n = std::atoi(a.c_str());
                if (n <= 0 || n > 200000) { std::cerr << "N invalido (1..200000)\n"; return false; }
                opts.nChars = n;
            }
            ++pos;
        } else {
            std::cerr << "Demasiados argumentos posicionales.\n";
            return false;
        }
    }
    // defensivo
    if (1LL * opts.width * opts.height > 10000LL * 10000LL) { std::cerr << "Resolucion gigante.\n"; return false; }
    if (opts.nChars > opts.width * opts.height) {
        std::cerr << "Advertencia: N > pixeles; ajustando N.\n";
        opts.nChars = opts.width * opts.height;
    }
    if (!file_exists("assets/fonts/Matrix-MZ4P.ttf")) {
        std::cerr << "No se encontro assets/fonts/Matrix-MZ4P.ttf. Ejecuta desde la raiz.\n";
        return false;
    }
    return true;
}
// -------------------- FIN CLI --------------------

static int run_loop(const CliOptions& opts, bool vsync = true) {
    sf::RenderWindow window(sf::VideoMode(opts.width, opts.height), "Matrix N caracteres");
    if (vsync) window.setVerticalSyncEnabled(true); else window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("assets/fonts/Matrix-MZ4P.ttf")) {
        std::cerr << "Error cargando fuente.\n"; return EXIT_FAILURE;
    }

    TextRender renderer(opts.nChars, font, 24, window.getSize(), opts.mode, opts.speed);
    sf::Clock clock;

    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();
            if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape) window.close();
            if (e.type == sf::Event::Resized) {
                sf::FloatRect visible(0.f, 0.f, float(e.size.width), float(e.size.height));
                window.setView(sf::View(visible));
                renderer.resize({ e.size.width, e.size.height });
            }
        }

        float dt = clock.restart().asSeconds();
        renderer.update(dt);

        window.clear(sf::Color::Black);
        renderer.render(window);
        window.display();
    }
    return EXIT_SUCCESS;
}

// Por ahora iguales; aquí enchufas paralelismo real si lo necesitas
static int run_sequential(const CliOptions& opts) { return run_loop(opts); }
static int run_parallel  (const CliOptions& opts) { (void)opts.threads; return run_loop(opts); }

int main(int argc, char** argv) {
    CliOptions opts;
    if (!parse_cli(argc, argv, opts)) { print_usage(argv[0]); return EXIT_FAILURE; }
    return opts.forceSequential ? run_sequential(opts) : run_parallel(opts);
}
