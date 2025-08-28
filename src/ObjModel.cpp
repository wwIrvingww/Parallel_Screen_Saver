#include "ObjModel.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream> // logs
#include <cmath>

static std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b - a);
}

bool ObjModel::parseFaceIndex(const std::string& tok, int& outIndex) {
    // token puede ser "i", "i/j", "i//k", "i/j/k"
    if (tok.empty()) return false;
    std::istringstream iss(tok);
    int idx = 0;
    if (!(iss >> idx)) return false;
    outIndex = idx - 1; // OBJ es 1-based
    return true;
}

void ObjModel::computeBounds() {
    if (vertices_.empty()) { min_ = max_ = {0,0,0}; return; }
    min_ = max_ = vertices_[0];
    for (auto& v : vertices_) {
        min_.x = std::min(min_.x, v.x);
        min_.y = std::min(min_.y, v.y);
        min_.z = std::min(min_.z, v.z);
        max_.x = std::max(max_.x, v.x);
        max_.y = std::max(max_.y, v.y);
        max_.z = std::max(max_.z, v.z);
    }
}

bool ObjModel::loadFromFile(const std::string& path) {
    vertices_.clear(); indices_.clear();
    min_ = max_ = {0,0,0};

    std::ifstream in(path);
    if (!in) return false;

    std::string line;
    std::vector<int> faceIdx;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ls(line);
        std::string tag; ls >> tag;
        if (tag == "v") {
            float x, y, z;
            if (ls >> x >> y >> z) {
                vertices_.push_back(sf::Vector3f(x, y, z));
            }
        } else if (tag == "f") {
            faceIdx.clear();
            std::string tok;
            while (ls >> tok) {
                size_t slash = tok.find('/');
                std::string head = (slash == std::string::npos) ? tok : tok.substr(0, slash);
                int idx0 = 0;
                if (!parseFaceIndex(head, idx0)) continue;
                if (idx0 < 0) idx0 = (int)vertices_.size() + idx0 + 1 - 1; // negativos: -1 = último
                faceIdx.push_back(idx0);
            }
            if (faceIdx.size() >= 3) {
                for (size_t i = 1; i + 1 < faceIdx.size(); ++i) {
                    indices_.push_back((unsigned)faceIdx[0]);
                    indices_.push_back((unsigned)faceIdx[i]);
                    indices_.push_back((unsigned)faceIdx[i+1]);
                }
            }
        }
    }

    if (vertices_.empty() || indices_.empty()) return false;
    computeBounds();

    std::cerr << "[OBJ] Cargado: " << vertexCount() << " vertices, "
              << triangleCount() << " tri, bbox=[("
              << min_.x << "," << min_.y << "," << min_.z << ")..("
              << max_.x << "," << max_.y << "," << max_.z << ")]\n";

    return true;
}

void ObjModel::drawProjectedAutoFit(sf::RenderWindow& window,
                                    sf::Vector2f center,
                                    float targetPx,
                                    sf::Color color) const
{
    if (empty()) return;

    // Centro y extensiones
    sf::Vector3f c{ 0.5f*(min_.x+max_.x), 0.5f*(min_.y+max_.y), 0.5f*(min_.z+max_.z) };
    float ex = max_.x - min_.x;
    float ey = max_.y - min_.y;
    float ez = max_.z - min_.z;

    // Plano de mayor área
    float aXY = ex*ey, aXZ = ex*ez, aYZ = ey*ez;
    enum Plane { XY, XZ, YZ } plane = XY;
    float sideU=ex, sideV=ey;
    if (aXZ >= aXY && aXZ >= aYZ) { plane = XZ; sideU = ex; sideV = ez; }
    else if (aYZ >= aXY && aYZ >= aXZ) { plane = YZ; sideU = ey; sideV = ez; }

    float modelSide = std::max(sideU, sideV);
    if (modelSide <= 1e-6f) modelSide = 1.f;
    float scale = targetPx / modelSide;

    sf::VertexArray tris(sf::Triangles);
    tris.resize(indices_.size());

    for (size_t t = 0; t < indices_.size(); ++t) {
        const sf::Vector3f& v = vertices_[indices_[t]];
        float u = 0.f, vv = 0.f;
        switch (plane) {
            case XY: u = (v.x - c.x); vv = (v.y - c.y); break;
            case XZ: u = (v.x - c.x); vv = (v.z - c.z); break;
            case YZ: u = (v.y - c.y); vv = (v.z - c.z); break;
        }
        float x = center.x + u * scale;
        float y = center.y - vv * scale; // invertimos Y para pantalla
        tris[t].position = sf::Vector2f(x, y);
        tris[t].color = color;
    }

    window.draw(tris);
}

void ObjModel::drawProjectedYawAndOffset(sf::RenderWindow& window,
                                         sf::Vector2f center,
                                         float targetPx,
                                         float yawDeg,
                                         sf::Vector2f screenOffset,
                                         sf::Color color) const
{
    if (empty()) return;

    // Centro y extensiones
    sf::Vector3f c{ 0.5f*(min_.x+max_.x), 0.5f*(min_.y+max_.y), 0.5f*(min_.z+max_.z) };
    float ex = max_.x - min_.x;
    float ey = max_.y - min_.y;
    float ez = max_.z - min_.z;

    // Para rotación en Y, el mayor ancho posible en pantalla será max( sqrt(ex^2+ez^2), ey )
    float exz = std::sqrt(ex*ex + ez*ez);
    float modelSide = std::max(exz, ey);
    if (modelSide <= 1e-6f) modelSide = 1.f;
    float scale = targetPx / modelSide;

    const float rad = yawDeg * 3.1415926535f / 180.f;
    const float cs = std::cos(rad);
    const float sn = std::sin(rad);

    sf::VertexArray tris(sf::Triangles);
    tris.resize(indices_.size());

    for (size_t t = 0; t < indices_.size(); ++t) {
        const sf::Vector3f& v = vertices_[indices_[t]];
        // Trasladar al centro local, rotar Y, proyectar XY:
        float dx = v.x - c.x;
        float dy = v.y - c.y;
        float dz = v.z - c.z;

        float x =  cs*dx + sn*dz;
        float y =  dy;
        // z' = -sn*dx + cs*dz; // no se usa en proyección ortográfica XY

        float sx = center.x + screenOffset.x + x * scale;
        float sy = center.y + screenOffset.y - y * scale; // invertimos Y

        tris[t].position = sf::Vector2f(sx, sy);
        tris[t].color = color;
    }

    window.draw(tris);
}
