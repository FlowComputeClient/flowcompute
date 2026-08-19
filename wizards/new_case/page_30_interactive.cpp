// Copyright 2026 FlowCompute LLC
//
// This file is part of FlowCompute.
//
// FlowCompute is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// FlowCompute is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with FlowCompute. If not, see <https://www.gnu.org/licenses/>.

#include "page_30_interactive.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

#include "wizard_new_case.h"

// Introduction page asks for the case name and platform
InteractivePage::InteractivePage(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Interactive Case Builder"));
    setStyleSheet("QRadioButton { padding-left: 15px; }"
                  "QCheckBox { padding-left: 15px; }");

    // Create vertical layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->addSpacing(10);

    // Create question for flow type
    QLabel* flowLabel = new QLabel(tr("<b>1. What best describes the "
                                      "flow in your simulation?</b>"));
    layout->addWidget(flowLabel);
    QRadioButton* incompressibleButton =
        new QRadioButton(tr("Liquid or slow-moving gas with nearly "
                            "constant density (incompressible)"));
    layout->addWidget(incompressibleButton);
    QRadioButton* compressibleButton =
        new QRadioButton(tr("Gas whose density changes with pressure, "
                            "speed, or temperature (compressible)"));
    layout->addWidget(compressibleButton);
    QRadioButton* multiphaseButton =
        new QRadioButton(tr("Interaction of two or more "
                            "fluids or phases (multiphase)"));
    layout->addWidget(multiphaseButton);

    // Create button group
    m_flowButtonGroup = new QButtonGroup(this);
    m_flowButtonGroup->addButton(incompressibleButton, 0);
    m_flowButtonGroup->addButton(compressibleButton, 1);
    m_flowButtonGroup->addButton(multiphaseButton, 2);
    incompressibleButton->setChecked(true);

    // Create question for time handling
    layout->addSpacing(10);
    QLabel* timeLabel = new QLabel(tr("<b>2. How should time be handled "
                                      "in the simulation?</b>"));
    layout->addWidget(timeLabel);
    QRadioButton* steadyButton =
        new QRadioButton(tr("Compute the final, settled flow (steady-state)"));
    layout->addWidget(steadyButton);
    QRadioButton* transientButton =
        new QRadioButton(tr("Compute changes moment-by-moment (transient)"));
    layout->addWidget(transientButton);

    // Create button group
    m_timeButtonGroup = new QButtonGroup(this);
    m_timeButtonGroup->addButton(steadyButton, 0);
    m_timeButtonGroup->addButton(transientButton, 1);
    steadyButton->setChecked(true);

    // Create question for turbulence
    layout->addSpacing(10);
    QLabel* turbulenceLabel = new QLabel(tr("<b>3. How should turbulence "
                                            "be modeled?</b>"));
    layout->addWidget(turbulenceLabel);
    QRadioButton* laminarButton =
        new QRadioButton(tr("No turbulence (laminar)"));
    layout->addWidget(laminarButton);
    QRadioButton* rasButton =
        new QRadioButton(tr("Average turbulence (RAS) - "
                            "balance accuracy with computational speed"));
    layout->addWidget(rasButton);
    QRadioButton* lesButton =
        new QRadioButton(tr("Detailed turbulence (LES) - "
                            "recommended for advanced transient analysis"));
    layout->addWidget(lesButton);

    // Create button group
    m_turbulenceButtonGroup = new QButtonGroup(this);
    m_turbulenceButtonGroup->addButton(laminarButton, 0);
    m_turbulenceButtonGroup->addButton(rasButton, 1);
    m_turbulenceButtonGroup->addButton(lesButton, 2);
    rasButton->setChecked(true);

    /*
    // Create question for physics
    QLabel* physicsLabel = new QLabel(tr("<b>4. What other physical effects "
                        "should be included in the simulation?</b>"));
    layout->addWidget(physicsLabel);
    m_heatCheck = new QCheckBox(tr("Heat transfer - "
                        "Temperature changes and thermal conduction"));
    layout->addWidget(m_heatCheck);
    m_radiationCheck = new QCheckBox(tr("Radiation - "
                        "Heat transfer by electromagnetic radiation"));
    layout->addWidget(m_radiationCheck);
    m_combustionCheck = new QCheckBox(tr("Combustion - "
                        "Burning fuels and reacting gases"));
    layout->addWidget(m_combustionCheck);

    // Select priority of computation speed and accuracy
    QVBoxLayout* priorityLayout = new QVBoxLayout();
    priorityLayout->setSpacing(10);
    QLabel* priorityLabel = new QLabel(tr("<b>5. Move the slider to set "
        "priority of speed versus accuracy</b>"));
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
    setLayout(layout);
    */

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
    connect(incompressibleButton, &QRadioButton::toggled, this,
            [=, this](bool checked){
        if (checked) {
            m_combustionCheck->setChecked(false);
            m_combustionCheck->setEnabled(false);
        } else {
            m_combustionCheck->setEnabled(true);
        }
    });
}

bool InteractivePage::validatePage() {
    // Access the case configuration structure
    NewCaseWizard* newCaseWizard = qobject_cast<NewCaseWizard*>(wizard());
    if (!newCaseWizard)
        return false;
    CaseConfig* caseConfig = &(newCaseWizard->getCaseConfig());

    // Set configuration settings
    caseConfig->flowConfig =
        static_cast<FlowConfig>(m_flowButtonGroup->checkedId());
    caseConfig->turbulenceConfig =
        static_cast<TurbulenceConfig>(m_turbulenceButtonGroup->checkedId());
    caseConfig->timeConfig =
        static_cast<TimeConfig>(m_timeButtonGroup->checkedId());
    /*
    caseConfig->heatConfig = m_heatCheck->isChecked();
    caseConfig->radiationConfig = m_radiationCheck->isChecked();
    caseConfig->combustionConfig = m_combustionCheck->isChecked();
    caseConfig->priorityConfig = m_prioritySlider->value();
    */
    return true;
}