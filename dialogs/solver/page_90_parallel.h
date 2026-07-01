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

#ifndef PAGE_90_PARALLEL_H
#define PAGE_90_PARALLEL_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
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
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWizardPage>

#include "solver_structs.h"

class SolverWizard;

class ParallelPage : public QWizardPage {
    Q_OBJECT

public:
    explicit ParallelPage(QWidget *parent);
    // int nextId() const override;

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    SolverWizard* solverWizard;
    ParallelConfig* m_cfg;

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

#endif  // PAGE_90_PARALLEL_H
