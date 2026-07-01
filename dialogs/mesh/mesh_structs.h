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
    int edgeLevel = 3;
    bool openEdges = true;
    bool writeObj = true;
};

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

#endif // MESH_STRUCTS_H
