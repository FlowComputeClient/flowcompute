#include "page_10_control.h"

#include "wizard_solver.h"

// Introduction page asks for the case name and platform
AlgorithmPage::AlgorithmPage(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Algorithm Configuration"));

    // Create a grid layout with two columns
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);

    // Solver group
    QGroupBox* solverGroup = new QGroupBox(tr("Field Solver Configuration"), this);
    layout->addWidget(solverGroup);
    QFormLayout* solverLayout = new QFormLayout(solverGroup);
    solverLayout->setSpacing(20);

    // Set field
    fieldCombo = new QComboBox(solverGroup);
    connect(fieldCombo, &QComboBox::currentIndexChanged, this, &AlgorithmPage::fieldChanged);
    solverLayout->addRow(tr("Select field for configuration: "), fieldCombo);

    // Set solver
    solverCombo = new QComboBox(solverGroup);
    QMetaEnum solverEnum = QMetaEnum::fromType<FlowCompute::LinearSolver>();
    for (int i = 0; i < solverEnum.keyCount(); ++i) {
        solverCombo->addItem(solverEnum.key(i), solverEnum.value(i));
    }
    connect(solverCombo, &QComboBox::currentIndexChanged, this, &AlgorithmPage::solverChanged);
    solverLayout->addRow(tr("Solver: "), solverCombo);

    // Preconditioner/smoother
    preconditionerSmootherLabel = new QLabel(tr("Preconditioner: "), solverGroup);
    preconditionerSmootherCombo = new QComboBox(solverGroup);
    solverLayout->addRow(preconditionerSmootherLabel, preconditionerSmootherCombo);

    // Tolerance
    absTolEdit = new QLineEdit(solverGroup);
    solverLayout->addRow(tr("Absolute tolerance: "), absTolEdit);
    relTolEdit = new QLineEdit(solverGroup);
    solverLayout->addRow(tr("Relative tolerance: "), relTolEdit);

    // Relaxation factor
    relaxationSpin = new QDoubleSpinBox(solverGroup);
    relaxationSpin->setRange(0.0, 1.0);
    relaxationSpin->setSingleStep(0.1);
    solverLayout->addRow(tr("Relaxation factor: "), relaxationSpin);

    // Algorithm group
    QGroupBox* algorithmGroup = new QGroupBox(tr("Algorithm Configuration"), this);
    layout->addWidget(algorithmGroup);
    QVBoxLayout* algorithmLayout = new QVBoxLayout(algorithmGroup);

    // Label - displays name of algorithm to be configured
    algorithmLabel = new QLabel(algorithmGroup);
    algorithmLayout->addWidget(algorithmLabel);

    // Create stacked widget
    algoStack = new QStackedWidget;
    algorithmLayout->addWidget(algoStack);

    // Add widgets for different algorithms
    simpleWidget = new SimpleWidget(algoStack);
    algoStack->addWidget(simpleWidget);
    pimpleWidget = new PimpleWidget(algoStack);
    algoStack->addWidget(pimpleWidget);
    pisoWidget = new PisoWidget(algoStack);
    algoStack->addWidget(pisoWidget);

    setLayout(layout);
}

void AlgorithmPage::initializePage() {
    solverWizard = qobject_cast<SolverWizard*>(this->wizard());
    if (!solverWizard) return;
    m_cfg = &(solverWizard->getMathConfig());

    // 1. Block signals and populate fields cleanly
    QStringList fields = solverWizard->getConfiguredFields();
    fieldCombo->blockSignals(true);
    fieldCombo->clear();
    fieldCombo->addItems(fields);
    fieldCombo->blockSignals(false);

    // 2. Pre-fill the background data structure with safe defaults
    for (const QString& f : std::as_const(fields)) {
        if (!m_cfg->fieldMathConfigs.contains(f)) {
            FieldMathConfig defaultCfg;
            defaultCfg.solver = FlowCompute::LinearSolver::GAMG;
            defaultCfg.absTolerance = 1e-6;
            defaultCfg.relTolerance = 0.1;
            defaultCfg.relaxationFactor = (f == "p") ? 0.3 : 0.7;

            m_cfg->fieldMathConfigs.insert(f, defaultCfg);
        }
    }

    // Update layout according to solver algorithm
    m_algorithm = solverWizard->getSolverAlgorithm();
    QMetaEnum algorithmEnum = QMetaEnum::fromType<FlowCompute::Algorithm>();
    int algoIndex = static_cast<int>(m_algorithm);
    QString algoName = algorithmEnum.valueToKey(algoIndex);
    algorithmLabel->setText(tr("Settings for the %1 algorithm:").arg(algoName));

    // Update stacked widget
    if (algoIndex < 3) {
        algoStack->setCurrentIndex(algoIndex);
    }

    // 3. Trigger the first UI load
    m_currentField = ""; // Add this to your header!
    if (!fields.isEmpty()) {
        fieldChanged(0);
    }
}

// Respond when a field is selected
void AlgorithmPage::fieldChanged(int index) {
    if (index < 0) return;

    QString newField = fieldCombo->itemText(index);
    if (newField.isEmpty() || newField == m_currentField) return;

    // ==========================================
    // STEP A: Save the state of the PREVIOUS field
    // ==========================================
    if (!m_currentField.isEmpty()) {
        FieldMathConfig& oldCfg = m_cfg->fieldMathConfigs[m_currentField];

        // Save Solver
        oldCfg.solver = static_cast<FlowCompute::LinearSolver>(solverCombo->currentData().toInt());

        // Save Preconditioner OR Smoother
        if (oldCfg.solver == FlowCompute::LinearSolver::PCG ||
            oldCfg.solver == FlowCompute::LinearSolver::PBiCG ||
            oldCfg.solver == FlowCompute::LinearSolver::PBiCGStab) {
            oldCfg.preconditioner = static_cast<FlowCompute::Preconditioner>(preconditionerSmootherCombo->currentData().toInt());
        }
        else if (oldCfg.solver == FlowCompute::LinearSolver::smoothSolver) {
            oldCfg.smoother = static_cast<FlowCompute::Smoother>(preconditionerSmootherCombo->currentData().toInt());
        }

        // Save Tolerances (Converting string to double)
        oldCfg.absTolerance = absTolEdit->text().toDouble();
        oldCfg.relTolerance = relTolEdit->text().toDouble();

        // Save Relaxation Factor
        oldCfg.relaxationFactor = relaxationSpin->value();
    }

    // ==========================================
    // STEP B: Update the state tracker
    // ==========================================
    m_currentField = newField;
    FieldMathConfig& newCfg = m_cfg->fieldMathConfigs[m_currentField];

    // ==========================================
    // STEP C: Load the state of the NEW field
    // ==========================================

    // 1. Set Solver (NOTE: This automatically triggers solverChanged()!)
    int solverIdx = solverCombo->findData(static_cast<int>(newCfg.solver));
    if (solverIdx >= 0) solverCombo->setCurrentIndex(solverIdx);

    // 2. Because solverChanged() just ran, preconditionerSmootherCombo is freshly populated.
    // We can now safely set its index based on the newly loaded solver.
    if (newCfg.solver == FlowCompute::LinearSolver::smoothSolver) {
        int smIdx = preconditionerSmootherCombo->findData(static_cast<int>(newCfg.smoother));
        if (smIdx >= 0) preconditionerSmootherCombo->setCurrentIndex(smIdx);
    }
    else if (newCfg.solver == FlowCompute::LinearSolver::PCG ||
             newCfg.solver == FlowCompute::LinearSolver::PBiCG ||
             newCfg.solver == FlowCompute::LinearSolver::PBiCGStab) {
        int prIdx = preconditionerSmootherCombo->findData(static_cast<int>(newCfg.preconditioner));
        if (prIdx >= 0) preconditionerSmootherCombo->setCurrentIndex(prIdx);
    }

    // 3. Load Tolerances (Converting double to scientific notation string)
    // 'g' formatting intelligently uses scientific notation (e.g. 1e-06) for small numbers
    absTolEdit->setText(QString::number(newCfg.absTolerance, 'g', 6));
    relTolEdit->setText(QString::number(newCfg.relTolerance, 'g', 6));

    // 4. Load Relaxation Factor
    relaxationSpin->setValue(newCfg.relaxationFactor);
}

// Respond when a solver is selected
void AlgorithmPage::solverChanged(int index) {

    // Get the selected solver
    QVariant data = solverCombo->itemData(index);
    auto solver = static_cast<FlowCompute::LinearSolver>(data.toInt());

    // Populate the preconditionerSmoother combo box
    preconditionerSmootherCombo->clear();
    if (solver == FlowCompute::LinearSolver::PCG ||
        solver == FlowCompute::LinearSolver::PBiCG ||
        solver == FlowCompute::LinearSolver::PBiCGStab) {

        preconditionerSmootherLabel->setText(tr("Preconditioner: "));
        preconditionerSmootherLabel->show();
        preconditionerSmootherCombo->show();

        // Populate with Preconditioners
        QMetaEnum precondEnum = QMetaEnum::fromType<FlowCompute::Preconditioner>();
        for (int i = 0; i < precondEnum.keyCount(); ++i) {
            preconditionerSmootherCombo->addItem(precondEnum.key(i), precondEnum.value(i));
        }
    }
    else if (solver == FlowCompute::LinearSolver::smoothSolver) {

        preconditionerSmootherLabel->setText(tr("Smoother: "));
        preconditionerSmootherLabel->show();
        preconditionerSmootherCombo->show();

        // Populate with Smoothers
        QMetaEnum smootherEnum = QMetaEnum::fromType<FlowCompute::Smoother>();
        for (int i = 0; i < smootherEnum.keyCount(); ++i) {
            preconditionerSmootherCombo->addItem(smootherEnum.key(i), smootherEnum.value(i));
        }
    }
    else {
        preconditionerSmootherLabel->hide();
        preconditionerSmootherCombo->hide();
    }
}