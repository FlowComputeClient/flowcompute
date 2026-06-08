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
    explicit ControlPage(const std::vector<FlowCompute::SolverFamily>& families, QWidget *parent);
    // int nextId() const override;

protected:
    void initializePage() override;
    // bool validatePage() override;

private:
    SolverWizard* solverWizard;
    ControlConfig* m_cfg;

    std::vector<FlowCompute::SolverFamily> m_families;

    QComboBox *familyBox, *solverBox, *startFromBox, *stopAtBox, *writeControlBox;
    QDoubleSpinBox *startTimeBox, *endTimeBox, *maxCourantBox, *writeIntervalBox, *deltaTBox;
    QCheckBox *adjustTimeStepBox;
    QSpinBox* purgeWriteBox;

private slots:
    void familyChanged(int);
    void solverChanged(int);
};

#endif  // PAGE_10_CONTROL_H
