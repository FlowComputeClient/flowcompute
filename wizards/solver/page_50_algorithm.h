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

#ifndef WIZARDS_SOLVER_PAGE_50_ALGORITHM_H_
#define WIZARDS_SOLVER_PAGE_50_ALGORITHM_H_

#include <QWizardPage>

#include "core_types.h"
#include "parser/fv_solution.h"

class SolverWizard;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QListWidget;
class QRadioButton;
class QSpinBox;
class QStackedWidget;

class AlgorithmPage : public QWizardPage {
    Q_OBJECT

 public:
    explicit AlgorithmPage(QWidget *parent);
    int nextId() const override;

 protected:
    void initializePage() override;
    // bool validatePage() override;

 private:
    SolverWizard* m_solverWizard;
    CaseIO::MathConfig* m_cfg;
    QString m_currentField;
    CaseIO::FieldMathConfig* m_currentMathConfig;

    QButtonGroup *m_relaxGroup;
    QCheckBox *m_finalIterationCheck;
    QComboBox *m_fieldCombo, *m_solverCombo, *m_preconditionerSmootherCombo;
    QDoubleSpinBox *m_relaxationSpin;
    QLabel *m_preconditionerSmootherLabel, *m_algorithmLabel;
    QLineEdit *m_absTolEdit, *m_relTolEdit, *m_finalAbsTolEdit,
        *m_finalRelTolEdit;
    QListWidget *m_fieldListWidget;
    QRadioButton *m_fieldsRadio, *m_equationsRadio;
    QSpinBox *m_numCorrectorsSpin, *m_nonOrthogonalSpin, *m_refCellSpin,
        *m_refValueSpin;
    QStackedWidget *m_solverStack;

    FlowCompute::Algorithm m_algorithm;

 private slots:
    void fieldSelectionChanged();
    void solverChanged(int);
};

#endif  // WIZARDS_SOLVER_PAGE_50_ALGORITHM_H_
