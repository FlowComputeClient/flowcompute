#ifndef PAGE_80_PISO_H
#define PAGE_80_PISO_H

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QWizardPage>
#include <QSpinBox>

#include "solver_structs.h"

class SolverWizard;

class PisoPage : public QWizardPage {
    Q_OBJECT

public:
    explicit PisoPage(QWidget *parent = nullptr);
    int nextId() const override;

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    SolverWizard* m_solverWizard;
    MathConfig* m_cfg;

    QDoubleSpinBox* m_pRefValueSpin;
    QSpinBox *m_nCorrectorsSpin, *m_nNonOrthogonalCorrectorsSpin, *m_pRefCellSpin;
};

#endif // PAGE_80_PISO_H
