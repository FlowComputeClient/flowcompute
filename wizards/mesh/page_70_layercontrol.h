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

#ifndef WIZARDS_MESH_PAGE_70_LAYERCONTROL_H_
#define WIZARDS_MESH_PAGE_70_LAYERCONTROL_H_

#include <QWizardPage>

#include "parser/snappy_hex_mesh_dict.h"

class MeshWizard;
class QCheckBox;
class QDoubleSpinBox;
class QSpinBox;
class QTableWidget;

class LayerControlPage : public QWizardPage {
    Q_OBJECT

 public:
    explicit LayerControlPage(QWidget *parent);

 protected:
    void initializePage() override;
    bool validatePage() override;

 private:
    CaseIO::LayerControlConfig *m_cfg;
    MeshWizard *meshWizard;

    QCheckBox *relativeSizesCheck;
    QDoubleSpinBox *expansionRatioSpin, *finalLayerSpin, *minThicknessSpin,
        *featureAngleSpin;
    QSpinBox *layerIterSpin, *surfaceSmoothingSpin, *internalSmoothingSpin,
        *thicknessSmoothingSpin;
    QTableWidget* surfaceLayerTable;
};

#endif  // WIZARDS_MESH_PAGE_70_LAYERCONTROL_H_
