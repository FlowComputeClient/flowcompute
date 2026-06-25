#include "page_10_control.h"

#include "wizard_solver.h"

// Introduction page asks for the case name and platform
ControlPage::ControlPage(const QString& caseName, const QStringList& cases,
                         const std::vector<FlowCompute::SolverFamily>& families,
                         QWidget *parent):
    QWizardPage(parent), m_caseName(caseName), m_families(families) {

    // Set title and style
    setTitle(tr("Control Configuration (controlDict)"));

    // Create a grid layout with two columns
    QFormLayout* layout = new QFormLayout(this);
    layout->setSpacing(15);

    // Get selected case
    m_caseCombo = new QComboBox(this);
    m_caseCombo->addItems(cases);
    m_caseCombo->setCurrentText(caseName);
    layout->addRow(tr("Select the OpenFOAM case:"), m_caseCombo);
    connect(m_caseCombo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        m_caseName = text;
    });

    // Get solver family
    m_familyCombo = new QComboBox(this);
    layout->addRow(tr("Select solver category:"), m_familyCombo);
    connect(m_familyCombo, &QComboBox::currentIndexChanged,
            this, &ControlPage::familyChanged);

    // Select solver
    m_solverCombo = new QComboBox(this);
    layout->addRow(tr("Select solver:"), m_solverCombo);
    connect(m_solverCombo, &QComboBox::currentIndexChanged,
            this, &ControlPage::solverChanged);

    // ---------------------------------------------------------
    // Solution Timing Group
    // ---------------------------------------------------------
    QGroupBox* timingGroup = new QGroupBox(tr("Solution Timing"), this);
    layout->addRow(timingGroup);
    QFormLayout* timingLayout = new QFormLayout(timingGroup);
    timingLayout->setSpacing(10);

    m_startFromCombo = new QComboBox(this);
    QMetaEnum metaEnum = QMetaEnum::fromType<Solver::StartSolverType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_startFromCombo->addItem(metaEnum.key(i));
    }
    timingLayout->addRow(tr("Start time source: "), m_startFromCombo);
    connect(m_startFromCombo, &QComboBox::currentTextChanged, this, [this](const QString& text){
        m_startTimeSpin->setEnabled(text == "startTime");
    });

    m_startTimeSpin = new QDoubleSpinBox(this);
    m_startTimeSpin->setRange(0.0, 1e6);
    m_startTimeSpin->setDecimals(5);
    timingLayout->addRow(tr("Start time: "), m_startTimeSpin);

    m_stopAtCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<Solver::EndSolverType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_stopAtCombo->addItem(metaEnum.key(i));
    }
    timingLayout->addRow(tr("Stop time source: "), m_stopAtCombo);
    connect(m_stopAtCombo, &QComboBox::currentTextChanged, this, [this](const QString& text){
        m_endTimeSpin->setEnabled(text == "endTime");
    });

    m_endTimeSpin = new QDoubleSpinBox(this);
    m_endTimeSpin->setRange(0.0, 1e6);
    m_endTimeSpin->setDecimals(5);
    m_endTimeSpin->setValue(0.5);
    timingLayout->addRow(tr("End time: "), m_endTimeSpin);

    m_deltaTSpin = new QDoubleSpinBox(this);
    m_deltaTSpin->setRange(1e-8, 1e4);
    m_deltaTSpin->setDecimals(6);
    m_deltaTSpin->setValue(1.0);
    timingLayout->addRow(tr("Time step (deltaT): "), m_deltaTSpin);

    // ---------------------------------------------------------
    // Writing Output Group
    // ---------------------------------------------------------
    QGroupBox* writeGroup = new QGroupBox(tr("Writing Output"), this);
    layout->addRow(writeGroup);
    QFormLayout* writeLayout = new QFormLayout(writeGroup);
    writeLayout->setSpacing(10);

    // Check boxes
    auto* checkLayout = new QHBoxLayout;
    m_modifiableCheck = new QCheckBox(tr("Enable runtime modification"), this);
    checkLayout->addWidget(m_modifiableCheck);
    m_compressCheck = new QCheckBox(tr("Compress output"), this);
    checkLayout->addWidget(m_compressCheck);
    writeLayout->addRow(checkLayout);

    m_writeFormatCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<Solver::WriteFormatType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_writeFormatCombo->addItem(metaEnum.key(i));
    }
    writeLayout->addRow(tr("Write format: "), m_writeFormatCombo);

    m_writeControlCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<Solver::WriteControlType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_writeControlCombo->addItem(metaEnum.key(i));
    }
    writeLayout->addRow(tr("Write control: "), m_writeControlCombo);
    connect(m_writeControlCombo, &QComboBox::currentIndexChanged,
            this, &ControlPage::writeControlChanged);

    m_writeIntervalEdit = new QLineEdit(this);
    writeLayout->addRow(tr("Write interval: "), m_writeIntervalEdit);

    // Create validator for write interval
    m_intValidator = new QIntValidator(1, 9999999, this);
    m_doubleValidator = new QDoubleValidator(0.0, 999999.0, 6, this);
    m_doubleValidator->setLocale(QLocale::C);

    m_purgeWriteSpin = new QSpinBox(this);
    m_purgeWriteSpin->setRange(0, 100);
    m_purgeWriteSpin->setValue(0);
    writeLayout->addRow(tr("Purge write (0 = keep all): "), m_purgeWriteSpin);

    // Register case name
    registerField("caseName", m_caseCombo, "currentText");

    // Set the initial validator based on the default combo box state
    writeControlChanged(m_writeControlCombo->currentIndex());

    setLayout(layout);
}

void ControlPage::initializePage() {

    // Access wizard data
    solverWizard = qobject_cast<SolverWizard*>(this->wizard());
    m_cfg = &(solverWizard->getControlConfig());

    // Update fields
    m_startFromCombo->setCurrentIndex(static_cast<int>(m_cfg->startFrom));
    m_startTimeSpin->setValue(m_cfg->startTime);
    m_stopAtCombo->setCurrentIndex(static_cast<int>(m_cfg->stopAt));
    m_endTimeSpin->setValue(m_cfg->endTime);
    m_deltaTSpin->setValue(m_cfg->deltaT);
    m_compressCheck->setChecked(m_cfg->writeCompression);
    m_modifiableCheck->setChecked(m_cfg->runTimeModifiable);
    m_writeFormatCombo->setCurrentIndex(static_cast<int>(m_cfg->writeFormat));
    m_writeControlCombo->setCurrentIndex(static_cast<int>(m_cfg->writeControl));
    m_writeIntervalEdit->setText(QString::number(m_cfg->writeInterval));
    m_purgeWriteSpin->setValue(m_cfg->purgeWrite);

    // Update families and solvers
    for (const auto& family: std::as_const(m_families)) {
        m_familyCombo->addItem(family.name);
    }
    if (m_cfg->solverCategory.isEmpty()) {
        m_familyCombo->setCurrentIndex(0);
    } else {
        m_familyCombo->setCurrentText(m_cfg->solverCategory);
    }
    if (m_cfg->solver.isEmpty()) {
        m_solverCombo->setCurrentIndex(0);
    } else {
        m_solverCombo->setCurrentText(m_cfg->solver);
    }
}

bool ControlPage::validatePage() {

    // Update wizard's case name
    solverWizard->setCaseName(m_caseName);

    if (!m_cfg) return false;
    m_cfg->solverCategory = m_familyCombo->currentText();
    m_cfg->solver = m_solverCombo->currentText();

    // Timing fields
    m_cfg->startFrom = static_cast<Solver::StartSolverType>(m_startFromCombo->currentIndex());
    m_cfg->startTime = m_startTimeSpin->value();
    m_cfg->stopAt = static_cast<Solver::EndSolverType>(m_stopAtCombo->currentIndex());
    m_cfg->endTime = m_endTimeSpin->value();
    m_cfg->deltaT = m_deltaTSpin->value();

    // Write output fields
    m_cfg->writeCompression = m_compressCheck->isChecked();
    m_cfg->runTimeModifiable = m_modifiableCheck->isChecked();
    m_cfg->writeFormat = static_cast<Solver::WriteFormatType>(m_writeFormatCombo->currentIndex());
    m_cfg->writeControl = static_cast<Solver::WriteControlType>(m_writeControlCombo->currentIndex());
    m_cfg->writeInterval = m_writeIntervalEdit->text().toDouble();
    m_cfg->purgeWrite = m_purgeWriteSpin->value();

    // Check if write interval is empty
    if (m_writeIntervalEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Input"), tr("Please enter a valid write interval."));
        m_writeIntervalEdit->setFocus();
        return false;
    }

    return true;
}

// Set next page of the wizard
int ControlPage::nextId() const {
    if (m_isSteadyState) {
        return Page_Physics;
    } else {
        return Page_Transient;
    }
}

// Populate list of solvers based on solver family
void ControlPage::familyChanged(int index) {
    m_solverCombo->clear();
    if (index >= 0 && index < m_families.size()) {
        for (const auto& solver: std::as_const(m_families[index].solvers)) {
            m_solverCombo->addItem(solver.name);
        }
    }
}

// Update values when a new solver is chosen
void ControlPage::solverChanged(int index) {

    // Check combo box states
    int familyIndex = m_familyCombo->currentIndex();
    if (familyIndex < 0 || familyIndex >= m_families.size()) return;
    if (index < 0 || index >= m_families[familyIndex].solvers.size()) return;

    // Determine if the solver is steady state
    m_isSteadyState = m_families[familyIndex].solvers[index].isSteadyState;

    // Lock timeStep field for steady-state simulation
    if (m_isSteadyState) {
        int timeStepIndex = m_writeControlCombo->findText("timeStep");
        if (timeStepIndex != -1) {
            m_writeControlCombo->setCurrentIndex(timeStepIndex);
        }
        m_writeControlCombo->setEnabled(false);
    } else {
        m_writeControlCombo->setEnabled(true);
    }
}

void ControlPage::writeControlChanged(int index) {
    QString controlText = m_writeControlCombo->itemText(index);

    // If writing by timeStep, enforce integers. Otherwise, allow decimals.
    if (controlText == "timeStep") {
        m_writeIntervalEdit->setValidator(m_intValidator);

        // Optional UX polish: Strip decimals if the user already typed them
        QString currentText = m_writeIntervalEdit->text();
        if (currentText.contains('.')) {
            m_writeIntervalEdit->setText(QString::number(currentText.toDouble(), 'f', 0));
        }
    } else {
        m_writeIntervalEdit->setValidator(m_doubleValidator);
    }
}