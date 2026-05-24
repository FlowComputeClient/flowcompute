#ifndef PAGE_80_EXECUTION_H
#define PAGE_80_EXECUTION_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
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
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWizardPage>

class MainWindow;
class MeshWizard;

class ExecutionPage : public QWizardPage {
    Q_OBJECT

public:
    explicit ExecutionPage(QWidget *parent);

protected:
    void initializePage() override;
    // bool validatePage() override;

private:
    MeshWizard* meshWizard;
    QWidget *m_snappyOptionsWidget;
    QGroupBox *m_blockMeshGroup, *m_extractGroup, *m_snappyGroup, *m_postGroup;
    QCheckBox *m_runBlockMeshBox, *m_runFeatureExtractBox, *m_runSnappyHexMeshBox;
    QCheckBox *m_snappyHexMeshOverwriteBox, *m_checkMeshBox, *m_displayMeshBox;
    QComboBox *m_snappyHexMeshModeBox;

private slots:
    void snappyHexMeshModeChanged();
};

#endif  // PAGE_80_EXECUTION_H
