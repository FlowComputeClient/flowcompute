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

#include "probes_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMetaEnum>
#include <QPushButton>
#include <QString>
#include <QTableWidget>
#include <QVBoxLayout>

ProbesDialog::ProbesDialog(const QStringList& fields,
    CaseIO::ProbesConfig& probesConfig, QWidget* parent): m_fields(fields),
    m_probesConfig(probesConfig), QDialog(parent) {
    // Set title and style
    setWindowTitle(tr("Probes Configuration"));
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
    m_nameEdit->setText(m_probesConfig.name);
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
            static_cast<int>(m_probesConfig.executeControl));
    if (execIndex != -1)
        m_executeCombo->setCurrentIndex(execIndex);
    timingLayout->addRow(tr("Execute control: "), m_executeCombo);

    // Execute interval
    m_executeSpin = new QDoubleSpinBox(this);
    m_executeSpin->setRange(0.00001, 1000000.0);
    m_executeSpin->setDecimals(5);
    m_executeSpin->setSingleStep(1.0);
    m_executeSpin->setValue(m_probesConfig.executeInterval);
    timingLayout->addRow(tr("Execute interval: "), m_executeSpin);

    // Write control
    m_writeCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<CaseIO::FunctionObject::ControlType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_writeCombo->addItem(metaEnum.key(i), metaEnum.value(i));
    }
    int writeIndex =
        m_writeCombo->findData(
            static_cast<int>(m_probesConfig.writeControl));
    if (writeIndex != -1)
        m_writeCombo->setCurrentIndex(writeIndex);
    timingLayout->addRow(tr("Write control: "), m_writeCombo);

    // Write interval
    m_writeSpin = new QDoubleSpinBox(this);
    m_writeSpin->setRange(0.00001, 1000000.0);
    m_writeSpin->setDecimals(5);
    m_writeSpin->setSingleStep(1.0);
    m_writeSpin->setValue(m_probesConfig.writeInterval);
    timingLayout->addRow(tr("Write interval: "), m_writeSpin);

    // Log output
    m_logCheck = new QCheckBox(tr("Log output"), this);
    m_logCheck->setChecked(m_probesConfig.logOutput);
    timingLayout->addRow(m_logCheck);

    // Probe group
    QGroupBox* probeGroup = new QGroupBox(tr("Probe Configuration"), this);
    QFormLayout* probeLayout = new QFormLayout(probeGroup);
    mainLayout->addWidget(probeGroup);

    // Create widget for fields
    m_fieldListWidget = new QListWidget(this);
    for (const QString& field : std::as_const(m_fields)) {
        QListWidgetItem* item = new QListWidgetItem(field, m_fieldListWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        if (m_probesConfig.fields.contains(field)) {
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
    probeLayout->addRow(tr("Fields to analyze:"), m_fieldListWidget);

    // Create buttons to add/remove probes
    QVBoxLayout* tableLayout = new QVBoxLayout();
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* addProbeButton = new QPushButton(tr("Add Probe"), this);
    buttonLayout->addWidget(addProbeButton);
    QPushButton* removeProbeButton = new QPushButton(tr("Remove Probe"), this);
    buttonLayout->addWidget(removeProbeButton);
    buttonLayout->addStretch();
    tableLayout->addLayout(buttonLayout);

    // Create table containing probe coordinates
    m_probesTable = new QTableWidget(this);
    m_probesTable->setColumnCount(3);
    m_probesTable->setHorizontalHeaderLabels({tr("X"), tr("Y"), tr("Z")});
    m_probesTable->horizontalHeader()->
        setSectionResizeMode(QHeaderView::Stretch);
    m_probesTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Configure table rows
    m_probesTable->setRowCount(m_probesConfig.probeLocations.size());
    for (int i = 0; i < m_probesConfig.probeLocations.size(); ++i) {
        const QVector3D& loc = m_probesConfig.probeLocations[i];
        m_probesTable->setItem(i, 0,
            new QTableWidgetItem(QString::number(loc.x())));
        m_probesTable->setItem(i, 1,
            new QTableWidgetItem(QString::number(loc.y())));
        m_probesTable->setItem(i, 2,
            new QTableWidgetItem(QString::number(loc.z())));
    }
    tableLayout->addWidget(m_probesTable);

    // Configure event handling for the Add Probe button
    connect(addProbeButton, &QPushButton::clicked, this, [this]() {
        int row = m_probesTable->rowCount();
        m_probesTable->insertRow(row);

        // Create row with default 0.0 coordinates
        m_probesTable->setItem(row, 0, new QTableWidgetItem("0.0"));
        m_probesTable->setItem(row, 1, new QTableWidgetItem("0.0"));
        m_probesTable->setItem(row, 2, new QTableWidgetItem("0.0"));
        m_probesTable->selectRow(row);
    });

    // Configure event handling for the Remove Probe button
    connect(removeProbeButton, &QPushButton::clicked, this, [this]() {
        QList<QTableWidgetItem*> selectedItems = m_probesTable->selectedItems();
        if (selectedItems.isEmpty())
            return;

        // Collect unique rows
        QSet<int> rows;
        for (QTableWidgetItem* item : std::as_const(selectedItems)) {
            rows.insert(item->row());
        }

        // Convert to list and sort in descending order
        QList<int> rowList = rows.values();
        std::sort(rowList.begin(), rowList.end(), std::greater<int>());
        for (int row : std::as_const(rowList)) {
            m_probesTable->removeRow(row);
        }
    });

    // Interpolation scheme
    m_interpolationCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<CaseIO::FunctionObject::InterpolationType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_interpolationCombo->addItem(metaEnum.key(i), metaEnum.value(i));
    }
    int interpolationIndex =
        m_interpolationCombo->findData(
            static_cast<int>(m_probesConfig.interpolationScheme));
    if (interpolationIndex != -1)
        m_interpolationCombo->setCurrentIndex(interpolationIndex);
    probeLayout->addRow(tr("Interpolation scheme: "), m_interpolationCombo);

    // Fixed location
    m_fixedLocCheck = new QCheckBox(tr("Fixed location"), this);
    m_fixedLocCheck->setChecked(m_probesConfig.fixedLocations);
    probeLayout->addRow(m_fixedLocCheck);

    // Include out-of-bounds
    m_outBoundsCheck = new QCheckBox(tr("Include out-of-bounds"), this);
    m_outBoundsCheck->setChecked(m_probesConfig.includeOutOfBounds);
    probeLayout->addRow(m_outBoundsCheck);

    // Verbose output
    m_verboseCheck = new QCheckBox(tr("Verbose output"), this);
    m_verboseCheck->setChecked(m_probesConfig.verbose);
    probeLayout->addRow(m_verboseCheck);

    // Sample on execute
    m_sampleCheck = new QCheckBox(tr("Sample on execute"), this);
    m_sampleCheck->setChecked(m_probesConfig.sampleOnExecute);
    probeLayout->addRow(m_sampleCheck);

    // Create OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &ProbesDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    setMinimumWidth(300);
    this->adjustSize();
}

void ProbesDialog::onOkClicked() {
    // Get list of fields
    QStringList selectedFields;
    for (int i = 0; i < m_fieldListWidget->count(); ++i) {
        QListWidgetItem* item = m_fieldListWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedFields << item->text();
        }
    }

    // Set probe locations
    m_probesConfig.probeLocations.clear();
    for (int row = 0; row < m_probesTable->rowCount(); ++row) {
        QTableWidgetItem* itemX = m_probesTable->item(row, 0);
        QTableWidgetItem* itemY = m_probesTable->item(row, 1);
        QTableWidgetItem* itemZ = m_probesTable->item(row, 2);

        // Convert table items to double
        double x = itemX ? itemX->text().toDouble() : 0.0;
        double y = itemY ? itemY->text().toDouble() : 0.0;
        double z = itemZ ? itemZ->text().toDouble() : 0.0;

        m_probesConfig.probeLocations.push_back(QVector3D(x, y, z));
    }

    // Update the ProbesConfig structure
    m_probesConfig.name = m_nameEdit->text();
    m_probesConfig.type = CaseIO::FunctionObject::FuncObjType::probes;
    int executeVal = m_executeCombo->currentData().toInt();
    m_probesConfig.executeControl =
        static_cast<CaseIO::FunctionObject::ControlType>(executeVal);
    m_probesConfig.executeInterval = m_executeSpin->value();
    int writeVal = m_writeCombo->currentData().toInt();
    m_probesConfig.writeControl =
        static_cast<CaseIO::FunctionObject::ControlType>(writeVal);
    m_probesConfig.writeInterval = m_writeSpin->value();
    m_probesConfig.logOutput = m_logCheck->isChecked();

    m_probesConfig.fields = selectedFields;
    int interpolateVal = m_interpolationCombo->currentData().toInt();
    m_probesConfig.interpolationScheme =
        static_cast<CaseIO::FunctionObject::InterpolationType>(interpolateVal);
    m_probesConfig.fixedLocations = m_fixedLocCheck->isChecked();
    m_probesConfig.includeOutOfBounds = m_outBoundsCheck->isChecked();
    m_probesConfig.verbose = m_verboseCheck->isChecked();
    m_probesConfig.sampleOnExecute = m_sampleCheck->isChecked();
    QDialog::accept();
}
