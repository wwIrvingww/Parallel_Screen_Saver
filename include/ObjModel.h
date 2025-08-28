#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class ObjModel {
public:
    // Carga OBJ simple: usa 'v' y 'f'. (f admite v, v/vt, v//vn, v/vt/vn)
    bool loadFromFile(const std::string& path);

    // Proyecta usando el plano de mayor área (XY/XZ/YZ) y escala para encajar targetPx
    void drawProjectedAutoFit(sf::RenderWindow& window,
                              sf::Vector2f center,
                              float targetPx,
                              sf::Color color = sf::Color(220,220,220,235)) const;

    // Proyección en plano XY, con rotación around-Y y desplazamiento en pantalla.
    // El escalado se calcula para que la *mayor* proyección posible al rotar en Y
    // quepa en targetPx (usa max( sqrt(ex^2+ez^2), ey )).
    void drawProjectedYawAndOffset(sf::RenderWindow& window,
                                   sf::Vector2f center,
                                   float targetPx,
                                   float yawDeg,
                                   sf::Vector2f screenOffset,
                                   sf::Color color = sf::Color(220,220,220,235)) const;

    // Stats y bbox
    const sf::Vector3f& minBounds() const { return min_; }
    const sf::Vector3f& maxBounds() const { return max_; }
    bool   empty()        const { return vertices_.empty() || indices_.empty(); }
    size_t vertexCount()  const { return vertices_.size(); }
    size_t triangleCount()const { return indices_.size() / 3; }

private:
    std::vector<sf::Vector3f> vertices_; // (x,y,z)
    std::vector<unsigned>     indices_;  // triangulado

    sf::Vector3f min_{0,0,0}, max_{0,0,0};

    static bool parseFaceIndex(const std::string& tok, int& outIndex); // 1-based -> 0-based
    void computeBounds();
};
