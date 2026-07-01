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

#include "page_70_pimple.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QTableWidget>

#include "wizard_solver.h"

QWidget* centerCB(QCheckBox* box);

PimplePage::PimplePage(QWidget *parent): QWizardPage(parent) {

    // Set title
    setTitle(tr("PIMPLE Algorithm Configuration (fvSolution)"));

    // Create layout
    QFormLayout* layout = new QFormLayout(this);
    layout->setSpacing(20);
    setLayout(layout);

    // Outer correctors
    m_nOuterCorrectorsSpin = new QSpinBox(this);
    m_nOuterCorrectorsSpin->setRange(1, 100);
    layout->addRow(tr("Number of outer correctors: "), m_nOuterCorrectorsSpin);

    // Correctors
    m_nCorrectorsSpin = new QSpinBox(this);
    m_nCorrectorsSpin->setRange(1, 100);
    layout->addRow(tr("Number of correctors: "), m_nCorrectorsSpin);

    // Non-orthogonal correctors
    m_nNonOrthogonalCorrectorsSpin = new QSpinBox(this);
    m_nNonOrthogonalCorrectorsSpin->setRange(0, 50);
    layout->addRow(tr("Number of non-orthogonal correctors: "), m_nNonOrthogonalCorrectorsSpin);

    // Ref Cell
    m_pRefCellSpin = new QSpinBox(this);
    m_pRefCellSpin->setRange(0, INT_MAX);
    layout->addRow(tr("Reference cell index for pressure: "), m_pRefCellSpin);

    // Pressure at reference cell
    m_pRefValueSpin = new QDoubleSpinBox(this);
    m_pRefValueSpin->setRange(-1e9, 1e9);
    m_pRefValueSpin->setDecimals(5);
    layout->addRow(tr("Pressure at reference cell: "), m_pRefValueSpin);

    // Residual control table
    m_resTable = new QTableWidget(this);
    m_resTable->setColumnCount(3);
    m_resTable->setHorizontalHeaderLabels({"Enabled", "Field", "Residual Control" });
    m_resTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_resTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_resTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    layout->addRow(m_resTable);

    layout->addItem(new QSpacerItem(0, 0,
        QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void PimplePage::initializePage() {

    m_solverWizard = qobject_cast<SolverWizard*>(this->wizard());
    if (!m_solverWizard) { return; }

    // Access field data and boundaries
    m_cfg = &(m_solverWizard->getMathConfig());
    PimpleConfig& cfg = std::get<PimpleConfig>(m_cfg->algorithmConfig);

    // Update fields
    m_nOuterCorrectorsSpin->setValue(cfg.nOuterCorrectors);
    m_nCorrectorsSpin->setValue(cfg.nCorrectors);
    m_nNonOrthogonalCorrectorsSpin->setValue(cfg.nNonOrthogonalCorrectors);
    m_pRefCellSpin->setValue(cfg.pRefCell);
    m_pRefValueSpin->setValue(cfg.pRefValue);

    // Populate control table
    m_resTable->setRowCount(cfg.resControls.size());
    for (int i = 0; i < cfg.resControls.size(); ++i) {

        // Column 0: Enabled
        QCheckBox* enableCheck = new QCheckBox(this);
        enableCheck->setChecked(cfg.resControls[i].isEnabled);
        m_resTable->setCellWidget(i, 0, centerCB(enableCheck));

        // Column 1: Field Name
        QTableWidgetItem* fieldItem = new QTableWidgetItem(cfg.resControls[i].fieldName);
        fieldItem->setFlags(fieldItem->flags() & ~Qt::ItemIsEditable);
        m_resTable->setItem(i, 1, fieldItem);

        // Column 2: Residual control (Validated Input)
        QLineEdit* toleranceEdit = new QLineEdit(cfg.resControls[i].tolerance, this);
        QDoubleValidator* validator = new QDoubleValidator(this);
        validator->setNotation(QDoubleValidator::ScientificNotation);
        toleranceEdit->setValidator(validator);

        m_resTable->setCellWidget(i, 2, toleranceEdit);
    }

    // Set table height
    int tableHeight = m_resTable->horizontalHeader()->height() +
                      (m_resTable->rowCount() * m_resTable->rowHeight(0)) + 3;
    m_resTable->setMinimumHeight(tableHeight);
}

bool PimplePage::validatePage() {
    if (!m_cfg) return false;

    // Update state data
    PimpleConfig& cfg = std::get<PimpleConfig>(m_cfg->algorithmConfig);
    cfg.nOuterCorrectors = m_nOuterCorrectorsSpin->value();
    cfg.nCorrectors = m_nCorrectorsSpin->value();
    cfg.nNonOrthogonalCorrectors = m_nNonOrthogonalCorrectorsSpin->value();
    cfg.pRefCell = m_pRefCellSpin->value();
    cfg.pRefValue = m_pRefValueSpin->value();

    // Access data from table
    for (int i = 0; i < m_resTable->rowCount(); ++i) {
        if (i >= cfg.resControls.size()) { break; }

        // Column 0: Enabled Checkbox
        QWidget* checkWidgetContainer = m_resTable->cellWidget(i, 0);
        if (checkWidgetContainer) {
            QCheckBox* cb = checkWidgetContainer->findChild<QCheckBox*>();
            if (cb) {
                cfg.resControls[i].isEnabled = cb->isChecked();
            }
        }

        // Column 2: Residual Control (Tolerance)
        QWidget* toleranceWidget = m_resTable->cellWidget(i, 2);
        if (toleranceWidget) {
            QLineEdit* le = qobject_cast<QLineEdit*>(toleranceWidget);
            if (le) {
                cfg.resControls[i].tolerance = le->text();
            }
        }
    }
    return true;
}

// Set next page of the wizard
int PimplePage::nextId() const {
    return Page_Parallel;
}

// Center checkboxes visually in a table cell
QWidget* centerCB(QCheckBox* box) {
    QWidget* widget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->addWidget(box);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    return widget;
}