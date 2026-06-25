#ifndef PAGE_70_PIMPLE_H
#define PAGE_70_PIMPLE_H

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QWizardPage>
#include <QSpinBox>
#include <QTableWidget>

#include "solver_structs.h"

class SolverWizard;

class PimplePage : public QWizardPage {
    Q_OBJECT

public:
    explicit PimplePage(QWidget *parent = nullptr);
    int nextId() const override;

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    SolverWizard* m_solverWizard;
    MathConfig* m_cfg;

    QSpinBox *m_nOuterCorrectorsSpin, *m_nCorrectorsSpin, *m_nNonOrthogonalCorrectorsSpin, *m_pRefCellSpin;
    QDoubleSpinBox* m_pRefValueSpin;
    QTableWidget* m_resTable;
};

#endif // PAGE_70_PIMPLE_H
