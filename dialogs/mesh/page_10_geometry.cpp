#include "page_10_geometry.h"

#include "wizard_mesh.h"
#include "../../main_window.h"

// Introduction page asks for the case name and platform
GeometryPage::GeometryPage(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Overall Mesh Configuration"));

    // Create a grid layout with two columns
    QGridLayout* layout = new QGridLayout(this);
    layout->setSpacing(20);

    // Get selected case
    layout->addWidget(new QLabel(tr("Select an OpenFOAM case:")), 0, 0);
    caseBox = new QComboBox(this);
    layout->addWidget(caseBox, 0, 1);
    connect(caseBox, &QComboBox::currentIndexChanged, this, &GeometryPage::caseChanged);

    // Select one or more geometry files
    layout->addWidget(new QLabel(tr("Select one or more geometry files:")), 1, 0, Qt::AlignTop);
    listWidget = new QListWidget(this);
    listWidget->setMaximumHeight(100);
    layout->addWidget(listWidget, 1, 1);

    // Select meshing stages
    QGroupBox* stageBox = new QGroupBox(tr("Mesh Stage Selection"), this);
    layout->addWidget(stageBox, 2, 0, 1, 2);
    QVBoxLayout* stageLayout = new QVBoxLayout(stageBox);

    // Create description label
    stageLayout->addWidget(new QLabel(tr("Select one or more stages in the meshing process:")));

    // Create checkable label
    QCheckBox* blockMeshCheckBox = new QCheckBox(tr("Generate base mesh (blockMesh)"), this);
    stageLayout->addWidget(blockMeshCheckBox);
    blockMeshCheckBox->setChecked(true);
    QCheckBox* extractCheckBox = new QCheckBox(tr("Extract surface features (surfaceFeatureExtract)"), this);
    stageLayout->addWidget(extractCheckBox);
    extractCheckBox->setChecked(true);
    QCheckBox* castellatedCheckBox = new QCheckBox(tr("Run castellated mesh (snappyHexMesh - phase 1)"), this);
    stageLayout->addWidget(castellatedCheckBox);
    castellatedCheckBox->setChecked(true);
    QCheckBox* snapCheckBox = new QCheckBox(tr("Run surface snapping (snappyHexMesh - phase 2)"), this);
    stageLayout->addWidget(snapCheckBox);
    snapCheckBox->setChecked(true);
    QCheckBox* layersCheckBox = new QCheckBox(tr("Run boundary layer addition (snappyHexMesh - phase 3)"), this);
    stageLayout->addWidget(layersCheckBox);
    layersCheckBox->setChecked(true);

    // Register values
    registerField("caseName", caseBox, "currentText");
    registerField("runBlockMesh", blockMeshCheckBox);
    registerField("runExtract", extractCheckBox);
    registerField("runCastellated", castellatedCheckBox);
    registerField("runSnap", snapCheckBox);
    registerField("runLayers", layersCheckBox);

    // Set the page layout
    setLayout(layout);
}

void GeometryPage::initializePage() {

    // Get cases
    meshWizard = qobject_cast<MeshWizard*>(this->wizard());
    mainWin = qobject_cast<MainWindow*>(this->wizard()->parentWidget());
    QStringList cases = mainWin->m_caseMap.keys();
    QString selectedCase = mainWin->getSelectedCase();

    caseBox->addItems(cases);
    if(!selectedCase.isEmpty()) {
        caseBox->setCurrentText(selectedCase);
    }
}

// Populate list of geometry files based on selected case
void GeometryPage::caseChanged(int index) {

    // Clear list widget
    listWidget->clear();

    // Get case path
    caseName = caseBox->currentText();
    int targetSystemId = mainWin->m_caseMap[caseName].targetSystemId;
    QString casePath = mainWin->m_caseMap[caseName].casePath;

    // Transfer geometry file
    QString path = casePath + "/" + caseName + "/constant/triSurface";
    QStringList geometryFiles = mainWin->targetSystems[targetSystemId]->getFiles(path);
    QListWidgetItem *item;
    for (int i = geometryFiles.size() - 1; i >= 0; --i) {        
        if (geometryFiles[i].endsWith('|')) {
            geometryFiles[i].chop(1);

            if (geometryFiles[i].endsWith("eMesh")) {
                continue;
            }

            // Add list item
            item = new QListWidgetItem(geometryFiles[i], listWidget);
            item->setData(Qt::UserRole, geometryFiles[i]);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
        } else {
            geometryFiles.removeAt(i);
        }
    }
}

bool GeometryPage::validatePage() {

    // Set case name
    caseName = caseBox->currentText();

    // Make sure at least one geometry file is selected
    geometryFiles.clear();
    for (int i = 0; i < listWidget->count(); ++i) {
        QListWidgetItem *item = listWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            geometryFiles << item->data(Qt::UserRole).toString();
        }
    }
    if (geometryFiles.empty()) {
        QMessageBox::critical(this, tr("No Geometry"),
                              tr("No geometry files selected. Please select at least one file."));
        return false;
    } else {
        geometryFiles.sort();
    }

    return meshWizard->loadParseFiles();
}

int GeometryPage::nextId() const {
    if (meshWizard->m_runBlockMesh) {
        return Page_BlockMesh1;
    } else if (meshWizard->m_runExtract) {
        return Page_SurfaceExtraction;
    } else if (meshWizard->m_runCastellated) {
        return Page_Castellation;
    } else if (meshWizard->m_runSnap) {
        return Page_SnapControl;
    } else if (meshWizard->m_runLayers) {
        return Page_LayerControl;
    }
    return -1;
}