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

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFormLayout>
#include <QString>

#include "../../../main_window.h"

RunMeshDialog::RunMeshDialog(const QString& selectedCase,
                             const QStringList& caseList,
                             QWidget* parent): QDialog(parent) {

    // Get pointer to main window
    m_mainWin = qobject_cast<MainWindow*>(this->parent());

    // Set title and style
    setWindowTitle(tr("Mesh Execution"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Use a standard vertical layout for stacking the group boxes
    QFormLayout* mainLayout = new QFormLayout(this);
    mainLayout->setSpacing(15);

    // Get selected case
    m_caseCombo = new QComboBox(this);
    m_caseCombo->addItems(caseList);
    m_caseCombo->setCurrentText(selectedCase);
    mainLayout->addRow(tr("Select an OpenFOAM case:"), m_caseCombo);
    connect(m_caseCombo, &QComboBox::currentTextChanged, this, &RunMeshDialog::onCaseChanged);

    // blockMesh group
    QGroupBox* blockMeshGroup = new QGroupBox(tr("Base Mesh Generation"), this);
    QVBoxLayout* blockMeshLayout = new QVBoxLayout(blockMeshGroup);
    m_runBlockMeshCheck = new QCheckBox(tr("Run blockMesh"), blockMeshGroup);
    m_runBlockMeshCheck->setChecked(true);
    blockMeshLayout->addWidget(m_runBlockMeshCheck);
    mainLayout->addRow(blockMeshGroup);

    // surfaceFeatureExtract group
    QGroupBox* extractGroup = new QGroupBox(tr("Feature Extraction"), this);
    QVBoxLayout* extractLayout = new QVBoxLayout(extractGroup);
    m_runFeatureExtractCheck = new QCheckBox(tr("Run surfaceFeatureExtract"), extractGroup);
    m_runFeatureExtractCheck->setChecked(true);
    extractLayout->addWidget(m_runFeatureExtractCheck);
    mainLayout->addRow(extractGroup);

    // snappyHexMesh group
    QGroupBox* snappyGroup = new QGroupBox(tr("snappyHexMesh Generation"), this);
    QVBoxLayout* snappyLayout = new QVBoxLayout(snappyGroup);

    // Master snappy toggle
    m_runSnappyHexMeshCheck = new QCheckBox(tr("Run snappyHexMesh"), snappyGroup);
    m_runSnappyHexMeshCheck->setChecked(true);
    snappyLayout->addWidget(m_runSnappyHexMeshCheck);
    connect(m_runSnappyHexMeshCheck, &QCheckBox::toggled, this, &RunMeshDialog::snappyCheckToggled);

    // Sub-options container (indented slightly for visual hierarchy)
    QWidget* snappyWidget = new QWidget(snappyGroup);
    QFormLayout* formLayout = new QFormLayout(snappyWidget);
    formLayout->setContentsMargins(20, 5, 0, 0);
    formLayout->setSpacing(10);

    // Select execution mode
    m_meshModeCombo = new QComboBox(snappyWidget);
    m_meshModeCombo->addItems({ tr("Generate Mesh"), tr("Dry Run"), tr("Check Geometry") });
    formLayout->addRow(tr("Execution mode:"), m_meshModeCombo);
    connect(m_meshModeCombo, &QComboBox::currentIndexChanged,
        this, &RunMeshDialog::snappyHexMeshModeChanged);

    // Create combo box for number of cores
    m_numCoresCombo = new QComboBox(snappyWidget);
    formLayout->addRow(tr("Number of cores:"), m_numCoresCombo);

    // Allow mesh overwrite
    m_meshOverwriteCheck = new QCheckBox(tr("Overwrite existing mesh"), snappyWidget);
    formLayout->addRow(m_meshOverwriteCheck);
    m_meshOverwriteCheck->setChecked(true);

    // Allow parallel reconstruction
    m_meshReconstructCheck = new QCheckBox(tr("Reconstruct mesh after parallel run"), snappyWidget);
    formLayout->addRow(m_meshReconstructCheck);
    m_meshReconstructCheck->setChecked(true);
    snappyLayout->addWidget(snappyWidget);
    mainLayout->addRow(snappyGroup);

    // Create OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &RunMeshDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Configure layout
    // mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    // Set number of cores
    onCaseChanged(selectedCase);

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

            // Set base command
            if (numCores > 1) {
                snappyCmd = "rm -rf processor*; decomposePar && mpirun -np " + QString::number(numCores) +
                            " snappyHexMesh -parallel";
            } else {
                snappyCmd = "snappyHexMesh ";
            }

            // Overwrite
            if (m_meshOverwriteCheck->isChecked()) {
                snappyCmd += " -overwrite";
            }

            // Reconstruct
            if ((numCores > 1) && (m_meshReconstructCheck->isChecked())) {
                snappyCmd += " && reconstructParMesh -constant && rm -rf processor*";
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
    m_mainWin->runMesh(caseName, m_runBlockMeshCheck->isChecked(),
                       m_runFeatureExtractCheck->isChecked(),
                       m_runSnappyHexMeshCheck->isChecked(),
                       snappyCmd, numCores);

    QDialog::accept();
}

void RunMeshDialog::onCaseChanged(QString caseName) {

    int numCores = 1;
    CaseData caseData = m_mainWin->m_caseMap[caseName];
    int targetId = caseData.targetSystemId;

    // Get the number of cores
    QString output;
    if (m_mainWin->targetSystems[targetId]->launchShortUtility("nproc", output) == 0) {
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
