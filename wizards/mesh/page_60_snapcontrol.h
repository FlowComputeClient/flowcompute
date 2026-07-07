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

#ifndef PAGE_60_SNAPCONTROL_H_
#define PAGE_60_SNAPCONTROL_H_

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
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

class SnapControlPage : public QWizardPage {
    Q_OBJECT

 public:
    explicit SnapControlPage(QWidget *parent);
    int nextId() const override;

 protected:
    void initializePage() override;
    bool validatePage() override;

 private:
    SnapControlConfig* m_cfg;
    MeshWizard* meshWizard;
    QSpinBox *smoothingBox, *maxSnappingBox, *relaxationBox, *snapIterationBox;
    QCheckBox *explicitSnapBox, *implicitSnapBox;
    QDoubleSpinBox *toleranceBox;
};

#endif  // PAGE_60_SNAPCONTROL_H_
