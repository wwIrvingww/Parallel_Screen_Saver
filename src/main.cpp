#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <regex>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include "TextRender.h"

#ifdef _OPENMP
  #include <omp.h>
#endif

static const char* mode_to_cstr(MotionMode m) {
    switch (m) {
        case MotionMode::Rain:   return "rain";
        case MotionMode::Bounce: return "bounce";
        case MotionMode::Spiral: return "spiral";
        case MotionMode::Nebula: return "nebula";
    }
    return "unknown";
}

struct CliOptions {
    int nChars = 200;
    int width  = 800;
    int height = 600;
    bool forceSequential = false;
    int threads = 0;
    MotionMode mode = MotionMode::Rain;
    float speed = 160.f;
    Palette palette = Palette::Mono;

    std::string benchPath;
    int benchFrames = 0;
};

static void print_usage(const char* prog) {
    std::cout
        << "Uso: " << prog << " [opciones] [N] [ANCHOxALTO]\n\n"
        << "Posicionales:\n"
        << "  N                     Numero de caracteres (defecto: 200)\n"
        << "  ANCHOxALTO            Resolucion, p.ej. 1024x768 (defecto: 800x600)\n\n"
        << "Opciones:\n"
        << "  --seq                 Fuerza modo secuencial\n"
        << "  --threads K           Sugerir K hilos\n"
        << "  --mode rain|bounce|spiral|nebula   (defecto: rain)\n"
        << "  --palette mono|neon|rainbow        (defecto: mono)\n"
        << "  --speed V             Velocidad base px/s (defecto: 160)\n"
        << "  --bench FILE          CSV de benchmark\n"
        << "  --bench-frames K      Detener tras K frames\n"
        << "  -h, --help            Ayuda\n";
}

static bool parse_resolution(const std::string& s, int& w, int& h) {
    std::regex re(R"(^\s*(\d+)\s*[xX]\s*(\d+)\s*$)");
    std::smatch m;
    if (std::regex_match(s, m, re)) {
        w = std::stoi(m[1]); h = std::stoi(m[2]);
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
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") { print_usage(argv[0]); std::exit(0); }
    }
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
            else if (m == "nebula") opts.mode = MotionMode::Nebula;
            else { std::cerr << "Error: --mode {rain|bounce|spiral|nebula}\n"; return false; }
            continue;
        }
        if (a == "--palette") {
            if (i + 1 >= argc) { std::cerr << "Error: --palette requiere valor.\n"; return false; }
            std::string p = argv[++i];
            if      (p == "mono")    opts.palette = Palette::Mono;
            else if (p == "neon")    opts.palette = Palette::Neon;
            else if (p == "rainbow") opts.palette = Palette::Rainbow;
            else { std::cerr << "Error: --palette {mono|neon|rainbow}\n"; return false; }
            continue;
        }
        if (a == "--speed") {
            if (i + 1 >= argc) { std::cerr << "Error: --speed V\n"; return false; }
            float v = std::atof(argv[++i]);
            if (v <= 0.f || v > 5000.f) { std::cerr << "Error: --speed 1..5000\n"; return false; }
            opts.speed = v; continue;
        }
        if (a == "--bench") {
            if (i + 1 >= argc) { std::cerr << "Error: --bench FILE\n"; return false; }
            opts.benchPath = argv[++i]; continue;
        }
        if (a == "--bench-frames") {
            if (i + 1 >= argc) { std::cerr << "Error: --bench-frames K\n"; return false; }
            int k = std::atoi(argv[++i]);
            if (k <= 0) { std::cerr << "Error: --bench-frames > 0\n"; return false; }
            opts.benchFrames = k; continue;
        }
    }
    int pos = 0;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--seq" || a == "--threads" || a == "--mode" || a == "--palette" || a == "--speed"
            || a == "--bench" || a == "--bench-frames" || a == "-h" || a == "--help") {
            if (a == "--threads" || a == "--mode" || a == "--palette" || a == "--speed"
                || a == "--bench" || a == "--bench-frames") ++i;
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
        } else { std::cerr << "Demasiados posicionales.\n"; return false; }
    }
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

// (resto del main igual que lo tenias)
#include <chrono>
static int run_loop(const CliOptions& opts, bool vsync = true) {
    namespace fs = std::filesystem;
    using clock_t = std::chrono::steady_clock;

    sf::RenderWindow window(sf::VideoMode(opts.width, opts.height), "Matrix N caracteres");
    if (vsync) window.setVerticalSyncEnabled(true);
    else window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("assets/fonts/Matrix-MZ4P.ttf")) {
        std::cerr << "Error cargando fuente.\n"; 
        return EXIT_FAILURE;
    }

    TextRender renderer(opts.nChars, font, 24, window.getSize(),
                        opts.mode, opts.speed, opts.palette);

    #ifdef _OPENMP
        int threads_eff = omp_get_max_threads();
    #else
        int threads_eff = 1;
    #endif
        const char* exec = opts.forceSequential ? "seq" : "omp";

    std::ofstream benchOut;
    const bool benchEnabled = !opts.benchPath.empty();
    if (benchEnabled) {
        bool newFile = !fs::exists(opts.benchPath);
        benchOut.open(opts.benchPath, std::ios::app);
        if (!benchOut) { std::cerr << "No pude abrir " << opts.benchPath << "\n"; return EXIT_FAILURE; }
        if (newFile) benchOut << "exec,mode,threads_req,threads_eff,width,height,N,speed,frame,dt_s,update_ms,render_ms,total_ms,fps\n";
    }

    sf::Clock dtClock;
    int frame = 0;

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

        float dt = dtClock.restart().asSeconds();

        auto t0 = clock_t::now();
        renderer.update(dt);
        auto t1 = clock_t::now();

        window.clear(sf::Color::Black);
        renderer.render(window);
        window.display();

        auto t2 = clock_t::now();

        double update_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        double render_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        double total_ms  = std::chrono::duration<double, std::milli>(t2 - t0).count();
        double fps       = (dt > 0.0f) ? (1.0 / dt) : 0.0;

        if (benchEnabled) {
            benchOut    << exec << ','
                        << mode_to_cstr(opts.mode) << ','
                        << 0 << ','
                        << threads_eff << ','
                        << opts.width << ','
                        << opts.height << ','
                        << opts.nChars << ','
                        << std::fixed << std::setprecision(3) << opts.speed << ','
                        << frame << ','
                        << std::setprecision(6) << dt << ','
                        << std::setprecision(3) << update_ms << ','
                        << render_ms << ','
                        << total_ms << ','
                        << std::setprecision(2) << fps
                        << '\n';
        }

        ++frame;
        if (opts.benchFrames > 0 && frame >= opts.benchFrames) window.close();
    }

    if (benchEnabled) benchOut.close();
    return EXIT_SUCCESS;
}

static int run_sequential(const CliOptions& opts) { return run_loop(opts); }

static int run_parallel(const CliOptions& opts) {
#ifdef _OPENMP
    if (opts.threads > 0) omp_set_num_threads(opts.threads);

    // Mensaje de arranque en una región paralela
    #pragma omp parallel
    {
        #pragma omp single
        {
            std::cout << "Iniciando ejecucion paralela con "
                      << omp_get_num_threads() << " hilos.\n";
        }
    }
#endif

    int result = 0;

#ifdef _OPENMP
    // Ejecuta el bucle principal dentro de una región paralela,
    // pero una sola hebra llama a run_loop
    #pragma omp parallel
    {
        #pragma omp single
        {
            result = run_loop(opts);
        }
        #pragma omp barrier
    }
#else
    result = run_loop(opts);
#endif

    return result;
}

int main(int argc, char** argv) {
    CliOptions opts;
    if (!parse_cli(argc, argv, opts)) { print_usage(argv[0]); return EXIT_FAILURE; }
    return opts.forceSequential ? run_sequential(opts) : run_parallel(opts);
}