#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <QHash>
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
Q_ENUM_NS(FieldClass)

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
    std::unordered_map<QString, BoundaryCondition> bcs;
};

struct BoundaryConditionDef {
    QString name;
    QStringList categories;
    QStringList types;
    QStringList patchTypes;
    QStringList parameters;
};

struct FieldData {
    QString dimension;
    QString fieldClass;
    QString internalField;
    std::unordered_map<QString, BoundaryCondition> bcs;
};

struct FieldDef {
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

inline QString getBaseMathType(FieldClass foamClass) {
    switch (foamClass) {
    case FieldClass::VolScalarField:
    case FieldClass::SurfaceScalarField:
    case FieldClass::PointScalarField:
        return "scalar";

    case FieldClass::VolVectorField:
    case FieldClass::SurfaceVectorField:
    case FieldClass::PointVectorField:
        return "vector";

    case FieldClass::VolSymmTensorField:
    case FieldClass::SurfaceSymmTensorField:
    case FieldClass::PointSymmTensorField:
        return "symmTensor";

    case FieldClass::VolTensorField:
    case FieldClass::SurfaceTensorField:
    case FieldClass::PointTensorField:
        return "tensor";

    case FieldClass::VolSphericalTensorField:
    case FieldClass::SurfaceSphericalTensorField:
    case FieldClass::PointSphericalTensorField:
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

#endif // CORE_TYPES_H
