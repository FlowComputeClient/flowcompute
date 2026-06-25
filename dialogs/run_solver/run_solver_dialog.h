#ifndef RUN_SOLVER_DIALOG_H
#define RUN_SOLVER_DIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QWidget>
#include <QDialogButtonBox>

class MainWindow;

class RunSolverDialog : public QDialog {
    Q_OBJECT
public:
    RunSolverDialog(const QString& selectedCase,
                  const QStringList& caseList,
                  QWidget* parent = nullptr);

private:
    MainWindow* m_mainWin;
    QString m_solverName;

    QCheckBox *m_potentialCheck, *m_updateVelocityCheck, *m_writePressureCheck;
    QCheckBox *m_runSolverCheck, *m_deleteFilesCheck, *m_reconstructCheck, *m_deleteProcessorCheck;
    QComboBox *m_caseCombo, *m_numCoresCombo;

private slots:
    void onOkClicked();
    void onCaseChanged(QString caseName);
    void potentialCheckToggled(bool enabled);
    void simulationCheckToggled(bool state);
};

#endif // RUN_SOLVER_DIALOG_H
