#include "page_40_surface_extraction.h"
#include "page_10_geometry.h"

#include "wizard_mesh.h"

// Introduction page asks for the case name and platform
SurfaceExtractionPage::SurfaceExtractionPage(QWidget *parent): QWizardPage(parent) {

    // Inside your ExtractPage constructor
    setTitle(tr("Feature Edge Extraction"));
    setSubTitle(tr("Configure how sharp corners are detected on the surface geometry."));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Create table to set surface extraction features
    featureTable = new QTableWidget(this);
    featureTable->setColumnCount(5);
    featureTable->setHorizontalHeaderLabels({"File Name", "Included Angle", "Open Edges", "Write OBJ File", "Edge Level"});
    featureTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    featureTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    featureTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    featureTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    featureTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    featureTable->verticalHeader()->setVisible(false);
    featureTable->setSelectionMode(QAbstractItemView::NoSelection);
    mainLayout->addWidget(featureTable);

    mainLayout->addStretch();

    // Set the page layout
    setLayout(mainLayout);
}

void SurfaceExtractionPage::initializePage() {
    meshWizard = qobject_cast<MeshWizard*>(wizard());
    if (!meshWizard) {
        qWarning() << "Failed to cast to MeshWizard!";
        return;
    }

    // Clear any existing data to prevent stacking issues on Back/Next navigation
    featureTable->setRowCount(0);

    // Get data from Geometry Page
    GeometryPage* geometryPage = qobject_cast<GeometryPage*>(wizard()->page(0));
    QStringList geometryFiles = geometryPage->getGeometryFiles();
    featureTable->setRowCount(geometryFiles.size());

    // List selected files
    for (int i = 0; i < geometryFiles.size(); ++i) {
        QString fileName = geometryFiles[i];

        SurfaceFeatureExtractEntry entry;
        auto it = meshWizard->getFeatureMap().find(fileName);
        if (it != meshWizard->getFeatureMap().end()) {
            entry = it->second;
        }

        // --- Column 0: File Name ---
        QTableWidgetItem* fileItem = new QTableWidgetItem(fileName);
        fileItem->setFlags(fileItem->flags() & ~Qt::ItemIsEditable);
        featureTable->setItem(i, 0, fileItem);

        // --- Column 1: Included Angle ---
        QDoubleSpinBox* angleSpin = new QDoubleSpinBox(this);
        angleSpin->setRange(0.0, 180.0);
        angleSpin->setDecimals(1);
        angleSpin->setSuffix(" °");

        // Ensure a safe fallback if entry is uninitialized
        double safeAngle = (entry.includedAngle > 0.0) ? entry.includedAngle : 150.0;
        angleSpin->setValue(safeAngle);
        featureTable->setCellWidget(i, 1, angleSpin);

        // --- Column 2: Open Edges ---
        QCheckBox* openEdgesCheck = new QCheckBox(this);
        openEdgesCheck->setChecked(entry.openEdges);
        featureTable->setCellWidget(i, 2, centerCheckBox(openEdgesCheck));

        // --- Column 3: Write OBJ ---
        QCheckBox* writeObjCheck = new QCheckBox(this);
        writeObjCheck->setChecked(entry.writeObj);
        featureTable->setCellWidget(i, 3, centerCheckBox(writeObjCheck));

        // --- Column 4: Edge Level ---
        QSpinBox* edgeLevelSpin = new QSpinBox(this);
        edgeLevelSpin->setRange(0, 8);
        edgeLevelSpin->setValue(3);
        featureTable->setCellWidget(i, 4, edgeLevelSpin);
    }

    // Set the table height
    int height = featureTable->horizontalHeader()->height() + featureTable->frameWidth() * 2;
    for (int row = 0; row < featureTable->rowCount(); ++row) {
        height += featureTable->rowHeight(row);
    }
    featureTable->setFixedHeight(height);
}
// Center checkboxes visually in a table cell
QWidget* SurfaceExtractionPage::centerCheckBox(QCheckBox* box) {
    QWidget* widget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->addWidget(box);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    return widget;
}

bool SurfaceExtractionPage::validatePage() {

    // Clear the existing map to prevent stale data
    meshWizard->getFeatureMap().clear();
    for (int i = 0; i < featureTable->rowCount(); ++i) {

        // Read the filename
        QString fileName = featureTable->item(i, 0)->text();

        // Access the table's widgets
        auto* angleSpin = qobject_cast<QDoubleSpinBox*>(featureTable->cellWidget(i, 1));
        auto* openEdgesWidget = featureTable->cellWidget(i, 2);
        auto* openEdgesCheck = openEdgesWidget->findChild<QCheckBox*>();
        auto* writeObjWidget = featureTable->cellWidget(i, 3);
        auto* writeObjCheck = writeObjWidget->findChild<QCheckBox*>();
        auto* edgeLevelSpin = qobject_cast<QSpinBox*>(featureTable->cellWidget(i, 4));

        // Build the struct and push to the map
        if (angleSpin && openEdgesCheck && writeObjCheck && edgeLevelSpin) {

            SurfaceFeatureExtractEntry newEntry;
            newEntry.includedAngle = angleSpin->value();
            newEntry.openEdges = openEdgesCheck->isChecked();
            newEntry.writeObj = writeObjCheck->isChecked();
            newEntry.edgeLevel = edgeLevelSpin->value();
            meshWizard->getFeatureMap()[fileName] = newEntry;
        }
    }
    return true;
}

int SurfaceExtractionPage::nextId() const {
    if (meshWizard->m_runCastellated) {
        return Page_Castellation;
    } else if (meshWizard->m_runSnap) {
        return Page_SnapControl;
    } else if (meshWizard->m_runLayers) {
        return Page_LayerControl;
    }
    return -1;
}