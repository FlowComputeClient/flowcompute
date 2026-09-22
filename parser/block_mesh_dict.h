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

#ifndef PARSER_BLOCK_MESH_DICT_H_
#define PARSER_BLOCK_MESH_DICT_H_

#include <QDebug>
#include <QRegularExpression>

#include <memory>

#include "open_foam_dictionary.h"
#include "common.h"

namespace CaseIO {

struct Patch {
    QString name;
    PatchType type;
    std::vector<std::array<int, 4>> faces;
};

// Store blockMeshDict data
struct BlockMeshConfig {
    double convertToMeters = 1.0;
    std::vector<std::array<double, 3>> vertices;
    QString shape = "hex";
    int nX = 40, nY = 40, nZ = 40;
    double gradingX = 1.0, gradingY = 1.0, gradingZ = 1.0;
    std::vector<Patch> patches;
};

// Parse blockMeshDict
BlockMeshConfig parseBlockMeshDict(std::shared_ptr<OpenFoamDictionary> dict);

// Update existing blockMeshDict
QString updateBlockMeshDict(std::shared_ptr<OpenFoamDictionary> dict,
                            const BlockMeshConfig& config);

// Create new blockMeshDict
QString createBlockMeshDict(const BlockMeshConfig& config,
                            const QString& openFoamPath);
};

#endif  // PARSER_BLOCK_MESH_DICT_H_
