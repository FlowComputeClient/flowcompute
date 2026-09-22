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
#include <QScrollArea>
#include <QVBoxLayout>

#include "wizard_new_case.h"

// Introduction page asks for the case name and platform
InteractivePage::InteractivePage(QWidget *parent): QWizardPage(parent) {
    // Set title and style
    setTitle(tr("Interactive Case Builder"));
    setStyleSheet("QRadioButton { padding-left: 15px; }"
                  "QCheckBox { padding-left: 15px; }");

    // Create the main layout
    QVBoxLayout *pageLayout = new QVBoxLayout(this);

    // Create a scroll area
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    // Create vertical layout
    QWidget *formContainer = new QWidget(scrollArea);
    QVBoxLayout* formLayout = new QVBoxLayout(formContainer);
    formLayout->setSpacing(10);
    formLayout->addSpacing(10);

    // Create question for time handling
    formLayout->addSpacing(10);
    QLabel* timeLabel = new QLabel(tr("<b>1. How should time be handled?</b>"));
    formLayout->addWidget(timeLabel);
    QRadioButton* steadyButton =
        new QRadioButton(tr("Compute the final, settled flow (steady-state)"));
    formLayout->addWidget(steadyButton);
    QRadioButton* transientButton =
        new QRadioButton(tr("Compute changes moment-by-moment (transient)"));
    formLayout->addWidget(transientButton);

    // Create button group
    m_timeButtonGroup = new QButtonGroup(this);
    m_timeButtonGroup->addButton(steadyButton, 0);
    m_timeButtonGroup->addButton(transientButton, 1);
    steadyButton->setChecked(true);

    // Create question for flow type
    QLabel* flowLabel = new QLabel(tr("<b>2. What best describes the "
                                      "flow in your simulation?</b>"));
    formLayout->addWidget(flowLabel);
    QRadioButton* incompressibleButton =
        new QRadioButton(tr("Liquid or slow-moving gas with nearly "
                            "constant density (incompressible)"));
    formLayout->addWidget(incompressibleButton);
    QRadioButton* compressibleButton =
        new QRadioButton(tr("Fluid whose density changes with pressure, "
                            "speed, or temperature (compressible)"));
    formLayout->addWidget(compressibleButton);

    // Create button group
    m_flowButtonGroup = new QButtonGroup(this);
    m_flowButtonGroup->addButton(incompressibleButton, 0);
    m_flowButtonGroup->addButton(compressibleButton, 1);
    incompressibleButton->setChecked(true);

    // Create question for phases
    formLayout->addSpacing(10);
    QLabel* phaseLabel =
        new QLabel(tr("<b>3. How many phases are involved?</b>"));
    formLayout->addWidget(phaseLabel);
    QRadioButton* singlePhaseButton = new QRadioButton(tr("One phase"));
    formLayout->addWidget(singlePhaseButton);
    QRadioButton* multiPhaseButton = new QRadioButton(tr("Multiple phases"));
    formLayout->addWidget(multiPhaseButton);

    // Create button group
    m_phaseButtonGroup = new QButtonGroup(this);
    m_phaseButtonGroup->addButton(singlePhaseButton, 0);
    m_phaseButtonGroup->addButton(multiPhaseButton, 1);
    singlePhaseButton->setChecked(true);

    // Create question for turbulence
    formLayout->addSpacing(10);
    QLabel* turbulenceLabel =
        new QLabel(tr("<b>4. How should turbulence be modeled?</b>"));
    formLayout->addWidget(turbulenceLabel);
    QRadioButton* laminarButton =
        new QRadioButton(tr("No turbulence (laminar)"));
    formLayout->addWidget(laminarButton);
    QRadioButton* rasButton =
        new QRadioButton(tr("Time-averaged turbulence (RAS) - "
                            "balances accuracy with computational speed"));
    formLayout->addWidget(rasButton);
    QRadioButton* lesButton =
        new QRadioButton(tr("Detailed transient turbulence (LES) - "
                            "high computational cost"));
    formLayout->addWidget(lesButton);

    // Create button group
    m_turbulenceButtonGroup = new QButtonGroup(this);
    m_turbulenceButtonGroup->addButton(laminarButton, 0);
    m_turbulenceButtonGroup->addButton(rasButton, 1);
    m_turbulenceButtonGroup->addButton(lesButton, 2);
    rasButton->setChecked(true);

    // Create question for heat transfer
    formLayout->addSpacing(10);
    QLabel* heatLabel =
        new QLabel(tr("<b>5. Does your simulation involve heat transfer?</b>"));
    formLayout->addWidget(heatLabel);
    QRadioButton* noHeatButton =
        new QRadioButton(tr("No heat transfer"));
    formLayout->addWidget(noHeatButton);
    QRadioButton* fluidHeatButton =
        new QRadioButton(tr("Heat is only transferred within a fluid"));
    formLayout->addWidget(fluidHeatButton);
    QRadioButton* conjugateHeatButton =
        new QRadioButton(tr("Heat flows between fluid and solid regions"));
    formLayout->addWidget(conjugateHeatButton);

    // Create button group
    m_heatButtonGroup = new QButtonGroup(this);
    m_heatButtonGroup->addButton(noHeatButton, 0);
    m_heatButtonGroup->addButton(fluidHeatButton, 1);
    m_heatButtonGroup->addButton(conjugateHeatButton, 2);
    noHeatButton->setChecked(true);

    // Create question for dynamic mesh
    formLayout->addSpacing(10);
    QLabel* dynamicMeshLabel = new QLabel(tr("<b>6. Does the mesh move or "
                                    "change during the simulation?</b>"));
    formLayout->addWidget(dynamicMeshLabel);
    QRadioButton* staticMeshButton =
        new QRadioButton(tr("No movement (static mesh)"));
    formLayout->addWidget(staticMeshButton);
    QRadioButton* mrfMeshButton =
        new QRadioButton(tr("Steady-state rotation of fluid zones (MRF)"));
    formLayout->addWidget(mrfMeshButton);
    QRadioButton* amiMeshButton =
        new QRadioButton(tr("Physically rotating or sliding mesh zones (AMI)"));
    formLayout->addWidget(amiMeshButton);
    QRadioButton* oversetMeshButton = new QRadioButton(tr("Complex, overlapping"
                                    " moving parts moving freely (overset)"));
    formLayout->addWidget(oversetMeshButton);
    QRadioButton* deformMeshButton =
        new QRadioButton(tr("Stretching or morphing boundaries (deforming)"));
    formLayout->addWidget(deformMeshButton);

    // Create button group
    m_meshButtonGroup = new QButtonGroup(this);
    m_meshButtonGroup->addButton(staticMeshButton, 0);
    m_meshButtonGroup->addButton(mrfMeshButton, 1);
    m_meshButtonGroup->addButton(amiMeshButton, 2);
    m_meshButtonGroup->addButton(oversetMeshButton, 3);
    m_meshButtonGroup->addButton(deformMeshButton, 4);
    staticMeshButton->setChecked(true);

    // Create question for physics
    QLabel* physicsLabel = new QLabel(tr("<b>7. What other effects "
                        "should be included?</b>"));
    formLayout->addWidget(physicsLabel);
    m_radiationCheck = new QCheckBox(tr("Radiation - "
                        "Heat transfer by electromagnetic radiation"));
    formLayout->addWidget(m_radiationCheck);
    m_combustionCheck = new QCheckBox(tr("Combustion - "
                        "Burning fuels and reacting gases"));
    formLayout->addWidget(m_combustionCheck);
    m_buoyancyCheck = new QCheckBox(tr("Buoyancy - "
                        "Gravity-driven flow and natural convection"));
    formLayout->addWidget(m_buoyancyCheck);
    m_particlesCheck = new QCheckBox(tr("Discrete Particles - "
                        "Tracking droplets, bubbles, or dust"));
    formLayout->addWidget(m_particlesCheck);

    // Disable heat-related checkboxes by default
    m_radiationCheck->setEnabled(false);
    m_buoyancyCheck->setEnabled(false);
    m_combustionCheck->setEnabled(false);

    // LES should only be enabled for transient simulation
    connect(steadyButton, &QRadioButton::toggled, this, [=](bool isSteady){
        if (isSteady) {
            lesButton->setChecked(false);
            lesButton->setEnabled(false);
            rasButton->setChecked(true);
        } else {
            lesButton->setEnabled(true);
        }
    });

    // Centralized physics state evaluation
    auto updatePhysicsToggles = [=, this]() {
        bool isCompressible = compressibleButton->isChecked();
        bool hasHeat = fluidHeatButton->isChecked() ||
                       conjugateHeatButton->isChecked();

        // Enable/Disable physics based on states
        m_radiationCheck->setEnabled(hasHeat);
        m_buoyancyCheck->setEnabled(hasHeat);
        m_combustionCheck->setEnabled(hasHeat && isCompressible);

        // Cascade uncheck if the requirements are no longer met
        if (!hasHeat) {
            m_radiationCheck->setChecked(false);
            m_buoyancyCheck->setChecked(false);
        }
        if (!(hasHeat && isCompressible)) {
            m_combustionCheck->setChecked(false);
        }
    };

    // Compressibility determines heat transfer
    connect(compressibleButton, &QRadioButton::toggled, this,
            [=](bool isCompressible) {
        if (isCompressible) {
            noHeatButton->setEnabled(false);
            if (noHeatButton->isChecked()) {
                fluidHeatButton->setChecked(true);
            }
        } else {
            noHeatButton->setEnabled(true);
        }
        updatePhysicsToggles();
    });

    // Connect remaining heat transfer buttons to the centralized update logic
    connect(noHeatButton, &QRadioButton::toggled, this, updatePhysicsToggles);
    connect(fluidHeatButton, &QRadioButton::toggled, this,
            updatePhysicsToggles);
    connect(conjugateHeatButton, &QRadioButton::toggled, this,
            updatePhysicsToggles);

    // Update the layout
    scrollArea->setWidget(formContainer);
    pageLayout->addWidget(scrollArea);
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
    caseConfig->phaseConfig =
        static_cast<PhaseConfig>(m_phaseButtonGroup->checkedId());
    caseConfig->heatConfig =
        static_cast<HeatConfig>(m_heatButtonGroup->checkedId());
    caseConfig->meshConfig =
        static_cast<MeshConfig>(m_meshButtonGroup->checkedId());
    caseConfig->radiationConfig = m_radiationCheck->isChecked();
    caseConfig->combustionConfig = m_combustionCheck->isChecked();
    caseConfig->buoyancyConfig = m_buoyancyCheck->isChecked();
    caseConfig->particlesConfig = m_particlesCheck->isChecked();
    return true;
}