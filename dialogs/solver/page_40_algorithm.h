#ifndef PAGE_40_ALGORITHM_H
#define PAGE_40_ALGORITHM_H

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
#include "simple_widget.h"
#include "pimple_widget.h"
#include "piso_widget.h"

class SolverWizard;

class AlgorithmPage : public QWizardPage {
    Q_OBJECT

public:
    explicit AlgorithmPage(QWidget *parent);
    // int nextId() const override;

protected:
    void initializePage() override;
    // bool validatePage() override;

private:
    SolverWizard* solverWizard;
    MathConfig* m_cfg;
    QString m_currentField;

    QLabel *preconditionerSmootherLabel, *algorithmLabel;
    QComboBox *fieldCombo, *solverCombo, *preconditionerSmootherCombo;
    QDoubleSpinBox *relaxationSpin;
    QLineEdit *absTolEdit, *relTolEdit;
    QSpinBox *numCorrectorsSpin, *nonOrthogonalSpin, *refCellSpin, *refValueSpin;
    QStackedWidget *algoStack;
    QWidget *simpleWidget, *pimpleWidget, *pisoWidget;

    FlowCompute::Algorithm m_algorithm;

private slots:
    void fieldChanged(int);
    void solverChanged(int);
};

#endif  // PAGE_40_ALGORITHM_H
