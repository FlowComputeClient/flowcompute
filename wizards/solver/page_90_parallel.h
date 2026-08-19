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

#ifndef WIZARDS_SOLVER_PAGE_90_PARALLEL_H_
#define WIZARDS_SOLVER_PAGE_90_PARALLEL_H_

#include <QWizardPage>

#include "parser/decompose_par_dict.h"

class SolverWizard;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QStackedWidget;

class ParallelPage : public QWizardPage {
    Q_OBJECT

 public:
    explicit ParallelPage(QWidget *parent);

 protected:
    void initializePage() override;
    bool validatePage() override;

 private:
    SolverWizard* solverWizard;
    CaseIO::ParallelConfig* m_cfg;

    QCheckBox *m_parallelCheck;
    QComboBox *m_methodCombo, *m_hierOrderCombo;
    QDoubleSpinBox *m_simpleDeltaSpin, *m_hierDeltaSpin;
    QSpinBox *m_simpleXSpin, *m_simpleYSpin, *m_simpleZSpin;
    QSpinBox *m_subdomainsSpin, *m_hierXSpin, *m_hierYSpin, *m_hierZSpin;
    QStackedWidget* m_methodStack;

 private slots:
    void parallelChanged(bool);
    void methodChanged(int);
};

#endif  // WIZARDS_SOLVER_PAGE_90_PARALLEL_H_
