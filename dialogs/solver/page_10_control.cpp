#include "page_10_control.h"

#include "wizard_solver.h"

// Introduction page asks for the case name and platform
ControlPage::ControlPage(const QList<FlowCompute::SolverFamily>& families,
                         QWidget *parent): QWizardPage(parent), m_families(families) {

    // Set title and style
    setTitle(tr("Control Configuration"));

    // Create a grid layout with two columns
    QFormLayout* layout = new QFormLayout(this);
    layout->setSpacing(20);

    // Get solver family
    familyBox = new QComboBox(this);
    layout->addRow(tr("Select solver category:"), familyBox);
    connect(familyBox, &QComboBox::currentIndexChanged,
            this, &ControlPage::familyChanged);

    // Select solver
    solverBox = new QComboBox(this);
    layout->addRow(tr("Select solver:"), solverBox);
    connect(solverBox, &QComboBox::currentIndexChanged,
            this, &ControlPage::solverChanged);

    // ---------------------------------------------------------
    // Solution Timing Group
    // ---------------------------------------------------------
    QGroupBox* timingBox = new QGroupBox(tr("Solution Timing"), this);
    layout->addRow(timingBox);
    QFormLayout* timingLayout = new QFormLayout(timingBox);

    startFromBox = new QComboBox(this);
    startFromBox->addItems({ "startTime", "firstTime", "latestTime" });
    timingLayout->addRow(tr("Start time source: "), startFromBox);

    startTimeBox = new QDoubleSpinBox(this);
    startTimeBox->setRange(0.0, 1e6);
    startTimeBox->setDecimals(5);
    timingLayout->addRow(tr("Start time: "), startTimeBox);

    stopAtBox = new QComboBox(this);
    stopAtBox->addItems({ "endTime", "writeNow", "noWriteNow", "nextWrite" });
    timingLayout->addRow(tr("Stop time source: "), stopAtBox);

    endTimeBox = new QDoubleSpinBox(this);
    endTimeBox->setRange(0.0, 1e6);
    endTimeBox->setDecimals(5);
    endTimeBox->setValue(0.5);
    timingLayout->addRow(tr("End time: "), endTimeBox);

    // Added: deltaT (Required by OpenFOAM)
    deltaTBox = new QDoubleSpinBox(this);
    deltaTBox->setRange(1e-8, 1e4);
    deltaTBox->setDecimals(6);
    deltaTBox->setValue(1.0);
    timingLayout->addRow(tr("Time step (deltaT): "), deltaTBox);

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

    // ---------------------------------------------------------
    // Writing Output Group
    // ---------------------------------------------------------
    QGroupBox* writeBox = new QGroupBox(tr("Writing Output"), this);
    layout->addRow(writeBox);
    QFormLayout* writeLayout = new QFormLayout(writeBox);

    writeControlBox = new QComboBox(this);
    writeControlBox->addItems({ "timeStep", "runTime", "adjustableRunTime",
                               "cpuTime", "clockTime", "none" });
    writeLayout->addRow(tr("Write control: "), writeControlBox);

    writeIntervalBox = new QDoubleSpinBox(this);
    writeIntervalBox->setRange(1e-5, 1e6);
    writeIntervalBox->setDecimals(4);
    writeIntervalBox->setValue(20.0);
    writeLayout->addRow(tr("Write interval: "), writeIntervalBox);

    purgeWriteBox = new QSpinBox(this);
    purgeWriteBox->setRange(0, 100);
    purgeWriteBox->setValue(0);
    writeLayout->addRow(tr("Purge write (0 = keep all): "), purgeWriteBox);

    // ---------------------------------------------------------
    // Field Registration
    // ---------------------------------------------------------
    registerField("solverFamily", familyBox, "currentText");
    registerField("solverName", solverBox, "currentText");

    registerField("startFrom", startFromBox, "currentText");
    registerField("startTime", startTimeBox);
    registerField("stopAt", stopAtBox, "currentText");
    registerField("endTime", endTimeBox);
    registerField("deltaT", deltaTBox);

    registerField("adjustTimeStep", adjustTimeStepBox);
    registerField("maxCo", maxCourantBox);

    registerField("writeControl", writeControlBox, "currentText");
    registerField("writeInterval", writeIntervalBox);
    registerField("purgeWrite", purgeWriteBox);

    setLayout(layout);
}

void ControlPage::initializePage() {

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

// Populate list of solvers based on solver family
void ControlPage::familyChanged(int index) {
    solverBox->clear();
    if (index >= 0 && index < m_families.size()) {
        for (const auto& solver: std::as_const(m_families[index].solvers)) {
            solverBox->addItem(solver.name);
        }
    }
}

// Update values when a new solver is chosen
void ControlPage::solverChanged(int index) {

    // Check combo box states
    int familyIndex = familyBox->currentIndex();
    if (familyIndex < 0 || familyIndex >= m_families.size()) return;
    if (index < 0 || index >= m_families[familyIndex].solvers.size()) return;

    // Get SolverDetails for the selected family and solver
    const auto& details = m_families[familyIndex].solvers[index];

    // Configure transient-only UI elements
    adjustTimeStepBox->setEnabled(!details.isSteadyState);
    maxCourantBox->setEnabled(!details.isSteadyState);
}