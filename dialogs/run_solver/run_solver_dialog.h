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

#include <QDialog>

class SystemManager;
class QCheckBox;
class QComboBox;
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
    bool m_isFoundation, m_isMultiPhase, m_isBuoyant, m_isCompressible,
        m_isTransient;
    QStackedWidget* m_layoutStack;

    QCheckBox *m_potentialCheck, *m_updateVelocityCheck, *m_writePressureCheck;
    QCheckBox *m_runSolverCheck, *m_deleteFilesCheck, *m_functionCheck,
        *m_reconstructCheck, *m_deleteProcessorCheck;
    QComboBox *m_caseCombo, *m_numCoresFoundationCombo, *m_selectSolverCombo,
         *m_numCoresCombo, *m_fileHandlingCombo;

 private slots:
    void onOkClicked();
    QByteArray addPhiBlock(const QByteArray& fvSolutionContent);
    void onCaseChanged(const QString& caseName);
    void potentialCheckToggled(bool enabled);
    void simulationCheckToggled(bool state);
};

#endif  // DIALOGS_RUN_SOLVER_RUN_SOLVER_DIALOG_H_
