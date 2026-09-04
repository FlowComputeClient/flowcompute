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
#include <QMessageBox>
#include <QMetaEnum>
#include <QString>
#include <QVBoxLayout>

ForcesDialog::ForcesDialog(const QStringList& patches,
    const QStringList& fields, CaseIO::ForcesConfig& forcesConfig,
    QWidget* parent): QDialog(parent), m_patches(patches), m_fields(fields),
    m_forcesConfig(forcesConfig) {
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
    m_pFieldCombo = new QComboBox(this);
    int index = m_fields.indexOf("p");
    if (index > 0)
        m_fields.move(index, 0);
    m_pFieldCombo->addItems(m_fields);
    fieldLayout->addRow(tr("Pressure field (p): "), m_pFieldCombo);

    // Set name for U
    m_UFieldCombo = new QComboBox(this);
    index = m_fields.indexOf("U");
    if (index > 0)
        m_fields.move(index, 0);
    m_UFieldCombo->addItems(m_fields);
    fieldLayout->addRow(tr("Velocity field (U): "), m_UFieldCombo);

    // pRef
    m_pRefSpin = new QDoubleSpinBox(this);
    m_pRefSpin->setRange(-1e9, 1e9);
    m_pRefSpin->setDecimals(4);
    m_pRefSpin->setSingleStep(1.0);
    m_pRefSpin->setValue(m_forcesConfig.pRef);
    fieldLayout->addRow(tr("Reference Pressure (pRef): "), m_pRefSpin);

    // Set rho field
    m_rhoFieldCombo = new QComboBox(this);
    QStringList rhoFields = m_fields;
    index = rhoFields.indexOf("rho");
    if (index > 0) {
        rhoFields.move(index, 0);
    }
    rhoFields.prepend("rhoInf");
    m_rhoFieldCombo->addItems(rhoFields);
    m_rhoFieldCombo->setCurrentIndex(0);
    fieldLayout->addRow(tr("Density field (rho): "), m_rhoFieldCombo);

    // rhoInf
    m_rhoInfSpin = new QDoubleSpinBox(this);
    m_rhoInfSpin->setRange(0.001, 20000.0);
    m_rhoInfSpin->setDecimals(3);
    m_rhoInfSpin->setSingleStep(100.0);
    m_rhoInfSpin->setValue(m_forcesConfig.rhoInf);
    fieldLayout->addRow(tr("Reference Density (rhoInf): "), m_rhoInfSpin);

    // Respond to rho field change
    connect(m_rhoFieldCombo,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this, fieldLayout](int idx) {
            bool isRhoInf = (idx == 0);
            m_rhoInfSpin->setEnabled(isRhoInf);
            if (QWidget* label = fieldLayout->labelForField(m_rhoInfSpin)) {
                label->setEnabled(isRhoInf);
            }
        });

    // Geometry group
    QGroupBox* geomGroup = new QGroupBox(tr("Geometry and Integration"), this);
    QFormLayout* geomLayout = new QFormLayout(geomGroup);
    geomLayout->setSpacing(10);
    mainLayout->addWidget(geomGroup);

    // Create widget for patches
    m_patchListWidget = new QListWidget(this);
    for (const QString& patch : std::as_const(m_patches)) {
        QListWidgetItem* item = new QListWidgetItem(patch, m_patchListWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        if (m_forcesConfig.patches.contains(patch)) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
    }

    // Display four patches at a time
    if (m_patchListWidget->count() > 0) {
        int itemHeight = m_patchListWidget->sizeHintForRow(0);
        m_patchListWidget->setMaximumHeight((itemHeight * 4) + 5);
    }
    geomLayout->addRow(tr("Boundary patches:"), m_patchListWidget);

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
    // Make sure a name has been provided
    if (m_nameEdit->text().isEmpty()) {
        QMessageBox::critical(this, tr("Missing Item"),
            tr("A name must be provided for the post-processing task."));
        return;
    }

    // Get list of patches
    QStringList selectedPatches;
    for (int i = 0; i < m_patchListWidget->count(); ++i) {
        QListWidgetItem* item = m_patchListWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedPatches << item->text();
        }
    }

    // Make sure at least one patch has been selected
    if (selectedPatches.isEmpty()) {
        QMessageBox::critical(this, tr("Missing Item"),
            tr("At least one patch must be selected."));
        return;
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

    m_forcesConfig.pName = m_pFieldCombo->currentText();
    m_forcesConfig.UName = m_UFieldCombo->currentText();
    m_forcesConfig.rhoName = m_rhoFieldCombo->currentText();
    m_forcesConfig.rhoInf = m_rhoInfSpin->value();
    m_forcesConfig.pRef = m_pRefSpin->value();

    m_forcesConfig.patches = selectedPatches;
    m_forcesConfig.centerOfRotation =
    { m_cofrSpin[0]->value(), m_cofrSpin[1]->value(), m_cofrSpin[2]->value() };
    m_forcesConfig.includePorosity = m_porosityCheck->isChecked();
    QDialog::accept();
}
