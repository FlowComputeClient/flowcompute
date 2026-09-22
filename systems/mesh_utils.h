#ifndef MESH_UTILS_H
#define MESH_UTILS_H

#include <istream>
#include <vector>
#include <cstdint>

namespace MeshUtils {

// Finds the start of binary data in a stream
bool skipToBinary(std::istream& file, size_t& outElementCount);

// Triangulates an arbitrary face and updates the buffers
void triangulatePolygon(const std::vector<uint32_t>& poly,
    std::vector<float>& vertexData, std::vector<uint32_t>& outIndices);
}

#endif  // MESH_UTILS_H
