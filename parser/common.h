#ifndef PARSER_COMMON_H_
#define PARSER_COMMON_H_

#include <QMetaEnum>
#include <QMap>
#include <QString>

namespace CaseIO {
    Q_NAMESPACE

enum class ParseErrorAction {
    EditFile,
    Overwrite,
    Cancel
};

// Patch type
enum class PatchType {
    patch = 0,
    wall,
    symmetryPlane,
    empty,
    wedge,
    cyclic,
    Count
};
Q_ENUM_NS(PatchType)

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

enum class TransportModel {
    Newtonian,
    CrossPowerLaw,
    BirdCarreau,
    HerschelBulkley
};
Q_ENUM_NS(TransportModel)

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

struct PhysicsConfig {
    QString simulationType = "RAS";
    QString model = "kOmegaSST";
    bool useTurbulence = true;
    DeltaModel deltaModel = DeltaModel::cubeRootVol;
    TransportModel transportModel = TransportModel::Newtonian;
    QMap<QString, QString> fluidProperties;
};

template<typename T>
QString enumToString(T value, const QString& fallback) {
    QMetaEnum metaEnum = QMetaEnum::fromType<T>();
    const char* keyString = metaEnum.valueToKey(static_cast<int>(value));
    if (keyString) {
        return QString::fromLatin1(keyString);
    }
    return fallback;
}

// Create header for dictionary file
QString createFoamHeader(const QString& objectName, const QString& foamPath,
                            const QString& className="dictionary");

// Create footer for dictionary file
QString createFoamFooter();

// Create message box for parsing error
ParseErrorAction showParsingErrorMessage(QString fileName, QWidget* parent);
};

#endif  // PARSER_COMMON_H_
