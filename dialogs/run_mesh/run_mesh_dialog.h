#ifndef RUN_MESH_DIALOG_H
#define RUN_MESH_DIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QWidget>
#include <QDialogButtonBox>

class MainWindow;

class RunMeshDialog : public QDialog {
    Q_OBJECT
public:
    RunMeshDialog(const QString& selectedCase,
                  const QStringList& caseList,
                  QWidget* parent = nullptr);

    QCheckBox *m_runBlockMeshCheck, *m_runFeatureExtractCheck, *m_runSnappyHexMeshCheck;
    QCheckBox *m_meshOverwriteCheck, *m_displayMeshCheck, *m_meshReconstructCheck;
    QComboBox *m_meshModeCombo, *m_caseCombo, *m_numCoresCombo;

private:
    MainWindow* m_mainWin;

private slots:
    void onOkClicked();
    void snappyCheckToggled(bool state);
    void snappyHexMeshModeChanged(int index);
    void onCaseChanged(QString caseName);
};

#endif // RUN_MESH_DIALOG_H
