#ifndef PARSER_SNAPPY_HEX_MESH_DICT_H_
#define PARSER_SNAPPY_HEX_MESH_DICT_H_

#include <array>
#include <map>
#include <memory>

#include "open_foam_dictionary.h"
#include "surface_feature.h"

namespace CaseIO {

struct RefinementRegion {
    QString name;
    int min = 2;
    int max = 3;
};

struct RefinementSurface {
    QString name;
    int min = 2;
    int max = 3;
    std::vector<RefinementRegion> regions;
};

struct CastellatedMeshConfig {
    // Cell limits
    int maxLocalCells = 100000;
    int maxGlobalCells = 2000000;
    bool allowFreeStandingZoneFaces = true;

    // Stability & feature angle
    int nCellsBetweenLevels = 2;
    double resolveFeatureAngle = 30.0;

    // Required location vector (defaulting to origin)
    std::array<double, 3> locationInMesh = {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN()};

    // The surfaces to refine
    std::vector<QString> geometryFiles;
    std::vector<RefinementSurface> refinementSurfaces;
};

struct SnapControlConfig {
    int nSmoothPatch = 3;
    double tolerance = 2.0;
    int nSolveIter = 15;
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
    int nLayerIter = 30;
    int nSmoothThickness = 10;
    int nSmoothNormals = 3;
    int nSmoothSurfaceNormals = 1;
};

// Parse castellation section of snappyHexMeshDict
CastellatedMeshConfig parseCastellatedMesh(
    const std::shared_ptr<OpenFoamDictionary> dict);

// Parse snap control section of snappyHexMeshDict
SnapControlConfig parseSnapControlConfig(
    const std::shared_ptr<OpenFoamDictionary> dict);

// Parse layer control section of snappyHexMeshDict
LayerControlConfig parseLayerControlConfig(
    const std::shared_ptr<OpenFoamDictionary> dict);

// Update existing snappyHexMeshDict file
QString updateSnappyHexMeshDict(std::shared_ptr<OpenFoamDictionary> dict,
    const CastellatedMeshConfig&, const SnapControlConfig&,
    const LayerControlConfig&);

// Create new snappyHexMeshDict file
QString createSnappyHexMeshDict(
    const std::map<QString, SurfaceFeatureEntry>& entryMap,
    const CastellatedMeshConfig&, const SnapControlConfig&,
    const LayerControlConfig&, const QString& openFoamPath);
};

#endif  // PARSER_SNAPPY_HEX_MESH_DICT_H_
