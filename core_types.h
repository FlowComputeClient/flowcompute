#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <QString>
#include <QStringList>

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
    VolScalarField = 0,
    VolVectorField,
    VolSphericalTensorField,
    VolSymmTensorField,
    VolTensorField,
    SurfaceScalarField,
    SurfaceVectorField,
    SurfaceSphericalTensorField,
    SurfaceSymmTensorField,
    SurfaceTensorField,
    PointScalarField,
    PointVectorField,
    PointSphericalTensorField,
    PointSymmTensorField,
    PointTensorField,
    Unknown,
};

enum class LinearSolver {
    PCG,
    PBiCG,
    PBiCGStab,
    smoothSolver,
    GAMG,
    diagonal
};
Q_ENUM_NS(LinearSolver)

enum class Smoother {
    symGaussSeidel,
    GaussSeidel,
    DICGaussSeidel,
    DILUGaussSeidel,
    Jacobi,
    NONE
};
Q_ENUM_NS(Smoother)

enum class Preconditioner {
    DIC,
    FDIC,
    DILU,
    ILU,
    GAMG,
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

struct BoundaryPatch {
    QString name;
    QString type;
};

struct BoundaryCondition {
    QString name;
    QStringList categories;
    QStringList types;
    QStringList patchTypes;
    QStringList parameters;
};

struct FieldData {
    QString dimensions;
    QString defaultValue;
    FieldClass fieldClass;
};

struct SolverDetails {
    QString name;
    Algorithm algorithm = Algorithm::PIMPLE;
    QStringList fields;
    bool isSteadyState = false;
};

struct SolverFamily {
    QString familyName;
    QList<SolverDetails> solvers;
};

struct TurbulenceModel {
    QString name;
    QString description;
    QStringList fields;
};

using TurbulenceDatabase = QMap<QString, QMap<QString, QList<FlowCompute::TurbulenceModel>>>;

};

#endif // CORE_TYPES_H
