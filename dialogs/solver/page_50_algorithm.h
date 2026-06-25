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
