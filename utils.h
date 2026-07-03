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

#ifndef UTILS_H_
#define UTILS_H_

#include <QString>
#include <QWidget>

#include "./core_types.h"

namespace Utils {

enum class ParseErrorAction {
    EditFile,
    Overwrite,
    Cancel
};

QString createFoamHeader(const QString& objectName, const QString& foamPath);
QString createFoamFooter();
QString createSurfacePatchDict(const QString& openFoamPath,
                               const QString& fileName, double featureAngle);
QString createDecomposeParDict(const QString& openFoamPath, int numCores);
QString createFieldFile(const QString& openFoamPath, QString fieldName,
                        FlowCompute::FieldData data);
ParseErrorAction showParsingErrorMessage(QString fileName, QWidget* parent);

};  // namespace Utils

#endif  // UTILS_H_
