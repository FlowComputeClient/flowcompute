#ifndef MESH_DATA_H
#define MESH_DATA_H

#include <QVector3D>

#include <array>
#include <vector>

const std::array<std::array<float, 4>, 10> patchColors = {{
    {0.3686274509803922, 0.20784313725490197, 0.6941176470588235, 1.0 }, // dark purple
    {0.9372549019607843, 0.3254901960784314, 0.3137254901960784, 1.0 }, // bright red
    {0.39215686274509803, 0.7098039215686275, 0.9647058823529412, 1.0 }, // light blue
    {0.2196078431372549, 0.5568627450980392, 0.23529411764705882, 1.0 }, // forest green
    {1.0, 0.9333333333333333, 0.34509803921568627, 1.0 }, // bright yellow
    {0.984313725490196, 0.5490196078431373, 0.0, 1.0 }, // vivid orange
    {0.4627450980392157, 1.0, 0.011764705882352941, 1.0 }, // lime green
    {0.9568627450980393, 0.5607843137254902, 0.6941176470588235, 1.0 }, // soft pink
    {0.6313725490196078, 0.5333333333333333, 0.4980392156862745, 1.0 }, // muted brown
    {0.5019607843137255, 0.796078431372549, 0.7686274509803922, 1.0 }, // pale teal
}};

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

struct MeshHeader {
    uint32_t magicNumber;
    uint32_t vertexFormat;
    uint32_t dataByteSize;
    uint32_t indexByteSize;
    uint32_t patchCount;
    float bounds[6];
};

struct MeshPatch {
    char name[64];
    uint32_t firstIndex;
    uint32_t indexCount;
};

struct MeshData {
    VertexFormat format;
    std::vector<float> data;
    std::vector<uint32_t> indices;
    std::vector<MeshPatch> patches;
    std::array<float, 3> boundingBoxMin;
    std::array<float, 3> boundingBoxMax;
};

#endif // MESH_DATA_H
