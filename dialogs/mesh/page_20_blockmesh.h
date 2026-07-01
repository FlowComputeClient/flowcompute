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

#ifndef PAGE_20_BLOCKMESH_H
#define PAGE_20_BLOCKMESH_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWizardPage>

#include "../../geometry/graphic_data.h"
#include "mesh_structs.h"

class MainWindow;
class MeshWizard;

class BlockMeshPage1 : public QWizardPage {
    Q_OBJECT

public:
    explicit BlockMeshPage1(QWidget *parent);

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    BlockMeshConfig* m_cfg;
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

#endif  // PAGE_20_BLOCKMESH_H
