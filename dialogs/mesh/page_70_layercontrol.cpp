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

#include "page_70_layercontrol.h"

#include "wizard_mesh.h"

QWidget* centerBox(QCheckBox* box);

// Introduction page asks for the case name and platform
LayerControlPage::LayerControlPage(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Boundary Layer Configuration"));

    // Create a vertical layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);

    // Create the group box and layout
    QGroupBox* globalLayerGroup =
        new QGroupBox(tr("Global Layer Geometry"), this);
    layout->addWidget(globalLayerGroup);
    QFormLayout* globalLayout = new QFormLayout(globalLayerGroup);
    globalLayout->setVerticalSpacing(10);

    // Relative sizes
    relativeSizesCheck = new QCheckBox(tr("Use relative sizes "
                                          "(based on background mesh)"), this);
    globalLayout->addRow(relativeSizesCheck);

    // Expansion ratio
    expansionRatioSpin = new QDoubleSpinBox(globalLayerGroup);
    expansionRatioSpin->setRange(1.0, 3.0);
    expansionRatioSpin->setSingleStep(0.05);
    expansionRatioSpin->setDecimals(2);
    globalLayout->addRow(tr("Expansion ratio:"), expansionRatioSpin);

    // Final layer thickness
    finalLayerSpin = new QDoubleSpinBox(globalLayerGroup);
    finalLayerSpin->setRange(0.001, 100.0);
    finalLayerSpin->setSingleStep(0.1);
    finalLayerSpin->setDecimals(3);
    globalLayout->addRow(tr("Final layer thickness:"), finalLayerSpin);

    // Minimum thickness
    minThicknessSpin = new QDoubleSpinBox(globalLayerGroup);
    minThicknessSpin->setRange(0.001, 100.0);
    minThicknessSpin->setSingleStep(0.05);
    minThicknessSpin->setDecimals(3);
    globalLayout->addRow(tr("Minimum thickness:"), minThicknessSpin);

    // Update UI based on the toggle
    connect(relativeSizesCheck, &QCheckBox::toggled, this,
            [this](bool checked) {
        if (checked) {
            // Relative mode: No units
            finalLayerSpin->setSuffix("");
            minThicknessSpin->setSuffix("");
            finalLayerSpin->setToolTip(tr("Thickness as a ratio of the "
                                          "adjacent background cell size."));
        } else {
            // Absolute mode: Add a unit suffix
            finalLayerSpin->setSuffix(tr(" m"));
            minThicknessSpin->setSuffix(tr(" m"));
            finalLayerSpin->setToolTip(tr("Absolute physical thickness "
                                          "of the layer."));
        }
    });

    // Set group box for surface assignment
    QGroupBox* surfaceAssignmentGroup =
        new QGroupBox(tr("Surface Assignments"), this);
    layout->addWidget(surfaceAssignmentGroup);
    QGridLayout* surfaceAssignmentLayout =
        new QGridLayout(surfaceAssignmentGroup);

    // Create label and table for mesh refinement
    surfaceAssignmentLayout->addWidget(
        new QLabel(tr("Set the number of layers for each geometry file")));
    surfaceLayerTable = new QTableWidget(surfaceAssignmentGroup);
    surfaceLayerTable->setColumnCount(3);
    QStringList headers =
        { tr("Enable Layer"), tr("Layer Name"), tr("Number of Layers") };
    surfaceLayerTable->setHorizontalHeaderLabels(headers);
    surfaceLayerTable->horizontalHeader()->setSectionResizeMode(0,
                                QHeaderView::ResizeToContents);
    surfaceLayerTable->horizontalHeader()->setSectionResizeMode(1,
                                QHeaderView::Stretch);
    surfaceLayerTable->horizontalHeader()->setSectionResizeMode(2,
                                QHeaderView::ResizeToContents);
    surfaceLayerTable->verticalHeader()->setVisible(false);
    surfaceLayerTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    surfaceAssignmentLayout->addWidget(surfaceLayerTable);

    // Create the group box and layout
    QGroupBox* extrusionGroup =
        new QGroupBox(tr("Layer Smoothing and Iterations"), this);
    layout->addWidget(extrusionGroup);
    QFormLayout* extrusionLayout = new QFormLayout(extrusionGroup);
    extrusionLayout->setVerticalSpacing(10);

    // Feature angle
    featureAngleSpin = new QDoubleSpinBox(extrusionGroup);
    featureAngleSpin->setRange(0.0, 180.0);
    featureAngleSpin->setSingleStep(1.0);
    featureAngleSpin->setSuffix(tr(" °"));
    extrusionLayout->addRow(tr("Maximum angle for continuous layer extrusion:"),
                            featureAngleSpin);

    // Final layer thickness
    layerIterSpin = new QSpinBox(extrusionGroup);
    layerIterSpin->setRange(1, 200);
    extrusionLayout->addRow(tr("Maximum iterations for adding "
                               "boundary layers:"), layerIterSpin);

    // Thickness smoothing
    thicknessSmoothingSpin = new QSpinBox(extrusionGroup);
    thicknessSmoothingSpin->setRange(0, 50);
    extrusionLayout->addRow(tr("Iterations to smooth layer thickness:"),
                            thicknessSmoothingSpin);

    // Smoothing surface normals
    surfaceSmoothingSpin = new QSpinBox(extrusionGroup);
    surfaceSmoothingSpin->setRange(0, 10);
    extrusionLayout->addRow(tr("Iterations to smooth patch surface normals:"),
                            surfaceSmoothingSpin);

    // Smoothing internal normals
    internalSmoothingSpin = new QSpinBox(extrusionGroup);
    internalSmoothingSpin->setRange(0, 20);
    extrusionLayout->addRow(tr("Iterations to smooth interior mesh movement:"),
                            internalSmoothingSpin);
}

void LayerControlPage::initializePage() {

    // Access the mesh wizard
    meshWizard = qobject_cast<MeshWizard*>(wizard());
    if (!meshWizard) {
        qWarning() << "Failed to cast to MeshWizard!";
        return;
    }
    m_cfg = &(meshWizard->getLayerControlConfig());

    // Update widgets in global geometry group
    relativeSizesCheck->setChecked(m_cfg->relativeSizes);
    expansionRatioSpin->setValue(m_cfg->expansionRatio);
    finalLayerSpin->setValue(m_cfg->finalLayerThickness);
    minThicknessSpin->setValue(m_cfg->minThickness);

    // Get list of layers
    QStringList geometryFiles = meshWizard->getGeometryMap().keys();
    QStringList layerNames;
    for (int i = 0; i < geometryFiles.size(); ++i) {
        std::vector<std::string> patches =
            meshWizard->getGeometryMap()[geometryFiles[i]].patches;
        if (patches.empty()) {
            layerNames.push_back(geometryFiles[i] + ".stl.*");
            continue;
        }
        for (auto const& patch: patches) {
            layerNames.push_back(QString::fromStdString(patch));
        }
    }

    // Initialize the table
    surfaceLayerTable->setRowCount(0);
    surfaceLayerTable->setRowCount(layerNames.size());
    for (int i = 0; i < layerNames.size(); ++i) {
        QString layerName = layerNames[i];

        // Enable Layer
        QCheckBox* enableLayersCheck = new QCheckBox(this);
        enableLayersCheck->setChecked(false);
        surfaceLayerTable->setCellWidget(i, 0, centerBox(enableLayersCheck));

        // Layer name
        QTableWidgetItem* fileItem = new QTableWidgetItem(layerName);
        fileItem->setFlags(fileItem->flags() & ~Qt::ItemIsEditable);
        fileItem->setTextAlignment(Qt::AlignCenter);
        surfaceLayerTable->setItem(i, 1, fileItem);

        // Number of layers
        QSpinBox* layerSpin = new QSpinBox(surfaceLayerTable);
        layerSpin->setRange(0, 20);
        if (m_cfg->nSurfaceLayers.contains(layerName)) {
            layerSpin->setValue(m_cfg->nSurfaceLayers[layerName]);
        } else {
            layerSpin->setValue(3);
        }
        surfaceLayerTable->setCellWidget(i, 2, layerSpin);
    }

    // Update widgets in the extrusion tolerance group
    featureAngleSpin->setValue(m_cfg->featureAngle);
    layerIterSpin->setValue(m_cfg->nLayerIter);
    thicknessSmoothingSpin->setValue(m_cfg->nSmoothThickness);
    surfaceSmoothingSpin->setValue(m_cfg->nSmoothSurfaceNormals);
    internalSmoothingSpin->setValue(m_cfg->nSmoothNormals);
}

// Center checkboxes visually in a table cell
QWidget* centerBox(QCheckBox* box) {
    QWidget* widget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->addWidget(box);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    return widget;
}

bool LayerControlPage::validatePage() {

    // Update struct from global geometry widgets
    m_cfg->relativeSizes = relativeSizesCheck->isChecked();
    m_cfg->expansionRatio = expansionRatioSpin->value();
    m_cfg->finalLayerThickness = finalLayerSpin->value();
    m_cfg->minThickness = minThicknessSpin->value();

    // Update struct from surface layer table
    m_cfg->nSurfaceLayers.clear();
    for (int i = 0; i < surfaceLayerTable->rowCount(); ++i) {

        // Access widgets for table items
        QWidget* wrapperWidget = surfaceLayerTable->cellWidget(i, 0);
        QCheckBox* enableBox =
            wrapperWidget ? wrapperWidget->findChild<QCheckBox*>() : nullptr;

        // Retrieve the other elements
        QTableWidgetItem* fileItem = surfaceLayerTable->item(i, 1);
        QSpinBox* layerBox =
            qobject_cast<QSpinBox*>(surfaceLayerTable->cellWidget(i, 2));

        // Ensure pointers are valid
        if (enableBox && fileItem && layerBox) {
            if (enableBox->isChecked()) {
                m_cfg->nSurfaceLayers[fileItem->text()] = layerBox->value();
            }
        } else {
            qWarning() << "Failed to retrieve one or more widgets for row" << i;
        }
    }

    // Update struct from extrusion tolerance widgets
    m_cfg->featureAngle = featureAngleSpin->value();
    m_cfg->nLayerIter = layerIterSpin->value();
    m_cfg->nSmoothThickness = thicknessSmoothingSpin->value();
    m_cfg->nSmoothSurfaceNormals = surfaceSmoothingSpin->value();
    m_cfg->nSmoothNormals = internalSmoothingSpin->value();

    return true;
}