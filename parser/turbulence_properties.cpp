#include "turbulence_properties.h"

#include <QDebug>
#include <QDir>
#include <QMetaEnum>
#include <QRegularExpression>

// Parse the turbulence properties file
CaseIO::PhysicsConfig CaseIO::parseTurbulenceProperties(
    std::shared_ptr<OpenFoamDictionary> dict) {
    PhysicsConfig cfg;
    if (!dict) { return cfg; }

    // Simulation Type
    QString simType = dict->getString("simulationType");
    if (!simType.isEmpty()) {
        cfg.simulationType = simType;
    }

    // Parsing depends based on simulationType
    if (cfg.simulationType == "RAS") {
        QString parsedModel = dict->getString("RAS/RASModel");
        if (!parsedModel.isEmpty()) cfg.model = parsedModel;

    } else if (cfg.simulationType == "LES") {
        QString parsedModel = dict->getString("LES/LESModel");
        if (!parsedModel.isEmpty()) cfg.model = parsedModel;

        // Extract delta specifically for LES
        QString deltaStr = dict->getString("LES/delta");
        if (!deltaStr.isEmpty()) {
            bool ok = false;
            int enumValue =
                QMetaEnum::fromType<CaseIO::DeltaModel>().keyToValue(
                    deltaStr.toUtf8().constData(), &ok);
            if (ok) {
                cfg.deltaModel = static_cast<CaseIO::DeltaModel>(enumValue);
            }
        }
    } else if (cfg.simulationType == "laminar") {
        cfg.model = "laminar";
        return cfg;
    }

    // useTurbulence
    QString turbulenceStr = dict->getString("turbulence").toLower();
    if (!turbulenceStr.isEmpty()) {
        cfg.useTurbulence = (turbulenceStr == "yes" || turbulenceStr == "true"
                             || turbulenceStr == "on");
    }

    return cfg;
}

// Create a new turbulence properties file
QString CaseIO::createTurbulenceProperties(const PhysicsConfig& cfg,
                                           const QString& openFoamPath) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Write the standard OpenFOAM header
    out << createFoamHeader("turbulenceProperties", openFoamPath);

    // Upgraded lambda with indentation support for sub-dictionaries
    auto writeEntry = [&out](const QString& keyword,
                             const QString& value, int indentLevel = 0,
                             bool addEmptyLine = false) {
        QString indent(indentLevel * 4, ' ');
        out << indent << keyword.leftJustified(20, ' ') << value << ";\n";
        if (addEmptyLine) out << "\n";
    };

    // Standard OpenFOAM switch formatting for turbulence settings
    auto toFoamSwitch = [](bool val) { return val ? "on" : "off"; };

    // Base simulation type (RAS, LES, or laminar)
    writeEntry("simulationType", cfg.simulationType, 0, true);

    // Generate the specific sub-dictionary if it's not laminar
    if (cfg.simulationType == "RAS" || cfg.simulationType == "LES") {
        out << cfg.simulationType << "\n{\n";

        // e.g., RASModel or LESModel
        writeEntry(cfg.simulationType + "Model", cfg.model, 1);

        // Turbulence toggle
        writeEntry("turbulence", toFoamSwitch(cfg.useTurbulence), 1);

        // Standard coefficient output switch
        writeEntry("printCoeffs", "on", 1);

        // If LES, write the delta model using the enumToString template
        if (cfg.simulationType == "LES") {
            writeEntry("delta", enumToString(cfg.deltaModel, "cubeRootVol"), 1);
        }

        out << "}\n\n";
    }

    // Write closing separator
    out << "// ************************************************************************* //\n";

    return dictStr;
}
