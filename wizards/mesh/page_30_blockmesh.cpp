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

#include "page_30_blockmesh.h"
#include "wizard_mesh.h"

// Define canonical faces (sorted)
const std::array<int, 4> X_MIN = {0, 3, 4, 7};
const std::array<int, 4> X_MAX = {1, 2, 5, 6};
const std::array<int, 4> Y_MIN = {0, 1, 4, 5};
const std::array<int, 4> Y_MAX = {2, 3, 6, 7};
const std::array<int, 4> Z_MIN = {0, 1, 2, 3};
const std::array<int, 4> Z_MAX = {4, 5, 6, 7};

// Introduction page asks for the case name and platform
BlockMeshPage2::BlockMeshPage2(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("BlockMesh Configuration: Patches and Grading"));

    // Create a grid layout with two columns
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);

    // Configure spin boxes
    for(int i = 0; i < 3; i++) {
        gradingSpinBox[i] = new QDoubleSpinBox(this);
        gradingSpinBox[i]->setRange(0.001, 1000.0);
        gradingSpinBox[i]->setDecimals(3);
        gradingSpinBox[i]->setValue(1.0);
        gradingSpinBox[i]->setSingleStep(0.1);
        gradingSpinBox[i]->setEnabled(false);
    }

    // Patch definition
    QGroupBox* patchBox = new QGroupBox(tr("Patch Definitions"), this);
    layout->addWidget(patchBox);
    QVBoxLayout* patchLayout = new QVBoxLayout(patchBox);

    // Add a label for the patch table
    patchLayout->addWidget(new QLabel(tr("Associate each face with a "
                                         "patch name and type:")));

    // Create the table for patches with 6 rows and 3 columns
    patchTable = new QTableWidget(6, 3, this);
    patchTable->setHorizontalHeaderLabels({"Block Face", "Patch Name",
                                           "Boundary Type"});
    patchTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    patchTable->verticalHeader()->setVisible(false);
    patchTable->setSelectionMode(QAbstractItemView::NoSelection);
    // patchTable->setAlternatingRowColors(true);
    patchLayout->addWidget(patchTable);

    // Set group box for mesh grading
    QGroupBox* gradingBox = new QGroupBox(tr("Mesh Grading"), this);
    layout->addWidget(gradingBox);
    QFormLayout* gradingLayout = new QFormLayout(gradingBox);

    // Create checkbox for grading
    gradingCheckBox = new QCheckBox(tr("Enable cell grading (Value > 1 "
        "compresses cells toward start, < 1 toward end)"), this);
    gradingCheckBox->setChecked(false);
    gradingLayout->addRow(gradingCheckBox);

    // Add warning
    QLabel* gradingWarning = new QLabel(tr("Warning: Non-uniform grading "
        "creates high-aspect-ratio background cells, which can cause "
        "snappyHexMesh to generate skewed boundary layers or fail."));
    gradingWarning->setStyleSheet(
        "QLabel { color: #d9534f; font-weight: bold; }");
    gradingWarning->setWordWrap(true);
    gradingWarning->setVisible(false);
    gradingLayout->addRow(gradingWarning);

    // Add widgets to grading group box
    QStringList dims = { "x", "y", "z" };
    for(int i = 0; i < 3; i++) {
        gradingLayout->addRow(tr("Expansion ratio in the %1-direction:")
                                  .arg(dims[i]), gradingSpinBox[i]);
    }

    // Connect check box to spin boxes
    connect(gradingCheckBox, &QCheckBox::toggled, this,
            [this, gradingWarning](bool checked) {
        gradingWarning->setVisible(checked);
        for(int i = 0; i < 3; i++) {
            gradingSpinBox[i]->setEnabled(checked);
            if (!checked) {
                gradingSpinBox[i]->setValue(1.0);
            }
        }
    });

    // Set the page layout
    setLayout(layout);
}

// Helper function to figure out which face an array represents
int BlockMeshPage2::identifyFaceIndex(std::array<int, 4> faceArray) {
    std::sort(faceArray.begin(), faceArray.end());

    for (int i = 0; i < 6; ++i) {
        if (faceArray == CANONICAL_FACES[i]) {
            return i;
        }
    }
    return -1;
}

void BlockMeshPage2::initializePage() {

    // Access block mesh configuration structure
    meshWizard = qobject_cast<MeshWizard*>(wizard());
    if (!meshWizard) {
        qWarning() << "Failed to cast to MeshWizard!";
        return;
    }
    m_cfg = &(meshWizard->getBlockMeshConfig());

    // Update widgets in the boundary group box
    updateBoundaryGroup();

    // Updte widgets in the grading group box
    updateGradingGroup();
}

void BlockMeshPage2::updateBoundaryGroup() {

    // Define standard labels and fallbacks
    QStringList faceLabels = {
        "X-Min (Left face)", "X-Max (Right face)",
        "Y-Min (Front face)", "Y-Max (Back face)",
        "Z-Min (Bottom face)", "Z-Max (Top face)"
    };
    QStringList defaultNames = { "minX", "maxX", "minY", "maxY", "minZ", "maxZ" };
    QStringList patchTypeNames = { "patch", "wall", "symmetryPlane", "empty", "wedge", "cyclic" };

    // Fill array with default values
    struct PatchData {
        QString name;
        QString type;
    };
    std::array<PatchData, 6> faceData;

    for (int i = 0; i < 6; ++i) {
        faceData[i] = { defaultNames[i], "patch" };
    }

    // Overwrite defaults with parsed file data
    for (const auto& patch : m_cfg->patches) {
        for (const auto& face : patch.faces) {
            int i = identifyFaceIndex(face);

            // Protect against -1 (unknown custom faces)
            if (i >= 0 && i < 6) {
                faceData[i].name = patch.name;

                // Safely map the enum to the string name
                int typeIndex = static_cast<int>(patch.type);
                if (typeIndex >= 0 && typeIndex < patchTypeNames.size()) {
                    faceData[i].type = patchTypeNames[typeIndex];
                }
            }
        }
    }

    // Initialize the table
    for (int i = 0; i < 6; i++) {
        // Column 0: Face label
        QTableWidgetItem* faceItem = new QTableWidgetItem(faceLabels[i]);
        faceItem->setFlags(faceItem->flags() & ~Qt::ItemIsEditable);
        patchTable->setItem(i, 0, faceItem);

        // Column 1: Patch name
        QLineEdit* nameEdit = new QLineEdit(this);
        nameEdit->setText(faceData[i].name);
        patchTable->setCellWidget(i, 1, nameEdit);

        // Column 2: Patch type
        QComboBox* typeCombo = new QComboBox(this);
        typeCombo->addItems(patchTypeNames);
        typeCombo->setCurrentText(faceData[i].type);
        patchTable->setCellWidget(i, 2, typeCombo);
    }

    // Resize the rows
    // patchTable->resizeRowsToContents();

    // Calculate the height using the newly updated row height
    int height = patchTable->horizontalHeader()->height() +
                 (patchTable->rowCount() * patchTable->rowHeight(0)) +
                 (patchTable->frameWidth() * 2) + 2;

    patchTable->setFixedHeight(height);
}

// Update widgets in grading group
void BlockMeshPage2::updateGradingGroup() {

    // Check if grading isn't used
    if ((std::abs(m_cfg->gradingX - 1.0) < 1e-5) &&
        (std::abs(m_cfg->gradingY - 1.0) < 1e-5) &&
        (std::abs(m_cfg->gradingZ - 1.0) < 1e-5)) {

        gradingCheckBox->setChecked(false);
        return;
    }

    // Updating grading widgets
    gradingCheckBox->setChecked(true);
    gradingSpinBox[0]->setValue(m_cfg->gradingX);
    gradingSpinBox[1]->setValue(m_cfg->gradingY);
    gradingSpinBox[2]->setValue(m_cfg->gradingZ);
}

bool BlockMeshPage2::validatePage() {

    // Clear existing patches
    m_cfg->patches.clear();

    // Create map to aggregate identical patch names
    std::map<QString, Patch> patchAggregator;

    // Iterate through the table rows
    for (int i = 0; i < 6; ++i) {
        // Safely extract the widgets from the table
        QLineEdit* nameEdit = qobject_cast<QLineEdit*>(patchTable->cellWidget(i, 1));
        QComboBox* typeCombo = qobject_cast<QComboBox*>(patchTable->cellWidget(i, 2));

        if (!nameEdit || !typeCombo) continue; // Safety check

        QString patchName = nameEdit->text().trimmed();
        QString typeStr = typeCombo->currentText();

        // Validation: Ensure no blank names
        if (patchName.isEmpty()) {
            QMessageBox::warning(this, tr("Missing Patch Name"),
                                 tr("Please provide a name for all 6 bounding box faces."));
            return false;
        }

        // Map the string to your C++ Enum
        PatchType patchType = PatchType::patch; // Fallback default
        if (typeStr == "wall") patchType = PatchType::wall;
        else if (typeStr == "symmetryPlane") patchType = PatchType::symmetryPlane;
        else if (typeStr == "empty") patchType = PatchType::empty;
        else if (typeStr == "wedge") patchType = PatchType::wedge;
        else if (typeStr == "cyclic") patchType = PatchType::cyclic;

        // 5. Aggregate logic
        if (patchAggregator.find(patchName) == patchAggregator.end()) {
            // This is a brand new patch name; create it and add the face
            Patch newPatch;
            newPatch.name = patchName;
            newPatch.type = patchType;
            newPatch.faces.push_back(CANONICAL_FACES[i]);
            patchAggregator[patchName] = newPatch;
        } else {
            // Validation: Ensure the user didn't select conflicting types for the same patch
            if (patchAggregator[patchName].type != patchType) {
                QMessageBox::warning(this, tr("Boundary Type Conflict"),
                                     tr("Faces sharing the same name ('%1') must have the same boundary type.")
                                         .arg(patchName));
                return false;
            }

            patchAggregator[patchName].faces.push_back(CANONICAL_FACES[i]);
        }
    }

    // Transfer the aggregated patches from the map into the master configuration struct
    for (const auto& pair : patchAggregator) {
        m_cfg->patches.push_back(pair.second);
    }

    // Update grading values
    if(gradingCheckBox->isChecked()) {
        m_cfg->gradingX = gradingSpinBox[0]->value();
        m_cfg->gradingY = gradingSpinBox[1]->value();
        m_cfg->gradingZ = gradingSpinBox[2]->value();
    } else {
        m_cfg->gradingX = 1.0;
        m_cfg->gradingY = 1.0;
        m_cfg->gradingZ = 1.0;
    }

    return true;
}

int BlockMeshPage2::nextId() const {
    if (meshWizard->m_runExtract) {
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