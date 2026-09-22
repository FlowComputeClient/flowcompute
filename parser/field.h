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

#ifndef PARSER_FIELD_H_
#define PARSER_FIELD_H_

#include <QObject>
#include <QString>

#include "open_foam_dictionary.h"
#include "common.h"
#include "core_types.h"

namespace CaseIO {

// Store data from a field file
struct FieldData {
    QString dimension = "[0 0 0 0 0 0 0]";
    FlowCompute::FieldClass fieldClass =
        FlowCompute::FieldClass::volScalarField;
    QString internalField = "uniform 0";
    std::vector<std::pair<QString, BoundaryCondition>> bcs;
};

// Parse a field file
void parseFieldFile(std::shared_ptr<OpenFoamDictionary> dict,
                    CaseIO::FieldData& fieldData);

// Create a new field file
QString createFieldFile(const QString& fieldName,
                        const CaseIO::FieldData& data,
                        const QString& openFoamPath);
};

#endif  // PARSER_FIELD_H_
