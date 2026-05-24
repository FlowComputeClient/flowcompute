#include "page_70_layercontrol.h"

#include "wizard_mesh.h"

// Introduction page asks for the case name and platform
LayerControlPage::LayerControlPage(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Boundary Layer Configuration"));

    // Create a vertical layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);

    // Create the group box and layout
    QGroupBox* globalLayerBox = new QGroupBox(tr("Global Layer Geometry"), this);
    layout->addWidget(globalLayerBox);
    QFormLayout* globalLayout = new QFormLayout(globalLayerBox);
    globalLayout->setVerticalSpacing(10);

    // Relative sizes
    relativeSizesBox = new QCheckBox(tr("Use relative sizes (based on background mesh)"), this);
    globalLayout->addRow(relativeSizesBox);

    // Expansion ratio
    expansionRatioBox = new QDoubleSpinBox(globalLayerBox);
    expansionRatioBox->setRange(1.0, 2.0);
    expansionRatioBox->setSingleStep(0.05);
    expansionRatioBox->setDecimals(2);
    globalLayout->addRow(tr("Expansion ratio:"), expansionRatioBox);

    // Final layer thickness
    finalLayerBox = new QDoubleSpinBox(globalLayerBox);
    finalLayerBox->setRange(0.001, 100.0);
    finalLayerBox->setSingleStep(0.1);
    finalLayerBox->setDecimals(3);
    globalLayout->addRow(tr("Final layer thickness:"), finalLayerBox);

    // Minimum thickness
    minThicknessBox = new QDoubleSpinBox(globalLayerBox);
    minThicknessBox->setRange(0.001, 100.0);
    minThicknessBox->setSingleStep(0.05);
    minThicknessBox->setDecimals(3);
    globalLayout->addRow(tr("Minimum thickness:"), minThicknessBox);

    // Update UI based on the toggle
    connect(relativeSizesBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            // Relative mode: No units
            finalLayerBox->setSuffix("");
            minThicknessBox->setSuffix("");
            finalLayerBox->setToolTip(tr("Thickness as a ratio of the adjacent background cell size."));
        } else {
            // Absolute mode: Add a unit suffix
            finalLayerBox->setSuffix(tr(" m"));
            minThicknessBox->setSuffix(tr(" m"));
            finalLayerBox->setToolTip(tr("Absolute physical thickness of the layer."));
        }
    });

    // Set group box for surface assignment
    QGroupBox* surfaceAssignmentBox = new QGroupBox(tr("Surface Assignments"), this);
    layout->addWidget(surfaceAssignmentBox);
    QGridLayout* surfaceAssignmentLayout = new QGridLayout(surfaceAssignmentBox);

    // Create label and table for mesh refinement
    surfaceAssignmentLayout->addWidget(new QLabel(tr("Set the number of layers for each geometry file")));
    surfaceLayerTable = new QTableWidget(surfaceAssignmentBox);
    surfaceLayerTable->setColumnCount(2);
    QStringList headers = { tr("Geometry"), tr("Number of Layers") };
    surfaceLayerTable->setHorizontalHeaderLabels(headers);
    surfaceLayerTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    surfaceLayerTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    surfaceLayerTable->verticalHeader()->setVisible(false);
    surfaceAssignmentLayout->addWidget(surfaceLayerTable);

    // Create the group box and layout
    QGroupBox* extrusionBox = new QGroupBox(tr("Extrusion Tolerances"), this);
    layout->addWidget(extrusionBox);
    QFormLayout* extrusionLayout = new QFormLayout(extrusionBox);
    extrusionLayout->setVerticalSpacing(10);

    // Expansion ratio
    featureAngleBox = new QDoubleSpinBox(extrusionBox);
    featureAngleBox->setRange(0.0, 180.0);
    featureAngleBox->setSingleStep(1.0);
    featureAngleBox->setSuffix(tr(" °"));
    extrusionLayout->addRow(tr("Minimum Angle to Identify Sharp Surface Edges:"), featureAngleBox);

    // Final layer thickness
    layerIterBox = new QSpinBox(extrusionBox);
    layerIterBox->setRange(10, 100);
    extrusionLayout->addRow(tr("Maximum Iterations for Adding Boundary Layers:"), layerIterBox);

    // Minimum thickness
    normalSmoothingBox = new QSpinBox(extrusionBox);
    normalSmoothingBox->setRange(1, 10);
    extrusionLayout->addRow(tr("Number of Iterations for Smoothing Surface Normals:"), normalSmoothingBox);
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
    relativeSizesBox->setChecked(m_cfg->relativeSizes);
    expansionRatioBox->setValue(m_cfg->expansionRatio);
    finalLayerBox->setValue(m_cfg->finalLayerThickness);
    minThicknessBox->setValue(m_cfg->minThickness);

    // Update the table
    QStringList geometryFiles = meshWizard->getGeometryMap().keys();
    surfaceLayerTable->setRowCount(0);
    surfaceLayerTable->setRowCount(geometryFiles.size());
    for (int i = 0; i < geometryFiles.size(); ++i) {
        QString file(geometryFiles[i]);

        // Filename
        QTableWidgetItem* fileItem = new QTableWidgetItem(file);
        fileItem->setFlags(fileItem->flags() & ~Qt::ItemIsEditable);
        surfaceLayerTable->setItem(i, 0, fileItem);

        // Number of layers
        QSpinBox* layerBox = new QSpinBox(surfaceLayerTable);
        layerBox->setRange(0, 20);
        if (m_cfg->nSurfaceLayers.contains(file)) {
            layerBox->setValue(m_cfg->nSurfaceLayers[file]);
        } else {
            layerBox->setValue(3);
        }
        surfaceLayerTable->setCellWidget(i, 1, layerBox);
    }

    // Update widgets in the extrusion tolerance group
    featureAngleBox->setValue(m_cfg->featureAngle);
    layerIterBox->setValue(m_cfg->nLayerIter);
    normalSmoothingBox->setValue(m_cfg->nSmoothSurfaceNormals);
}

bool LayerControlPage::validatePage() {

    // Update struct from global geometry widgets
    m_cfg->relativeSizes = relativeSizesBox->isChecked();
    m_cfg->expansionRatio = expansionRatioBox->value();
    m_cfg->finalLayerThickness = finalLayerBox->value();
    m_cfg->minThickness = minThicknessBox->value();

    // Update struct from surface layer table
    m_cfg->nSurfaceLayers.clear();
    for (int i = 0; i < surfaceLayerTable->rowCount(); ++i) {
        QTableWidgetItem* fileItem = surfaceLayerTable->item(i, 0);
        QSpinBox* layerBox = qobject_cast<QSpinBox*>(surfaceLayerTable->cellWidget(i, 1));

        if (fileItem && layerBox) {
            m_cfg->nSurfaceLayers[fileItem->text()] = layerBox->value();
        }
    }

    // Update struct from extrusion tolerance widgets
    m_cfg->featureAngle = featureAngleBox->value();
    m_cfg->nLayerIter = layerIterBox->value();
    m_cfg->nSmoothSurfaceNormals = normalSmoothingBox->value();

    return true;
}