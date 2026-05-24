#include "page_20_blockmesh.h"
#include "page_10_geometry.h"

#include "../../main_window.h"
#include "wizard_mesh.h"

// Introduction page asks for the case name and platform
BlockMeshPage1::BlockMeshPage1(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Define Mesh Domain"));

    // Create a grid layout with two columns
    QFormLayout* layout = new QFormLayout(this);
    layout->setSpacing(20);

    // Configure spin boxes
    for(int i = 0; i < 6; i++) {
        dimSpinBox[i] = new QDoubleSpinBox(this);
        dimSpinBox[i]->setRange(-1e9, 1e9);
        dimSpinBox[i]->setDecimals(4);
        connect(dimSpinBox[i], &QDoubleSpinBox::editingFinished, this, &BlockMeshPage1::updateCellCount);
    }

    // Set scale factor
    scaleFactorBox = new QComboBox(this);
    scaleFactorBox->addItem(tr("1 meter (1.0)"), 1.0);
    scaleFactorBox->addItem(tr("10 centimeters (0.1)"), 0.1);
    scaleFactorBox->addItem(tr("1 centimeter (0.01)"), 0.01);
    scaleFactorBox->addItem(tr("1 millimeter (0.001)"), 0.001);
    scaleFactorBox->addItem(tr("1 inch (0.0254)"), 0.0254);
    layout->addRow(tr("Geometry scale factor:"), scaleFactorBox);

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
        boundsLayout->addWidget(dimSpinBox[2*i]);
        QLabel* separator = new QLabel(tr(" to "), this);
        separator->setAlignment(Qt::AlignCenter);
        boundsLayout->addWidget(separator);
        boundsLayout->addWidget(dimSpinBox[2*i+1]);
        domainLayout->addRow(tr("Domain bounds (%1-direction):").arg(dims[i]), boundsLayout);
    }

    // Add label and push button
    QPushButton *fitBoundsButton = new QPushButton("Fit Bounds");
    domainLayout->addRow(tr("Auto-fit to geometry content:"), fitBoundsButton);
    connect(fitBoundsButton, &QPushButton::clicked, this, &BlockMeshPage1::fitBoundsPressed);

    // Group box for cell sizes
    QGroupBox* cellBox = new QGroupBox(tr("Cell Resolution"), this);
    layout->addRow(cellBox);
    QFormLayout* cellLayout = new QFormLayout(cellBox);

    // Absolute cell size
    targetCellSizeBox = new QDoubleSpinBox(cellBox);
    targetCellSizeBox->setDecimals(4);
    targetCellSizeBox->setMinimum(0.0001);
    targetCellSizeBox->setSuffix(" m");
    targetCellSizeBox->setValue(0.1);
    cellLayout->addRow(tr("Target cell dimension:"), targetCellSizeBox);
    connect(targetCellSizeBox, &QDoubleSpinBox::editingFinished, this, &BlockMeshPage1::updateCellCount);

    // A helper lambda to quickly create standard read-only line edits
    auto createReadOnlyBox = [this](QFormLayout* layout, const QString& labelText) -> QLineEdit* {
        QLineEdit* lineEdit = new QLineEdit(this);
        lineEdit->setReadOnly(true);
        lineEdit->setStyleSheet("QLineEdit { background-color: #f0f0f0; color: #333333; }");
        layout->addRow(labelText, lineEdit);
        return lineEdit;
    };

    // Display cell counts
    cellCountX     = createReadOnlyBox(cellLayout, tr("Cells (x-dimension):"));
    cellCountY     = createReadOnlyBox(cellLayout, tr("Cells (y-dimension):"));
    cellCountZ     = createReadOnlyBox(cellLayout, tr("Cells (z-dimension):"));
    cellCountTotal = createReadOnlyBox(cellLayout, tr("Estimated Total Base Cells:"));

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

    // Set scale factor from file
    double scaleValue = m_cfg->convertToMeters;
    int index = scaleFactorBox->findData(scaleValue);
    if (index != -1) {
        scaleFactorBox->setCurrentIndex(index);
    }

    // Set bounding box values
    setBoundingBox();

    // Set cell counts
    updateCellCount();
}

void BlockMeshPage1::setBoundingBox() {

    // Get data from Geometry Page
    GeometryPage* geometryPage = qobject_cast<GeometryPage*>(wizard()->page(0));
    QString caseName = geometryPage->getCaseName();
    QStringList geometryFiles = geometryPage->getGeometryFiles();

    // Get bounding boxes of geometry files
    QByteArray fileData;
    QVector<BoundingBox> boxes;
    MainWindow* mainWin = qobject_cast<MainWindow*>(this->wizard()->parentWidget());
    CaseData caseData = mainWin->caseMap[caseName];
    int targetSystemId = mainWin->caseMap[caseName].targetSystemId;
    QString fullPath = caseData.casePath + "/" + caseName + "/constant/triSurface";

    // Iterate through geometry files
    meshWizard->getGeometryMap().clear();
    GeometryMetrics metrics;
    for (auto& file: std::as_const(geometryFiles)) {
        fileData = mainWin->targetSystems[targetSystemId]->getFileContent(fullPath + "/" + file);
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

    // Update domain dimensions
    double xMin = geomBox.min.x(), yMin = geomBox.min.y(), zMin = geomBox.min.z();
    double xMax = geomBox.max.x(), yMax = geomBox.max.y(), zMax = geomBox.max.z();
    geometryLabel->setText(tr("Geometry bounds: (%1, %2, %3) to (%4, %5, %6)")
                               .arg(xMin).arg(yMin).arg(zMin).arg(xMax).arg(yMax).arg(zMax));

    // Add padding to geometry bounds
    double padX = (xMax - xMin) * 0.05;
    double padY = (yMax - yMin) * 0.05;
    double padZ = (zMax - zMin) * 0.05;
    xMin -= padX; xMax += padX;
    yMin -= padY; yMax += padY;
    zMin -= padZ; zMax += padZ;

    // Update arrays and spin boxes
    minGeometry[0] = xMin; minGeometry[1] = yMin; minGeometry[2] = zMin;
    maxGeometry[0] = xMax; maxGeometry[1] = yMax; maxGeometry[2] = zMax;
    dimSpinBox[0]->setValue(xMin);
    dimSpinBox[1]->setValue(xMax);
    dimSpinBox[2]->setValue(yMin);
    dimSpinBox[3]->setValue(yMax);
    dimSpinBox[4]->setValue(zMin);
    dimSpinBox[5]->setValue(zMax);

    // Read data from original blockMeshDict file
    std::vector<std::array<double, 3>> vertices = m_cfg->vertices;

    // Iterate through the extracted numbers in chunks of 3 (X, Y, Z)
    double minX = dimSpinBox[0]->value(), maxX = dimSpinBox[1]->value();
    double minY = dimSpinBox[2]->value(), maxY = dimSpinBox[3]->value();
    double minZ = dimSpinBox[4]->value(), maxZ = dimSpinBox[5]->value();

    for (const auto& pt : vertices) {
        minX = std::min(minX, pt[0]);
        maxX = std::max(maxX, pt[0]);
        minY = std::min(minY, pt[1]);
        maxY = std::max(maxY, pt[1]);
        minZ = std::min(minZ, pt[2]);
        maxZ = std::max(maxZ, pt[2]);
    }

    // Update the boxes that display the bounding box values
    dimSpinBox[0]->setValue(minX);
    dimSpinBox[1]->setValue(maxX);
    dimSpinBox[2]->setValue(minY);
    dimSpinBox[3]->setValue(maxY);
    dimSpinBox[4]->setValue(minZ);
    dimSpinBox[5]->setValue(maxZ);
}

void BlockMeshPage1::updateCellCount() {

    // Compute physical axis lengths
    double diffX = dimSpinBox[1]->value() - dimSpinBox[0]->value();
    double diffY = dimSpinBox[3]->value() - dimSpinBox[2]->value();
    double diffZ = dimSpinBox[5]->value() - dimSpinBox[4]->value();

    if (m_cellSize == -1.0) {

        // 2. Protect against divide-by-zero from a malformed or default config
        int cfgNx = std::max(1, m_cfg->nX);
        int cfgNy = std::max(1, m_cfg->nY);
        int cfgNz = std::max(1, m_cfg->nZ);

        // Find the most restrictive resolution to enforce cubic cells
        m_cellSize = std::max(0.0001, std::min({diffX / cfgNx, diffY / cfgNy, diffZ / cfgNz}));

        // Safely update the UI input without triggering feedback loops
        targetCellSizeBox->blockSignals(true);
        targetCellSizeBox->setValue(m_cellSize);
        targetCellSizeBox->blockSignals(false);
    } else {
        m_cellSize = targetCellSizeBox->value();
    }

    // Compute cell counts
    int actualNx = std::max(1, static_cast<int>(std::ceil(diffX / m_cellSize)));
    int actualNy = std::max(1, static_cast<int>(std::ceil(diffY / m_cellSize)));
    int actualNz = std::max(1, static_cast<int>(std::ceil(diffZ / m_cellSize)));

    // Snap the bounding box max values outward to enforce perfectly cubic cells
    double snappedMaxX = dimSpinBox[0]->value() + (actualNx * m_cellSize);
    double snappedMaxY = dimSpinBox[2]->value() + (actualNy * m_cellSize);
    double snappedMaxZ = dimSpinBox[4]->value() + (actualNz * m_cellSize);

    // Update spin boxes
    dimSpinBox[1]->blockSignals(true);
    dimSpinBox[3]->blockSignals(true);
    dimSpinBox[5]->blockSignals(true);

    dimSpinBox[1]->setValue(snappedMaxX);
    dimSpinBox[3]->setValue(snappedMaxY);
    dimSpinBox[5]->setValue(snappedMaxZ);

    dimSpinBox[1]->blockSignals(false);
    dimSpinBox[3]->blockSignals(false);
    dimSpinBox[5]->blockSignals(false);

    // Compute total cells
    long long totalCells = static_cast<long long>(actualNx) * actualNy * actualNz;

    // Display the cell count results individually
    cellCountX->setText(QString::number(actualNx));
    cellCountY->setText(QString::number(actualNy));
    cellCountZ->setText(QString::number(actualNz));

    // QLocale adds the thousands separators (e.g., 1,500,000)
    cellCountTotal->setText(QLocale().toString(totalCells));
}

bool BlockMeshPage1::validatePage() {

    // Validate domain bounds
    for (int i = 0; i < 3; ++i) {
        double minVal = dimSpinBox[2 * i]->value();
        double maxVal = dimSpinBox[2 * i + 1]->value();

        if (maxVal <= minVal) {
            QMessageBox::warning(this, tr("Invalid Bounds"),
                                 tr("Maximum bounds must be strictly greater than minimum bounds."));
            return false;
        }
    }

    // Write scale factor to struct
    m_cfg->convertToMeters = scaleFactorBox->currentData().toDouble();

    // Write cell counts to struct
    m_cfg->nX = cellCountX->text().toInt();
    m_cfg->nY = cellCountY->text().toInt();
    m_cfg->nZ = cellCountZ->text().toInt();

    // Write vertices to struct
    double minX = dimSpinBox[0]->value(), maxX = dimSpinBox[1]->value();
    double minY = dimSpinBox[2]->value(), maxY = dimSpinBox[3]->value();
    double minZ = dimSpinBox[4]->value(), maxZ = dimSpinBox[5]->value();
    m_cfg->vertices = {
        {minX, minY, minZ}, // Vertex 0
        {maxX, minY, minZ}, // Vertex 1
        {maxX, maxY, minZ}, // Vertex 2
        {minX, maxY, minZ}, // Vertex 3
        {minX, minY, maxZ}, // Vertex 4
        {maxX, minY, maxZ}, // Vertex 5
        {maxX, maxY, maxZ}, // Vertex 6
        {minX, maxY, maxZ}  // Vertex 7
    };

    return true;
}

void BlockMeshPage1::fitBoundsPressed() {
    dimSpinBox[0]->setValue(minGeometry[0]);
    dimSpinBox[1]->setValue(maxGeometry[0]);
    dimSpinBox[2]->setValue(minGeometry[1]);
    dimSpinBox[3]->setValue(maxGeometry[1]);
    dimSpinBox[4]->setValue(minGeometry[2]);
    dimSpinBox[5]->setValue(maxGeometry[2]);
    updateCellCount();
}