#ifndef PARSER_FUNCTION_OBJECT_H_
#define PARSER_FUNCTION_OBJECT_H_

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector3D>

#include <array>

namespace CaseIO {

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

// Create overall functions block
QString createFunctionsBlock(
    const std::vector<std::unique_ptr<FunctionObject>>& functions);

// Create text for forces function object
void createForcesBlock(QTextStream& stream, const ForcesConfig* config);

// Create text for forceCoeffs function object
void createForceCoeffsBlock(QTextStream& stream,
                            const ForceCoeffsConfig* config);

// Create text for fieldMinMax function object
void createFieldMinMaxBlock(QTextStream& stream,
                            const FieldMinMaxConfig* config);

// Create text for probes function object
void createProbesBlock(QTextStream& stream, const ProbesConfig* config);

// Create text for surfaces function object
void createSurfacesBlock(QTextStream& stream, const SurfacesConfig* config);

// Create text for yPlus function object
void createYPlusBlock(QTextStream& stream, const YPlusConfig* config);
};

#endif  // PARSER_FUNCTION_OBJECT_H_
