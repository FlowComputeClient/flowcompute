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

#include "wizards/solver/homogeneous_mixture_widget.h"

#include <QTabWidget>
#include <QVBoxLayout>

#include "wizards/solver/pure_mixture_widget.h"

HomogeneousMixtureWidget::HomogeneousMixtureWidget(const QString& thermo,
    const QString& transport, const QString& eos, QWidget* parent)
    : MixtureWidget(parent) {

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QTabWidget* tabWidget = new QTabWidget(this);

    // Create child widgets
    m_reactantsWidget = new PureMixtureWidget(thermo, transport, eos, this);
    m_productsWidget = new PureMixtureWidget(thermo, transport, eos, this);

    // Add them to the tab widget
    tabWidget->addTab(m_reactantsWidget, tr("Reactants"));
    tabWidget->addTab(m_productsWidget, tr("Products"));

    mainLayout->addWidget(tabWidget);
}

CaseIO::MixtureVariant HomogeneousMixtureWidget::getData() const {
    CaseIO::HomogeneousMixtureConfig config;

    // Retrieve the MixtureVariant from the child widgets
    CaseIO::MixtureVariant reactantsVariant = m_reactantsWidget->getData();
    CaseIO::MixtureVariant productsVariant = m_productsWidget->getData();

    // Safely extract the PureMixtureConfig from the returned variants
    if (std::holds_alternative<CaseIO::PureMixtureConfig>(reactantsVariant)) {
        config.reactants =
            std::get<CaseIO::PureMixtureConfig>(reactantsVariant);
    }

    if (std::holds_alternative<CaseIO::PureMixtureConfig>(productsVariant)) {
        config.products = std::get<CaseIO::PureMixtureConfig>(productsVariant);
    }

    return config;
}