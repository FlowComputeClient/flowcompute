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
    featureTable->setHorizontalHeaderLabels({"File Name", "Included Angle", "Non-manifold Edges", "Open Edges", "Write OBJ File"});
    featureTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    featureTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    featureTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    featureTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    featureTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    featureTable->verticalHeader()->setVisible(false);
    featureTable->setSelectionMode(QAbstractItemView::NoSelection);
    mainLayout->addWidget(featureTable);

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
    featureTable->clearContents();

    QStringList geometryFiles = meshWizard->getGeometryMap().keys();
    featureTable->setRowCount(geometryFiles.size());

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
        QDoubleSpinBox* angleBox = new QDoubleSpinBox(this);
        angleBox->setRange(0.0, 180.0);
        angleBox->setDecimals(1);
        angleBox->setSuffix(" °");
        angleBox->setValue(entry.includedAngle);
        featureTable->setCellWidget(i, 1, angleBox);

        // --- Column 2: Non-Manifold Edges ---
        QCheckBox* nonManifoldBox = new QCheckBox(this);
        nonManifoldBox->setChecked(entry.nonManifoldEdges);
        featureTable->setCellWidget(i, 2, centerCheckBox(nonManifoldBox));

        // --- Column 3: Open Edges ---
        QCheckBox* openEdgesBox = new QCheckBox(this);
        openEdgesBox->setChecked(entry.openEdges);
        featureTable->setCellWidget(i, 3, centerCheckBox(openEdgesBox));

        // --- Column 4: Write OBJ ---
        QCheckBox* writeObjBox = new QCheckBox(this);
        writeObjBox->setChecked(entry.writeObj);
        featureTable->setCellWidget(i, 4, centerCheckBox(writeObjBox));
    }

    // 2. Set a sensible minimum height so it doesn't look crushed with 1 file,
    // but allow the layout to handle scrolling if there are 50 files.
    int minHeight = featureTable->horizontalHeader()->height() + (35 * std::max(1, (int)geometryFiles.size()));
    featureTable->setMinimumHeight(std::min(minHeight, 300)); // Caps the minimum expansion at 300px
}
// Center checkboxes visually in a table cell
QWidget* SurfaceExtractionPage::centerCheckBox(QCheckBox* box) {
    QWidget* widget = new QWidget(this);
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

        // Extract the widgets safely
        auto* angleBox = qobject_cast<QDoubleSpinBox*>(featureTable->cellWidget(i, 1));

        // To get the checkbox, we have to unpack the centering QWidget
        auto* nonManifoldWidget = featureTable->cellWidget(i, 2);
        auto* nonManifoldBox = nonManifoldWidget->findChild<QCheckBox*>();

        auto* openEdgesWidget = featureTable->cellWidget(i, 3);
        auto* openEdgesBox = openEdgesWidget->findChild<QCheckBox*>();

        auto* writeObjWidget = featureTable->cellWidget(i, 4);
        auto* writeObjBox = writeObjWidget->findChild<QCheckBox*>();

        // Build the struct and push to the map
        if (angleBox && nonManifoldBox && openEdgesBox && writeObjBox) {
            SurfaceFeatureExtractEntry newEntry;
            newEntry.includedAngle = angleBox->value();
            newEntry.nonManifoldEdges = nonManifoldBox->isChecked();
            newEntry.openEdges = openEdgesBox->isChecked();
            newEntry.writeObj = writeObjBox->isChecked();
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
    return Page_Execution;
}