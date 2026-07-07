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

#ifndef PAGE_10_CONTROL_H
#define PAGE_10_CONTROL_H

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
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWizardPage>

#include "../../core_types.h"
#include "solver_structs.h"

class SolverWizard;

class ControlPage : public QWizardPage {
    Q_OBJECT

 public:
    explicit ControlPage(const QString& caseName, const QStringList& cases,
                         const std::vector<FlowCompute::SolverFamily>& families,
                         QWidget *parent);
    int nextId() const override;

 protected:
    void initializePage() override;
    bool validatePage() override;

 private:
    SolverWizard* solverWizard;
    ControlConfig* m_cfg;
    bool m_isSteadyState = true;
    QString m_caseName;

    QIntValidator* m_intValidator;
    QDoubleValidator* m_doubleValidator;

    std::vector<FlowCompute::SolverFamily> m_families;

    QCheckBox *m_compressCheck, *m_modifiableCheck;
    QComboBox *m_caseCombo, *m_familyCombo, *m_solverCombo, *m_startFromCombo,
        *m_stopAtCombo, *m_writeControlCombo, *m_writeFormatCombo;
    QDoubleSpinBox *m_startTimeSpin, *m_endTimeSpin, *m_deltaTSpin;
    QLineEdit *m_writeIntervalEdit;
    QSpinBox* m_purgeWriteSpin;

 private slots:
    void writeControlChanged(int index);
    void familyChanged(int);
    void solverChanged(int);
};

#endif  // PAGE_10_CONTROL_H
