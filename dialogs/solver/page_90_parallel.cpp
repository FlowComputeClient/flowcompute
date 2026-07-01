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

#include "page_90_parallel.h"

#include "wizard_solver.h"
#include "../../core_types.h"

// Configures decomposeParDict
ParallelPage::ParallelPage(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Parallel Mesh Decomposition (decomposeParDict)"));
    QVBoxLayout* layout = new QVBoxLayout(this);

    // Parallel group
    QGroupBox* parallelGroup = new QGroupBox(tr("Decomposition Settings"), this);
    layout->addWidget(parallelGroup);
    QFormLayout* parallelLayout = new QFormLayout(parallelGroup);
    parallelLayout->setSpacing(15);
    parallelLayout->setContentsMargins(10, 15, 10, 10);

    // Determine if decomposition should be used
    m_parallelCheck = new QCheckBox(parallelGroup);
    parallelLayout->addRow(tr("Employ parallel decomposition:"), m_parallelCheck);
    connect(m_parallelCheck, &QCheckBox::toggled,
            this, &ParallelPage::parallelChanged);

    // Set the number of subdomains
    m_subdomainsSpin = new QSpinBox(this);
    m_subdomainsSpin->setMinimum(1);
    parallelLayout->addRow(tr("Set the number of subdomains:"), m_subdomainsSpin);

    // Select the method
    m_methodCombo = new QComboBox(this);
    QMetaEnum methodEnum = QMetaEnum::fromType<FlowCompute::DecompositionMethod>();
    for (int i = 0; i < methodEnum.keyCount(); ++i) {
        m_methodCombo->addItem(methodEnum.key(i), methodEnum.value(i));
    }
    connect(m_methodCombo, &QComboBox::currentIndexChanged, this, &ParallelPage::methodChanged);
    parallelLayout->addRow(tr("Decomposition method: "), m_methodCombo);

    // Create and indent the stacked widget
    m_methodStack = new QStackedWidget(parallelGroup);
    QHBoxLayout* indentLayout = new QHBoxLayout();
    indentLayout->addSpacing(15);
    indentLayout->addWidget(m_methodStack);
    parallelLayout->addRow(indentLayout);

    // Scotch
    QWidget* automaticPage = new QWidget();
    QVBoxLayout* automaticLayout = new QVBoxLayout(automaticPage);
    automaticLayout->addWidget(new QLabel(""));
    m_methodStack->addWidget(automaticPage);

    // Simple
    QWidget* simplePage = new QWidget();
    QFormLayout* simpleLayout = new QFormLayout(simplePage);
    simpleLayout->setSpacing(15);

    // Subdomains in the X-direction
    m_simpleXSpin = new QSpinBox();
    m_simpleXSpin->setMinimum(1);
    simpleLayout->addRow(tr("Subdomains in the x-direction: "), m_simpleXSpin);

    // Subdomains in the Y-direction
    m_simpleYSpin = new QSpinBox();
    m_simpleYSpin->setMinimum(1);
    simpleLayout->addRow(tr("Subdomains in the y-direction: "), m_simpleYSpin);

    // Subdomains in the Z-direction
    m_simpleZSpin = new QSpinBox();
    m_simpleZSpin->setMinimum(1);
    simpleLayout->addRow(tr("Subdomains in the z-direction: "), m_simpleZSpin);

    // Delta
    m_simpleDeltaSpin = new QDoubleSpinBox();
    m_simpleDeltaSpin->setDecimals(4);
    m_simpleDeltaSpin->setSingleStep(0.001);
    m_simpleDeltaSpin->setMinimum(0.0);
    simpleLayout->addRow(tr("Cell skew factor (delta): "), m_simpleDeltaSpin);

    m_methodStack->addWidget(simplePage);

    // Hierarchical
    QWidget* hierarchicalPage = new QWidget();
    QFormLayout* hierarchicalLayout = new QFormLayout(hierarchicalPage);
    hierarchicalLayout->setSpacing(15);

    // Subdomains in the X-direction
    m_hierXSpin = new QSpinBox();
    m_hierXSpin->setMinimum(1);
    hierarchicalLayout->addRow(tr("Subdomains in the x-direction: "), m_hierXSpin);

    // Subdomains in the Y-direction
    m_hierYSpin = new QSpinBox();
    m_hierYSpin->setMinimum(1);
    hierarchicalLayout->addRow(tr("Subdomains in the y-direction: "), m_hierYSpin);

    // Subdomains in the Z-direction
    m_hierZSpin = new QSpinBox();
    m_hierZSpin->setMinimum(1);
    hierarchicalLayout->addRow(tr("Subdomains in the z-direction: "), m_hierZSpin);

    // Decomposition order
    m_hierOrderCombo = new QComboBox();
    m_hierOrderCombo->addItems({"xyz", "xzy", "yxz", "yzx", "zxy", "zyx"});
    hierarchicalLayout->addRow(tr("Decomposition order: "), m_hierOrderCombo);

    // Delta
    m_hierDeltaSpin = new QDoubleSpinBox();
    m_hierDeltaSpin->setDecimals(4);
    m_hierDeltaSpin->setSingleStep(0.001);
    m_hierDeltaSpin->setMinimum(0.0);
    hierarchicalLayout->addRow(tr("Cell skew factor (delta): "), m_hierDeltaSpin);

    m_methodStack->addWidget(hierarchicalPage);

    layout->addStretch();
    setLayout(layout);
}

void ParallelPage::initializePage() {

    // Access wizard data
    solverWizard = qobject_cast<SolverWizard*>(this->wizard());
    if (!solverWizard) return;
    m_cfg = &(solverWizard->getParallelConfig());

    // Populate the UI with existing config data
    m_parallelCheck->setChecked(m_cfg->useParallel);
    m_subdomainsSpin->setValue(m_cfg->numSubdomains);

    // Find the combo box index that matches the saved enum
    int methodIndex = m_methodCombo->findData(QVariant::fromValue(m_cfg->method));
    if (methodIndex != -1) {
        m_methodCombo->setCurrentIndex(methodIndex);
    }

    // Update simple/hierarchcial controls
    m_simpleXSpin->setValue(m_cfg->nx);
    m_simpleYSpin->setValue(m_cfg->ny);
    m_simpleZSpin->setValue(m_cfg->nz);
    m_simpleDeltaSpin->setValue(m_cfg->delta);
    m_hierXSpin->setValue(m_cfg->nx);
    m_hierYSpin->setValue(m_cfg->ny);
    m_hierZSpin->setValue(m_cfg->nz);
    m_hierOrderCombo->setCurrentText(m_cfg->order);
    m_hierDeltaSpin->setValue(m_cfg->delta);

    // Ensure the UI state matches the checkbox
    parallelChanged(m_cfg->useParallel);
}

bool ParallelPage::validatePage() {
    if (!m_cfg) return false;

    // Check parallelism usage
    m_cfg->useParallel = m_parallelCheck->isChecked();
    if (!m_cfg->useParallel) {
        return true;
    }

    // Set shared parameters
    m_cfg->numSubdomains = m_subdomainsSpin->value();
    m_cfg->method = static_cast<FlowCompute::DecompositionMethod>(m_methodCombo->currentData().toInt());

    // Consolidate Simple and Hierarchical validation
    if (m_cfg->method == FlowCompute::DecompositionMethod::Simple ||
        m_cfg->method == FlowCompute::DecompositionMethod::Hierarchical) {

        // Determine which set of widgets to read from based on the active method
        QSpinBox* activeX = (m_cfg->method == FlowCompute::DecompositionMethod::Simple) ? m_simpleXSpin : m_hierXSpin;
        QSpinBox* activeY = (m_cfg->method == FlowCompute::DecompositionMethod::Simple) ? m_simpleYSpin : m_hierYSpin;
        QSpinBox* activeZ = (m_cfg->method == FlowCompute::DecompositionMethod::Simple) ? m_simpleZSpin : m_hierZSpin;
        QDoubleSpinBox* activeDelta = (m_cfg->method == FlowCompute::DecompositionMethod::Simple) ? m_simpleDeltaSpin : m_hierDeltaSpin;

        int nx = activeX->value();
        int ny = activeY->value();
        int nz = activeZ->value();

        // Validate geometry
        if ((nx * ny * nz) != m_cfg->numSubdomains) {
            QMessageBox::warning(this, tr("Invalid Geometry"),
                                 tr("The product of x, y, and z must equal the number of subdomains."));

            // UX Polish: Snap focus back to the input so the user can immediately type a fix
            activeX->setFocus();
            activeX->selectAll();
            return false;
        }

        // Apply validated values
        m_cfg->nx = nx;
        m_cfg->ny = ny;
        m_cfg->nz = nz;
        m_cfg->delta = activeDelta->value();

        // Apply method-specific values
        if (m_cfg->method == FlowCompute::DecompositionMethod::Hierarchical) {
            m_cfg->order = m_hierOrderCombo->currentText();
        }
    }
    return true;
}

// Populate list of solvers based on solver family
void ParallelPage::parallelChanged(bool checked) {
    m_subdomainsSpin->setEnabled(checked);
    m_methodCombo->setEnabled(checked);
    m_methodStack->setEnabled(checked);
}

// Update values when a new decomposition method is chosen
void ParallelPage::methodChanged(int index) {

    // Get the selected decomposition method
    QVariant data = m_methodCombo->itemData(index);
    auto currentMethod = static_cast<FlowCompute::DecompositionMethod>(data.toInt());

    switch (currentMethod) {
    case FlowCompute::DecompositionMethod::Scotch:
    case FlowCompute::DecompositionMethod::Metis:
        m_methodStack->setCurrentIndex(0);
        break;
    case FlowCompute::DecompositionMethod::Simple:
        m_methodStack->setCurrentIndex(1);
        break;
    case FlowCompute::DecompositionMethod::Hierarchical:
        m_methodStack->setCurrentIndex(2);
        break;
    }
}