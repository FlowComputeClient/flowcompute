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

#ifndef PAGE_30_BLOCKMESH_H_
#define PAGE_30_BLOCKMESH_H_

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWizardPage>

#include "mesh_structs.h"

class MainWindow;
class MeshWizard;

class BlockMeshPage2 : public QWizardPage {
    Q_OBJECT

public:
    explicit BlockMeshPage2(QWidget *parent);
    int nextId() const override;

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    BlockMeshConfig* m_cfg;
    MeshWizard* meshWizard;
    QTableWidget* patchTable;
    QCheckBox* gradingCheckBox;
    std::array<QDoubleSpinBox*, 3> gradingSpinBox;

    void updateBoundaryGroup();
    void updateGradingGroup();
    int identifyFaceIndex(std::array<int, 4> faceArray);

    // A single, shared source of truth for the 6 hex faces
    static constexpr std::array<std::array<int, 4>, 6> CANONICAL_FACES = {{
        {0, 3, 4, 7}, // 0: X-Min (Left)
        {1, 2, 5, 6}, // 1: X-Max (Right)
        {0, 1, 4, 5}, // 2: Y-Min (Front)
        {2, 3, 6, 7}, // 3: Y-Max (Back)
        {0, 1, 2, 3}, // 4: Z-Min (Bottom)
        {4, 5, 6, 7}  // 5: Z-Max (Top)
    }};
};

#endif  // PAGE_30_BLOCKMESH_H_
