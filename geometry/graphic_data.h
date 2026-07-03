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

const std::array<std::array<float, 4>, 10> patchColors = {{
    // dark purple
    {0.3686274509803922, 0.20784313725490197, 0.6941176470588235, 1.0 },
    // bright red
    {0.9372549019607843, 0.3254901960784314, 0.3137254901960784, 1.0 },
    // forest green
    {0.2196078431372549, 0.5568627450980392, 0.23529411764705882, 1.0 },
    // light blue
    {0.39215686274509803, 0.7098039215686275, 0.9647058823529412, 1.0 },
    // bright yellow
    {1.0, 0.9333333333333333, 0.34509803921568627, 1.0 },
    // vivid orange
    {0.984313725490196, 0.5490196078431373, 0.0, 1.0 },
    // lime green
    {0.4627450980392157, 1.0, 0.011764705882352941, 1.0 },
    // soft pink
    {0.9568627450980393, 0.5607843137254902, 0.6941176470588235, 1.0 },
    // muted brown
    {0.6313725490196078, 0.5333333333333333, 0.4980392156862745, 1.0 },
    // pale teal
    {0.5019607843137255, 0.796078431372549, 0.7686274509803922, 1.0 },
}};

enum class RenderType {
    Surface = 0,
    Mesh,
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
    std::vector<std::string> patches;
    BoundingBox bbox;
    QVector3D intpoint;
    bool isValid;
};

struct RenderPatch {
    char name[64];
    uint32_t first;
    uint32_t count;
};

struct RenderData {
    RenderType format;
    std::vector<float> data;
    std::vector<uint32_t> indices;
    std::vector<RenderPatch> patches;
    std::array<float, 3> boundingBoxMin;
    std::array<float, 3> boundingBoxMax;
};

#endif  // GEOMETRY_GRAPHIC_DATA_H_
