#include "page_30_interactive.h"

#include "wizard_new_case.h"

// Introduction page asks for the case name and platform
InteractivePage::InteractivePage(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Interactive Case Builder"));
    setStyleSheet("QRadioButton { padding-left: 15px; }"
                  "QCheckBox { padding-left: 15px; }");

    // Create vertical layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);

    // Create question for flow type
    QVBoxLayout* flowLayout = new QVBoxLayout();
    flowLayout->setSpacing(7);
    QLabel* flowLabel = new QLabel(tr("<b>1. What best describes the flow in your simulation?</b>"));
    flowLayout->addWidget(flowLabel);
    QRadioButton* incompressibleButton =
        new QRadioButton(tr("Liquid or slow-moving gas with nearly constant density (incompressible)"));
    flowLayout->addWidget(incompressibleButton);
    QRadioButton* compressibleButton =
        new QRadioButton(tr("Gas whose density changes with pressure, speed, or temperature (compressible)"));
    flowLayout->addWidget(compressibleButton);
    QRadioButton* multiphaseButton =
        new QRadioButton(tr("Interaction of two or more fluids or phases (multiphase)"));
    flowLayout->addWidget(multiphaseButton);
    layout->addLayout(flowLayout);

    // Create button group
    m_flowButtonGroup = new QButtonGroup(this);
    m_flowButtonGroup->addButton(incompressibleButton, 0);
    m_flowButtonGroup->addButton(compressibleButton, 1);
    m_flowButtonGroup->addButton(multiphaseButton, 2);
    incompressibleButton->setChecked(true);

    // Create question for turbulence
    QVBoxLayout* turbulenceLayout = new QVBoxLayout();
    turbulenceLayout->setSpacing(7);
    QLabel* turbulenceLabel = new QLabel(tr("<b>2. How should turbulence be modeled?</b>"));
    turbulenceLayout->addWidget(turbulenceLabel);
    QRadioButton* laminarButton =
        new QRadioButton(tr("No turbulence (laminar)"));
    turbulenceLayout->addWidget(laminarButton);
    QRadioButton* rasButton =
        new QRadioButton(tr("Average turbulence (RAS) - balance accuracy with computational speed"));
    turbulenceLayout->addWidget(rasButton);
    QRadioButton* lesButton =
        new QRadioButton(tr("Detailed turbulence (LES) - recommended for advanced transient analysis"));
    turbulenceLayout->addWidget(lesButton);
    layout->addLayout(turbulenceLayout);

    // Create button group
    m_turbulenceButtonGroup = new QButtonGroup(this);
    m_turbulenceButtonGroup->addButton(laminarButton, 0);
    m_turbulenceButtonGroup->addButton(rasButton, 1);
    m_turbulenceButtonGroup->addButton(lesButton, 2);
    rasButton->setChecked(true);

    // Create question for time handling
    QVBoxLayout* timeLayout = new QVBoxLayout();
    timeLayout->setSpacing(7);
    QLabel* timeLabel = new QLabel(tr("<b>3. How should time be handled in the simulation?</b>"));
    timeLayout->addWidget(timeLabel);
    QRadioButton* steadyButton =
        new QRadioButton(tr("Compute the final, settled flow (steady-state)"));
    timeLayout->addWidget(steadyButton);
    QRadioButton* transientButton =
        new QRadioButton(tr("Compute changes moment-by-moment (transient)"));
    timeLayout->addWidget(transientButton);
    layout->addLayout(timeLayout);

    // Create button group
    m_timeButtonGroup = new QButtonGroup(this);
    m_timeButtonGroup->addButton(steadyButton, 0);
    m_timeButtonGroup->addButton(transientButton, 1);
    steadyButton->setChecked(true);

    // Create question for physics
    QVBoxLayout* physicsLayout = new QVBoxLayout();
    physicsLayout->setSpacing(7);
    QLabel* physicsLabel = new QLabel(tr("<b>4. What other physical effects should be included in the simulation?</b>"));
    physicsLayout->addWidget(physicsLabel);
    m_heatCheck = new QCheckBox(tr("Heat transfer - Temperature changes and thermal conduction"));
    physicsLayout->addWidget(m_heatCheck);
    m_radiationCheck = new QCheckBox(tr("Radiation - Heat transfer by electromagnetic radiation"));
    physicsLayout->addWidget(m_radiationCheck);
    m_combustionCheck = new QCheckBox(tr("Combustion - Burning fuels and reacting gases"));
    physicsLayout->addWidget(m_combustionCheck);
    layout->addLayout(physicsLayout);

    // Select priority of computation speed and accuracy
    QVBoxLayout* priorityLayout = new QVBoxLayout();
    priorityLayout->setSpacing(7);
    QLabel* priorityLabel = new QLabel(tr("<b>5. Move the slider to indicate priority of speed versus accuracy</b>"));
    priorityLayout->addWidget(priorityLabel);
    m_prioritySlider = new QSlider(Qt::Horizontal, this);
    m_prioritySlider->setRange(0, 4);
    m_prioritySlider->setValue(2);
    m_prioritySlider->setTickPosition(QSlider::TicksBelow);
    m_prioritySlider->setTickInterval(1);
    m_prioritySlider->setSingleStep(1);
    m_prioritySlider->setPageStep(1);
    priorityLayout->addWidget(m_prioritySlider);

    // Set labels for the ticks
    QHBoxLayout* labelLayout = new QHBoxLayout();
    QLabel* fastLabel = new QLabel("High Speed", this);
    fastLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    labelLayout->addWidget(fastLabel);
    QLabel* accurateLabel = new QLabel("High Accuracy", this);
    accurateLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    labelLayout->addWidget(accurateLabel);
    priorityLayout->addLayout(labelLayout);
    layout->addLayout(priorityLayout);

    // LES should only be enabled for transient simulation
    connect(steadyButton, &QRadioButton::toggled, this, [=](bool checked){
        if (checked) {
            lesButton->setChecked(false);
            lesButton->setEnabled(false);
            rasButton->setChecked(true);
        } else {
            lesButton->setEnabled(true);
        }
    });

    // Combustion shouldn't be enabled for incompressible simulation
    connect(incompressibleButton, &QRadioButton::toggled, this, [=, this](bool checked){
        if (checked) {
            m_combustionCheck->setChecked(false);
            m_combustionCheck->setEnabled(false);
        } else {
            m_combustionCheck->setEnabled(true);
        }
    });

    // Set the page layout
    setLayout(layout);
}

bool InteractivePage::validatePage() {

    // Access the case configuration structure
    NewCaseWizard* newCaseWizard = qobject_cast<NewCaseWizard*>(wizard());
    if (!newCaseWizard) {
        return false;
    }
    CaseConfig* caseConfig = &(newCaseWizard->getCaseConfig());

    // Set configuration settings
    caseConfig->flowConfig =
        static_cast<FlowConfig>(m_flowButtonGroup->checkedId());
    caseConfig->turbulenceConfig =
        static_cast<TurbulenceConfig>(m_turbulenceButtonGroup->checkedId());
    caseConfig->timeConfig =
        static_cast<TimeConfig>(m_timeButtonGroup->checkedId());
    caseConfig->heatConfig = m_heatCheck->isChecked();
    caseConfig->radiationConfig = m_radiationCheck->isChecked();
    caseConfig->combustionConfig = m_combustionCheck->isChecked();
    caseConfig->priorityConfig = m_prioritySlider->value();

    return true;
}