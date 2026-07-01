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

#include "stl_reader.h"

#include <algorithm>
#include <cstring>
#include <limits>

static_assert(true);
#pragma pack(push, 1)
struct StlTriangle {
    float normal[3];
    float v1[3];
    float v2[3];
    float v3[3];
    uint16_t attributeByteCount;
};
#pragma pack(pop)

// Custom hasher for QVector3D to use in std::unordered_map
struct VertexHasher {
    std::size_t operator()(const std::array<float, 3>& v) const {
        std::size_t h1 = std::hash<float>{}(v[0]);
        std::size_t h2 = std::hash<float>{}(v[1]);
        std::size_t h3 = std::hash<float>{}(v[2]);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

std::pair<RenderData, bool> StlReader::readStlFile(const QString& fileName, const QByteArray& fileData) {

    RenderData mesh;
    mesh.format = RenderType::Surface;
    if (fileData.isEmpty()) return std::make_pair(mesh, false);

    // Initialize bounding box to extreme limits
    mesh.boundingBoxMin = { std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max() };
    mesh.boundingBoxMax = { std::numeric_limits<float>::lowest(),
                           std::numeric_limits<float>::lowest(),
                           std::numeric_limits<float>::lowest() };

    std::unordered_map<std::array<float, 3>, uint32_t, VertexHasher> vertexCache;

    // Set patch name (keeps std::string for intermediate logic)
    auto resolvePatchName = [&](std::string name) -> std::string {
        if (name.empty()) {
            return fileName.toStdString();
        }
        return name;
    };

    // Lambda to handle vertex deduplication and bounding box calculations
    auto addVertex = [&](float px, float py, float pz) -> uint32_t {
        // Create the standard array for caching
        std::array<float, 3> v = {px, py, pz};

        auto it = vertexCache.find(v);
        if (it != vertexCache.end()) {
            return it->second; // Return existing index
        }

        // Assuming PositionOnly format: 3 floats per vertex
        uint32_t index = static_cast<uint32_t>(mesh.data.size() / 3);

        mesh.data.push_back(px);
        mesh.data.push_back(py);
        mesh.data.push_back(pz);
        vertexCache[v] = index;

        // Update bounding box using array indices
        mesh.boundingBoxMin[0] = std::min(mesh.boundingBoxMin[0], px);
        mesh.boundingBoxMin[1] = std::min(mesh.boundingBoxMin[1], py);
        mesh.boundingBoxMin[2] = std::min(mesh.boundingBoxMin[2], pz);

        mesh.boundingBoxMax[0] = std::max(mesh.boundingBoxMax[0], px);
        mesh.boundingBoxMax[1] = std::max(mesh.boundingBoxMax[1], py);
        mesh.boundingBoxMax[2] = std::max(mesh.boundingBoxMax[2], pz);

        return index;
    };

    // Robustly check if the STL is binary.
    bool isBinary = false;
    if (fileData.size() >= 84) {
        const char* ptr = fileData.constData();
        uint32_t numTriangles = *reinterpret_cast<const uint32_t*>(ptr + 80);
        if (fileData.size() == static_cast<int>(84 + numTriangles * 50)) {
            isBinary = true;
        }
    }

    if (isBinary) {
        RenderPatch patch;

        // Safely copy the resolved name into the char[64] array
        std::string resolvedName = resolvePatchName("");
        std::strncpy(patch.name, resolvedName.c_str(), sizeof(patch.name) - 1);
        patch.name[sizeof(patch.name) - 1] = '\0'; // Guarantee null-termination

        patch.first = 0;
        patch.count = 0;

        const char* ptr = fileData.constData();
        uint32_t numTriangles = *reinterpret_cast<const uint32_t*>(ptr + 80);
        ptr += 84;

        for (uint32_t i = 0; i < numTriangles; ++i) {
            ptr += 12;

            for (int j = 0; j < 3; ++j) {
                float x = *reinterpret_cast<const float*>(ptr);
                float y = *reinterpret_cast<const float*>(ptr + 4);
                float z = *reinterpret_cast<const float*>(ptr + 8);
                ptr += 12;

                uint32_t index = addVertex(x, y, z);
                mesh.indices.push_back(index);
                patch.count++;
            }
            ptr += 2;
        }

        if (patch.count > 0) {
            mesh.patches.push_back(patch);
        }

        return std::make_pair(mesh, true);

    } else {

        // ASCII Parsing with Bucketing
        QTextStream stream(fileData);
        QString line;

        std::string currentPatchName;
        bool inSolid = false;

        // Use a vector to maintain the order patches were discovered
        std::vector<std::string> patchOrder;
        // Use a map to bucket indices belonging to the same patch
        std::unordered_map<std::string, std::vector<uint32_t>> patchBuckets;

        static const QRegularExpression wsRe("\\s+");

        while (stream.readLineInto(&line)) {
            line = line.trimmed();
            if (line.isEmpty()) continue;

            if (line.startsWith("solid", Qt::CaseInsensitive)) {
                QString namePart = line.mid(5).trimmed();
                currentPatchName = resolvePatchName(namePart.toStdString());

                // If this is the first time seeing this patch name, record its order
                if (patchBuckets.find(currentPatchName) == patchBuckets.end()) {
                    patchOrder.push_back(currentPatchName);
                }
                inSolid = true;
            }
            else if (line.startsWith("endsolid", Qt::CaseInsensitive)) {
                inSolid = false;
            }
            else if (inSolid && line.startsWith("vertex", Qt::CaseInsensitive)) {
                QStringList parts = line.split(wsRe, Qt::SkipEmptyParts);
                if (parts.size() >= 4) {
                    float x = parts[1].toFloat();
                    float y = parts[2].toFloat();
                    float z = parts[3].toFloat();

                    uint32_t index = addVertex(x, y, z);
                    patchBuckets[currentPatchName].push_back(index);
                }
            }
        }

        // Flatten the buckets into the final contiguous mesh data
        for (const std::string& name : patchOrder) {
            const std::vector<uint32_t>& bucketIndices = patchBuckets[name];

            if (!bucketIndices.empty()) {
                RenderPatch patch;

                // Safely copy the bucket name into the char[64] array
                std::strncpy(patch.name, name.c_str(), sizeof(patch.name) - 1);
                patch.name[sizeof(patch.name) - 1] = '\0'; // Guarantee null-termination

                patch.first = static_cast<uint32_t>(mesh.indices.size());
                patch.count = static_cast<uint32_t>(bucketIndices.size());

                // Append the entire bucket to the final indices vector
                mesh.indices.insert(mesh.indices.end(), bucketIndices.begin(), bucketIndices.end());
                mesh.patches.push_back(patch);
            }
        }
        return std::make_pair(mesh, false);
    }
}

// Read the bounding box and
GeometryMetrics StlReader::readMetrics(const QByteArray& fileData) {
    GeometryMetrics metrics;
    metrics.isValid = false;

    // Variables needed for the interior point
    QVector3D firstTriangleCentroid;
    QVector3D firstTriangleNormal;
    bool firstTriangleCached = false;

    // Initialize bounding box to extreme limits
    constexpr float maxFloat = std::numeric_limits<float>::max();
    constexpr float minFloat = std::numeric_limits<float>::lowest();
    metrics.bbox.min = QVector3D(maxFloat, maxFloat, maxFloat);
    metrics.bbox.max = QVector3D(minFloat, minFloat, minFloat);

    // Use QBuffer to interface with the QByteArray as a QIODevice
    QBuffer buffer;
    buffer.setData(fileData);
    if (!buffer.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Failed to open STL byte array for reading.");
    }

    qint64 fileSize = buffer.size();
    bool hasData = false;
    QRegularExpression ws("\\s+"); // Used for whitespace splitting

    // --- Binary STL Parsing ---
    if (fileSize >= 84) {
        quint32 triangleCount = 0;
        buffer.seek(80);
        if (buffer.read(reinterpret_cast<char*>(&triangleCount), sizeof(triangleCount)) == sizeof(triangleCount)) {
            qint64 expectedSize = 84 + static_cast<qint64>(triangleCount) * 50;

            // If the file size matches the expected binary size
            if (fileSize == expectedSize) {
                const char* dataPtr = fileData.constData() + 84;

                // --- NEW: Parse the 80-byte header for a patch name ---
                char header[81] = {0};
                std::memcpy(header, fileData.constData(), 80);
                QString headerStr = QString::fromLocal8Bit(header).trimmed();
                QString patchName = "solid"; // Default for binary

                if (headerStr.startsWith("solid", Qt::CaseInsensitive)) {
                    QString extracted = headerStr.mid(5).trimmed();
                    if (!extracted.isEmpty()) {
                        // Take the first word after "solid"
                        patchName = extracted.split(ws, Qt::SkipEmptyParts).first();
                    }
                }
                metrics.patches.push_back(patchName.toStdString());

                auto updateBoundingBox = [&](const float v[3]) {
                    metrics.bbox.min.setX(std::min(metrics.bbox.min.x(), v[0]));
                    metrics.bbox.min.setY(std::min(metrics.bbox.min.y(), v[1]));
                    metrics.bbox.min.setZ(std::min(metrics.bbox.min.z(), v[2]));

                    metrics.bbox.max.setX(std::max(metrics.bbox.max.x(), v[0]));
                    metrics.bbox.max.setY(std::max(metrics.bbox.max.y(), v[1]));
                    metrics.bbox.max.setZ(std::max(metrics.bbox.max.z(), v[2]));
                };

                for (quint32 i = 0; i < triangleCount; ++i) {
                    const StlTriangle* tri = reinterpret_cast<const StlTriangle*>(dataPtr + i * 50);
                    updateBoundingBox(tri->v1);
                    updateBoundingBox(tri->v2);
                    updateBoundingBox(tri->v3);

                    // Cache the first triangle for the interior point
                    if (!firstTriangleCached) {
                        QVector3D p1(tri->v1[0], tri->v1[1], tri->v1[2]);
                        QVector3D p2(tri->v2[0], tri->v2[1], tri->v2[2]);
                        QVector3D p3(tri->v3[0], tri->v3[1], tri->v3[2]);

                        firstTriangleCentroid = (p1 + p2 + p3) / 3.0f;

                        // Extract normal from binary struct
                        QVector3D explicitNormal(tri->normal[0], tri->normal[1], tri->normal[2]);
                        if (explicitNormal.isNull()) {
                            firstTriangleNormal = QVector3D::crossProduct(p2 - p1, p3 - p1).normalized();
                        } else {
                            firstTriangleNormal = explicitNormal.normalized();
                        }

                        firstTriangleCached = true;
                    }
                }

                if (triangleCount > 0) hasData = true;
            }
        }
    }

    // --- ASCII Fallback Parsing ---
    if (!hasData) {
        buffer.seek(0);
        QTextStream in(&buffer);
        QString line;

        QVector<QVector3D> currentFaceVertices;

        while (in.readLineInto(&line)) {
            line = line.trimmed();
            if (line.isEmpty()) continue;

            // --- NEW: Check for solid names (patches) ---
            if (line.startsWith("solid", Qt::CaseInsensitive)) {
                QString patchName = line.mid(5).trimmed();
                if (patchName.isEmpty()) {
                    patchName = "solid";
                }

                std::string pNameStr = patchName.toStdString();

                // Add to vector if it is unique
                if (std::find(metrics.patches.begin(), metrics.patches.end(), pNameStr) == metrics.patches.end()) {
                    metrics.patches.push_back(pNameStr);
                }
                continue;
            }

            if (line.startsWith("vertex", Qt::CaseInsensitive)) {
                QStringList parts = line.split(ws, Qt::SkipEmptyParts);
                if (parts.size() >= 4) {
                    float vx = parts[1].toFloat();
                    float vy = parts[2].toFloat();
                    float vz = parts[3].toFloat();

                    // Update bounding box
                    metrics.bbox.min.setX(std::min(metrics.bbox.min.x(), vx));
                    metrics.bbox.min.setY(std::min(metrics.bbox.min.y(), vy));
                    metrics.bbox.min.setZ(std::min(metrics.bbox.min.z(), vz));

                    metrics.bbox.max.setX(std::max(metrics.bbox.max.x(), vx));
                    metrics.bbox.max.setY(std::max(metrics.bbox.max.y(), vy));
                    metrics.bbox.max.setZ(std::max(metrics.bbox.max.z(), vz));

                    hasData = true;

                    // Cache vertices for the first triangle
                    if (!firstTriangleCached) {
                        currentFaceVertices.push_back(QVector3D(vx, vy, vz));
                        if (currentFaceVertices.size() == 3) {
                            firstTriangleCentroid = (currentFaceVertices[0] + currentFaceVertices[1] + currentFaceVertices[2]) / 3.0f;
                            // Calculate normal geometrically
                            firstTriangleNormal = QVector3D::crossProduct(
                                                      currentFaceVertices[1] - currentFaceVertices[0],
                                                      currentFaceVertices[2] - currentFaceVertices[0]
                                                      ).normalized();

                            firstTriangleCached = true;
                        }
                    }
                }
            }
        }
    }

    if (!hasData) {
        throw std::runtime_error("Data does not appear to be a valid Binary or ASCII STL or contains no vertices.");
    }

    // --- Calculate the Interior Point ---
    if (firstTriangleCached) {
        // Calculate the physical size of the geometry
        QVector3D diagonal = metrics.bbox.max - metrics.bbox.min;
        float diagLength = diagonal.length();

        // Define a safe inward step: 1% of the bounding box diagonal
        float stepSize = diagLength * 0.01f;

        // If the geometry is extremely small/flat, enforce a minimum step
        if (stepSize < 1e-6f) {
            stepSize = 1e-6f;
        }

        // Project inward along the inverted normal
        metrics.intpoint = firstTriangleCentroid - (firstTriangleNormal * stepSize);
        metrics.isValid = true;
    }

    return metrics;
}