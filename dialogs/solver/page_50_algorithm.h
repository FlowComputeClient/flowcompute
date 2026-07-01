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

#ifndef PAGE_50_ALGORITHM_H
#define PAGE_50_ALGORITHM_H

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
#include <QMetaEnum>
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWizardPage>

#include "../../core_types.h"
#include "solver_structs.h"

class SolverWizard;

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
    MathConfig* m_cfg;
    QString m_currentField;
    FieldMathConfig* m_currentMathConfig;

    QButtonGroup *m_relaxGroup;
    QCheckBox *m_finalIterationCheck;
    QComboBox *m_fieldCombo, *m_solverCombo, *m_preconditionerSmootherCombo;
    QDoubleSpinBox *m_relaxationSpin;
    QLabel *m_preconditionerSmootherLabel, *m_algorithmLabel;
    QLineEdit *m_absTolEdit, *m_relTolEdit, *m_finalAbsTolEdit, *m_finalRelTolEdit;
    QListWidget *m_fieldListWidget;
    QRadioButton *m_fieldsRadio, *m_equationsRadio;
    QSpinBox *m_numCorrectorsSpin, *m_nonOrthogonalSpin, *m_refCellSpin, *m_refValueSpin;
    QStackedWidget *m_solverStack;

    FlowCompute::Algorithm m_algorithm;

    /*
    SimpleWidget *m_simpleWidget;
    PimpleWidget *m_pimpleWidget;
    PisoWidget *m_pisoWidget;
    */

private slots:
    void fieldSelectionChanged();
    void solverChanged(int);
};

#endif  // PAGE_50_ALGORITHM_H
