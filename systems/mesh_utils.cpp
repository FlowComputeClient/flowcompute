#include "mesh_utils.h"
#include <string>
#include <cctype>

// Anonymous namespace for private geometric helpers
namespace {
struct Vec3 { float x, y, z; };

inline Vec3 getVertex(const std::vector<float>& data, uint32_t localIdx) {
    size_t base = static_cast<size_t>(localIdx) * 3;
    return { data[base], data[base+1], data[base+2] };
}

inline float distSq(const Vec3& a, const Vec3& b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return dx*dx + dy*dy + dz*dz;
}
}

namespace MeshUtils {

bool skipToBinary(std::istream& file, size_t& outElementCount) {
    std::string token;
    while (file >> token) {
        if (!token.empty() &&
            std::isdigit(static_cast<unsigned char>(token[0]))) {
            try {
                size_t count = std::stoull(token);
                char c;
                while (file.get(c)) {
                    if (std::isspace(static_cast<unsigned char>(c))) {
                        continue;
                    }
                    if (c == '(') {
                        outElementCount = count;
                        return true;
                    }
                    file.unget();
                    break;
                }
            } catch (...) {
                continue;
            }
        }
    }
    return false;
}

void triangulatePolygon(const std::vector<uint32_t>& poly,
                        std::vector<float>& vertexData,
                        std::vector<uint32_t>& outIndices) {

    const size_t N = poly.size();
    if (N < 3) return;

    // Base Case: Triangle is already perfectly wound
    if (N == 3) {
        outIndices.insert(outIndices.end(), {poly[0], poly[1], poly[2]});
        return;
    }

    // Calculate the face centroid
    Vec3 center = {0.0f, 0.0f, 0.0f};
    for (uint32_t idx : poly) {
        Vec3 v = getVertex(vertexData, idx);
        center.x += v.x;
        center.y += v.y;
        center.z += v.z;
    }
    center.x /= N;
    center.y /= N;
    center.z /= N;

    // Append the new centroid to the vertex buffer
    uint32_t centerIdx = static_cast<uint32_t>(vertexData.size() / 3);
    vertexData.insert(vertexData.end(), {center.x, center.y, center.z});

    // Create a triangle fan around the centroid
    for (size_t i = 0; i < N; ++i) {
        uint32_t nextI = (i + 1) % N;
        outIndices.insert(outIndices.end(), {poly[i], poly[nextI], centerIdx});
    }
}

}