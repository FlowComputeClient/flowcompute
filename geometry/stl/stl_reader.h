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

#ifndef GEOMETRY_STL_STL_READER_H_
#define GEOMETRY_STL_STL_READER_H_

#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

#include <utility>

#include "../graphic_data.h"

namespace StlReader {

std::pair<RenderData, bool> readStlFile(const QString& fileName,
                                    const QByteArray& fileData);
GeometryMetrics readMetrics(const QByteArray& fileData);
};

#endif  // GEOMETRY_STL_STL_READER_H_
