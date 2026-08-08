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

#include "forces_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMetaEnum>
#include <QString>
#include <QVBoxLayout>

ForcesDialog::ForcesDialog(const QStringList& patches,
    CaseIO::ForcesConfig& forcesConfig, QWidget* parent): m_patches(patches),
    m_forcesConfig(forcesConfig), QDialog(parent) {
    // Set title and style
    setWindowTitle(tr("Forces Function Configuration"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Create layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    // Timing/output group
    QGroupBox* timingGroup =
        new QGroupBox(tr("Timing and Output Control"), this);
    QFormLayout* timingLayout = new QFormLayout(timingGroup);
    timingLayout->setSpacing(10);
    mainLayout->addWidget(timingGroup);

    // Set name for function object
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setText(m_forcesConfig.name);
    timingLayout->addRow(tr("Name of function object: "), m_nameEdit);

    // Execute control
    m_executeCombo = new QComboBox(this);
    QMetaEnum metaEnum =
        QMetaEnum::fromType<CaseIO::FunctionObject::ControlType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_executeCombo->addItem(metaEnum.key(i), metaEnum.value(i));
    }
    int execIndex =
        m_executeCombo->findData(
            static_cast<int>(m_forcesConfig.executeControl));
    if (execIndex != -1)
        m_executeCombo->setCurrentIndex(execIndex);
    timingLayout->addRow(tr("Execute control: "), m_executeCombo);

    // Execute interval
    m_executeSpin = new QDoubleSpinBox(this);
    m_executeSpin->setRange(0.00001, 1000000.0);
    m_executeSpin->setDecimals(5);
    m_executeSpin->setSingleStep(1.0);
    m_executeSpin->setValue(m_forcesConfig.executeInterval);
    timingLayout->addRow(tr("Execute interval: "), m_executeSpin);

    // Write control
    m_writeCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<CaseIO::FunctionObject::ControlType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_writeCombo->addItem(metaEnum.key(i), metaEnum.value(i));
    }
    int writeIndex =
        m_writeCombo->findData(
            static_cast<int>(m_forcesConfig.writeControl));
    if (writeIndex != -1)
        m_writeCombo->setCurrentIndex(writeIndex);
    timingLayout->addRow(tr("Write control: "), m_writeCombo);

    // Write interval
    m_writeSpin = new QDoubleSpinBox(this);
    m_writeSpin->setRange(0.00001, 1000000.0);
    m_writeSpin->setDecimals(5);
    m_writeSpin->setSingleStep(1.0);
    m_writeSpin->setValue(m_forcesConfig.writeInterval);
    timingLayout->addRow(tr("Write interval: "), m_writeSpin);

    // Write fields
    m_writeFieldsCheck = new QCheckBox(tr("Write fields"), this);
    m_writeFieldsCheck->setChecked(m_forcesConfig.writeFields);
    timingLayout->addRow(m_writeFieldsCheck);

    // Log output
    m_logCheck = new QCheckBox(tr("Log output"), this);
    m_logCheck->setChecked(m_forcesConfig.logOutput);
    timingLayout->addRow(m_logCheck);

    // Field definitions group
    QGroupBox* fieldGroup = new QGroupBox(tr("Field Definitions"), this);
    QFormLayout* fieldLayout = new QFormLayout(fieldGroup);
    fieldLayout->setSpacing(10);
    mainLayout->addWidget(fieldGroup);

    // Set name for p
    m_pNameEdit = new QLineEdit(this);
    m_pNameEdit->setText(m_forcesConfig.pName);
    fieldLayout->addRow(tr("Name for pressure (p): "), m_pNameEdit);

    // Set name for U
    m_UNameEdit = new QLineEdit(this);
    m_UNameEdit->setText(m_forcesConfig.UName);
    fieldLayout->addRow(tr("Name for velocity (U): "), m_UNameEdit);

    // pRef
    m_pRefSpin = new QDoubleSpinBox(this);
    m_pRefSpin->setRange(-1e9, 1e9);
    m_pRefSpin->setDecimals(4);
    m_pRefSpin->setSingleStep(1.0);
    m_pRefSpin->setValue(m_forcesConfig.pRef);
    fieldLayout->addRow(tr("Reference Pressure (pRef): "), m_pRefSpin);

    // Set flow type
    m_flowTypeCombo = new QComboBox(this);
    m_flowTypeCombo->addItems(
        {"Incompressible (constant)", "Compressible (Field Name)"} );
    fieldLayout->addRow(tr("Flow type: "), m_flowTypeCombo);

    // rhoInf (Incompressible only)
    m_rhoInfSpin = new QDoubleSpinBox(this);
    m_rhoInfSpin->setRange(0.001, 20000.0);
    m_rhoInfSpin->setDecimals(3);
    m_rhoInfSpin->setSingleStep(100.0);
    m_rhoInfSpin->setValue(m_forcesConfig.rhoInf);
    fieldLayout->addRow(tr("Reference Density (rhoInf): "), m_rhoInfSpin);

    // rho (Compressible only)
    m_rhoEdit = new QLineEdit(this);
    m_rhoEdit->setText(m_forcesConfig.rhoName);
    fieldLayout->addRow(tr("Density field: "), m_rhoEdit);

    // Respond to flow type changes
    connect(m_flowTypeCombo,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this, fieldLayout](int index) {
        bool isIncompressible = (index == 0);
        fieldLayout->setRowVisible(m_rhoInfSpin, isIncompressible);
        fieldLayout->setRowVisible(m_rhoEdit, !isIncompressible);
    });

    // Set initial state
    int initialMode = m_forcesConfig.rhoName.isEmpty() ? 0 : 1;
    m_flowTypeCombo->setCurrentIndex(initialMode);
    fieldLayout->setRowVisible(m_rhoInfSpin, (initialMode == 0));
    fieldLayout->setRowVisible(m_rhoEdit, (initialMode == 1));

    // Geometry group
    QGroupBox* geomGroup = new QGroupBox(tr("Geometry and Integration"), this);
    QFormLayout* geomLayout = new QFormLayout(geomGroup);
    geomLayout->setSpacing(10);
    mainLayout->addWidget(geomGroup);

    // Create widget for patches
    m_patchesListWidget = new QListWidget(this);
    for (const QString& patch : std::as_const(m_patches)) {
        QListWidgetItem* item = new QListWidgetItem(patch, m_patchesListWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        if (m_forcesConfig.patches.contains(patch)) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
    }

    // Display four patches at a time
    if (m_patchesListWidget->count() > 0) {
        int itemHeight = m_patchesListWidget->sizeHintForRow(0);
        m_patchesListWidget->setMaximumHeight((itemHeight * 4) + 5);
    }
    geomLayout->addRow(tr("Boundary patches:"), m_patchesListWidget);

    // Set center of rotation
    QHBoxLayout* cofrLayout = new QHBoxLayout;
    cofrLayout->setSpacing(10);
    cofrLayout->setContentsMargins(0, 0, 0, 0);
    for (int i=0; i<3; i++) {
        m_cofrSpin[i] = new QDoubleSpinBox(this);
        m_cofrSpin[i]->setRange(-1000000.0, 1000000.0);
        m_cofrSpin[i]->setDecimals(5);
        m_cofrSpin[i]->setSingleStep(0.1);
        m_cofrSpin[i]->setValue(m_forcesConfig.centerOfRotation[i]);
        cofrLayout->addWidget(m_cofrSpin[i]);
    }
    geomLayout->addRow(tr("Center of rotation: "), cofrLayout);

    // Include porosity
    m_porosityCheck = new QCheckBox(tr("Include porosity"), this);
    m_porosityCheck->setChecked(m_forcesConfig.includePorosity);
    geomLayout->addRow(m_porosityCheck);

    // Create OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &ForcesDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    setMinimumWidth(300);
    this->adjustSize();
}

void ForcesDialog::onOkClicked() {
    // Get list of patches
    QStringList selectedPatches;
    for (int i = 0; i < m_patchesListWidget->count(); ++i) {
        QListWidgetItem* item = m_patchesListWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedPatches << item->text();
        }
    }

    // Update the ForcesConfig structure
    m_forcesConfig.name = m_nameEdit->text();
    m_forcesConfig.type = CaseIO::FunctionObject::FuncObjType::forces;
    int executeVal = m_executeCombo->currentData().toInt();
    m_forcesConfig.executeControl =
        static_cast<CaseIO::FunctionObject::ControlType>(executeVal);
    m_forcesConfig.executeInterval = m_executeSpin->value();
    int writeVal = m_writeCombo->currentData().toInt();
    m_forcesConfig.writeControl =
        static_cast<CaseIO::FunctionObject::ControlType>(writeVal);
    m_forcesConfig.writeInterval = m_writeSpin->value();
    m_forcesConfig.writeFields = m_writeFieldsCheck->isChecked();
    m_forcesConfig.logOutput = m_logCheck->isChecked();

    m_forcesConfig.pName = m_pNameEdit->text();
    m_forcesConfig.UName = m_UNameEdit->text();
    m_forcesConfig.rhoName = m_rhoEdit->text();
    m_forcesConfig.rhoInf = m_rhoInfSpin->value();
    m_forcesConfig.pRef = m_pRefSpin->value();

    m_forcesConfig.patches = selectedPatches;
    m_forcesConfig.centerOfRotation =
    { m_cofrSpin[0]->value(), m_cofrSpin[1]->value(), m_cofrSpin[2]->value() };
    m_forcesConfig.includePorosity = m_porosityCheck->isChecked();
    QDialog::accept();
}
