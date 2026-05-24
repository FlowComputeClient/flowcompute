#include "page_50_castellation.h"
#include "page_10_geometry.h"

#include "wizard_mesh.h"

// Introduction page asks for the case name and platform
CastellationPage::CastellationPage(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Initial (Castellation) Mesh Configuration"));

    // Create a vertical layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);

    // Set group box for global constraints
    QGroupBox* globalBox = new QGroupBox(tr("Global Settings"), this);
    layout->addWidget(globalBox);
    QFormLayout* globalLayout = new QFormLayout(globalBox);

    // Create combo box to select meshing region
    meshRegionBox = new QComboBox(this);
    globalLayout->addRow(tr("Select mesh location:"), meshRegionBox);
    connect(meshRegionBox, &QComboBox::currentIndexChanged,
            this, &CastellationPage::onMeshRegionChanged);

    // Label for custom coordinates
    QHBoxLayout *locationLayout = new QHBoxLayout;
    for(int i = 0; i < 3; i++) {
        loc[i] = new QDoubleSpinBox(this);
        loc[i]->setRange(-1e9, 1e9);
        loc[i]->setDecimals(5);
        loc[i]->setSingleStep(0.1);
        loc[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        loc[i]->setMaximumWidth(80);
        locationLayout->addWidget(loc[i]);
    }
    globalLayout->addRow(tr("Active coordinates (x, y, z):"), locationLayout);
    connect(loc[0], &QDoubleSpinBox::valueChanged, this, &CastellationPage::onMeshLocationChanged);
    connect(loc[1], &QDoubleSpinBox::valueChanged, this, &CastellationPage::onMeshLocationChanged);
    connect(loc[2], &QDoubleSpinBox::valueChanged, this, &CastellationPage::onMeshLocationChanged);

    // Create spin box for max global cells
    maxGlobalCellBox = new QSpinBox(globalBox);
    maxGlobalCellBox->setRange(100000, 100000000);
    maxGlobalCellBox->setSingleStep(100000);
    maxGlobalCellBox->setValue(2000000);
    globalLayout->addRow(tr("Maximum number of cells in the mesh:"), maxGlobalCellBox);

    // Create spin box for max local cells
    maxLocalCellBox = new QSpinBox(globalBox);
    maxLocalCellBox->setRange(100000, 5000000);
    maxLocalCellBox->setSingleStep(100000);
    maxLocalCellBox->setValue(1000000);
    globalLayout->addRow(tr("Maximum number of cells per processor:"), maxLocalCellBox);

    // Create check box to allow free-standing zone faces
    freeStandingZoneBox = new QCheckBox(tr("Retain internal face zones without enclosed volumes"), this);
    globalLayout->addRow(freeStandingZoneBox);

    // Set group box for mesh transition
    QGroupBox* transitionBox = new QGroupBox(tr("Mesh Transition"), this);
    layout->addWidget(transitionBox);
    QFormLayout* transitionLayout = new QFormLayout(transitionBox);

    // Create spin box for cells between levels
    cellLevelBox = new QSpinBox(transitionBox);
    cellLevelBox->setRange(1, 10);
    transitionLayout->addRow(tr("Thickness of layer between refinement levels:"), cellLevelBox);

    // Create spin box for feature angle resolution
    featureAngleBox = new QDoubleSpinBox(this);
    featureAngleBox->setRange(0.0, 100.0);
    featureAngleBox->setSingleStep(1.0);
    featureAngleBox->setValue(30.0);
    featureAngleBox->setSuffix(tr(" °"));
    transitionLayout->addRow(tr("Resolve feature angle:"), featureAngleBox);

    // Set group box for surface refinement
    QGroupBox* surfaceRefinementBox = new QGroupBox(tr("Surface Refinement Levels"), this);
    layout->addWidget(surfaceRefinementBox);
    QVBoxLayout* surfaceRefinementLayout = new QVBoxLayout(surfaceRefinementBox);

    // Create label and table for mesh refinement
    surfaceRefinementLayout->addWidget(new QLabel(tr("Set mesh refinement levels for each geometry file")));
    meshRefinementTable = new QTableWidget(surfaceRefinementBox);
    meshRefinementTable->setColumnCount(4);
    QStringList headers = { tr("Geometry File"), tr("Min Level"), tr("Max Level"), tr("Edge Level") };
    meshRefinementTable->setHorizontalHeaderLabels(headers);
    meshRefinementTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    meshRefinementTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    meshRefinementTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    meshRefinementTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    meshRefinementTable->verticalHeader()->setVisible(false);
    surfaceRefinementLayout->addWidget(meshRefinementTable);
}

void CastellationPage::initializePage() {

    // Access the mesh wizard
    meshWizard = qobject_cast<MeshWizard*>(wizard());
    if (!meshWizard) {
        qWarning() << "Failed to cast to MeshWizard!";
        return;
    }
    m_cfg = &(meshWizard->getCastellatedMeshConfig());

    // Update widgets
    updateGlobalSettings();
    updateMeshTransition();
    updateSurfaceRefinement();
}

// Update widgets in the Global Constraints group
void CastellationPage::updateGlobalSettings() {

    const auto& geometryMap = meshWizard->getGeometryMap();

    // Update the combo box
    meshRegionBox->blockSignals(true);
    meshRegionBox->clear();
    meshRegionBox->addItem(tr("External flow"), QVariant::fromValue(computeExternalPoint()));
    for (const auto& [file, geometryData] : geometryMap.asKeyValueRange()) {
        meshRegionBox->addItem(tr("Inside %1").arg(file),
                               QVariant::fromValue(geometryData.intpoint));
    }
    meshRegionBox->addItem(tr("Custom coordinates"));

    // Check parsed data
    if (!std::isnan(m_cfg->locationInMesh[0])) {
        meshRegionBox->setCurrentText(tr("Custom coordinates"));
        for(int i = 0; i < 3; i++) {
            loc[i]->blockSignals(true);
            loc[i]->setValue(m_cfg->locationInMesh[i]);
            loc[i]->blockSignals(false);
        }
    } else {
        meshRegionBox->setCurrentIndex(0);
    }

    // Allow combo box events
    meshRegionBox->blockSignals(false);
    onMeshRegionChanged();

    // Update the other fields
    maxGlobalCellBox->setValue(m_cfg->maxGlobalCells);
    maxLocalCellBox->setValue(m_cfg->maxLocalCells);
    freeStandingZoneBox->setChecked(m_cfg->allowFreeStandingZoneFaces);
}

// Update widgets in the Mesh Transition group
void CastellationPage::updateMeshTransition() {

    // Update layer thickness between refinement levels
    cellLevelBox->setValue(m_cfg->nCellsBetweenLevels);

    // Update resolve feature angle
    featureAngleBox->setValue(m_cfg->resolveFeatureAngle);
}

void CastellationPage::updateSurfaceRefinement() {

    // Get data from Geometry Page
    QStringList geometryFiles = meshWizard->getGeometryMap().keys();

    // Safely destroy all existing rows and cell widgets
    meshRefinementTable->setRowCount(0);
    meshRefinementTable->setRowCount(geometryFiles.size());

    // Update the table
    for (int i = 0; i < geometryFiles.size(); ++i) {
        QString file(geometryFiles[i]);

        // Column 0: Filename (Read-only)
        QTableWidgetItem* fileItem = new QTableWidgetItem(file);
        fileItem->setFlags(fileItem->flags() & ~Qt::ItemIsEditable);
        meshRefinementTable->setItem(i, 0, fileItem);

        // Column 1: Min Level SpinBox
        QSpinBox* minBox = new QSpinBox(meshRefinementTable);
        minBox->setRange(0, 10);
        meshRefinementTable->setCellWidget(i, 1, minBox);
        minBox->setValue(m_cfg->refinements[file].surfaceMin);

        // Column 2: Max Level SpinBox
        QSpinBox* maxBox = new QSpinBox(meshRefinementTable);
        maxBox->setRange(0, 10);
        meshRefinementTable->setCellWidget(i, 2, maxBox);
        maxBox->setValue(m_cfg->refinements[file].surfaceMax);

        // Column 3: Refinement level for edges
        QSpinBox* edgeLevelBox = new QSpinBox(meshRefinementTable);
        edgeLevelBox->setRange(0, 10);
        meshRefinementTable->setCellWidget(i, 3, edgeLevelBox);
        edgeLevelBox->setValue(m_cfg->refinements[file].edgeLevel);

        // Link the Min and Max boxes dynamically
        connect(minBox, QOverload<int>::of(&QSpinBox::valueChanged), maxBox, [maxBox](int value){
            if (maxBox->value() < value) {
                maxBox->setValue(value);
            }
            maxBox->setMinimum(value);
        });

        // Link Max box to Edge Level box dynamically
        connect(maxBox, QOverload<int>::of(&QSpinBox::valueChanged), edgeLevelBox, [edgeLevelBox](int value){
            if (edgeLevelBox->value() < value) {
                edgeLevelBox->setValue(value);
            }
        });
    }
}

void CastellationPage::onMeshRegionChanged() {

    int index = meshRegionBox->currentIndex();
    QString regionText = meshRegionBox->itemText(index);

    // If the user selected Custom, do nothing and let them type.
    if (regionText == tr("Custom coordinates")) {
        return;
    }

    // Retrieve the pre-calculated point invisibly attached to the dropdown item
    QVariant data = meshRegionBox->itemData(index);
    if (!data.isValid()) {
        qWarning() << "Warning: No point data attached to combo box item.";
        return;
    }
    QVector3D targetPoint = data.value<QVector3D>();

    // Block signals to prevent triggering the "Custom" auto-switch
    for(int i = 0; i < 3; i++) {
        loc[i]->blockSignals(true);
    }

    // Update the UI
    loc[0]->setValue(targetPoint.x());
    loc[1]->setValue(targetPoint.y());
    loc[2]->setValue(targetPoint.z());

    // Re-enable signals
    for(int i = 0; i < 3; i++) {
        loc[i]->blockSignals(false);
    }
}

void CastellationPage::onMeshLocationChanged() {

    // Find the exact index of the custom option (safely handles translation)
    int customIdx = meshRegionBox->findText(tr("Custom coordinates"));

    // Shouldn't trigger if "Custom coordinates" is set
    if (customIdx != -1 && meshRegionBox->currentIndex() != customIdx) {
        meshRegionBox->blockSignals(true);
        meshRegionBox->setCurrentIndex(customIdx);
        meshRegionBox->blockSignals(false);
    }
}

QVector3D CastellationPage::computeExternalPoint() {
    const auto& points = meshWizard->getBlockMeshConfig().vertices;

    if (points.empty()) {
        qWarning() << "Warning: blockMesh vertices are empty.";
        return QVector3D(0.0, 0.0, 0.0);
    }

    double xMin = std::numeric_limits<double>::max();
    double xMax = std::numeric_limits<double>::lowest();
    double yMin = std::numeric_limits<double>::max();
    double yMax = std::numeric_limits<double>::lowest();
    double zMin = std::numeric_limits<double>::max();
    double zMax = std::numeric_limits<double>::lowest();

    for (const auto& pt : points) {
        xMin = std::min(xMin, pt[0]);
        xMax = std::max(xMax, pt[0]);
        yMin = std::min(yMin, pt[1]);
        yMax = std::max(yMax, pt[1]);
        zMin = std::min(zMin, pt[2]);
        zMax = std::max(zMax, pt[2]);
    }

    double offset = 0.02;
    return QVector3D(
        xMax - ((xMax - xMin) * offset),
        yMax - ((yMax - yMin) * offset),
        zMax - ((zMax - zMin) * offset)
        );
}

bool CastellationPage::validatePage() {
    if (!m_cfg) return false;

    // 1. Save Global Settings
    if (meshRegionBox->currentText() == tr("Custom coordinates")) {
        m_cfg->locationInMesh = {loc[0]->value(), loc[1]->value(), loc[2]->value()};
    } else {
        // Extract the QVector3D hidden in the current combo box item
        QVariant data = meshRegionBox->currentData();
        if (data.isValid()) {
            QVector3D pt = data.value<QVector3D>();
            m_cfg->locationInMesh = {pt.x(), pt.y(), pt.z()};
        }
    }

    m_cfg->maxGlobalCells = maxGlobalCellBox->value();
    m_cfg->maxLocalCells = maxLocalCellBox->value();
    m_cfg->allowFreeStandingZoneFaces = freeStandingZoneBox->isChecked();

    // 2. Save Mesh Transition Settings
    m_cfg->nCellsBetweenLevels = cellLevelBox->value();
    m_cfg->resolveFeatureAngle = featureAngleBox->value();

    // 3. Save Surface Refinement Table
    for (int i = 0; i < meshRefinementTable->rowCount(); ++i) {
        // The file name is in column 0
        QTableWidgetItem* item = meshRefinementTable->item(i, 0);
        if (!item) continue;
        QString fileName = item->text();

        // Safely cast the cell widgets back to QSpinBoxes
        QSpinBox* minBox = qobject_cast<QSpinBox*>(meshRefinementTable->cellWidget(i, 1));
        QSpinBox* maxBox = qobject_cast<QSpinBox*>(meshRefinementTable->cellWidget(i, 2));
        QSpinBox* edgeBox = qobject_cast<QSpinBox*>(meshRefinementTable->cellWidget(i, 3));

        if (minBox && maxBox && edgeBox) {
            m_cfg->refinements[fileName].surfaceMin = minBox->value();
            m_cfg->refinements[fileName].surfaceMax = maxBox->value();
            m_cfg->refinements[fileName].edgeLevel = edgeBox->value();
        }
    }
    return true;
}

int CastellationPage::nextId() const {
    if (meshWizard->m_runSnap) {
        return Page_SnapControl;
    } else if (meshWizard->m_runLayers) {
        return Page_LayerControl;
    }
    return Page_Execution;
}