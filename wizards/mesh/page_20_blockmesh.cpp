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

#include "page_20_blockmesh.h"

#include <QDir>
#include <QPushButton>

#include "page_10_geometry.h"
#include "geometry/stl/stl_reader.h"
#include "wizard_mesh.h"

// Introduction page asks for the case name and platform
BlockMeshPage1::BlockMeshPage1(const SystemManager& systemMgr,
    QWidget *parent): m_systemMgr(systemMgr), QWizardPage(parent) {
    // Set title and style
    setTitle(tr("BlockMesh Configuration: Define Mesh Domain"));

    // Create a grid layout with two columns
    QFormLayout* layout = new QFormLayout(this);
    layout->setSpacing(20);

    // Configure spin boxes
    for(int i = 0; i < 6; i++) {
        dimSpin[i] = new QDoubleSpinBox(this);
        dimSpin[i]->setRange(-1e9, 1e9);
        dimSpin[i]->setDecimals(6);
        dimSpin[i]->setSingleStep(0.1);
        connect(dimSpin[i], &QDoubleSpinBox::editingFinished, this,
                &BlockMeshPage1::updateCellCount);
    }

    // Set scale factor
    m_scaleFactorCombo = new QComboBox(this);
    m_scaleFactorCombo->setEditable(true);
    m_scaleFactorCombo->addItem(tr("1 meter (1.0)"), 1.0);
    m_scaleFactorCombo->addItem(tr("10 centimeters (0.1)"), 0.1);
    m_scaleFactorCombo->addItem(tr("1 centimeter (0.01)"), 0.01);
    m_scaleFactorCombo->addItem(tr("1 millimeter (0.001)"), 0.001);
    m_scaleFactorCombo->addItem(tr("1 inch (0.0254)"), 0.0254);
    layout->addRow(tr("Geometry scale factor:"), m_scaleFactorCombo);
    connect(m_scaleFactorCombo, &QComboBox::currentTextChanged, this,
            &BlockMeshPage1::onScaleFactorChanged);

    // Set group box for domain bounds
    QGroupBox* domainBox = new QGroupBox(tr("Domain Bounds"), this);
    layout->addRow(domainBox);
    QFormLayout* domainLayout = new QFormLayout(domainBox);

    // Add widgets to domain group box
    geometryLabel = new QLabel("");
    domainLayout->addRow(geometryLabel);

    // Add spin boxes for dimensions
    QStringList dims = { "x", "y", "z" };
    for(int i = 0; i < 3; i++) {
        QHBoxLayout* boundsLayout = new QHBoxLayout;
        boundsLayout->setContentsMargins(0, 0, 0, 0);
        boundsLayout->addWidget(dimSpin[2*i]);
        QLabel* separator = new QLabel(tr(" to "), this);
        separator->setAlignment(Qt::AlignCenter);
        boundsLayout->addWidget(separator);
        boundsLayout->addWidget(dimSpin[2*i+1]);
        domainLayout->addRow(tr("Domain bounds (%1-direction):")
                                 .arg(dims[i]), boundsLayout);
    }

    // Add label and push button
    QPushButton *fitBoundsButton = new QPushButton("Fit Bounds");
    domainLayout->addRow(tr("Auto-fit to geometry content:"), fitBoundsButton);
    connect(fitBoundsButton, &QPushButton::clicked, this,
            &BlockMeshPage1::fitBoundsPressed);

    // Group box for cell sizes
    QGroupBox* cellBox = new QGroupBox(tr("Cell Resolution"), this);
    layout->addRow(cellBox);
    QFormLayout* cellLayout = new QFormLayout(cellBox);

    // Target cell size
    targetCellSizeSpin = new QDoubleSpinBox(cellBox);
    targetCellSizeSpin->setDecimals(6);
    targetCellSizeSpin->setMinimum(0.0001);
    targetCellSizeSpin->setValue(0.1);
    cellLayout->addRow(tr("Estimated cell dimension:"), targetCellSizeSpin);
    connect(targetCellSizeSpin, &QDoubleSpinBox::editingFinished,
            this, &BlockMeshPage1::updateCellCount);

    // Display actual cell sizes
    QHBoxLayout* sizesLayout = new QHBoxLayout;
    sizesLayout->setContentsMargins(0, 0, 0, 0);
    for(int i=0; i<3; i++) {
        actualSizeEdits[i] = new QLineEdit();
        actualSizeEdits[i]->setReadOnly(true);
        sizesLayout->addWidget(actualSizeEdits[i]);
    }
    cellLayout->addRow(tr("Actual cell sizes:"), sizesLayout);

    // Display cell counts
    QHBoxLayout* numCellsLayout = new QHBoxLayout;
    numCellsLayout->setContentsMargins(0, 0, 0, 0);
    for(int i=0; i<3; i++) {
        cellCountEdits[i] = new QLineEdit();
        cellCountEdits[i]->setReadOnly(true);
        numCellsLayout->addWidget(cellCountEdits[i]);
    }
    cellLayout->addRow(tr("Number of cells (Nx, Ny, Nz):"), numCellsLayout);

    // Display total cell count
    cellCountTotalEdit = new QLineEdit();
    cellCountTotalEdit->setReadOnly(true);
    cellLayout->addRow(tr("Estimated cell count:"), cellCountTotalEdit);

    // Display maximum aspect ratio
    maxAspectRatioEdit = new QLineEdit();
    maxAspectRatioEdit->setReadOnly(true);
    cellLayout->addRow(tr("Maximum aspect ratio:"), maxAspectRatioEdit);

    // Set the page layout
    setLayout(layout);
}

void BlockMeshPage1::initializePage() {

    // Access block mesh configuration structure
    meshWizard = qobject_cast<MeshWizard*>(wizard());
    if (!meshWizard) {
        qWarning() << "Failed to cast to MeshWizard!";
        return;
    }
    m_cfg = &(meshWizard->getBlockMeshConfig());

    {
        const QSignalBlocker blocker(m_scaleFactorCombo);

        // Set scale factor from file
        double scaleValue = m_cfg->convertToMeters;
        m_previousScaleFactor = scaleValue;
        int index = m_scaleFactorCombo->findData(scaleValue);
        if (index != -1) {
            m_scaleFactorCombo->setCurrentIndex(index);
        } else {
            m_scaleFactorCombo->setCurrentText(
                QString::number(scaleValue, 'f', 4));
        }
    }

    // Set bounding box values
    setBoundingBox();
}

void BlockMeshPage1::setBoundingBox() {

    // Get data from Geometry Page
    GeometryPage* geometryPage = qobject_cast<GeometryPage*>(wizard()->page(0));
    QString caseName = geometryPage->getCaseName();
    QStringList geometryFiles = geometryPage->getGeometryFiles();

    // Get bounding boxes of geometry files
    QByteArray fileData;
    QVector<BoundingBox> boxes;
    CaseData caseData = m_systemMgr.getData(caseName);
    QString openFoamPath = caseData.openFoamPath;

    // Determine which OpenFOAM release is used
    QString dirName = QDir(openFoamPath).dirName();
    const QRegularExpression foundationRegex("^openfoam\\d{2}$",
        QRegularExpression::CaseInsensitiveOption);
    bool isFoundation = foundationRegex.match(dirName).hasMatch();

    // Form path to geometry directory
    QString subDir =
        (isFoundation) ? "/constant/geometry" : "/constant/triSurface";
    QString fullPath = caseData.casePath + "/" + caseName + subDir;

    // Iterate through geometry files
    meshWizard->getGeometryMap().clear();
    GeometryMetrics metrics;
    for (auto const& file: std::as_const(geometryFiles)) {
        fileData = m_systemMgr.getSystem(caseName)->getFileContent(
            fullPath + "/" + file);
        if(file.endsWith(".stl")) {
            metrics = StlReader::readMetrics(fileData);
            boxes.emplaceBack(metrics.bbox);
            meshWizard->getGeometryMap().insert(file, metrics);
        }
    }

    // Combine bounding boxes
    BoundingBox geomBox = boxes.first();
    for (int i = 1; i < boxes.size(); ++i) {
        const BoundingBox& currentBox = boxes[i];
        geomBox.min.setX(std::min(geomBox.min.x(), currentBox.min.x()));
        geomBox.min.setY(std::min(geomBox.min.y(), currentBox.min.y()));
        geomBox.min.setZ(std::min(geomBox.min.z(), currentBox.min.z()));
        geomBox.max.setX(std::max(geomBox.max.x(), currentBox.max.x()));
        geomBox.max.setY(std::max(geomBox.max.y(), currentBox.max.y()));
        geomBox.max.setZ(std::max(geomBox.max.z(), currentBox.max.z()));
    }

    // Store the bounding box
    m_rawGeomBox = geomBox;

    // Compute default cell size
    double diffX = m_rawGeomBox.max.x() - m_rawGeomBox.min.x();
    double diffY = m_rawGeomBox.max.y() - m_rawGeomBox.min.y();
    double diffZ = m_rawGeomBox.max.z() - m_rawGeomBox.min.z();
    double maxDim = std::max({diffX, diffY, diffZ});
    double defaultSize = maxDim / 20.0;

    // Set default cell size
    targetCellSizeSpin->blockSignals(true);
    targetCellSizeSpin->setValue(defaultSize);
    targetCellSizeSpin->blockSignals(false);

    // Run scale update
    onScaleFactorChanged(m_scaleFactorCombo->currentText());
}

void BlockMeshPage1::updateCellCount() {

    std::array<double, 3> diff = {
        dimSpin[1]->value() - dimSpin[0]->value(),
        dimSpin[3]->value() - dimSpin[2]->value(),
        dimSpin[5]->value() - dimSpin[4]->value() };

    if (diff[0] <= 0 || diff[1] <= 0 || diff[2] <= 0) return;

    // Compute cell counts
    std::array<int, 3> cellCounts;
    double targetSize = targetCellSizeSpin->value();
    for(int i=0; i<3; i++) {
        cellCounts[i] =
            std::max(1, static_cast<int>(std::round(diff[i] / targetSize)));
        cellCountEdits[i]->setText(QString::number(cellCounts[i]));
    }

    // Compute actual cell sizes
    std::array<double, 3> actualSizes;
    for(int i=0; i<3; i++) {
        actualSizes[i] = diff[i]/cellCounts[i];
        actualSizeEdits[i]->setText(QString::number(actualSizes[i]));
    }

    // Update cell count total
    long long totalCells =
        static_cast<long long>(cellCounts[0]) * cellCounts[1] * cellCounts[2];
    cellCountTotalEdit->setText(QLocale().toString(totalCells));

    // Update aspect ratio
    double maxDim = *std::max_element(actualSizes.begin(), actualSizes.end());
    double minDim = *std::min_element(actualSizes.begin(), actualSizes.end());
    double aspectRatio = 1.0;
    if (minDim > 0.0) {
        aspectRatio = maxDim / minDim;
    }
    maxAspectRatioEdit->setText(QString::number(aspectRatio, 'f', 3));
}

bool BlockMeshPage1::validatePage() {

    // Validate domain bounds
    for (int i = 0; i < 3; ++i) {
        double minVal = dimSpin[2 * i]->value();
        double maxVal = dimSpin[2 * i + 1]->value();

        if (maxVal <= minVal) {
            QMessageBox::warning(this, tr("Invalid Bounds"),
                tr("Maximum bounds must be greater than minimum bounds."));
            return false;
        }
    }

    // Write scale factor to struct
    m_cfg->convertToMeters =
        getCurrentScaleFactor(m_scaleFactorCombo->currentText());

    // Write cell counts to struct
    m_cfg->nX = cellCountEdits[0]->text().toInt();
    m_cfg->nY = cellCountEdits[1]->text().toInt();
    m_cfg->nZ = cellCountEdits[2]->text().toInt();

    // Write vertices to struct
    double minX = dimSpin[0]->value(), maxX = dimSpin[1]->value();
    double minY = dimSpin[2]->value(), maxY = dimSpin[3]->value();
    double minZ = dimSpin[4]->value(), maxZ = dimSpin[5]->value();
    m_cfg->vertices = {
        {minX, minY, minZ},
        {maxX, minY, minZ},
        {maxX, maxY, minZ},
        {minX, maxY, minZ},
        {minX, minY, maxZ},
        {maxX, minY, maxZ},
        {maxX, maxY, maxZ},
        {minX, maxY, maxZ}
    };
    return true;
}

void BlockMeshPage1::onScaleFactorChanged(const QString& text) {

    // Pass the text directly instead of relying on the GUI state
    double scale = getCurrentScaleFactor(text);

    // Abort if the user is in an intermediate typing state (like "0.")
    if (scale <= 0.0) return;

    double scaleRatio = m_previousScaleFactor / scale;

    // Read directly from the widget instead of a detached member variable
    double currentCellSize = targetCellSizeSpin->value();

    if (currentCellSize > 0.0) {
        currentCellSize *= scaleRatio;
        targetCellSizeSpin->blockSignals(true);
        targetCellSizeSpin->setValue(currentCellSize);
        targetCellSizeSpin->blockSignals(false);
    }

    m_previousScaleFactor = scale;

    // Compute new geometry bounds
    double xMin = m_rawGeomBox.min.x() / scale;
    double yMin = m_rawGeomBox.min.y() / scale;
    double zMin = m_rawGeomBox.min.z() / scale;
    double xMax = m_rawGeomBox.max.x() / scale;
    double yMax = m_rawGeomBox.max.y() / scale;
    double zMax = m_rawGeomBox.max.z() / scale;

    // Update the geometry label
    geometryLabel->setText(
        tr("Geometry bounds: (%1, %2, %3) to (%4, %5, %6)")
            .arg(xMin).arg(yMin).arg(zMin).arg(xMax).arg(yMax).arg(zMax));

    // Re-apply the 5% padding to the scaled bounds
    double padX = std::max((xMax - xMin) * 0.05, 0.001);
    double padY = std::max((yMax - yMin) * 0.05, 0.001);
    double padZ = std::max((zMax - zMin) * 0.05, 0.001);
    xMin -= padX; xMax += padX;
    yMin -= padY; yMax += padY;
    zMin -= padZ; zMax += padZ;

    // Update the cached arrays for the "Fit Bounds" button
    minGeometry[0] = xMin; minGeometry[1] = yMin; minGeometry[2] = zMin;
    maxGeometry[0] = xMax; maxGeometry[1] = yMax; maxGeometry[2] = zMax;

    // Consolidated blocker
    for (int i = 0; i < 6; ++i) {
        dimSpin[i]->blockSignals(true);
    }
    dimSpin[0]->setValue(xMin);
    dimSpin[1]->setValue(xMax);
    dimSpin[2]->setValue(yMin);
    dimSpin[3]->setValue(yMax);
    dimSpin[4]->setValue(zMin);
    dimSpin[5]->setValue(zMax);
    for (int i = 0; i < 6; ++i) {
        dimSpin[i]->blockSignals(false);
    }

    updateCellCount();
}

void BlockMeshPage1::fitBoundsPressed() {
    dimSpin[0]->setValue(minGeometry[0]);
    dimSpin[1]->setValue(maxGeometry[0]);
    dimSpin[2]->setValue(minGeometry[1]);
    dimSpin[3]->setValue(maxGeometry[1]);
    dimSpin[4]->setValue(minGeometry[2]);
    dimSpin[5]->setValue(maxGeometry[2]);
    updateCellCount();
}

double BlockMeshPage1::getCurrentScaleFactor(const QString& text) const {
    // 1. Try to find a perfectly matching dropdown item first
    int index = m_scaleFactorCombo->findText(text);
    if (index != -1) {
        return m_scaleFactorCombo->itemData(index).toDouble();
    }

    // 2. Otherwise, attempt to parse it as custom typed input
    bool ok;
    double customScale = text.toDouble(&ok);

    // Return -1.0 for invalid text to prevent premature/broken scaling
    return (ok && customScale > 0.0) ? customScale : -1.0;
}
