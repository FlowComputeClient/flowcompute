#ifndef SOLVER_STRUCTS_H
#define SOLVER_STRUCTS_H

#include <QMap>
#include <QString>

#include "../../core_types.h"

enum class WriteControlType {
    timeStep = 0,
    runTime,
    adjustableRunTime,
    cpuTime,
    clockTime,
    none,
    Count
};

enum class StartSolverType {
    startTime = 0,
    latestTime,
    firstTime
};

enum class EndSolverType {
    endTime = 0,
    writeNow,
    noWriteNow,
    nextWrite
};

struct ControlConfig {
    QString solverCategory;
    QString solver;

    // Run control
    StartSolverType startFrom = StartSolverType::startTime;
    double startTime = 0.0;
    EndSolverType stopAt = EndSolverType::endTime;
    double endTime = 0.5;
    double deltaT = 1.0;

    // Time step adjustment
    bool adjustTimeStep = false;
    double maxCo = 1.0;

    // Data writing
    WriteControlType writeControl = WriteControlType::timeStep;
    double writeInterval = 20.0;
    int purgeWrite = 0;
};

enum class TransportModel {
    Newtonian,
    CrossPowerLaw,
    BirdCarreau,
    HerschelBulkley,
    UNKNOWN
};

enum class DeltaType {
    cubeRootVol = 0,
    maxDeltaxyz,
    smoothDelta,
    vanDriest,
    Prandtl,
    IDDESDelta,
    SLADelta,
    calculated
};

struct PhysicsConfig {

    QString simulationType;
    QString model;
    DeltaType delta;
    TransportModel transportModel;
    QMap<QString, QString> fluidProperties;
};

struct SimpleConfig {
    int nNonOrthogonalCorrectors = 0;
    bool consistent = false;
    int pRefCell = 0;
    double pRefValue = 0.0;
};

struct PisoConfig {
    int nCorrectors = 2;
    int nNonOrthogonalCorrectors = 0;
    int pRefCell = 0;
    double pRefValue = 0.0;
};

struct PimpleConfig {
    int nOuterCorrectors = 1;
    int nCorrectors = 2;
    int nNonOrthogonalCorrectors = 0;
    int pRefCell = 0;
    double pRefValue = 0.0;
};

struct FieldMathConfig {
    FlowCompute::LinearSolver solver;
    FlowCompute::Smoother smoother = FlowCompute::Smoother::NONE;
    FlowCompute::Preconditioner preconditioner = FlowCompute::Preconditioner::NONE;
    double absTolerance;
    double relTolerance;
    double relaxationFactor;
};

struct MathConfig {
    QMap<QString, FieldMathConfig> fieldMathConfigs;
    std::variant<SimpleConfig, PimpleConfig, PisoConfig> algorithmConfig;
};

struct ParallelConfig {
    bool useParallel;
    int numSubdomains;
    FlowCompute::DecompositionMethod method;
    int nx = 1;
    int ny = 1;
    int nz = 1;
    QString order = "xyz";
};

#endif // SOLVER_STRUCTS_H
