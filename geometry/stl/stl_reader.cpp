#include "stl_reader.h"

#include <limits>
#include <algorithm>

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
    std::size_t operator()(const QVector3D& v) const {
        std::size_t h1 = std::hash<float>{}(v.x());
        std::size_t h2 = std::hash<float>{}(v.y());
        std::size_t h3 = std::hash<float>{}(v.z());
        // Combine hashes (simple bitwise XOR and shift)
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

struct VertexEqual {
    bool operator()(const QVector3D& a, const QVector3D& b) const {
        return a.x() == b.x() && a.y() == b.y() && a.z() == b.z();
    }
};

MeshData StlReader::readStlFile(const QString& fileName, const QByteArray& fileData) {
    MeshData mesh;
    mesh.format = VertexFormat::Flat;
    if (fileData.isEmpty()) return mesh;

    // Initialize bounding box to extreme limits
    mesh.boundingBoxMin = QVector3D(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    mesh.boundingBoxMax = QVector3D(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    std::unordered_map<std::string, int> nameTracker;
    std::unordered_map<QVector3D, uint32_t, VertexHasher, VertexEqual> vertexCache;

    // Lambda to handle the name resolution logic
    auto resolvePatchName = [&](std::string name) -> std::string {
        if (name.empty()) {
            name = fileName.toStdString();
        }

        if (nameTracker.find(name) == nameTracker.end()) {
            nameTracker[name] = 0; // First time seeing this name
            return name;
        } else {
            std::string resolvedName = name + std::to_string(nameTracker[name]);
            nameTracker[name]++;
            return resolvedName;
        }
    };

    // Lambda to handle vertex deduplication and bounding box calculations
    auto addVertex = [&](const QVector3D& v) -> uint32_t {
        auto it = vertexCache.find(v);
        if (it != vertexCache.end()) {
            return it->second; // Return existing index
        }

        // Assuming PositionOnly format: 3 floats per vertex
        uint32_t index = static_cast<uint32_t>(mesh.data.size() / 3);

        mesh.data.push_back(v.x());
        mesh.data.push_back(v.y());
        mesh.data.push_back(v.z());
        vertexCache[v] = index;

        // Update bounding box
        mesh.boundingBoxMin.setX(std::min(mesh.boundingBoxMin.x(), v.x()));
        mesh.boundingBoxMin.setY(std::min(mesh.boundingBoxMin.y(), v.y()));
        mesh.boundingBoxMin.setZ(std::min(mesh.boundingBoxMin.z(), v.z()));
        mesh.boundingBoxMax.setX(std::max(mesh.boundingBoxMax.x(), v.x()));
        mesh.boundingBoxMax.setY(std::max(mesh.boundingBoxMax.y(), v.y()));
        mesh.boundingBoxMax.setZ(std::max(mesh.boundingBoxMax.z(), v.z()));
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
        MeshPatch patch;
        patch.name = resolvePatchName("");
        patch.firstIndex = 0;
        patch.indexCount = 0;

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

                uint32_t index = addVertex(QVector3D(x, y, z));
                mesh.indices.push_back(index);
                patch.indexCount++;
            }
            ptr += 2;
        }

        if (patch.indexCount > 0) {
            mesh.patches.push_back(patch);
        }

    } else {
        // ASCII Parsing
        QTextStream stream(fileData);
        QString line;
        MeshPatch currentPatch;
        bool inSolid = false;

        // QRegularExpression is safer/faster for arbitrary whitespace than splitting by a single space
        static const QRegularExpression wsRe("\\s+");

        while (stream.readLineInto(&line)) {
            line = line.trimmed();
            if (line.isEmpty()) continue;

            if (line.startsWith("solid", Qt::CaseInsensitive)) {
                QString namePart = line.mid(5).trimmed();
                currentPatch.name = resolvePatchName(namePart.toStdString());
                currentPatch.firstIndex = static_cast<uint32_t>(mesh.indices.size());
                currentPatch.indexCount = 0;
                inSolid = true;
            }
            else if (line.startsWith("endsolid", Qt::CaseInsensitive)) {
                if (inSolid && currentPatch.indexCount > 0) {
                    mesh.patches.push_back(currentPatch);
                }
                inSolid = false;
            }
            else if (line.startsWith("vertex", Qt::CaseInsensitive)) {
                QStringList parts = line.split(wsRe, Qt::SkipEmptyParts);
                if (parts.size() >= 4) {
                    float x = parts[1].toFloat();
                    float y = parts[2].toFloat();
                    float z = parts[3].toFloat();

                    uint32_t index = addVertex(QVector3D(x, y, z));
                    mesh.indices.push_back(index);
                    currentPatch.indexCount++;
                }
            }
        }

        // Catch the edge case where an ASCII file lacks a final 'endsolid' statement
        if (inSolid && currentPatch.indexCount > 0) {
            mesh.patches.push_back(currentPatch);
        }
    }
    return mesh;
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

    // --- Binary STL Parsing ---
    if (fileSize >= 84) {
        quint32 triangleCount = 0;
        buffer.seek(80);
        if (buffer.read(reinterpret_cast<char*>(&triangleCount), sizeof(triangleCount)) == sizeof(triangleCount)) {
            qint64 expectedSize = 84 + static_cast<qint64>(triangleCount) * 50;

            // If the file size matches the expected binary size
            if (fileSize == expectedSize) {
                const char* dataPtr = fileData.constData() + 84;

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

                        // Extract normal from binary struct (assuming tri->normal exists)
                        // Fallback to cross product if normal is strictly (0,0,0)
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
        QRegularExpression ws("\\s+");

        QVector<QVector3D> currentFaceVertices;

        while (in.readLineInto(&line)) {
            line = line.trimmed();
            if (line.isEmpty()) continue;

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
                            // Calculate normal geometrically to avoid relying on standard ASCII 'facet normal' lines
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