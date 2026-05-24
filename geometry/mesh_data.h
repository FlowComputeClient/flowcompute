#ifndef MESH_DATA_H
#define MESH_DATA_H

#include <QVector3D>

#include <vector>

enum class VertexFormat {
    Flat = 0,
    Wireframe,
    Color,
    Count
};

enum class GeometryType {
    STL = 0,
    OBJ,
    MESH,
    Count
};

struct BoundingBox {
    QVector3D min;
    QVector3D max;
};

struct GeometryMetrics {
    BoundingBox bbox;
    QVector3D intpoint;
    bool isValid;
};

struct MeshPatch {
    std::string name;
    uint32_t firstIndex;
    uint32_t indexCount;
};

struct MeshData {
    VertexFormat format;
    std::vector<float> data;
    std::vector<uint32_t> indices;
    std::vector<MeshPatch> patches;
    QVector3D boundingBoxMin;
    QVector3D boundingBoxMax;
};

#endif // MESH_DATA_H
