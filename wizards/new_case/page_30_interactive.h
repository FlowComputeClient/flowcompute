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

#ifndef PAGE_30_INTERACTIVE_H_
#define PAGE_30_INTERACTIVE_H_

#include <QButtonGroup>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QRadioButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QWizardPage>

class NewCaseWizard;

class InteractivePage : public QWizardPage {
    Q_OBJECT

 public:
    explicit InteractivePage(QWidget *parent);

 protected:
    bool validatePage() override;

 private:
    QButtonGroup *m_flowButtonGroup, *m_timeButtonGroup, *m_turbulenceButtonGroup;
    QCheckBox *m_heatCheck, *m_radiationCheck, *m_combustionCheck;
    QSlider *m_prioritySlider;
};

#endif  // PAGE_30_INTERACTIVE_H_
