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

#ifndef WIZARDS_NEW_CASE_PAGE_30_INTERACTIVE_H_
#define WIZARDS_NEW_CASE_PAGE_30_INTERACTIVE_H_

#include <QWizardPage>

class NewCaseWizard;
class QButtonGroup;
class QCheckBox;
class QSlider;

class InteractivePage : public QWizardPage {
    Q_OBJECT

 public:
    explicit InteractivePage(QWidget *parent);

 protected:
    bool validatePage() override;

 private:
    QButtonGroup *m_timeButtonGroup, *m_flowButtonGroup, *m_phaseButtonGroup;
    QButtonGroup *m_turbulenceButtonGroup, *m_heatButtonGroup;
    QButtonGroup *m_meshButtonGroup;
    QCheckBox *m_radiationCheck, *m_combustionCheck;
    QCheckBox *m_buoyancyCheck, *m_particlesCheck;
    QSlider *m_prioritySlider;
};

#endif  // WIZARDS_NEW_CASE_PAGE_30_INTERACTIVE_H_
