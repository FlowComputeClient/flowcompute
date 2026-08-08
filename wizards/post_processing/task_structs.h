#ifndef WIZARDS_POST_PROCESSING_TASK_STRUCTS_H_
#define WIZARDS_POST_PROCESSING_TASK_STRUCTS_H_

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector3D>

#include <array>

// Base structure
struct FunctionObject {
    Q_GADGET

 public:
    enum class FuncObjType {
        forces = 0,
        forceCoeffs,
        fieldMinMax,
        probes,
        surfaces,
        yPlus
    };
    Q_ENUM(FuncObjType)

    enum class ControlType {
        timeStep = 0,
        writeTime,
        runTime,
        adjustableRunTime,
        clockTime,
        cpuTime,
        onEnd,
        none
    };
    Q_ENUM(ControlType)

    enum class InterpolationType {
        cell,
        cellPatchConstrained,
        cellPoint,
        cellPointFace,
        cellPointWallModified,
        pointMVC
    };
    Q_ENUM(InterpolationType)

    QString name;
    FuncObjType type;
    bool active = true;
    ControlType executeControl = ControlType::timeStep;
    double executeInterval = 1.0;
    ControlType writeControl = ControlType::writeTime;
    double writeInterval = 1.0;
    bool logOutput = false;
    virtual ~FunctionObject() = default;
};

// forces function object
struct ForcesConfig : public FunctionObject {
    bool writeFields = false;
    QStringList patches;
    QString pName = "p";
    QString rhoName = "rho";
    double rhoInf = 1.224;
    QString UName = "U";
    double pRef = 0.0;
    std::array<double, 3> centerOfRotation = {0.0, 0.0, 0.0};
    bool includePorosity = false;
};

// forceCoeffs function object
struct ForceCoeffsConfig : public FunctionObject {
    bool writeFields = false;
    QStringList patches;
    QString pName = "p";
    QString rhoName = "rho";
    double rhoInf = 1.224;
    QString UName = "U";
    double magUInf = 1.0;
    double lRef = 1.0;
    double aRef = 1.0;
    double pRef = 0.0;
    std::array<double, 3> centerOfRotation = {0.0, 0.0, 0.0};
    std::array<double, 3> liftDir = {0.0, 0.0, 1.0};
    std::array<double, 3> dragDir = {1.0, 0.0, 0.0};
    std::array<double, 3> pitchAxis = {0.0, 1.0, 0.0};
    bool includePorosity = false;
};

struct FieldMinMaxConfig : public FunctionObject {
    Q_GADGET

 public:
    QStringList fields;
    enum class Mode {
        Magnitude,
        Component
    };
    Q_ENUM(Mode)
    Mode mode = Mode::Magnitude;
    bool location = true;
};

struct ProbesConfig : public FunctionObject {
    QStringList fields = {};
    std::vector<QVector3D> probeLocations = {};
    InterpolationType interpolationScheme = InterpolationType::cell;
    bool fixedLocations = true;
    bool includeOutOfBounds = true;
    bool verbose = false;
    bool sampleOnExecute = false;
};

struct SurfaceDef {
    Q_GADGET

 public:
    enum class SurfaceType {
        patch = 0,
        cuttingPlane,
        isoSurface,
        meshedSurface,
        isoSurfaceCell,
        distanceSurface
    };
    Q_ENUM(SurfaceType)

    QString name;
    SurfaceType type;
    std::unordered_map<QString, QString> parameters;
};

struct SurfacesConfig : public FunctionObject {
    Q_GADGET

 public:
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
    Q_ENUM(SurfaceFormat)

    SurfaceFormat surfaceFormat = SurfaceFormat::vtk;
    InterpolationType interpolationScheme = InterpolationType::cell;
    QStringList fields = {};
    std::vector<SurfaceDef> surfaces = {};
};

// forceCoeffs function object
struct YPlusConfig : public FunctionObject {
    QStringList patches;
};

#endif  // WIZARDS_POST_PROCESSING_TASK_STRUCTS_H_
