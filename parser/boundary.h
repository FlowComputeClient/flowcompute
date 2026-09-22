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

#ifndef PARSER_BOUNDARY_H_
#define PARSER_BOUNDARY_H_

#include <memory>

#include "open_foam_dictionary.h"
#include "common.h"

namespace CaseIO {

struct BoundaryFileParts {
    QByteArray header;
    QByteArray payload;
    QByteArray footer;
};

// Parse boundary file
std::vector<MeshPatch> parseBoundary(const QByteArray& fileData);
QStringList getPatches(const QByteArray& fileData);

// Update boundary file
QString updateBoundaryFile(std::shared_ptr<OpenFoamDictionary> dict,
    const std::vector<MeshPatch>& filtered);

BoundaryFileParts splitBoundaryFile(const QByteArray& rawData);
QByteArray updateHeaderCount(const QByteArray& header, int removedCount);
QByteArray removeEmptyPatches(const QByteArray& boundaryData);
};

#endif  // PARSER_BOUNDARY_H_
