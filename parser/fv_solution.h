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

#ifndef PARSER_FV_SOLUTION_H_
#define PARSER_FV_SOLUTION_H_

#include <memory>

#include <QMap>
#include <QObject>

#include "core_types.h"
#include "open_foam_dictionary.h"

namespace CaseIO {

struct ResidualControl {
    bool isEnabled;
    QString fieldName;
    QString tolerance;
};

struct SimpleConfig {
    unsigned int nNonOrthogonalCorrectors = 2;
    bool consistent = false;
    unsigned int pRefCell = 0;
    double pRefValue = 0.0;
    std::vector<ResidualControl> resControls;
};

struct PisoConfig {
    bool momentumPredictor = true;
    unsigned int nCorrectors = 2;
    unsigned int nNonOrthogonalCorrectors = 2;
    unsigned int pRefCell = 0;
    double pRefValue = 0.0;
};

struct PimpleConfig {
    bool momentumPredictor = true;
    unsigned int nOuterCorrectors = 2;
    unsigned int nCorrectors = 2;
    unsigned int nNonOrthogonalCorrectors = 1;
    unsigned int pRefCell = 0;
    double pRefValue = 0.0;
    std::vector<ResidualControl> resControls;
};

struct FieldMathConfig {
    FlowCompute::LinearSolver solver = FlowCompute::LinearSolver::GAMG;
    FlowCompute::Smoother smoother = FlowCompute::Smoother::NONE;
    FlowCompute::Preconditioner preconditioner =
        FlowCompute::Preconditioner::NONE;

    // Standard tolerances
    double absTolerance = 1e-6;
    double relTolerance = 0.1;

    // Final iteration overrides
    bool hasFinalOverride = false;
    double finalAbsTolerance = 1e-6;
    double finalRelTolerance = 0.0;
    double relaxationFactor = 0.5;
    bool isFieldsRelaxation = false;
};

struct MathConfig {
    QMap<QString, FieldMathConfig> fieldMathConfigs;
    std::variant<std::monostate, SimpleConfig, PimpleConfig, PisoConfig>
        algorithmConfig;
};

// Parse fvSolution
void parseFvSolution(std::shared_ptr<OpenFoamDictionary> dict,
                     MathConfig& config);

// Create new fvSolution
QString createFvSolution(const MathConfig& cfg, const QString& openFoamPath,
                         bool isCompressible, bool isTransient, bool isOpenCFD);
};

#endif  // PARSER_FV_SOLUTION_H_
