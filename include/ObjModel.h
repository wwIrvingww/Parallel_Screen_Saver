#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <utility>
#include <cstdint>

class ObjModel {
public:
    // Carga .obj (solo 'v' y 'f' necesarios para wireframe). Normaliza al cubo unidad.
    bool loadFromOBJ(const std::string& path);

    // Dibuja en 1 sola draw call usando el VertexArray interno.
    // angleY en radianes. scale en píxeles (multiplica al modelo normalizado).
    void drawProjected(sf::RenderWindow& window,
                       sf::Vector2f center,
                       float scale,
                       float angleY,
                       sf::Color color);

    bool loaded() const { return loaded_; }

private:
    struct Vec3 { float x, y, z; };

    // Datos originales y buffers reutilizados
    std::vector<Vec3> vertices_;              // v originales (normalizados)
    std::vector<Vec3> vtxRot_;                // rotados (Y) para el frame actual
    std::vector<sf::Vector2f> vtx2d_;         // proyectados 2D para el frame actual

    // Aristas únicas (indices en vertices_)
    std::vector<std::pair<uint32_t,uint32_t>> edges_;

    // Un solo VA para todas las líneas (2 vértices por arista)
    sf::VertexArray lines_{sf::Lines};

    bool loaded_ = false;

    // Helpers
    static bool parseFaceIndex(const std::string& tok, int nVerts, uint32_t& out0based);
};
