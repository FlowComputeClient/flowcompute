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

#ifndef DIALOGS_RUN_SOLVER_RUN_SOLVER_DIALOG_H_
#define DIALOGS_RUN_SOLVER_RUN_SOLVER_DIALOG_H_

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QWidget>
#include <QDialogButtonBox>

class SystemManager;
class QStackedWidget;

class RunSolverDialog : public QDialog {
    Q_OBJECT

 public:
    RunSolverDialog(const QString& selectedCase, bool isFoundation,
                    const SystemManager& systemMgr, QWidget* parent = nullptr);

 signals:
    void requestRunSolver(QString caseName, QString cmd);

 private:
    QWidget* createFoundationWidget();
    QWidget* createKeySightWidget();

    const SystemManager& m_systemMgr;
    QString m_solverName;
    bool m_isFoundation;
    QStackedWidget* m_layoutStack;

    QCheckBox *m_potentialCheck, *m_updateVelocityCheck, *m_writePressureCheck;
    QCheckBox *m_runSolverCheck, *m_deleteFilesKeySightCheck, *m_functionCheck,
        *m_reconstructKeySightCheck, *m_deleteProcessorKeySightCheck,
        *m_deleteProcessorFoundationCheck, *m_reconstructFoundationCheck,
        *m_deleteFilesFoundationCheck;
    QComboBox *m_caseCombo, *m_numCoresFoundationCombo, *m_selectSolverCombo,
         *m_numCoresKeySightCombo, *m_fileHandlingCombo;

 private slots:
    void onOkClicked();
    void onCaseChanged(QString caseName);
    void potentialCheckToggled(bool enabled);
    void simulationCheckToggled(bool state);
};

#endif  // DIALOGS_RUN_SOLVER_RUN_SOLVER_DIALOG_H_
