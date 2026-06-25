#ifndef PAGE_60_SIMPLE_H
#define PAGE_60_SIMPLE_H

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QWizardPage>
#include <QSpinBox>
#include <QTableWidget>

#include "solver_structs.h"

class SolverWizard;

class SimplePage : public QWizardPage {
    Q_OBJECT

public:
    explicit SimplePage(QWidget *parent = nullptr);
    int nextId() const override;

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    SolverWizard* m_solverWizard;
    MathConfig* m_cfg;

    QCheckBox *m_consistentCheck;
    QDoubleSpinBox *m_pRefValueSpin;
    QSpinBox *m_nNonOrthogonalCorrectorsSpin, *m_pRefCellSpin;
    QTableWidget* m_resTable;
};

#endif // PAGE_60_SIMPLE_H
