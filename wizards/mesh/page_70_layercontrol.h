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

#ifndef PAGE_70_LAYERCONTROL_H_
#define PAGE_70_LAYERCONTROL_H_

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

class LayerControlPage : public QWizardPage {
    Q_OBJECT

 public:
    explicit LayerControlPage(QWidget *parent);

 protected:
    void initializePage() override;
    bool validatePage() override;

 private:
    LayerControlConfig *m_cfg;
    MeshWizard *meshWizard;

    QCheckBox *relativeSizesCheck;
    QDoubleSpinBox *expansionRatioSpin, *finalLayerSpin, *minThicknessSpin,
        *featureAngleSpin;
    QSpinBox *layerIterSpin, *surfaceSmoothingSpin, *internalSmoothingSpin,
        *thicknessSmoothingSpin;
    QTableWidget* surfaceLayerTable;
};

#endif  // PAGE_70_LAYERCONTROL_H_
