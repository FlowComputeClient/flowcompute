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

#include "field_minmax_dialog.h"

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

FieldMinMaxDialog::FieldMinMaxDialog(const QStringList& fields,
    CaseIO::FieldMinMaxConfig& fieldMinMaxConfig, QWidget* parent):
    m_fields(fields), m_fieldMinMaxConfig(fieldMinMaxConfig), QDialog(parent) {
    // Set title and style
    setWindowTitle(tr("Field Min/Max Configuration"));
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
    m_nameEdit->setText(m_fieldMinMaxConfig.name);
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
            static_cast<int>(m_fieldMinMaxConfig.executeControl));
    if (execIndex != -1)
        m_executeCombo->setCurrentIndex(execIndex);
    timingLayout->addRow(tr("Execute control: "), m_executeCombo);

    // Execute interval
    m_executeSpin = new QDoubleSpinBox(this);
    m_executeSpin->setRange(0.00001, 1000000.0);
    m_executeSpin->setDecimals(5);
    m_executeSpin->setSingleStep(1.0);
    m_executeSpin->setValue(m_fieldMinMaxConfig.executeInterval);
    timingLayout->addRow(tr("Execute interval: "), m_executeSpin);

    // Write control
    m_writeCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<CaseIO::FunctionObject::ControlType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_writeCombo->addItem(metaEnum.key(i), metaEnum.value(i));
    }
    int writeIndex =
        m_writeCombo->findData(
            static_cast<int>(m_fieldMinMaxConfig.writeControl));
    if (writeIndex != -1)
        m_writeCombo->setCurrentIndex(writeIndex);
    timingLayout->addRow(tr("Write control: "), m_writeCombo);

    // Write interval
    m_writeSpin = new QDoubleSpinBox(this);
    m_writeSpin->setRange(0.00001, 1000000.0);
    m_writeSpin->setDecimals(5);
    m_writeSpin->setSingleStep(1.0);
    m_writeSpin->setValue(m_fieldMinMaxConfig.writeInterval);
    timingLayout->addRow(tr("Write interval: "), m_writeSpin);

    // Log output
    m_logCheck = new QCheckBox(tr("Log output"), this);
    m_logCheck->setChecked(m_fieldMinMaxConfig.logOutput);
    timingLayout->addRow(m_logCheck);

    // Geometry group
    QGroupBox* fieldGroup = new QGroupBox(tr("Field Configuration"), this);
    QFormLayout* fieldLayout = new QFormLayout(fieldGroup);
    mainLayout->addWidget(fieldGroup);

    // Create widget for fields
    m_fieldListWidget = new QListWidget(this);
    for (const QString& field : std::as_const(m_fields)) {
        QListWidgetItem* item = new QListWidgetItem(field, m_fieldListWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        if (m_fieldMinMaxConfig.fields.contains(field)) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
    }

    // Display four fields at a time
    if (m_fieldListWidget->count() > 0) {
        int itemHeight = m_fieldListWidget->sizeHintForRow(0);
        m_fieldListWidget->setMaximumHeight((itemHeight * 4) + 5);
    }
    fieldLayout->addRow(tr("Fields to analyze:"), m_fieldListWidget);

    // Mode
    m_modeCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<CaseIO::FieldMinMaxConfig::Mode>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_modeCombo->addItem(metaEnum.key(i), metaEnum.value(i));
    }
    int modeIndex =
        m_modeCombo->findData(
            static_cast<int>(m_fieldMinMaxConfig.mode));
    if (modeIndex != -1)
        m_modeCombo->setCurrentIndex(modeIndex);
    fieldLayout->addRow(tr("Mode: "), m_modeCombo);

    // Location
    m_locationCheck = new QCheckBox(tr("Location"), this);
    m_locationCheck->setChecked(m_fieldMinMaxConfig.location);
    fieldLayout->addRow(m_locationCheck);

    // Create OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &FieldMinMaxDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    setMinimumWidth(300);
    this->adjustSize();
}

void FieldMinMaxDialog::onOkClicked() {
    // Get list of fields
    QStringList selectedFields;
    for (int i = 0; i < m_fieldListWidget->count(); ++i) {
        QListWidgetItem* item = m_fieldListWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedFields << item->text();
        }
    }

    // Update the FieldMinMaxConfig structure
    m_fieldMinMaxConfig.name = m_nameEdit->text();
    m_fieldMinMaxConfig.type = CaseIO::FunctionObject::FuncObjType::fieldMinMax;
    int executeVal = m_executeCombo->currentData().toInt();
    m_fieldMinMaxConfig.executeControl =
        static_cast<CaseIO::FunctionObject::ControlType>(executeVal);
    m_fieldMinMaxConfig.executeInterval = m_executeSpin->value();
    int writeVal = m_writeCombo->currentData().toInt();
    m_fieldMinMaxConfig.writeControl =
        static_cast<CaseIO::FunctionObject::ControlType>(writeVal);
    m_fieldMinMaxConfig.writeInterval = m_writeSpin->value();
    m_fieldMinMaxConfig.logOutput = m_logCheck->isChecked();

    m_fieldMinMaxConfig.fields = selectedFields;
    int modeVal = m_modeCombo->currentData().toInt();
    m_fieldMinMaxConfig.mode =
        static_cast<CaseIO::FieldMinMaxConfig::Mode>(modeVal);
    m_fieldMinMaxConfig.location = m_locationCheck->isChecked();
    QDialog::accept();
}
