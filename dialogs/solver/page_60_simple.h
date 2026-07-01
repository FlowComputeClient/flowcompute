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

#ifndef PAGE_60_SIMPLE_H
#define PAGE_60_SIMPLE_H

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QWizardPage>
#include <QSpinBox>
#include <QTableWidget>

#include "solver_structs.h"

class SolverWizard;

class SimplePage : public QWizardPage {
    Q_OBJECT

public:
    explicit SimplePage(QWidget *parent = nullptr);
    int nextId() const override;

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    SolverWizard* m_solverWizard;
    MathConfig* m_cfg;

    QCheckBox *m_consistentCheck;
    QDoubleSpinBox *m_pRefValueSpin;
    QSpinBox *m_nNonOrthogonalCorrectorsSpin, *m_pRefCellSpin;
    QTableWidget* m_resTable;
};

#endif // PAGE_60_SIMPLE_H
