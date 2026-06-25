#ifndef SOLVER_STRUCTS_H
#define SOLVER_STRUCTS_H

#include <QMap>
#include <QString>

#include "../../core_types.h"

enum {
    Page_Control = 0,
    Page_Transient = 1,
    Page_Physics = 2,
    Page_Boundary = 3,
    Page_Algorithm = 4,
    Page_Simple = 5,
    Page_Pimple = 6,
    Page_Piso = 7,
    Page_Parallel = 8,
    Page_Visualization1 = 9
};

namespace Solver {
Q_NAMESPACE

enum class WriteControlType {
    timeStep = 0,
    runTime,
    adjustableRunTime,
    cpuTime,
    clockTime,
    none
};
Q_ENUM_NS(WriteControlType)

enum class WriteFormatType {
    ascii = 0,
    binary
};
Q_ENUM_NS(WriteFormatType)

enum class StartSolverType {
    startTime = 0,
    latestTime,
    firstTime
};
Q_ENUM_NS(StartSolverType)

enum class EndSolverType {
    endTime = 0,
    writeNow,
    noWriteNow,
    nextWrite
};
Q_ENUM_NS(EndSolverType)

enum class TransportModel {
    Newtonian,
    CrossPowerLaw,
    BirdCarreau,
    HerschelBulkley
};
Q_ENUM_NS(TransportModel)

enum class DeltaModel {
    cubeRootVol = 0,
    maxDeltaxyz,
    smoothDelta,
    vanDriest,
    Prandtl,
    IDDESDelta,
    SLADelta,
    calculated
};
Q_ENUM_NS(DeltaModel)

enum class SurfaceFormat {
    vtk = 0,
    obj,
    raw,
    ensight,
    foam,
    stl,
    nastran,
    tri,
    x3d,
    ac3d
};
Q_ENUM_NS(SurfaceFormat)

enum class VisWriteControlType {
    timeStep = 0,
    writeTime,
    runTime,
    adjustableRunTime,
    onEnd,
    clockTime,
    cpuTime,
    none
};
Q_ENUM_NS(VisWriteControlType)

enum class InterpolationType {
    cell = 0,
    cellPoint,
    cellPointFace,
    cellPointWallModified,
    pointMVC
};
Q_ENUM_NS(InterpolationType)

enum class SurfaceType {
    patch = 0,
    cuttingPlane,
    isoSurface,
    meshedSurface,
    isoSurfaceCell,
    distanceSurface
};
Q_ENUM_NS(SurfaceType)
}

struct ControlConfig {
    QString solverCategory;
    QString solver;

    // Run control
    Solver::StartSolverType startFrom = Solver::StartSolverType::startTime;
    double startTime = 0.0;
    Solver::EndSolverType stopAt = Solver::EndSolverType::endTime;
    double endTime = 0.5;
    double deltaT = 1.0;

    // Time step adjustment
    bool adjustTimeStep = false;
    double maxCo = 1.0;

    // Data writing
    bool writeCompression = false;
    bool runTimeModifiable = true;
    Solver::WriteFormatType writeFormat = Solver::WriteFormatType::binary;
    Solver::WriteControlType writeControl = Solver::WriteControlType::timeStep;
    double writeInterval = 20.0;
    int purgeWrite = 0;
};

struct PhysicsConfig {
    QString simulationType = "RAS";
    QString model = "kOmegaSST";
    bool useTurbulence = true;
    Solver::DeltaModel deltaModel = Solver::DeltaModel::cubeRootVol;
    Solver::TransportModel transportModel = Solver::TransportModel::Newtonian;
    QMap<QString, QString> fluidProperties;
};

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
    unsigned int nOuterCorrectors = 1;
    unsigned int nCorrectors = 2;
    unsigned int nNonOrthogonalCorrectors = 2;
    unsigned int pRefCell = 0;
    double pRefValue = 0.0;
    std::vector<ResidualControl> resControls;
};

struct FieldMathConfig {
    FlowCompute::LinearSolver solver = FlowCompute::LinearSolver::GAMG;
    FlowCompute::Smoother smoother = FlowCompute::Smoother::NONE;
    FlowCompute::Preconditioner preconditioner = FlowCompute::Preconditioner::NONE;

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
    std::variant<std::monostate, SimpleConfig, PimpleConfig, PisoConfig> algorithmConfig;
};

struct ParallelConfig {
    bool useParallel = false;
    unsigned int numSubdomains = 4;
    FlowCompute::DecompositionMethod method = FlowCompute::DecompositionMethod::Scotch;
    unsigned int nx = 2; unsigned int ny = 2; unsigned int nz = 1;
    QString order = "xyz";
    double delta = 0.001;
};

struct VisualizationSurface {
    QString name;
    Solver::SurfaceType type;
    std::unordered_map<QString, QString> parameters;
};

struct VisualizationConfig {
    Solver::SurfaceFormat surfaceFormat = Solver::SurfaceFormat::vtk;
    Solver::VisWriteControlType writeControl = Solver::VisWriteControlType::timeStep;
    double writeInterval = 50.0;
    Solver::InterpolationType interpolationType = Solver::InterpolationType::cell;
    std::vector<QString> fieldNames;
    std::vector<VisualizationSurface> surfaces;
};

#endif // SOLVER_STRUCTS_H
