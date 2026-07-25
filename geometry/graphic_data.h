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

#ifndef GEOMETRY_GRAPHIC_DATA_H_
#define GEOMETRY_GRAPHIC_DATA_H_

#include <QVector3D>

#include <array>
#include <string>
#include <vector>

enum class RenderType {
    Surface = 0,
    Mesh,
    Result,
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
    std::vector<std::string> patches;
    BoundingBox bbox;
    QVector3D intpoint;
    bool isValid;
};

struct RenderPatch {
    char name[64];
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t firstLineIndex;
    uint32_t lineIndexCount;
};

struct RenderData {
    RenderType format;
    std::vector<float> data;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> lineIndices;
    std::vector<RenderPatch> patches;
    std::array<float, 3> boundingBoxMin;
    std::array<float, 3> boundingBoxMax;
};

#endif  // GEOMETRY_GRAPHIC_DATA_H_
