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

#ifndef WIZARDS_MESH_PAGE_20_BLOCKMESH_H_
#define WIZARDS_MESH_PAGE_20_BLOCKMESH_H_

#include <QWizardPage>

#include "geometry/graphic_data.h"
#include "parser/block_mesh_dict.h"
#include "systems/system_manager.h"

class MeshWizard;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;

class BlockMeshPage1 : public QWizardPage {
    Q_OBJECT

public:
    BlockMeshPage1(const SystemManager& systemMgr, QWidget *parent);

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    const SystemManager& m_systemMgr;
    CaseIO::BlockMeshConfig* m_cfg;
    MeshWizard* meshWizard;
    BoundingBox m_rawGeomBox;
    QComboBox *m_scaleFactorCombo;
    QLabel* geometryLabel;
    std::array<QDoubleSpinBox*, 6> dimSpin;
    std::array<double, 3> minGeometry, maxGeometry;
    void setBoundingBox();
    void updateCellCount();
    double m_cellSize = -1.0;
    QDoubleSpinBox *targetCellSizeSpin;
    std::array<QLineEdit*, 3> cellCountEdits, actualSizeEdits;
    QLineEdit *cellCountTotalEdit, *maxAspectRatioEdit;

    double m_previousScaleFactor = 1.0;
    double getCurrentScaleFactor(const QString& text) const;

private slots:
    void fitBoundsPressed();
    void onScaleFactorChanged(const QString& text);
};

#endif  // WIZARDS_MESH_PAGE_20_BLOCKMESH_H_
