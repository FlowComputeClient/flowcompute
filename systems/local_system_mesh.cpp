// Copyright 2026 FlowCompute LLC
//
// This file is part of FlowCompute.
//
// FlowCompute is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// FlowCompute is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with FlowCompute. If not, see <https://www.gnu.org/licenses/>.

#include "local_system.h"

#include <fstream>

// Helpful vector operations
struct Vec3 { float x, y, z; };
inline Vec3 getVertex(const std::vector<float>& data, uint32_t localIdx) {
    size_t base = static_cast<size_t>(localIdx) * 3;
    return { data[base], data[base+1], data[base+2] };
}
inline float distSq(const Vec3& a, const Vec3& b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return dx*dx + dy*dy + dz*dz;
}

// Determine if a file is ASCII or Binary
FoamFormat getFoamFormat(const fs::path& path) {
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("format") != std::string::npos) {
            if (line.find("ascii") != std::string::npos)
                return FoamFormat::Ascii;
            if (line.find("binary") != std::string::npos)
                return FoamFormat::Binary;
        }
        if (line.find("// * * * *") != std::string::npos)
            break;
    }
    return FoamFormat::Unknown;
}

// Finds the start of binary data
bool skipToBinary(std::ifstream& file, size_t& outElementCount) {
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

// Parses boundary file to extract patches
std::vector<RenderPatch> parseBoundary(const fs::path& boundaryPath) {
    std::vector<RenderPatch> patches;
    std::ifstream file(boundaryPath);
    if (!file.is_open()) return patches;

    // 1. Pre-process the stream to add spaces around punctuation
    std::stringstream ss;
    char c;
    while (file.get(c)) {
        if (c == '(' || c == ')' || c == '{' || c == '}' || c == ';') {
            ss << ' ' << c << ' ';
        } else {
            ss << c;
        }
    }

    std::string word;
    // Skip header until the first '('
    while (ss >> word && word != "(") {}

    RenderPatch currentPatch;
    bool inPatch = false;

    // Parse the stringstream
    while (ss >> word) {
        // Break at the final closing parenthesis
        if (!inPatch && word == ")") break;

        // Safely ignore parentheses that belong to fields like inGroups
        if (word == "(" || word == ")") continue;

        if (word == "{") {
            inPatch = true;
        } else if (word == "}") {
            if (currentPatch.indexCount > 0) {
                patches.push_back(currentPatch);
            }
            inPatch = false;
        } else if (inPatch && word == "nFaces") {
            ss >> currentPatch.indexCount;
            std::string semi;
            ss >> semi;
        } else if (inPatch && word == "startFace") {
            ss >> currentPatch.firstIndex;
            std::string semi;
            ss >> semi;
        } else if (!inPatch &&
                   !std::isdigit(static_cast<unsigned char>(word[0]))) {
            // Read patch name
            std::strncpy(currentPatch.name, word.c_str(),
                         sizeof(currentPatch.name) - 1);
            currentPatch.name[sizeof(currentPatch.name) - 1] = '\0';
            currentPatch.indexCount = 0;
            currentPatch.firstIndex = 0;
        }
    }
    return patches;
}

std::vector<double> readFoamPoints(const fs::path& path) {
    FoamFormat format = getFoamFormat(path);
    std::vector<double> rawPoints;

    if (format == FoamFormat::Binary) {
        std::ifstream file(path, std::ios::binary);
        size_t numPoints = 0;
        if (skipToBinary(file, numPoints)) {
            rawPoints.resize(numPoints * 3);
            file.read(reinterpret_cast<char*>(
                          rawPoints.data()), numPoints * 3 * sizeof(double));
        }
    } else if (format == FoamFormat::Ascii) {
        std::ifstream file(path);
        std::string token;
        size_t numPoints = 0;

        while (file >> token) {
            if (std::isdigit(token[0])) {
                numPoints = std::stoull(token);
                break;
            }
        }

        if (numPoints > 0) {
            rawPoints.reserve(numPoints * 3);
            char c;
            while (file >> c && c != '(') {}
            for (size_t i = 0; i < numPoints; ++i) {
                while (file >> c && c != '(') {}
                double x, y, z;
                file >> x >> y >> z;
                while (file >> c && c != ')') {}
                rawPoints.push_back(x);
                rawPoints.push_back(y);
                rawPoints.push_back(z);
            }
        }
    }
    return rawPoints;
}

FaceList readFoamFaces(const fs::path& path) {
    FaceList result;
    FoamFormat format = getFoamFormat(path);

    if (format == FoamFormat::Binary) {
        std::ifstream file(path, std::ios::binary);
        size_t totalOffsets = 0;
        if (skipToBinary(file, totalOffsets)) {
            result.offsets.resize(totalOffsets);
            file.read(reinterpret_cast<char*>(
                result.offsets.data()), totalOffsets * sizeof(int32_t));
        }
        size_t totalIndices = 0;
        if (skipToBinary(file, totalIndices)) {
            result.connectivity.resize(totalIndices);
            file.read(reinterpret_cast<char*>(
                result.connectivity.data()), totalIndices * sizeof(int32_t));
        }
    } else if (format == FoamFormat::Ascii) {
        std::ifstream file(path);
        std::string line;

        while (std::getline(file, line)) {
            if (line.find("// * * * *") != std::string::npos) break;
        }

        size_t size1 = 0;
        std::string token;
        while (file >> token) {
            if (std::isdigit(token[0])) {
                size1 = std::stoull(token);
                break;
            }
        }

        char c;
        while (file >> c && c != '(') {}

        int32_t firstVal;
        file >> firstVal;
        file >> std::ws;
        c = file.peek();

        if (c == '(') {
            // Parses a Standard OpenFOAM Face List
            result.offsets.reserve(size1 + 1);
            result.offsets.push_back(0);

            file >> c; // consume '('
            for(int i = 0; i < firstVal; ++i) {
                int32_t idx; file >> idx;
                result.connectivity.push_back(idx);
            }
            while (file >> c && c != ')') {}
            result.offsets.push_back(result.connectivity.size());

            for (size_t f = 1; f < size1; ++f) {
                int32_t numVerts;
                file >> numVerts >> c;
                for(int i = 0; i < numVerts; ++i) {
                    int32_t idx; file >> idx;
                    result.connectivity.push_back(idx);
                }
                while (file >> c && c != ')') {}
                result.offsets.push_back(result.connectivity.size());
            }
        } else {
            // Parses a Compact OpenFOAM Face List
            result.offsets.reserve(size1);
            result.offsets.push_back(firstVal);
            for (size_t i = 1; i < size1; ++i) {
                int32_t val;
                file >> val;
                result.offsets.push_back(val);
            }
            while (file >> c && c != ')') {}

            size_t size2 = 0;
            while (file >> token) {
                if (std::isdigit(token[0])) {
                    size2 = std::stoull(token);
                    break;
                }
            }
            while (file >> c && c != '(') {}
            result.connectivity.reserve(size2);
            for (size_t i = 0; i < size2; ++i) {
                int32_t val;
                file >> val;
                result.connectivity.push_back(val);
            }
        }
    }
    return result;
}

// Triangulate polygons
void triangulatePolygon(const std::vector<uint32_t>& poly,
        std::vector<float>& vertexData, std::vector<uint32_t>& outIndices) {

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
    vertexData.push_back(center.x);
    vertexData.push_back(center.y);
    vertexData.push_back(center.z);

    // Create a triangle fan around the centroid
    for (size_t i = 0; i < N; ++i) {
        uint32_t nextI = (i + 1) % N;
        outIndices.insert(outIndices.end(), {poly[i], poly[nextI], centerIdx});
    }
}

RenderData LocalSystem::getMeshData(const QString& path) {
    RenderData renderData;

    // Read boundary data
    std::filesystem::path casePath(path.toStdU16String());
    fs::path polyDir = casePath / "constant" / "polyMesh";
    renderData.patches = parseBoundary(polyDir / "boundary");
    if (renderData.patches.empty()) {
        // std::cerr << "Failed to parse boundary file or no patches found.\n";
        return renderData;
    }

    std::vector<double> rawPoints = readFoamPoints(polyDir / "points");
    if (rawPoints.empty()) {
        // std::cerr << "Failed to parse points file.\n";
        return renderData;
    }

    FaceList faces = readFoamFaces(polyDir / "faces");
    if (faces.offsets.empty() || faces.connectivity.empty()) {
        // std::cerr << "Failed to parse faces file.\n";
        return renderData;
    }

    // Declare containers
    const uint32_t totalRawPoints = static_cast<uint32_t>(rawPoints.size() / 3);
    std::vector<uint32_t> globalToLocal(totalRawPoints, UINT32_MAX);
    std::vector<std::pair<uint32_t, uint32_t>> flatAdjacency;

    float minX = std::numeric_limits<float>::max(), minY = minX, minZ = minX;
    float maxX = std::numeric_limits<float>::lowest(), maxY = maxX, maxZ = maxX;

    for (auto& patch : renderData.patches) {
        uint32_t foamStartFace = patch.firstIndex;
        uint32_t foamNFaces = patch.indexCount;

        patch.firstIndex = static_cast<uint32_t>(renderData.indices.size());
        patch.firstLineIndex =
            static_cast<uint32_t>(renderData.lineIndices.size());

        for (uint32_t f = foamStartFace; f < foamStartFace + foamNFaces; ++f) {

            // Ensure face index doesn't exceed offset array
            if (f + 1 >= faces.offsets.size()) {
                // std::cerr <<
                // "Error: Face index " << f << " out of bounds.\n";
                break;
            }

            int32_t startOffset = faces.offsets[f];
            int32_t endOffset   = faces.offsets[f + 1];

            // Make sure connectivity array is large enough
            if (static_cast<size_t>(endOffset) > faces.connectivity.size()) {
                // std::cerr << "Error: Connectivity offset out of bounds.\n";
                break;
            }

            std::vector<uint32_t> localFaceLoop;
            renderData.data.reserve(rawPoints.size());
            flatAdjacency.reserve(faces.connectivity.size());
            localFaceLoop.reserve(endOffset - startOffset);

            for (int32_t idx = startOffset; idx < endOffset; ++idx) {
                uint32_t globalPtIdx =
                    static_cast<uint32_t>(faces.connectivity[idx]);

                // Don't read past the end of rawPoints or globalToLocal
                if (globalPtIdx >= totalRawPoints) {
                    // std::cerr << "Error: Global point index " <<
                    // globalPtIdx << " out of bounds.\n";
                    continue;
                }

                uint32_t localPtIdx;
                if (globalToLocal[globalPtIdx] == UINT32_MAX) {
                    // Point not yet inserted
                    localPtIdx =
                        static_cast<uint32_t>(renderData.data.size() / 3);
                    globalToLocal[globalPtIdx] = localPtIdx;

                    float x = static_cast<float>(rawPoints[globalPtIdx * 3]);
                    float y =
                        static_cast<float>(rawPoints[globalPtIdx * 3 + 1]);
                    float z =
                        static_cast<float>(rawPoints[globalPtIdx * 3 + 2]);

                    renderData.data.push_back(x);
                    renderData.data.push_back(y);
                    renderData.data.push_back(z);

                    if (x < minX) minX = x;
                    if (x > maxX) maxX = x;
                    if (y < minY) minY = y;
                    if (y > maxY) maxY = y;
                    if (z < minZ) minZ = z;
                    if (z > maxZ) maxZ = z;
                } else {
                    // Point already exists
                    localPtIdx = globalToLocal[globalPtIdx];
                }

                localFaceLoop.push_back(localPtIdx);

                // Add pair to flat adjacency list
                flatAdjacency.push_back({localPtIdx, f});
            }

            const size_t numVerts = localFaceLoop.size();
            for (size_t i = 0; i < numVerts; ++i) {
                renderData.lineIndices.push_back(localFaceLoop[i]);
                renderData.lineIndices.push_back(
                    localFaceLoop[(i + 1) % numVerts]);
            }

            triangulatePolygon(std::move(localFaceLoop),
                               renderData.data, renderData.indices);
        }

        patch.indexCount =
            static_cast<uint32_t>(renderData.indices.size()) -
            patch.firstIndex;
        patch.lineIndexCount =
            static_cast<uint32_t>(renderData.lineIndices.size()) -
            patch.firstLineIndex;
    }

    renderData.boundingBoxMin = {minX, minY, minZ};
    renderData.boundingBoxMax = {maxX, maxY, maxZ};

    const uint32_t numLocalPoints =
        static_cast<uint32_t>(renderData.data.size() / 3);

    // Count the degree of each point
    std::vector<uint32_t> pointDegrees(numLocalPoints, 0);
    for (const auto& pair : flatAdjacency) {
        pointDegrees[pair.first]++;
    }

    // Compute the prefix sum to build the pointOffsets array
    renderData.pointOffsets.reserve(numLocalPoints + 1);
    renderData.pointOffsets.push_back(0);

    uint32_t currentSum = 0;
    for (uint32_t degree : pointDegrees) {
        currentSum += degree;
        renderData.pointOffsets.push_back(currentSum);
    }

    // Create a working copy of the offsets
    std::vector<uint32_t> currentInsertPos = renderData.pointOffsets;

    // Pre-allocate the adjacency array
    renderData.faceAdjacency.resize(flatAdjacency.size());

    for (const auto& pair : flatAdjacency) {
        uint32_t ptIdx = pair.first;
        uint32_t faceIdx = pair.second;

        // Find the correct destination index for this face,
        uint32_t insertIdx = currentInsertPos[ptIdx]++;
        renderData.faceAdjacency[insertIdx] = faceIdx;
    }
    return renderData;
}