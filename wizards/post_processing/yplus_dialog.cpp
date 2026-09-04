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

#include "yplus_dialog.h"

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

// Configures the yPlus function object
YPlusDialog::YPlusDialog(const QStringList& patches,
    CaseIO::YPlusConfig& yPlusConfig, QWidget* parent): m_patches(patches),
    m_yPlusConfig(yPlusConfig), QDialog(parent) {
    // Set title and style
    setWindowTitle(tr("Wall Distance Validation"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Create layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    // Timing/output group
    QGroupBox* timingGroup =
        new QGroupBox(tr("Timing and Output Control"), this);
    QFormLayout* timingLayout = new QFormLayout(timingGroup);
    mainLayout->addWidget(timingGroup);

    // Set name for function object
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setText(m_yPlusConfig.name);
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
            static_cast<int>(m_yPlusConfig.executeControl));
    if (execIndex != -1)
        m_executeCombo->setCurrentIndex(execIndex);
    timingLayout->addRow(tr("Execute control: "), m_executeCombo);

    // Execute interval
    m_executeSpin = new QDoubleSpinBox(this);
    m_executeSpin->setRange(0.00001, 1000000.0);
    m_executeSpin->setDecimals(5);
    m_executeSpin->setSingleStep(1.0);
    m_executeSpin->setValue(m_yPlusConfig.executeInterval);
    timingLayout->addRow(tr("Execute interval: "), m_executeSpin);

    // Write control
    m_writeCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<CaseIO::FunctionObject::ControlType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_writeCombo->addItem(metaEnum.key(i), metaEnum.value(i));
    }
    int writeIndex =
        m_writeCombo->findData(
            static_cast<int>(m_yPlusConfig.writeControl));
    if (writeIndex != -1)
        m_writeCombo->setCurrentIndex(writeIndex);
    timingLayout->addRow(tr("Write control: "), m_writeCombo);

    // Write interval
    m_writeSpin = new QDoubleSpinBox(this);
    m_writeSpin->setRange(0.00001, 1000000.0);
    m_writeSpin->setDecimals(5);
    m_writeSpin->setSingleStep(1.0);
    m_writeSpin->setValue(m_yPlusConfig.writeInterval);
    timingLayout->addRow(tr("Write interval: "), m_writeSpin);

    // Log output
    m_logCheck = new QCheckBox(tr("Log output"), this);
    m_logCheck->setChecked(m_yPlusConfig.logOutput);
    timingLayout->addRow(m_logCheck);

    // Patch selection group
    QGroupBox* patchGroup = new QGroupBox(tr("Wall Patches"), this);
    QVBoxLayout* patchLayout = new QVBoxLayout(patchGroup);
    mainLayout->addWidget(patchGroup);

    m_patchListWidget = new QListWidget(this);
    for (const QString& field : std::as_const(m_patches)) {
        QListWidgetItem* item = new QListWidgetItem(field, m_patchListWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        // Check if the patch is already in the config
        if (m_yPlusConfig.patches.contains(field)) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
    }
    patchLayout->addWidget(m_patchListWidget);

    // Create OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &YPlusDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    setMinimumWidth(300);
    this->adjustSize();
}

void YPlusDialog::onOkClicked() {
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
    m_yPlusConfig.patches = selectedPatches;

    // Make sure at least one patch has been selected
    if (selectedPatches.isEmpty()) {
        QMessageBox::critical(this, tr("Missing Item"),
            tr("At least one patch must be selected."));
        return;
    }

    // Update the YPlusConfig structure
    m_yPlusConfig.name = m_nameEdit->text();
    m_yPlusConfig.type = CaseIO::FunctionObject::FuncObjType::yPlus;
    int executeVal = m_executeCombo->currentData().toInt();
    m_yPlusConfig.executeControl =
        static_cast<CaseIO::FunctionObject::ControlType>(executeVal);
    m_yPlusConfig.executeInterval = m_executeSpin->value();
    int writeVal = m_writeCombo->currentData().toInt();
    m_yPlusConfig.writeControl =
        static_cast<CaseIO::FunctionObject::ControlType>(writeVal);
    m_yPlusConfig.writeInterval = m_writeSpin->value();
    m_yPlusConfig.logOutput = m_logCheck->isChecked();
    accept();
}
