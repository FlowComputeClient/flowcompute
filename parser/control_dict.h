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

#ifndef PARSER_CONTROL_DICT_H_
#define PARSER_CONTROL_DICT_H_

#include <memory>

#include "open_foam_dictionary.h"
#include "common.h"

namespace CaseIO {

struct ControlConfig {
    QString solverFamily;
    QString application;
    QString solver;

    // Run control
    StartSolverType startFrom = StartSolverType::startTime;
    double startTime = 0.0;
    EndSolverType stopAt = EndSolverType::endTime;
    double endTime = 0.5;
    double deltaT = 1.0;

    // Time step adjustment
    bool adjustTimeStep = false;
    double maxCo = 3.0;

    // Data writing
    bool writeCompression = false;
    bool runTimeModifiable = true;
    WriteFormatType writeFormat = WriteFormatType::binary;
    WriteControlType writeControl = WriteControlType::timeStep;
    double writeInterval = 20.0;
    int purgeWrite = 0;
};

// Parse controlDict
ControlConfig parseControlDict(std::shared_ptr<OpenFoamDictionary> dict);

// Update existing controlDict
QString updateControlDict(std::shared_ptr<OpenFoamDictionary> dict,
    ControlConfig& cfg, const QString& funcString);

// Create new controlDict file
QString createControlDict(const ControlConfig& cfg, const QString& openFoamPath,
    const QString& funcString);
};

#endif  // PARSER_CONTROL_DICT_H_
