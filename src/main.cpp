#include <SFML/Graphics.hpp>
#include "TextRender.h"  
#include <iostream>

//parseo de argumentos y progra defensiva
#include <iostream>
#include <string>
#include <regex>
#include <cstdlib>
#include <filesystem>

struct CliOptions {
    int nChars = 200;
    int width  = 800;
    int height = 600;
    bool forceSequential = false; // enlazar con código secuencial
    int threads = 0;        // 0 = auto / no forzado (para versión paralela)
};

static void print_usage(const char* prog) {
    std::cout
        << "Uso: " << prog << " [opciones] [N] [ANCHOxALTO]\n\n"
        << "Posicionales:\n"
        << "  N               Numero de caracteres a renderizar (defecto: 200)\n"
        << "  ANCHOxALTO      Resolucion de ventana, p.ej. 1024x768 (defecto: 800x600)\n\n"
        << "Opciones:\n"
        << "  --seq           Fuerza modo secuencial (enlaza con codigo secuencial)\n"
        << "  --threads K     Sugerir K hilos para modo paralelo (si aplica)\n"
        << "  -h, --help      Muestra esta ayuda\n\n"
        << "Ejemplos:\n"
        << "  " << prog << "\n"
        << "  " << prog << " 150 1024x768\n"
        << "  " << prog << " --seq 300\n"
        << "  " << prog << " --threads 8 400 1920x1080\n";
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
    // 1) Parse opciones tipo --flag/--threads
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        }
    }

    // Pasada 2: consumir opciones con argumento
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--seq") {
            opts.forceSequential = true;
            continue;
        }
        if (a == "--threads") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --threads requiere un valor.\n";
                return false;
            }
            int k = std::atoi(argv[++i]);
            if (k <= 0 || k > 1024) {
                std::cerr << "Error: --threads K debe ser 1..1024.\n";
                return false;
            }
            opts.threads = k;
            continue;
        }
        // No es opción -> tratamos como posicional; lo procesamos abajo
    }

    // 2) Posicionales (N y ANCHOxALTO) en el orden que aparezcan
    int positionalCount = 0;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--seq" || a == "--threads" || a == "-h" || a == "--help") {
            if (a == "--threads") ++i; // saltar valor ya consumido
            continue;
        }
        if (positionalCount == 0) {
            // Puede ser N o ANCHOxALTO
            int w, h;
            if (parse_resolution(a, w, h)) {
                opts.width = w;
                opts.height = h;
            } else {
                // Intentar N
                int n = std::atoi(a.c_str());
                if (n <= 0 || n > 200000) {
                    std::cerr << "Error: N invalido. Usa un entero en 1..200000.\n";
                    return false;
                }
                opts.nChars = n;
            }
            positionalCount++;
        } else if (positionalCount == 1) {
            // Lo que no fue el primero, debe ser el segundo (resolución o N)
            int w, h;
            if (parse_resolution(a, w, h)) {
                opts.width = w;
                opts.height = h;
            } else {
                int n = std::atoi(a.c_str());
                if (n <= 0 || n > 200000) {
                    std::cerr << "Error: segundo argumento invalido. Esperaba N o ANCHOxALTO.\n";
                    return false;
                }
                opts.nChars = n;
            }
            positionalCount++;
        } else {
            std::cerr << "Error: demasiados argumentos posicionales.\n";
            return false;
        }
    }

    // 3) Validaciones defensivas adicionales
    if (opts.width * 1LL * opts.height > 10000LL * 10000LL) {
        std::cerr << "Error: resolucion demasiado grande.\n";
        return false;
    }
    if (opts.nChars > opts.width * opts.height) {
        std::cerr << "Advertencia: N supera pixeles disponibles; ajustando N.\n";
        opts.nChars = opts.width * opts.height;
    }

    // Verificar que la fuente exista (ejecución desde raíz del repo)
    const std::string fontPath = "assets/fonts/Matrix-MZ4P.ttf";
    if (!file_exists(fontPath)) {
        std::cerr << "Error: no se encontro la fuente '" << fontPath
                  << "'. Ejecuta el binario desde la raiz del proyecto.\n";
        return false;
    }

    return true;
}




static int run_loop(int N, unsigned width, unsigned height, bool vsync = true) {
    sf::RenderWindow window(sf::VideoMode(width, height), "Matrix N caracteres"); // <-- aquí
    if (vsync) window.setVerticalSyncEnabled(true);
    else window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("assets/fonts/Matrix-MZ4P.ttf")) {
        std::cerr << "Error al cargar fuente en assets/fonts/Matrix-MZ4P.ttf\n";
        return EXIT_FAILURE;
    }

    TextRender renderer(N, font, 24, window.getSize());

    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();
            if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape) window.close();
            if (e.type == sf::Event::Resized) {
                sf::FloatRect visibleArea(0.f, 0.f, static_cast<float>(e.size.width), static_cast<float>(e.size.height));
                window.setView(sf::View(visibleArea));
                renderer.resize({e.size.width, e.size.height});
            }
        }

        window.clear(sf::Color::Black);
        renderer.render(window);
        window.display();
    }
    return EXIT_SUCCESS;
}


// Cuando avancemos vamos a cambiar esto
static int run_sequential(const CliOptions& opts) {
    return run_loop(opts.nChars, opts.width, opts.height);
}
static int run_parallel(const CliOptions& opts) {
    // para la versión paralela, por el momento solo vamos a usar la secuencial
    (void)opts.threads; // silenciar warning hasta implementar
    return run_loop(opts.nChars, opts.width, opts.height);
}

int main(int argc, char** argv) {
    CliOptions opts;
    if (!parse_cli(argc, argv, opts)) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Usa SIEMPRE los valores validados
    if (opts.forceSequential) {
        return run_sequential(opts);
    } else {
        return run_parallel(opts);
    }
}
