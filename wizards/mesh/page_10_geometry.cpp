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

#include "page_10_geometry.h"

#include <QDir>

#include "wizard_mesh.h"

// Introduction page asks for the case name and platform
GeometryPage::GeometryPage(const QString& caseName,
    const SystemManager& systemMgr, QWidget *parent):
    m_caseName(caseName), m_systemMgr(systemMgr), QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Overall Mesh Configuration"));

    // Create a grid layout with two columns
    QGridLayout* layout = new QGridLayout(this);
    layout->setSpacing(20);

    // Get selected case
    layout->addWidget(new QLabel(tr("Select the OpenFOAM case:")), 0, 0);
    m_caseCombo = new QComboBox(this);
    m_caseCombo->addItems(m_systemMgr.getCases());
    m_caseCombo->setCurrentText(m_caseName);
    layout->addWidget(m_caseCombo, 0, 1);
    connect(m_caseCombo, &QComboBox::currentTextChanged, this,
            &GeometryPage::caseChanged);

    // Select one or more geometry files
    layout->addWidget(new QLabel(tr("Select one or more geometry files:")), 1,
                      0, Qt::AlignTop);
    m_geometryList = new QListWidget(this);
    m_geometryList->setMaximumHeight(100);
    layout->addWidget(m_geometryList, 1, 1);

    // Select meshing stages
    QGroupBox* stageBox = new QGroupBox(tr("Mesh Stage Selection"), this);
    layout->addWidget(stageBox, 2, 0, 1, 2);
    QVBoxLayout* stageLayout = new QVBoxLayout(stageBox);

    // Create description label
    stageLayout->addWidget(new QLabel(tr("Select meshing stages:")));

    // Create checkable label
    m_blockMeshCheck = new QCheckBox(tr("Create base mesh (blockMesh)"), this);
    stageLayout->addWidget(m_blockMeshCheck);
    m_blockMeshCheck->setChecked(true);
    m_extractCheck = new QCheckBox(tr("Extract surface features "
                                      "(surfaceFeatureExtract)"), this);
    stageLayout->addWidget(m_extractCheck);
    m_extractCheck->setChecked(true);
    m_castellatedCheck = new QCheckBox(tr("Run castellated mesh "
                                          "snappyHexMesh - phase 1)"), this);
    stageLayout->addWidget(m_castellatedCheck);
    m_castellatedCheck->setChecked(true);
    m_snapCheck = new QCheckBox(tr("Run surface snapping "
                                   "(snappyHexMesh - phase 2)"), this);
    stageLayout->addWidget(m_snapCheck);
    m_snapCheck->setChecked(true);
    m_layersCheck = new QCheckBox(tr("Run boundary layer addition "
                                     "(snappyHexMesh - phase 3)"), this);
    stageLayout->addWidget(m_layersCheck);
    m_layersCheck->setChecked(true);

    // Register case name
    registerField("caseName", m_caseCombo, "currentText");

    // Set the page layout
    setLayout(layout);
}

void GeometryPage::initializePage() {

    // Get cases
    meshWizard = qobject_cast<MeshWizard*>(this->wizard());

    // Start event processing
    caseChanged(m_caseName);
}

// Populate list of geometry files based on selected case
void GeometryPage::caseChanged(const QString& caseName) {

    // Clear list widget
    m_geometryList->clear();

    // Get case path
    m_caseName = caseName;
    QString casePath = m_systemMgr.getData(m_caseName).casePath;
    QString openFoamPath = m_systemMgr.getData(m_caseName).openFoamPath;

    // Determine which OpenFOAM release is used
    QString dirName = QDir(openFoamPath).dirName();
    const QRegularExpression foundationRegex("^openfoam\\d{2}$",
        QRegularExpression::CaseInsensitiveOption);
    bool isFoundation = foundationRegex.match(dirName).hasMatch();

    // Read geometry file
    QString subDir =
        (isFoundation) ? "/constant/geometry" : "/constant/triSurface";
    QString path = casePath + "/" + m_caseName + subDir;
    QStringList geometryFiles =
        m_systemMgr.getSystem(m_caseName)->processPaths(path,
            PathOperationType::LIST);
    QListWidgetItem *item;
    for (int i = geometryFiles.size() - 1; i >= 0; --i) {        
        if (geometryFiles[i].endsWith('|')) {
            geometryFiles[i].chop(1);

            if (geometryFiles[i].endsWith("eMesh")) {
                continue;
            }

            // Add list item
            item = new QListWidgetItem(geometryFiles[i], m_geometryList);
            item->setData(Qt::UserRole, geometryFiles[i]);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked);
        } else {
            geometryFiles.removeAt(i);
        }
    }
}

bool GeometryPage::validatePage() {

    // Update wizard
    meshWizard->m_runBlockMesh = m_blockMeshCheck->isChecked();
    meshWizard->m_runExtract = m_extractCheck->isChecked();
    meshWizard->m_runCastellated = m_castellatedCheck->isChecked();
    meshWizard->m_runSnap = m_snapCheck->isChecked();
    meshWizard->m_runLayers = m_layersCheck->isChecked();

    // Set case name
    m_caseName = m_caseCombo->currentText();

    // Make sure at least one geometry file is selected
    m_geometryFiles.clear();
    for (int i = 0; i < m_geometryList->count(); ++i) {
        QListWidgetItem *item = m_geometryList->item(i);
        if (item->checkState() == Qt::Checked) {
            m_geometryFiles << item->data(Qt::UserRole).toString();
        }
    }
    if (m_geometryFiles.empty()) {
        QMessageBox::critical(this, tr("No Geometry"),
            tr("No geometry files selected. Please select at least one file."));
        return false;
    } else {
        m_geometryFiles.sort();
    }

    return meshWizard->loadParseFiles();
}

int GeometryPage::nextId() const {
    if (m_blockMeshCheck->isChecked()) {
        return Page_BlockMesh1;
    } else if (m_extractCheck->isChecked()) {
        return Page_SurfaceExtraction;
    } else if (m_castellatedCheck->isChecked()) {
        return Page_Castellation;
    } else if (m_snapCheck->isChecked()) {
        return Page_SnapControl;
    } else if (m_layersCheck->isChecked()) {
        return Page_LayerControl;
    }
    return -1;
}