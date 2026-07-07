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

#ifndef SOLVER_IO_H
#define SOLVER_IO_H

#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include "../../parser/open_foam_dictionary.h"
#include "solver_structs.h"

namespace SolverIO {

    struct BoundaryFileParts {
        QByteArray header;
        QByteArray payload;
        QByteArray footer;
    };

    // Parse functions
    ControlConfig parseControlConfig(std::shared_ptr<OpenFoamDictionary> dict);
    PhysicsConfig parseTurbulenceProperties(std::shared_ptr<OpenFoamDictionary> dict);
    void parseTransportProperties(std::shared_ptr<OpenFoamDictionary> dict,
                                  PhysicsConfig& cfg);
    void parseFieldFile(std::shared_ptr<OpenFoamDictionary> dict,
                        FlowCompute::FieldData& fieldData);
    void parseSolutionFile(std::shared_ptr<OpenFoamDictionary> dict, MathConfig& config);
    void parseParallelFile(std::shared_ptr<OpenFoamDictionary> dict, ParallelConfig& config);

    // Dealing with boundary files
    std::vector<FlowCompute::MeshPatch> parseBoundaryPatches(const QByteArray& fileData);
    QString updateBoundaryFile(std::shared_ptr<OpenFoamDictionary> dict,
        const std::vector<FlowCompute::MeshPatch>& filtered);
    BoundaryFileParts splitBoundaryFile(const QByteArray& rawData);
    QByteArray updateHeaderCount(const QByteArray& header, int removedCount);
    QByteArray removeEmptyPatches(const QByteArray& boundaryData);

    // Creation functions
    QString createControlDict(const ControlConfig& config, const VisualizationConfig& visCfg,
                              const QString& openFoamPath);
    QString createTurbulenceProperties(const PhysicsConfig& config, const QString& openFoamPath);
    QString createTransportProperties(const PhysicsConfig& config, const QString& openFoamPath);
    QString createFieldFile(const QString& fieldName, const FlowCompute::FieldData& data,
                            const QString& openFoamPath);
    QString createSolutionFile(const MathConfig& cfg, const QString& openFoamPath);
    QString createParallelFile(const ParallelConfig& cfg, const QString& openFoamPath);
}

#endif  // SOLVER_IO_H
