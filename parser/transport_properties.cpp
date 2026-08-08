#include <QDebug>
#include <QDir>
#include <QRegularExpression>

#include "transport_properties.h"

// Parse transport properties file
void CaseIO::parseTransportProperties(std::shared_ptr<OpenFoamDictionary> dict,
                                        PhysicsConfig& cfg) {
    if (!dict) { return; }

    // transportModel
    QString transportModelStr = dict->getString("transportModel");
    if (!transportModelStr.isEmpty()) {
        bool ok = false;
        int enumValue = QMetaEnum::fromType<TransportModel>().keyToValue(
            transportModelStr.toUtf8().constData(), &ok);
        if (ok) {
            cfg.transportModel = static_cast<TransportModel>(enumValue);
        }
    }

    // Standard properties
    QStringList standardProperties = {
        "nu", "rho", "Pr", "Prt", "TRef", "Cp", "Cv", "k", "n", "alpha"
    };

    // Extract properties into the dynamic map
    for (const QString& propName : standardProperties) {
        QString propValue = dict->getString(propName);
        if (!propValue.isEmpty()) {
            cfg.fluidProperties.insert(propName, propValue);
        }
    }
}

// Create new transport properties file
QString CaseIO::createTransportProperties(const PhysicsConfig& cfg,
                                          const QString& openFoamPath) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Write the standard OpenFOAM header
    out << createFoamHeader("transportProperties", openFoamPath);

    // Standard lambda for formatting dictionary entries
    auto writeEntry = [&out](const QString& keyword, const QString& value,
                             bool addEmptyLine = false) {
        out << keyword.leftJustified(20, ' ') << value << ";\n";
        if (addEmptyLine) out << "\n";
    };

    // Write the transport model using the enum template
    writeEntry("transportModel", enumToString(cfg.transportModel,
                                              "Newtonian"), true);

    // Iterate through the fluid properties map
    if (!cfg.fluidProperties.isEmpty()) {
        for (auto it = cfg.fluidProperties.cbegin();
            it != cfg.fluidProperties.cend(); ++it) {
            writeEntry(it.key(), it.value());
        }
        out << "\n";
    }

    // Write closing separator
    out << "// ************************************************************************* //\n";

    return dictStr;
}
