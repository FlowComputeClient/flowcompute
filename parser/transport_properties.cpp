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

#include <QDebug>
#include <QRegularExpression>

#include "transport_properties.h"

// Parse transport properties file
void CaseIO::parseTransportProperties(std::shared_ptr<OpenFoamDictionary> dict,
                                        TransportConfig& cfg) {
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
QString CaseIO::createTransportProperties(const TransportConfig& cfg,
                        const QString& openFoamPath, bool isOpenCFD) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Write the standard OpenFOAM header
    if (isOpenCFD) {
        out << createFoamHeader("transportProperties", openFoamPath);
    } else {
        out << createFoamHeader("physicalProperties", openFoamPath);
    }

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
    out << createFoamFooter();
    return dictStr;
}
