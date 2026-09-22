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

#ifndef PARSER_THERMOPHYSICAL_PROPERTIES_H_
#define PARSER_THERMOPHYSICAL_PROPERTIES_H_

#include <memory>

#include "open_foam_dictionary.h"

#include <QMap>

namespace CaseIO {

struct ThermoTypeConfig {
    QString type;
    QString mixture;
    QString transport;
    QString thermo;
    QString equationOfState;
    QString specie;
    QString energy;
};

struct PureMixtureConfig {
    QMap<QString, std::variant<double, QVector<double>>> properties;
};

struct MultiComponentConfig {
    QMap<QString, PureMixtureConfig> speciesProperties;
};

struct ReactingMixtureConfig {
    QStringList participatingSpecies;
};

struct PureZoneMixtureConfig {
    QMap<QString, PureMixtureConfig> zoneMixtures;
};

struct HomogeneousMixtureConfig {
    PureMixtureConfig reactants;
    PureMixtureConfig products;
};

struct InhomogeneousMixtureConfig {
    PureMixtureConfig fuel;
    PureMixtureConfig oxidant;
    PureMixtureConfig products;
};

struct EgrMixtureConfig {
    PureMixtureConfig fuel;
    PureMixtureConfig oxidant;
    PureMixtureConfig products;
    PureMixtureConfig egr;
};

struct CustomMixtureConfig {
    QString dictionaryText;
};

using MixtureVariant = std::variant<
    std::monostate,
    CaseIO::PureMixtureConfig,
    CaseIO::MultiComponentConfig,
    CaseIO::ReactingMixtureConfig,
    CaseIO::PureZoneMixtureConfig,
    CaseIO::HomogeneousMixtureConfig,
    CaseIO::InhomogeneousMixtureConfig,
    CaseIO::EgrMixtureConfig,
    CaseIO::CustomMixtureConfig>;

struct ThermoConfig {
    ThermoTypeConfig thermoType;
    MixtureVariant mixtureConfig;
    QString inertSpecie;
    QString chemistryReader;
    QString foamChemistryFile;
    QString foamChemistryThermoFile;
};

// Parse transport properties file
void parseThermophysicalProperties(std::shared_ptr<OpenFoamDictionary> dict,
                              ThermoConfig& cfg);


// Create transport properties file
QString createThermophysicalProperties(const ThermoConfig& cfg,
                    const QString& openFoamPath, bool isOpenCFD);
};

#endif  // PARSER_THERMOPHYSICAL_PROPERTIES_H_
