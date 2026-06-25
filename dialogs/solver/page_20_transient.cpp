#include "page_20_transient.h"

// Introduction page asks for the case name and platform
TransientPage::TransientPage(const std::vector<FlowCompute::SolverFamily>& families,
                         QWidget *parent): QWizardPage(parent), m_families(families) {

    // Set title and style
    setTitle(tr("Transient Simulation Configuration"));

    // Create a grid layout with two columns
    QFormLayout* layout = new QFormLayout(this);
    layout->setSpacing(20);

    // ---------------------------------------------------------
    // Time Step Adjustment Group
    // ---------------------------------------------------------
    QGroupBox* stepBox = new QGroupBox(tr("Time Step Adjustment"), this);
    layout->addRow(stepBox);
    QFormLayout* stepLayout = new QFormLayout(stepBox);

    adjustTimeStepBox = new QCheckBox("Adjust time step (Courant number driven)", this);
    stepLayout->addRow(adjustTimeStepBox);

    maxCourantBox = new QDoubleSpinBox(this);
    maxCourantBox->setRange(0.1, 20.0);
    maxCourantBox->setDecimals(2);
    maxCourantBox->setSingleStep(0.1);
    maxCourantBox->setValue(1.0);
    stepLayout->addRow(tr("Max Courant number (maxCo): "), maxCourantBox);

    registerField("adjustTimeStep", adjustTimeStepBox);
    registerField("maxCo", maxCourantBox);

    setLayout(layout);
}

/*
void TransientPage::initializePage() {

    // Access the parsed structure
    solverWizard = qobject_cast<SolverWizard*>(this->wizard());
    m_cfg = &(solverWizard->getControlConfig());

    // Update fields
    startFromBox->setCurrentIndex(static_cast<int>(m_cfg->startFrom));
    startTimeBox->setValue(m_cfg->startTime);
    stopAtBox->setCurrentIndex(static_cast<int>(m_cfg->stopAt));
    endTimeBox->setValue(m_cfg->endTime);
    deltaTBox->setValue(m_cfg->deltaT);
    adjustTimeStepBox->setChecked(m_cfg->adjustTimeStep);
    maxCourantBox->setValue(m_cfg->maxCo);
    writeControlBox->setCurrentIndex(static_cast<int>(m_cfg->writeControl));
    writeIntervalBox->setValue(m_cfg->writeInterval);
    purgeWriteBox->setValue(m_cfg->purgeWrite);

    // Update families and solvers
    for (const auto& family: std::as_const(m_families)) {
        familyBox->addItem(family.familyName);
    }
    if (m_cfg->solverCategory.isEmpty()) {
        familyBox->setCurrentIndex(0);
    } else {
        familyBox->setCurrentText(m_cfg->solverCategory);
    }
    if (m_cfg->solver.isEmpty()) {
        solverBox->setCurrentIndex(0);
    } else {
        solverBox->setCurrentText(m_cfg->solver);
    }
}
*/