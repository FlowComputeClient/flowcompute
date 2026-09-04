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

#include "wizards/post_processing/surfaces_dialog.h"

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
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>

#include "wizards/post_processing/add_surface_dialog.h"

SurfacesDialog::SurfacesDialog(const QStringList& fields,
    CaseIO::SurfacesConfig& surfacesConfig, QWidget* parent): QDialog(parent),
    m_fields(fields), m_surfacesConfig(surfacesConfig) {
    // Set title and style
    setWindowTitle(tr("Surfaces Configuration"));
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
    m_nameEdit->setText(m_surfacesConfig.name);
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
            static_cast<int>(m_surfacesConfig.executeControl));
    if (execIndex != -1)
        m_executeCombo->setCurrentIndex(execIndex);
    timingLayout->addRow(tr("Execute control: "), m_executeCombo);

    // Execute interval
    m_executeSpin = new QDoubleSpinBox(this);
    m_executeSpin->setRange(0.00001, 1000000.0);
    m_executeSpin->setDecimals(5);
    m_executeSpin->setSingleStep(1.0);
    m_executeSpin->setValue(m_surfacesConfig.executeInterval);
    timingLayout->addRow(tr("Execute interval: "), m_executeSpin);

    // Write control
    m_writeCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<CaseIO::FunctionObject::ControlType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_writeCombo->addItem(metaEnum.key(i), metaEnum.value(i));
    }
    int writeIndex =
        m_writeCombo->findData(
            static_cast<int>(m_surfacesConfig.writeControl));
    if (writeIndex != -1)
        m_writeCombo->setCurrentIndex(writeIndex);
    timingLayout->addRow(tr("Write control: "), m_writeCombo);

    // Write interval
    m_writeSpin = new QDoubleSpinBox(this);
    m_writeSpin->setRange(0.00001, 1000000.0);
    m_writeSpin->setDecimals(5);
    m_writeSpin->setSingleStep(1.0);
    m_writeSpin->setValue(m_surfacesConfig.writeInterval);
    timingLayout->addRow(tr("Write interval: "), m_writeSpin);

    // Log output
    m_logCheck = new QCheckBox(tr("Log output"), this);
    m_logCheck->setChecked(m_surfacesConfig.logOutput);
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
        if (m_surfacesConfig.fields.contains(field)) {
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

    // Surface format
    m_surfaceFormatCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<CaseIO::SurfacesConfig::SurfaceFormat>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_surfaceFormatCombo->addItem(metaEnum.key(i),
            static_cast<int>(metaEnum.value(i)));
    }
    fieldLayout->addRow(tr("Surface format: "), m_surfaceFormatCombo);

    // Interpolation scheme
    m_interpolationCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<CaseIO::FunctionObject::InterpolationType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_interpolationCombo->addItem(metaEnum.key(i),
            static_cast<int>(metaEnum.value(i)));
    }
    fieldLayout->addRow(tr("Interpolation scheme: "), m_interpolationCombo);

    // Surface definitions group
    QGroupBox* surfaceGroup = new QGroupBox(tr("Surface Definitions"), this);
    mainLayout->addWidget(surfaceGroup);
    QVBoxLayout* surfaceLayout = new QVBoxLayout(surfaceGroup);
    surfaceLayout->setSpacing(10);
    surfaceLayout->setContentsMargins(10, 15, 10, 10);

    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    // Add surface button
    QPushButton* addSurfaceButton = new QPushButton(tr("Add Surface"), this);
    buttonLayout->addWidget(addSurfaceButton, 1);
    addSurfaceButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(addSurfaceButton, &QPushButton::clicked, this,
            &SurfacesDialog::addSurface);

    // Remove surface button
    QPushButton* removeSurfaceButton =
        new QPushButton(tr("Remove Surface"), this);
    buttonLayout->addWidget(removeSurfaceButton, 1);
    removeSurfaceButton->setSizePolicy(
        QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(removeSurfaceButton, &QPushButton::clicked, this,
            &SurfacesDialog::removeSurface);
    surfaceLayout->addLayout(buttonLayout);

    // List/stack layout
    QHBoxLayout* listStackLayout = new QHBoxLayout();
    listStackLayout->setSpacing(10);

    // List widget
    m_surfaceListWidget = new QListWidget(this);
    listStackLayout->addWidget(m_surfaceListWidget, 1);

    // Stack widget
    m_surfaceStack = new QStackedWidget(this);
    listStackLayout->addWidget(m_surfaceStack, 2);
    surfaceLayout->addLayout(listStackLayout);

    // Event handling
    connect(m_surfaceListWidget, &QListWidget::currentRowChanged,
            m_surfaceStack, &QStackedWidget::setCurrentIndex);

    // Create OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &SurfacesDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    setMinimumWidth(300);
    this->adjustSize();
}

void SurfacesDialog::addSurface() {
    // Create dialog
    AddSurfaceDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString surfaceName = dialog.getSurfaceName();
        int surfaceType = dialog.getSurfaceType();

        // Check for duplicate names in the QListWidget
        if (!m_surfaceListWidget->findItems(
                surfaceName, Qt::MatchExactly).isEmpty()) {
            return;
        }

        // Update the list widget
        QListWidgetItem* item =
            new QListWidgetItem(surfaceName, m_surfaceListWidget);
        item->setData(Qt::UserRole, surfaceType);

        // Generate the parameter widget for the stacked widget
        QWidget* paramWidget = new QWidget(m_surfaceStack);
        QFormLayout* paramLayout = new QFormLayout();
        CaseIO::SurfaceDef::SurfaceType typeEnum =
            static_cast<CaseIO::SurfaceDef::SurfaceType>(surfaceType);
        auto it = m_surfaceParameters.find(typeEnum);
        if (it != m_surfaceParameters.end()) {
            for (const QString& paramName : it->second) {
                QLineEdit* lineEdit = new QLineEdit();
                lineEdit->setProperty("paramName", paramName);
                paramLayout->addRow(paramName + ":", lineEdit);
            }
        }

        // Set the layout
        paramWidget->setLayout(paramLayout);
        m_surfaceStack->addWidget(paramWidget);
        m_surfaceListWidget->setCurrentItem(item);
    }
}

void SurfacesDialog::removeSurface() {
    // Identify the current selected row
    int currentRow = m_surfaceListWidget->currentRow();

    // If no item is selected, do nothing
    if (currentRow < 0) {
        return;
    }

    // Remove and delete the parameter widget from the stacked widget
    QWidget* paramWidget = m_surfaceStack->widget(currentRow);
    if (paramWidget) {
        m_surfaceStack->removeWidget(paramWidget);
        delete paramWidget;
    }

    // Remove and delete the item from the list widget
    QListWidgetItem* item = m_surfaceListWidget->takeItem(currentRow);
    if (item)
        delete item;
}

void SurfacesDialog::onOkClicked() {
    // Make sure a name has been provided
    if (m_nameEdit->text().isEmpty()) {
        QMessageBox::critical(this, tr("Missing Item"),
            tr("A name must be provided for the post-processing task."));
        return;
    }

    // Get list of fields
    QStringList selectedFields;
    for (int i = 0; i < m_fieldListWidget->count(); ++i) {
        QListWidgetItem* item = m_fieldListWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedFields << item->text();
        }
    }

    // Make sure at least one field has been selected
    if (selectedFields.isEmpty()) {
        QMessageBox::critical(this, tr("Missing Item"),
                              tr("At least one field must be selected."));
        return;
    }

    // Update the SurfacesConfig structure
    m_surfacesConfig.name = m_nameEdit->text();
    m_surfacesConfig.type = CaseIO::FunctionObject::FuncObjType::surfaces;
    int executeVal = m_executeCombo->currentData().toInt();
    m_surfacesConfig.executeControl =
        static_cast<CaseIO::FunctionObject::ControlType>(executeVal);
    m_surfacesConfig.executeInterval = m_executeSpin->value();
    int writeVal = m_writeCombo->currentData().toInt();
    m_surfacesConfig.writeControl =
        static_cast<CaseIO::FunctionObject::ControlType>(writeVal);
    m_surfacesConfig.writeInterval = m_writeSpin->value();
    m_surfacesConfig.logOutput = m_logCheck->isChecked();
    m_surfacesConfig.fields = selectedFields;

    // Clear existing surfaces
    m_surfacesConfig.surfaces.clear();

    // Build surface definitions
    for (int i = 0; i < m_surfaceListWidget->count(); ++i) {
        QListWidgetItem* item = m_surfaceListWidget->item(i);
        CaseIO::SurfaceDef sDef;

        // Extract name and type from the list item
        sDef.name = item->text();
        sDef.type = static_cast<CaseIO::SurfaceDef::SurfaceType>(
            item->data(Qt::UserRole).toInt());

        // Get the corresponding parameter widget from the stack
        QWidget* paramWidget = m_surfaceStack->widget(i);
        if (paramWidget) {
            // Find all QLineEdits associated with this surface's parameters
            QList<QLineEdit*> lineEdits =
                paramWidget->findChildren<QLineEdit*>();
            for (QLineEdit* lineEdit : std::as_const(lineEdits)) {
                QString paramName = lineEdit->property("paramName").toString();
                QString paramValue = lineEdit->text().trimmed();

                // Only store the parameter if the user provided a value
                if (!paramValue.isEmpty()) {
                    sDef.parameters[paramName] = paramValue;
                }
            }
        }
        m_surfacesConfig.surfaces.push_back(sDef);
    }

    QDialog::accept();
}
