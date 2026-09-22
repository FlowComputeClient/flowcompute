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

#ifndef PARSER_SURFACE_FEATURE_H_
#define PARSER_SURFACE_FEATURE_H_

#include <map>
#include <memory>

#include "open_foam_dictionary.h"

namespace CaseIO {

struct SurfaceFeatureEntry {
    double angle = 150.0;
    int edgeLevel = 3;
    bool openEdges = true;
    bool writeObj = true;
};

// Parse surface feature file
std::map<QString, SurfaceFeatureEntry> parseSurfaceFeatureData(
    const std::shared_ptr<OpenFoamDictionary> dict,
    const QStringList& geometryFiles);

// Update existing surface feature file
QString updateSurfaceFeatureDict(
    std::shared_ptr<OpenFoamDictionary> dict,
    const std::map<QString, SurfaceFeatureEntry>& entryMap);

// Create new surface feature file
QString createSurfaceFeatureDict(
    const std::map<QString, SurfaceFeatureEntry>& entryMap,
    QString openFoamPath);
};

#endif  // PARSER_SURFACE_FEATURE_H_
