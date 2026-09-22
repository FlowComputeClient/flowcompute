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

#ifndef PARSER_DECOMPOSE_PAR_DICT_H_
#define PARSER_DECOMPOSE_PAR_DICT_H_

#include <QObject>
#include <QString>

#include "open_foam_dictionary.h"

namespace CaseIO {

struct ParallelConfig {
    Q_GADGET

 public:
    enum class DecompositionMethod {
        Scotch,
        Metis,
        Simple,
        Hierarchical
    };
    Q_ENUM(DecompositionMethod)

    bool useParallel = false;
    unsigned int numSubdomains = 4;
    DecompositionMethod method = DecompositionMethod::Scotch;
    unsigned int nx = 2; unsigned int ny = 2; unsigned int nz = 1;
    QString order = "xyz";
    double delta = 0.001;
};

// Parse decomposeParDict
void parseDecomposeParDict(std::shared_ptr<OpenFoamDictionary> dict,
                            ParallelConfig& config);

// Create new decomposeParDict from a structure
QString createDecomposeParDict(const ParallelConfig& cfg,
                               const QString& openFoamPath);

// Create new decomposeParDict from the number of cores
QString createDecomposeParDict(const QString& openFoamPath, int numCores);
};

#endif  // PARSER_DECOMPOSE_PAR_DICT_H_
