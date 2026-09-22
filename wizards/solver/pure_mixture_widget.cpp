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

#include "wizards/solver/pure_mixture_widget.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>

PureMixtureWidget::PureMixtureWidget(const QString& thermo,
    const QString& transport, const QString& eos, QWidget* parent):
    MixtureWidget(parent) {

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Group boxes for visual organization
    QGroupBox* eosGroup =
        new QGroupBox(tr("Equation of State Properties"), this);
    QFormLayout* eosLayout = new QFormLayout(eosGroup);

    QGroupBox* thermoGroup =
        new QGroupBox(tr("Thermodynamic Properties"), this);
    QFormLayout* thermoLayout = new QFormLayout(thermoGroup);

    QGroupBox* transportGroup = new QGroupBox(tr("Transport Properties"), this);
    QFormLayout* transportLayout = new QFormLayout(transportGroup);

    // Check equation of state
    if (eos == "perfectGas" || eos == "incompressiblePerfectGas") {
        addScalarField(eosLayout, tr("Molecular Weight (molWeight):"),
            "molWeight");
    }
    else if (eos == "rhoConst") {
        addScalarField(eosLayout, tr("Density (rho):"), "rho");
    }
    else if (eos == "Boussinesq") {
        addScalarField(eosLayout, tr("Reference Density (rho0):"), "rho0");
        addScalarField(eosLayout, tr("Reference Temperature (T0):"), "T0");
        addScalarField(eosLayout, tr("Thermal Expansion (beta):"), "beta");
    }
    else {
        addVectorField(eosLayout, tr("Custom EOS Data:"), "customEos");
    }

    // Check thermodynamic model
    if (thermo == "hConst") {
        addScalarField(thermoLayout, tr("Heat Capacity (Cp):"), "Cp");
        addScalarField(thermoLayout, tr("Heat of Formation (Hf):"), "Hf");
    }
    else if (thermo == "eConst") {
        addScalarField(thermoLayout, tr("Heat Capacity (Cv):"), "Cv");
        addScalarField(thermoLayout, tr("Heat of Formation (Hf):"), "Hf");
    }
    else if (thermo == "janaf") {
        addScalarField(thermoLayout, tr("Lower Temp Limit (Tlow):"), "Tlow");
        addScalarField(thermoLayout, tr("Upper Temp Limit (Thigh):"), "Thigh");
        addScalarField(thermoLayout, tr("Common Temp (Tcommon):"), "Tcommon");
        addVectorField(thermoLayout, tr("High Temp Coefficients:"),
                       "highCpCoeffs");
        addVectorField(thermoLayout, tr("Low Temp Coefficients:"),
                       "lowCpCoeffs");
    }
    else {
        addVectorField(thermoLayout, tr("Custom Thermo Data:"), "customThermo");
    }

    // Check transport model
    if (transport == "const") {
        addScalarField(transportLayout, tr("Dynamic Viscosity (mu):"), "mu");
        addScalarField(transportLayout, tr("Prandtl Number (Pr):"), "Pr");
    }
    else if (transport == "sutherland") {
        addScalarField(transportLayout, tr("Sutherland Constant (As):"), "As");
        addScalarField(transportLayout, tr("Sutherland Temp (Ts):"), "Ts");
    }
    else if (transport == "polynomial") {
        addVectorField(transportLayout,
            tr("Viscosity Coefficients (muCoeffs):"), "muCoeffs");
        addVectorField(transportLayout,
            tr("Thermal Cond. Coefficients (kappaCoeffs):"), "kappaCoeffs");
    }
    else {
        addVectorField(transportLayout,
            tr("Custom Transport Data:"), "customTransport");
    }

    // Add populated groups to main layout
    mainLayout->addWidget(eosGroup);
    mainLayout->addWidget(thermoGroup);
    mainLayout->addWidget(transportGroup);
    mainLayout->addStretch();
}

CaseIO::MixtureVariant PureMixtureWidget::getData() const {
    CaseIO::PureMixtureConfig config;
    // Extract scalar values directly from the spin boxes
    for (auto it = m_scalarInputs.constBegin();
         it != m_scalarInputs.constEnd(); ++it) {
        config.properties.insert(it.key(), it.value()->value());
    }

    // Extract and parse vector values from the line edits
    for (auto it = m_vectorInputs.constBegin();
         it != m_vectorInputs.constEnd(); ++it) {
        QVector<double> vectorData;

        // Split the space-separated string into discrete values
        QString text = it.value()->text().trimmed();
        if (!text.isEmpty()) {
            QStringList parts = text.split(QRegularExpression("\\s+"),
                                           Qt::SkipEmptyParts);
            for (const QString& part : std::as_const(parts)) {
                bool ok;
                double val = part.toDouble(&ok);
                if (ok)
                    vectorData.append(val);
            }
        }
        config.properties.insert(it.key(), vectorData);
    }
    return config;
}