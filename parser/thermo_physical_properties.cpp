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

#include "thermo_physical_properties.h"
#include "common.h"

// Parse transport properties file
void CaseIO::parseThermophysicalProperties(
    std::shared_ptr<OpenFoamDictionary> dict, ThermoConfig& cfg) {
}

// Create new thermophysical properties file
QString CaseIO::createThermophysicalProperties(const ThermoConfig& cfg,
                            const QString& openFoamPath, bool isOpenCFD) {
    QString dictStr;
    QTextStream out(&dictStr);

    // Write the standard header
    if (isOpenCFD) {
        out << createFoamHeader("thermophysicalProperties", openFoamPath);
    } else {
        out << createFoamHeader("physicalProperties", openFoamPath);
    }

    // Write thermoType sub-dictionary
    out << "thermoType\n"
        << "{\n"
        << "    type            " << cfg.thermoType.type << ";\n"
        << "    mixture         " << cfg.thermoType.mixture << ";\n"
        << "    transport       " << cfg.thermoType.transport << ";\n"
        << "    thermo          " << cfg.thermoType.thermo << ";\n"
        << "    equationOfState " << cfg.thermoType.equationOfState << ";\n"
        << "    specie          " << cfg.thermoType.specie << ";\n"
        << "    energy          " << cfg.thermoType.energy << ";\n"
        << "}\n\n";

    // Write Root-Level Fields
    if (!cfg.inertSpecie.isEmpty()) {
        out << "inertSpecie     " << cfg.inertSpecie << ";\n\n";
    }

    if (!cfg.chemistryReader.isEmpty()) {
        out << "chemistryReader " << cfg.chemistryReader << ";\n\n";
    }

    if (!cfg.foamChemistryFile.isEmpty()) {
        // Adjust the keyword based on the selected reader
        QString fileKeyword = (cfg.chemistryReader == "chemkinReader")
                        ? "CHEMKINFile" : "foamChemistryFile";
        QString thermoKeyword = (cfg.chemistryReader == "chemkinReader")
                        ? "CHEMKINThermoFile" : "foamChemistryThermoFile";

        out << fileKeyword << "      \"" << cfg.foamChemistryFile << "\";\n";
        if (!cfg.foamChemistryThermoFile.isEmpty()) {
            out << thermoKeyword << " \"" <<
                cfg.foamChemistryThermoFile << "\";\n";
        }
        out << "\n";
    }

    // Helper lambda to cleanly serialize pure mixture properties
    auto writePureConfig = [&out, isOpenCFD](const PureMixtureConfig& pureCfg,
                                const QString& blockName, int indentLvl) {
        QString indent(indentLvl * 4, ' ');
        if (!blockName.isEmpty()) {
            out << indent << blockName << "\n" << indent << "{\n";
            indent += "    ";
        }

        // Set the thermo block name based on the OpenFOAM version
        QString thermoKey = isOpenCFD ? "thermodynamics" : "thermo";

        // Define OpenFOAM property categories
        QStringList specieKeys = {"molWeight", "nMoles"};
        QStringList
            transportKeys = {"mu", "Pr", "As", "Ts", "K", "n", "omega", "TRef"};
        QStringList eosKeys = {"rho", "R", "p0", "T0", "B", "C"};

        // Temporary containers to group the properties
        QMap<QString, QMap<QString, std::variant<double,
                        QVector<double>>>> groupedProps;

        // Iterate through the properties map and categorize them
        for (auto it = pureCfg.properties.constBegin();
             it != pureCfg.properties.constEnd(); ++it) {

            if (specieKeys.contains(it.key())) {
                groupedProps["specie"].insert(it.key(), it.value());
            } else if (transportKeys.contains(it.key())) {
                groupedProps["transport"].insert(it.key(), it.value());
            } else if (eosKeys.contains(it.key())) {
                groupedProps["equationOfState"].insert(it.key(), it.value());
            } else {
                // Default remaining properties to the thermo block
                groupedProps[thermoKey].insert(it.key(), it.value());
            }
        }

        // Helper lambda to write each sub-dictionary block
        auto writeSubDict = [&](const QString& dictName) {
            if (!groupedProps.contains(dictName) ||
                groupedProps[dictName].isEmpty()) {
                return;
            }

            out << indent << dictName << "\n" << indent << "{\n";
            const auto& props = groupedProps[dictName];

            for (auto it = props.constBegin(); it != props.constEnd(); ++it) {
                out << indent << "    " << it.key() << " ";

                if (std::holds_alternative<double>(it.value())) {
                    out << std::get<double>(it.value()) << ";\n";
                } else if
                    (std::holds_alternative<QVector<double>>(it.value())) {
                    const auto& vec = std::get<QVector<double>>(it.value());
                    out << "( ";
                    for (double val : vec) {
                        out << val << " ";
                    }
                    out << ");\n";
                }
            }
            out << indent << "}\n";
        };

        // Write blocks in the standard OpenFOAM order
        writeSubDict("specie");
        writeSubDict("equationOfState");
        writeSubDict(thermoKey);
        writeSubDict("transport");

        if (!blockName.isEmpty()) {
            out << indent.left(indent.length() - 4) << "}\n";
        }
    };

    // Write Mixture Block using the configured Variant
    if (std::holds_alternative<PureMixtureConfig>(cfg.mixtureConfig)) {
        out << "mixture\n{\n";
        writePureConfig(std::get<PureMixtureConfig>(cfg.mixtureConfig), "", 1);
        out << "}\n\n";
    }
    else if (std::holds_alternative<MultiComponentConfig>(cfg.mixtureConfig)) {
        const auto& multiCfg =
            std::get<MultiComponentConfig>(cfg.mixtureConfig);
        out << "mixture\n{\n";
        out << "    species\n    (\n";
        for (const QString& specieName : multiCfg.speciesProperties.keys()) {
            out << "        " << specieName << "\n";
        }
        out << "    );\n\n";

        // Write the individual dictionary blocks for each species
        for (auto it = multiCfg.speciesProperties.constBegin();
             it != multiCfg.speciesProperties.constEnd(); ++it) {
            writePureConfig(it.value(), it.key(), 1);
            out << "\n";
        }
        out << "}\n\n";
    }
    else if (std::holds_alternative<ReactingMixtureConfig>(cfg.mixtureConfig)) {
        const auto& reactCfg =
            std::get<ReactingMixtureConfig>(cfg.mixtureConfig);
        out << "mixture\n{\n";
        out << "    species\n    (\n";
        for (const QString& specieName : reactCfg.participatingSpecies) {
            out << "        " << specieName << "\n";
        }
        out << "    );\n}\n\n";
    }
    else if (std::holds_alternative<PureZoneMixtureConfig>(cfg.mixtureConfig)) {
        const auto& zoneCfg =
            std::get<PureZoneMixtureConfig>(cfg.mixtureConfig);
        out << "mixture\n{\n";
        for (auto it = zoneCfg.zoneMixtures.constBegin();
             it != zoneCfg.zoneMixtures.constEnd(); ++it) {
            writePureConfig(it.value(), it.key(), 1);
            out << "\n";
        }
        out << "}\n\n";
    }
    else if (std::holds_alternative<
                   HomogeneousMixtureConfig>(cfg.mixtureConfig)) {
        const auto& homoCfg =
            std::get<HomogeneousMixtureConfig>(cfg.mixtureConfig);
        out << "mixture\n{\n";
        writePureConfig(homoCfg.reactants, "reactants", 1);
        out << "\n";
        writePureConfig(homoCfg.products, "burntProducts", 1);
        out << "}\n\n";
    }
    else if (std::holds_alternative<
                   InhomogeneousMixtureConfig>(cfg.mixtureConfig)) {
        const auto& inhomoCfg =
            std::get<InhomogeneousMixtureConfig>(cfg.mixtureConfig);
        out << "mixture\n{\n";
        // Write fuel, oxidant, and products
        writePureConfig(inhomoCfg.fuel, "fuel", 1);
        out << "\n";
        writePureConfig(inhomoCfg.oxidant, "oxidant", 1);
        out << "\n";
        writePureConfig(inhomoCfg.products, "burntProducts", 1);
        out << "}\n\n";
    }
    else if (std::holds_alternative<EgrMixtureConfig>(cfg.mixtureConfig)) {
        const auto& egrCfg = std::get<EgrMixtureConfig>(cfg.mixtureConfig);
        out << "mixture\n{\n";
        // Write fuel, oxidant, products, and egr components
        writePureConfig(egrCfg.fuel, "fuel", 1);
        out << "\n";
        writePureConfig(egrCfg.oxidant, "oxidant", 1);
        out << "\n";
        writePureConfig(egrCfg.products, "burntProducts", 1);
        out << "\n";
        writePureConfig(egrCfg.egr, "egr", 1);
        out << "}\n\n";
    }
    else if (std::holds_alternative<CustomMixtureConfig>(cfg.mixtureConfig)) {
        const auto& customCfg =
            std::get<CustomMixtureConfig>(cfg.mixtureConfig);
        out << customCfg.dictionaryText << "\n\n";
    }

    // Write closing separator
    out << createFoamFooter();
    return dictStr;
}
