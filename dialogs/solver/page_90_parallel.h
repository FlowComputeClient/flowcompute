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
