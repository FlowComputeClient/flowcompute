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

#include "obj_reader.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

RenderData ObjReader::readObjFile(const QString& fileName,
                                  const QByteArray& fileData) {

    RenderData mesh;
    mesh.format = RenderType::Surface;
    if (fileData.isEmpty()) return mesh;

    // Initialize bounding box to extreme limits
    mesh.boundingBoxMin = { std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max(),
                           std::numeric_limits<float>::max() };
    mesh.boundingBoxMax = { std::numeric_limits<float>::lowest(),
                           std::numeric_limits<float>::lowest(),
                           std::numeric_limits<float>::lowest() };

    // OpenFOAM assigns faces before the first 'g' tag to "zone0"
    std::string currentPatchName = "zone0";

    // Set patch name (keeps std::string for intermediate logic)
    auto resolvePatchName = [&](std::string name) -> std::string {
        if (name.empty()) {
            return "zone0";
        }
        return name;
    };

    // Lambda to parse OBJ index
    auto parseObjIndex = [&](const QString& token) -> uint32_t {
        int slashPos = token.indexOf('/');
        QString vStr = (slashPos != -1) ? token.left(slashPos) : token;
        int vIdx = vStr.toInt();

        uint32_t totalVertices = static_cast<uint32_t>(mesh.data.size() / 3);

        if (vIdx > 0) {
            // Standard 1-based indexing
            uint32_t idx = static_cast<uint32_t>(vIdx - 1);
            return (idx < totalVertices) ? idx : 0;
        } else if (vIdx < 0) {
            // Relative indexing
            int idx = static_cast<int>(totalVertices) + vIdx;
            return (idx >= 0 && idx < static_cast<int>(totalVertices)) ?
                       static_cast<uint32_t>(idx) : 0;
        }
        return 0; // Fallback for malformed '0' index
    };

    // Parse text
    QTextStream stream(fileData);
    QString line;

    // Use a vector to maintain the order patches were discovered
    std::vector<std::string> patchOrder;
    // Use a map to bucket indices belonging to the same patch
    std::unordered_map<std::string, std::vector<uint32_t>> patchBuckets;

    static const QRegularExpression wsRe("\\s+");

    while (stream.readLineInto(&line)) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        // Ensure exact match for vertex position 'v' (ignore 'vt' or 'vn')
        if (line.startsWith("v ") || line.startsWith("v\t")) {
            QStringList parts = line.split(wsRe, Qt::SkipEmptyParts);
            if (parts.size() >= 4) {
                float px = parts[1].toFloat();
                float py = parts[2].toFloat();
                float pz = parts[3].toFloat();

                // OBJ is already indexed; directly append vertex data
                mesh.data.push_back(px);
                mesh.data.push_back(py);
                mesh.data.push_back(pz);

                // Update bounding box
                mesh.boundingBoxMin[0] = std::min(mesh.boundingBoxMin[0], px);
                mesh.boundingBoxMin[1] = std::min(mesh.boundingBoxMin[1], py);
                mesh.boundingBoxMin[2] = std::min(mesh.boundingBoxMin[2], pz);
                mesh.boundingBoxMax[0] = std::max(mesh.boundingBoxMax[0], px);
                mesh.boundingBoxMax[1] = std::max(mesh.boundingBoxMax[1], py);
                mesh.boundingBoxMax[2] = std::max(mesh.boundingBoxMax[2], pz);
            }
        }
        // Group ('g') or Object ('o') tags ONLY set the active patch name
        else if (line.startsWith("g ") || line.startsWith("g\t") ||
                 line.startsWith("o ") || line.startsWith("o\t")) {
            QString namePart = line.mid(2).trimmed();
            currentPatchName = resolvePatchName(namePart.toStdString());
        }
        // Face ('f') definitions
        else if (line.startsWith("f ") || line.startsWith("f\t")) {
            QStringList parts = line.split(wsRe, Qt::SkipEmptyParts);
            if (parts.size() < 4) continue;

            // Register patch name ONLY when its first face is encountered
            if (patchBuckets.find(currentPatchName) == patchBuckets.end()) {
                patchOrder.push_back(currentPatchName);
            }

            std::vector<uint32_t>& bucket = patchBuckets[currentPatchName];

            // Extract vertex indices for the face
            std::vector<uint32_t> faceIndices;
            faceIndices.reserve(parts.size() - 1);
            for (int i = 1; i < parts.size(); ++i) {
                faceIndices.push_back(parseObjIndex(parts[i]));
            }

            // Fan triangulation
            for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
                bucket.push_back(faceIndices[0]);
                bucket.push_back(faceIndices[i]);
                bucket.push_back(faceIndices[i + 1]);
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
            patch.name[sizeof(patch.name) - 1] = '\0';

            patch.first = static_cast<uint32_t>(mesh.indices.size());
            patch.count = static_cast<uint32_t>(bucketIndices.size());

            // Append the entire bucket to the final indices vector
            mesh.indices.insert(mesh.indices.end(), bucketIndices.begin(),
                                bucketIndices.end());
            mesh.patches.push_back(patch);
        }
    }

    return mesh;
}

/*
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
    QRegularExpression ws("\\s+");  // Used for whitespace splitting

    // Binary STL Parsing ---
    if (fileSize >= 84) {
        quint32 triangleCount = 0;
        buffer.seek(80);
        if (buffer.read(reinterpret_cast<char*>(&triangleCount),
                        sizeof(triangleCount)) == sizeof(triangleCount)) {
            qint64 expectedSize = 84 + static_cast<qint64>(triangleCount) * 50;

            // If the file size matches the expected binary size
            if (fileSize == expectedSize) {
                const char* dataPtr = fileData.constData() + 84;

                // Parse the 80-byte header for a patch name
                char header[81] = {0};
                std::memcpy(header, fileData.constData(), 80);
                QString headerStr = QString::fromLocal8Bit(header).trimmed();
                QString patchName = "solid";  // Default for binary

                if (headerStr.startsWith("solid", Qt::CaseInsensitive)) {
                    QString extracted = headerStr.mid(5).trimmed();
                    if (!extracted.isEmpty()) {
                        // Take the first word after "solid"
                        patchName =
                            extracted.split(ws, Qt::SkipEmptyParts).first();
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
                    const StlTriangle* tri =
                        reinterpret_cast<const StlTriangle*>(dataPtr + i * 50);
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
                        QVector3D explicitNormal(tri->normal[0], tri->normal[1],
                                                 tri->normal[2]);
                        if (explicitNormal.isNull()) {
                            firstTriangleNormal =
                        QVector3D::crossProduct(p2 - p1, p3 - p1).normalized();
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

    // ASCII Parsing
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
                if (std::find(metrics.patches.begin(), metrics.patches.end(),
                              pNameStr) == metrics.patches.end()) {
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
                            firstTriangleCentroid =
                                (currentFaceVertices[0] + currentFaceVertices[1]
                                    + currentFaceVertices[2]) / 3.0f;
                            // Calculate normal geometrically
                            firstTriangleNormal =
                                QVector3D::crossProduct(
                                    currentFaceVertices[1] -
                                    currentFaceVertices[0],
                                    currentFaceVertices[2] -
                                    currentFaceVertices[0]).normalized();

                            firstTriangleCached = true;
                        }
                    }
                }
            }
        }
    }

    if (!hasData) {
        throw std::runtime_error("Data does not appear to be a valid binary "
                                 "or ASCII STL or contains no vertices.");
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
        metrics.intpoint =
            firstTriangleCentroid - (firstTriangleNormal * stepSize);
        metrics.isValid = true;
    }

    return metrics;
}
*/
