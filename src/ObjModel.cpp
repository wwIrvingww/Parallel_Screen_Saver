#include "ObjModel.h"
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <cmath>

static inline uint64_t edge_key(uint32_t a, uint32_t b) {
    if (a > b) std::swap(a, b);
    return (uint64_t(a) << 32) | uint64_t(b);
}

bool ObjModel::parseFaceIndex(const std::string& tok, int nVerts, uint32_t& out0based) {
    // Soporta "i", "i/j", "i//k", "i/j/k". Admite negativos según .obj (desde el final).
    // Tomamos solo el primer campo (posición).
    if (tok.empty()) return false;
    int slash = tok.find('/');
    std::string s = (slash == int(std::string::npos)) ? tok : tok.substr(0, slash);

    int idx = 0;
    try { idx = std::stoi(s); } catch (...) { return false; }
    if (idx == 0) return false;

    int zeroBased = (idx > 0) ? (idx - 1) : (nVerts + idx); // idx < 0 => desde el final
    if (zeroBased < 0 || zeroBased >= nVerts) return false;
    out0based = static_cast<uint32_t>(zeroBased);
    return true;
}

bool ObjModel::loadFromOBJ(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;

    vertices_.clear();
    edges_.clear();
    vtxRot_.clear();
    vtx2d_.clear();
    lines_.clear();

    std::string line;
    vertices_.reserve(10000);

    // 1) Parsear vertices y caras
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string tag; iss >> tag;
        if (tag == "v") {
            Vec3 v{};
            iss >> v.x >> v.y >> v.z;
            vertices_.push_back(v);
        }
        // ignoramos 'vn', 'vt', 'o', 'g', etc.
    }

    if (vertices_.empty()) {
        loaded_ = false;
        return false;
    }

    // Reposicionar para segunda pasada (caras)
    in.clear();
    in.seekg(0);

    std::unordered_set<uint64_t> uniq;
    uniq.reserve(50000);

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line.size() >= 2 && line[0] == 'f' && std::isspace(static_cast<unsigned char>(line[1]))) {
            std::istringstream iss(line);
            std::string tag; iss >> tag; // 'f'

            std::vector<uint32_t> idx;
            std::string tok;
            while (iss >> tok) {
                uint32_t i0;
                if (parseFaceIndex(tok, (int)vertices_.size(), i0)) idx.push_back(i0);
            }
            if (idx.size() < 2) continue;

            // Agregar aristas del polígono (cerrado)
            for (size_t i = 0; i < idx.size(); ++i) {
                uint32_t a = idx[i];
                uint32_t b = idx[(i + 1) % idx.size()];
                if (a == b) continue;
                uniq.insert(edge_key(a, b));
            }
        }
    }

    // Pasar set -> vector ordenado (opcional)
    edges_.reserve(uniq.size());
    for (auto k : uniq) {
        uint32_t a = uint32_t(k >> 32);
        uint32_t b = uint32_t(k & 0xffffffffu);
        edges_.emplace_back(a, b);
    }
    std::sort(edges_.begin(), edges_.end());

    // 2) Normalizar modelo al cubo unidad (centro en origen; tamaño ~1)
    Vec3 mn = vertices_[0], mx = vertices_[0];
    for (const auto& v : vertices_) {
        mn.x = std::min(mn.x, v.x); mn.y = std::min(mn.y, v.y); mn.z = std::min(mn.z, v.z);
        mx.x = std::max(mx.x, v.x); mx.y = std::max(mx.y, v.y); mx.z = std::max(mx.z, v.z);
    }
    Vec3 c{ (mn.x + mx.x)*0.5f, (mn.y + mx.y)*0.5f, (mn.z + mx.z)*0.5f };
    float sx = mx.x - mn.x, sy = mx.y - mn.y, sz = mx.z - mn.z;
    float maxDim = std::max({ sx, sy, sz, 1e-6f });
    float inv = 1.0f / maxDim;

    for (auto& v : vertices_) {
        v.x = (v.x - c.x) * inv;
        v.y = (v.y - c.y) * inv;
        v.z = (v.z - c.z) * inv;
    }
    for (auto& v : vertices_) {
        v.y = -v.y;
        v.z = -v.z;
    }

    // 3) Reservar buffers reutilizables
    vtxRot_.resize(vertices_.size());
    vtx2d_.resize(vertices_.size());
    lines_.resize(edges_.size() * 2);
    lines_.setPrimitiveType(sf::Lines);

    loaded_ = true;
    return true;
}

void ObjModel::drawProjected(sf::RenderWindow& window,
                             sf::Vector2f center,
                             float scale,
                             float angleY,
                             sf::Color color) {
    if (!loaded_) return;

    // 1) Rotación Y (una sola sin/cos)
    float c = std::cos(angleY);
    float s = std::sin(angleY);
    for (size_t i = 0; i < vertices_.size(); ++i) {
        const auto& v = vertices_[i];
        float xr =  c * v.x + s * v.z;
        float zr = -s * v.x + c * v.z;
        vtxRot_[i] = { xr, v.y, zr };
    }

    // 2) Proyección (perspectiva simple)
    const float F = 800.0f;
    for (size_t i = 0; i < vtxRot_.size(); ++i) {
        const auto& v = vtxRot_[i];
        float w = F / (F + (v.z * scale * 0.8f)); // z afecta un poco el zoom
        vtx2d_[i].x = center.x + (v.x * scale) * w;
        vtx2d_[i].y = center.y + (v.y * scale) * w;
    }

    // 3) Construir VertexArray (2 vértices por arista) y dibujar 1 vez
    for (size_t k = 0; k < edges_.size(); ++k) {
        auto [a, b] = edges_[k];
        sf::Vertex& v0 = lines_[2 * k + 0];
        sf::Vertex& v1 = lines_[2 * k + 1];
        v0.position = vtx2d_[a];
        v1.position = vtx2d_[b];
        v0.color = color;
        v1.color = color;
    }
    window.draw(lines_);
}
