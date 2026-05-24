#ifndef MESH_STRUCTS_H
#define MESH_STRUCTS_H

#include <QString>
#include <QVector3D>

#include <map>
#include <vector>
#include <array>

enum class PatchType {
    patch = 0,
    wall,
    symmetryPlane,
    empty,
    wedge,
    cyclic,
    Count
};

struct Patch {
    QString name;
    PatchType type;
    std::vector<std::array<int, 4>> faces;
};

struct BlockMeshConfig {
    double convertToMeters = 1.0;
    std::vector<std::array<double, 3>> vertices;
    QString shape = "hex";
    int nX = 40, nY = 40, nZ = 40;
    double gradingX = 1.0, gradingY = 1.0, gradingZ = 1.0;
    std::vector<Patch> patches;
};

struct SurfaceFeatureExtractEntry {
    double includedAngle = 150.0;
    bool nonManifoldEdges = false;
    bool openEdges = true;
    bool writeObj = true;
};

struct GeometryRefinement {
    int surfaceMin = 2;
    int surfaceMax = 4;
    int edgeLevel = 4;
};

struct CastellatedMeshConfig {

    // Cell limits
    int maxLocalCells = 100000;
    int maxGlobalCells = 2000000;
    bool allowFreeStandingZoneFaces = true;

    // Stability & feature angle
    int nCellsBetweenLevels = 3;
    double resolveFeatureAngle = 30.0;

    // Required location vector (defaulting to origin)
    std::array<double, 3> locationInMesh = {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN()
    };

    // The actual STL surfaces to refine
    std::map<QString, GeometryRefinement> refinements;
};

struct SnapControlConfig {
    int nSmoothPatch = 3;
    double tolerance = 2.0;
    int nSolveIter = 30;
    int nRelaxIter = 5;
    int nFeatureSnapIter = 10;
    bool explicitFeatureSnap = true;
    bool implicitFeatureSnap = false;
};

struct LayerControlConfig {
    // Sizing rules
    bool relativeSizes = true;
    double expansionRatio = 1.2;
    double finalLayerThickness = 0.3;
    double minThickness = 0.1;

    // The patches and layer counts
    std::map<QString, int> nSurfaceLayers;

    // Angles and transitions
    double featureAngle = 130.0;
    int nLayerIter = 50;
    int nSmoothSurfaceNormals = 1;
};

#endif // MESH_STRUCTS_H
