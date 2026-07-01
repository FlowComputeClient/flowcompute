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

#include "page_93_visualization.h"

#include "wizard_solver.h"
#include "../surface/surface_dialog.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMetaEnum>
#include <QVBoxLayout>

// Configures decomposeParDict
VisualizationPage::VisualizationPage(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Visualization Configuration"));
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(15);

    // Global settings group
    QGroupBox* globalGroup = new QGroupBox(tr("Global Visualization Settings"), this);
    layout->addWidget(globalGroup);
    QFormLayout* globalLayout = new QFormLayout(globalGroup);
    globalLayout->setSpacing(15);
    globalLayout->setContentsMargins(10, 15, 10, 10);

    // Surface format
    m_surfaceFormatCombo = new QComboBox(this);
    QMetaEnum metaEnum = QMetaEnum::fromType<Solver::SurfaceFormat>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_surfaceFormatCombo->addItem(metaEnum.key(i), static_cast<int>(metaEnum.value(i)));
    }
    globalLayout->addRow(tr("Surface format: "), m_surfaceFormatCombo);

    // Write control
    m_writeControlCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<Solver::VisWriteControlType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_writeControlCombo->addItem(metaEnum.key(i), static_cast<int>(metaEnum.value(i)));
    }
    globalLayout->addRow(tr("Write control: "), m_writeControlCombo);

    // Write interval
    m_writeIntervalEdit = new QLineEdit(this);
    globalLayout->addRow(tr("Write interval: "), m_writeIntervalEdit);
    m_writeIntervalEdit->setText("50");

    // Create validator for write interval
    m_intValidator = new QIntValidator(1, 9999999, this);
    m_doubleValidator = new QDoubleValidator(0.0, 999999.0, 6, this);
    m_doubleValidator->setLocale(QLocale::C);

    // Interpolation scheme
    m_interpolationCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<Solver::InterpolationType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_interpolationCombo->addItem(metaEnum.key(i), static_cast<int>(metaEnum.value(i)));
    }
    globalLayout->addRow(tr("Interpolation scheme: "), m_interpolationCombo);

    // Field selection
    m_fieldListWidget = new QListWidget(this);
    m_fieldListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    globalLayout->addRow(tr("Select one or more fields: "), m_fieldListWidget);

    // Surface definitions group
    QGroupBox* surfaceGroup = new QGroupBox(tr("Surface Definitions"), this);
    layout->addWidget(surfaceGroup);
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
    connect(addSurfaceButton, &QPushButton::clicked, this, &VisualizationPage::addSurface);

    // Remove surface button
    QPushButton* removeSurfaceButton = new QPushButton(tr("Remove Surface"), this);
    buttonLayout->addWidget(removeSurfaceButton, 1);
    removeSurfaceButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(removeSurfaceButton, &QPushButton::clicked, this, &VisualizationPage::removeSurface);
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

    // Add space
    layout->addItem(new QSpacerItem(0, 0,
        QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void VisualizationPage::initializePage() {

    // Access wizard data
    solverWizard = qobject_cast<SolverWizard*>(this->wizard());
    if (!solverWizard) return;

    // Update list of fields
    m_fieldListWidget->clear();
    QStringList fields = solverWizard->getConfiguredFields();
    for (const auto& field: std::as_const(fields)) {
        m_fieldListWidget->addItem(field);
    }

    // Set height of list widget
    int visibleRows = 5;
    int rowHeight = m_fieldListWidget->sizeHintForRow(0);
    if (rowHeight > 0) {
        int frame = 2 * m_fieldListWidget->frameWidth();
        m_fieldListWidget->setFixedHeight(rowHeight * visibleRows + frame);
    }
}

bool VisualizationPage::validatePage() {
    if (!solverWizard) return true;

    // Get structure from wizard
    VisualizationConfig* cfg = &(solverWizard->getVisualizationConfig());

    // Update global settings
    cfg->surfaceFormat = static_cast<Solver::SurfaceFormat>(m_surfaceFormatCombo->currentData().toInt());
    cfg->writeControl = static_cast<Solver::VisWriteControlType>(m_writeControlCombo->currentData().toInt());
    cfg->writeInterval = m_writeIntervalEdit->text().toDouble();
    cfg->interpolationType = static_cast<Solver::InterpolationType>(m_interpolationCombo->currentData().toInt());

    // Update selected fields
    cfg->fieldNames.clear();
    QList<QListWidgetItem*> items = m_fieldListWidget->selectedItems();
    QStringList selectedTexts;
    for (QListWidgetItem* item : std::as_const(items)) {
        cfg->fieldNames.push_back(item->text());
    }

    // Update surface definitions
    cfg->surfaces.clear();

    for (int i = 0; i < m_surfaceListWidget->count(); ++i) {
        QListWidgetItem* item = m_surfaceListWidget->item(i);

        VisualizationSurface surface;
        surface.name = item->text();
        surface.type = static_cast<Solver::SurfaceType>(item->data(Qt::UserRole).toInt());

        // Retrieve the corresponding form widget from the stack
        QWidget* detailWidget = m_surfaceStack->widget(i);
        if (detailWidget) {
            QList<QLineEdit*> lineEdits = detailWidget->findChildren<QLineEdit*>();
            for (QLineEdit* lineEdit : std::as_const(lineEdits)) {
                QString paramName = lineEdit->property("paramName").toString();
                if (!paramName.isEmpty()) {
                    surface.parameters[paramName] = lineEdit->text().trimmed();
                }
            }
        }
        cfg->surfaces.push_back(surface);
    }
    return true;
}

void VisualizationPage::addSurface() {

    // Create dialog
    AddSurfaceDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString surfaceName = dialog.getSurfaceName();
        int surfaceType = dialog.getSurfaceType();

        // Check for duplicate names in the QListWidget
        if (!m_surfaceListWidget->findItems(surfaceName, Qt::MatchExactly).isEmpty()) {
            return;
        }

        // Update the list widget
        QListWidgetItem* item = new QListWidgetItem(surfaceName, m_surfaceListWidget);
        item->setData(Qt::UserRole, surfaceType);

        // Generate the parameter widget for the stacked widget
        QWidget* paramWidget = new QWidget(m_surfaceStack);
        QFormLayout* paramLayout = new QFormLayout();
        Solver::SurfaceType typeEnum = static_cast<Solver::SurfaceType>(surfaceType);
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

void VisualizationPage::removeSurface() {

}
