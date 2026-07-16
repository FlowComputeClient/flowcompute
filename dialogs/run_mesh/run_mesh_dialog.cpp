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

#include "run_mesh_dialog.h"

#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFormLayout>
#include <QString>

#include "systems/system_manager.h"

RunMeshDialog::RunMeshDialog(const QString& caseName,
    const SystemManager& systemMgr, const QStringList& files,
    bool isFoundation, QWidget* parent): m_systemMgr(systemMgr),
    m_files(files), m_isFoundation(isFoundation), QDialog(parent) {
    // Set title and style
    setWindowTitle(tr("Mesh Execution"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Use a standard vertical layout for stacking the group boxes
    QFormLayout* mainLayout = new QFormLayout(this);
    mainLayout->setSpacing(15);

    // Get selected case
    m_caseCombo = new QComboBox(this);
    m_caseCombo->addItems(systemMgr.getCases());
    m_caseCombo->setCurrentText(caseName);
    mainLayout->addRow(tr("Select an OpenFOAM case:"), m_caseCombo);
    connect(m_caseCombo, &QComboBox::currentTextChanged, this,
            &RunMeshDialog::onCaseChanged);

    // blockMesh group
    QGroupBox* blockMeshGroup = new QGroupBox(tr("Base Mesh Generation"), this);
    QVBoxLayout* blockMeshLayout = new QVBoxLayout(blockMeshGroup);
    m_runBlockMeshCheck = new QCheckBox(tr("Run blockMesh"), blockMeshGroup);
    if (m_files[0] == "0") {
        m_runBlockMeshCheck->setChecked(true);
    } else {
        m_runBlockMeshCheck->setChecked(false);
    }
    blockMeshLayout->addWidget(m_runBlockMeshCheck);
    mainLayout->addRow(blockMeshGroup);

    // surfaceFeature group
    QString utilName =
        (isFoundation) ? "surfaceFeatures" : "surfaceFeatureExtract";
    QGroupBox* extractGroup =
        new QGroupBox(tr("Surface Feature Identification"), this);
    QVBoxLayout* extractLayout = new QVBoxLayout(extractGroup);
    m_runSurfaceFeatureCheck =
        new QCheckBox(tr("Run ") + utilName, extractGroup);
    if (m_files[1] == "0") {
        m_runSurfaceFeatureCheck->setChecked(true);
    } else {
        m_runSurfaceFeatureCheck->setChecked(false);
    }
    extractLayout->addWidget(m_runSurfaceFeatureCheck);
    mainLayout->addRow(extractGroup);

    // snappyHexMesh group
    QGroupBox* snappyGroup =
        new QGroupBox(tr("snappyHexMesh Generation"), this);
    QVBoxLayout* snappyLayout = new QVBoxLayout(snappyGroup);

    // Master snappy toggle
    m_runSnappyHexMeshCheck =
        new QCheckBox(tr("Run snappyHexMesh"), snappyGroup);
    if (m_files[2] == "0") {
        m_runSnappyHexMeshCheck->setChecked(true);
    } else {
        m_runSnappyHexMeshCheck->setChecked(false);
    }
    snappyLayout->addWidget(m_runSnappyHexMeshCheck);
    connect(m_runSnappyHexMeshCheck, &QCheckBox::toggled, this,
            &RunMeshDialog::snappyCheckToggled);

    // Sub-options container (indented slightly for visual hierarchy)
    QWidget* snappyWidget = new QWidget(snappyGroup);
    QFormLayout* formLayout = new QFormLayout(snappyWidget);
    formLayout->setContentsMargins(20, 5, 0, 0);
    formLayout->setSpacing(10);

    // Select execution mode
    m_meshModeCombo = new QComboBox(snappyWidget);
    m_meshModeCombo->addItems({ tr("Generate Mesh"), tr("Dry Run"),
                               tr("Check Geometry") });
    formLayout->addRow(tr("Execution mode:"), m_meshModeCombo);
    connect(m_meshModeCombo, &QComboBox::currentIndexChanged,
        this, &RunMeshDialog::snappyHexMeshModeChanged);

    // Create combo box for number of cores
    m_numCoresCombo = new QComboBox(snappyWidget);
    formLayout->addRow(tr("Number of cores:"), m_numCoresCombo);

    // Allow mesh overwrite
    m_meshOverwriteCheck =
        new QCheckBox(tr("Overwrite existing mesh"), snappyWidget);
    formLayout->addRow(m_meshOverwriteCheck);
    m_meshOverwriteCheck->setChecked(true);

    // Allow parallel reconstruction
    m_meshReconstructCheck =
        new QCheckBox(tr("Reconstruct mesh after parallel run"), snappyWidget);
    formLayout->addRow(m_meshReconstructCheck);
    m_meshReconstructCheck->setChecked(true);
    snappyLayout->addWidget(snappyWidget);
    mainLayout->addRow(snappyGroup);

    // Create OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &RunMeshDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    // Set number of cores
    onCaseChanged(caseName);

    setMinimumWidth(300);
    this->adjustSize();
}

void RunMeshDialog::onOkClicked() {
    // Get case name
    QString caseName = m_caseCombo->currentText();

    // Number of cores
    int numCores = m_numCoresCombo->currentText().toInt();

    // Create command for snappyHexMesh
    QString snappyCmd;
    if (m_runSnappyHexMeshCheck->isChecked()) {
        bool isGenerating = (m_meshModeCombo->currentIndex() == 0);
        if (isGenerating) {
            if (numCores > 1) {
                snappyCmd = "rm -rf processor*; decomposePar";
                if (m_isFoundation) {
                    snappyCmd += " && (for dir in processor*/constant; do "
                        "[ -d constant/geometry ] && "
                        "cp -rf constant/geometry \"$dir/\"; done; true)";
                }
                snappyCmd += " && mpirun -np " + QString::number(numCores) +
                             " snappyHexMesh -parallel";
            } else {
                snappyCmd = "snappyHexMesh ";
            }

            // Overwrite
            if (!m_isFoundation && m_meshOverwriteCheck->isChecked()) {
                snappyCmd += " -overwrite";
            } else if (m_isFoundation && !m_meshOverwriteCheck->isChecked()) {
                snappyCmd += " -noOverwrite";
            }

            // Reconstruct
            if ((numCores > 1) && (m_meshReconstructCheck->isChecked())) {

                // Select the reconstruction tool
                QString reconUtil = m_isFoundation ?
                    "reconstructPar" : "reconstructParMesh";

                // Determine how to reconstruct
                QString reconFlag = m_meshOverwriteCheck->isChecked() ?
                    " -constant" : " -latestTime";

                // Append the command
                snappyCmd += " && " + reconUtil + reconFlag +
                    " && rm -rf processor*";
            }
        } else {
            numCores = 1;
            if (m_meshModeCombo->currentIndex() == 1) {
                snappyCmd = "snappyHexMesh -dry-run";
            } else if (m_meshModeCombo->currentIndex() == 2) {
                snappyCmd = "snappyHexMesh -checkGeometry";
            }
        }
    }

    // Launch the mesh utilities
    emit requestRunMesh(caseName, m_runBlockMeshCheck->isChecked(),
        m_runSurfaceFeatureCheck->isChecked(),
        m_runSnappyHexMeshCheck->isChecked(),
        snappyCmd, numCores);
    QDialog::accept();
}

void RunMeshDialog::onCaseChanged(QString caseName) {
    // Determine OpenFoam Path
    CaseData caseData = m_systemMgr.getData(caseName);
    QString casePath = caseData.casePath + "/" + caseName;
    QString openFoamPath = caseData.openFoamPath;

    // Check if using Foundation release of OpenFOAM
    QString dirName = QDir(openFoamPath).dirName();
    const QRegularExpression foundationRegex("^openfoam\\d{2}$",
        QRegularExpression::CaseInsensitiveOption);
    m_isFoundation = foundationRegex.match(dirName).hasMatch();

    // Check if mesh configuration files are present
    QStringList meshConfigFiles;
    if (m_isFoundation) {
        meshConfigFiles = {casePath + "/system/blockMeshDict",
                           casePath + "/system/surfaceFeaturesDict",
                           casePath + "/system/snappyHexMeshDict"};
    } else {
        meshConfigFiles = {casePath + "/system/blockMeshDict",
                           casePath + "/system/surfaceFeatureExtractDict",
                           casePath + "/system/snappyHexMeshDict"};
    }
    QString meshConfigFileString = meshConfigFiles.join("\n");
    m_files = m_systemMgr.getSystem(caseName)->
        processPaths(meshConfigFileString, PathOperationType::CHECK);

    // Update check boxes
    if (m_files[0] == "0") {
        m_runBlockMeshCheck->setChecked(true);
    } else {
        m_runBlockMeshCheck->setChecked(false);
    }
    if (m_isFoundation) {
        m_runSurfaceFeatureCheck->setText(tr("Run surfaceFeatures"));
    } else {
        m_runSurfaceFeatureCheck->setText(tr("Run surfaceFeatureExtract"));
    }
    if (m_files[1] == "0") {
        m_runSurfaceFeatureCheck->setChecked(true);
    } else {
        m_runSurfaceFeatureCheck->setChecked(false);
    }
    if (m_files[2] == "0") {
        m_runSnappyHexMeshCheck->setChecked(true);
    } else {
        m_runSnappyHexMeshCheck->setChecked(false);
    }

    // Get the number of cores
    int numCores = 1;
    QString output;
    auto system = m_systemMgr.getSystem(caseName);
    if (system->launchShortUtility("nproc", output) == 0) {
        output.remove("\n");
        numCores = output.toInt();
    }

    // Generate QStringList for number of cores
    QStringList coreVals = { "1" };
    for (int i=2; i<numCores; i++) {
        coreVals.append(QString::number(i));
    }
    m_numCoresCombo->addItems(coreVals);
    if (numCores > 1) {
        m_numCoresCombo->setCurrentText(coreVals[coreVals.size() - 2]);
    } else {
        m_numCoresCombo->setCurrentText("1");
    }
}

void RunMeshDialog::snappyCheckToggled(bool enabled) {
    m_meshModeCombo->setEnabled(enabled);
    m_numCoresCombo->setEnabled(enabled);
    m_meshOverwriteCheck->setEnabled(enabled);
    m_meshReconstructCheck->setEnabled(enabled);
    if (!enabled) {
        m_meshOverwriteCheck->setChecked(false);
        m_meshReconstructCheck->setChecked(false);
    }
}

void RunMeshDialog::snappyHexMeshModeChanged(int index) {
    bool isGenerating = (index == 0);

    m_numCoresCombo->setEnabled(isGenerating);
    m_meshOverwriteCheck->setEnabled(isGenerating);
    m_meshReconstructCheck->setEnabled(isGenerating);

    if (!isGenerating) {
        m_meshOverwriteCheck->setEnabled(false);
        m_meshReconstructCheck->setEnabled(false);
    }
}
