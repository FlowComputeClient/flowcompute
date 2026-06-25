#include "page_80_piso.h"

#include "wizard_solver.h"

PisoPage::PisoPage(QWidget *parent): QWizardPage(parent) {

    // Set title
    setTitle(tr("Piso Algorithm Configuration (fvSolution)"));

    // Create layout
    QFormLayout* layout = new QFormLayout(this);
    layout->setSpacing(20);
    setLayout(layout);

    // Correctors
    m_nCorrectorsSpin = new QSpinBox(this);
    m_nCorrectorsSpin->setRange(1, 100);
    layout->addRow(tr("Number of correctors: "), m_nCorrectorsSpin);

    // Non-orthogonal correctors
    m_nNonOrthogonalCorrectorsSpin = new QSpinBox(this);
    m_nNonOrthogonalCorrectorsSpin->setRange(0, 50);
    layout->addRow(tr("Number of non-orthogonal correctors: "), m_nNonOrthogonalCorrectorsSpin);

    // Ref Cell
    m_pRefCellSpin = new QSpinBox(this);
    m_pRefCellSpin->setRange(0, INT_MAX);
    layout->addRow(tr("Reference cell index for pressure: "), m_pRefCellSpin);

    // Pressure at reference cell
    m_pRefValueSpin = new QDoubleSpinBox(this);
    m_pRefValueSpin->setRange(-1e9, 1e9);
    m_pRefValueSpin->setDecimals(5);
    layout->addRow(tr("Pressure at reference cell: "), m_pRefValueSpin);

    layout->addItem(new QSpacerItem(0, 0,
        QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void PisoPage::initializePage() {

    m_solverWizard = qobject_cast<SolverWizard*>(this->wizard());
    if (!m_solverWizard) { return; }

    // Access field data and boundaries
    m_cfg = &(m_solverWizard->getMathConfig());
    PisoConfig& cfg = std::get<PisoConfig>(m_cfg->algorithmConfig);

    m_nCorrectorsSpin->setValue(cfg.nCorrectors);
    m_nNonOrthogonalCorrectorsSpin->setValue(cfg.nNonOrthogonalCorrectors);
    m_pRefCellSpin->setValue(cfg.pRefCell);
    m_pRefValueSpin->setValue(cfg.pRefValue);
}

bool PisoPage::validatePage() {
    if (!m_cfg) return false;

    // Update state data
    PisoConfig& cfg = std::get<PisoConfig>(m_cfg->algorithmConfig);
    cfg.nCorrectors = m_nCorrectorsSpin->value();
    cfg.nNonOrthogonalCorrectors = m_nNonOrthogonalCorrectorsSpin->value();
    cfg.pRefCell = m_pRefCellSpin->value();
    cfg.pRefValue = m_pRefValueSpin->value();

    return true;
}

// Set next page of the wizard
int PisoPage::nextId() const {
    return Page_Parallel;
}