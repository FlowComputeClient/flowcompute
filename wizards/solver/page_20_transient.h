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

#ifndef WIZARDS_SOLVER_PAGE_20_TRANSIENT_H_
#define WIZARDS_SOLVER_PAGE_20_TRANSIENT_H_

#include <QWizardPage>

#include "core_types.h"
#include "parser/control_dict.h"

class SolverWizard;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;

class TransientPage : public QWizardPage {
    Q_OBJECT

 public:
    explicit TransientPage(const std::vector<FlowCompute::SolverFamily>& families, QWidget *parent);
    // int nextId() const override;

 protected:
    // void initializePage() override;
    // bool validatePage() override;

 private:
    SolverWizard* m_solverWizard;
    CaseIO::ControlConfig* m_cfg;

    std::vector<FlowCompute::SolverFamily> m_families;

    QComboBox *familyBox, *solverBox, *startFromBox, *stopAtBox, *writeControlBox;
    QDoubleSpinBox *startTimeBox, *endTimeBox, *maxCourantBox, *writeIntervalBox, *deltaTBox;
    QCheckBox *adjustTimeStepBox;
    QSpinBox* purgeWriteBox;
};

#endif  // WIZARDS_SOLVER_PAGE_20_TRANSIENT_H_
