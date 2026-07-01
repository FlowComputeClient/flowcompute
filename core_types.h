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

#ifndef CORE_TYPES_H_
#define CORE_TYPES_H_

#include <QHash>
#include <QString>
#include <QStringList>

#include <vector>
#include <unordered_map>

namespace FlowCompute {
    Q_NAMESPACE

enum class Algorithm {
    SIMPLE = 0,
    PIMPLE,
    PISO,
    CENTRAL_UPWIND,
    NONE,
    UNKNOWN
};
Q_ENUM_NS(Algorithm)

enum class FieldClass {
    volScalarField = 0,
    volVectorField,
    volSphericalTensorField,
    volSymmTensorField,
    volTensorField,
    surfaceScalarField,
    surfaceVectorField,
    surfaceSphericalTensorField,
    surfaceSymmTensorField,
    surfaceTensorField,
    pointScalarField,
    pointVectorField,
    pointSphericalTensorField,
    pointSymmTensorField,
    pointTensorField,
    Unknown,
};
Q_ENUM_NS(FieldClass)

enum class LinearSolver {
    GAMG = 0,
    smoothSolver,
    PBiCGStab,
    PCG,
    PBiCG,
    diagonal
};
Q_ENUM_NS(LinearSolver)

enum class Smoother {
    symGaussSeidel = 0,
    GaussSeidel,
    DICGaussSeidel,
    DILUGaussSeidel,
    Jacobi,
    NONE
};
Q_ENUM_NS(Smoother)

enum class Preconditioner {
    DILU = 0,
    DIC,
    GAMG,
    FDIC,
    ILU,
    NONE
};
Q_ENUM_NS(Preconditioner)

enum class DecompositionMethod {
    Scotch,
    Metis,
    Simple,
    Hierarchical
};
Q_ENUM_NS(DecompositionMethod)

inline const QStringList patchTypes = { "patch", "wall", "empty", "symmetry", "wedge" };

// Store entered results
struct BoundaryCondition {
    QString type;
    std::unordered_map<QString, QString> parameters;
};

struct MeshPatch {
    QString name;
    QString newName;
    QString type;
    bool nameChanged = false;
    bool typeChanged = false;
};

struct BoundaryConditionDef {
    QString name;
    QStringList categories;
    QStringList types;
    QStringList patchTypes;
    QStringList parameters;
};

struct FieldData {
    QString dimension = "[0 0 0 0 0 0 0]";
    FieldClass fieldClass = FieldClass::volScalarField;
    QString internalField = "uniform 0";
    std::vector<std::pair<QString, BoundaryCondition>> bcs;
};

struct FieldDef {
    QString dimensions;
    QString defaultValue;
    FieldClass fieldClass;
};

struct SolverDef {
    QString name;
    Algorithm algorithm = Algorithm::PIMPLE;
    QStringList fields;
    QStringList transportProperties;
    QStringList thermalProperties;
    bool isSteadyState = false;
};

struct TransportPropertyDef {
    QString name;
    QString dimensions;
    QString defaultVal;
};

struct SolverFamily {
    QString name;
    QList<SolverDef> solvers;
};

inline QString getBaseMathType(FieldClass foamClass) {
    switch (foamClass) {
    case FieldClass::volScalarField:
    case FieldClass::surfaceScalarField:
    case FieldClass::pointScalarField:
        return "scalar";

    case FieldClass::volVectorField:
    case FieldClass::surfaceVectorField:
    case FieldClass::pointVectorField:
        return "vector";

    case FieldClass::volSymmTensorField:
    case FieldClass::surfaceSymmTensorField:
    case FieldClass::pointSymmTensorField:
        return "symmTensor";

    case FieldClass::volTensorField:
    case FieldClass::surfaceTensorField:
    case FieldClass::pointTensorField:
        return "tensor";

    case FieldClass::volSphericalTensorField:
    case FieldClass::surfaceSphericalTensorField:
    case FieldClass::pointSphericalTensorField:
        return "sphericalTensor";

    default:
        return "unknown";
    }
}

struct TurbulenceModel {
    QString name;
    QString description;
    QStringList fields;
};

using TurbulenceDatabase = QMap<QString, QMap<QString, std::vector<TurbulenceModel>>>;

};

#endif // CORE_TYPES_H_
