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

#include "wizards/solver/page_10_control.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDoubleValidator>
#include <QFormLayout>
#include <QGroupBox>
#include <QIntValidator>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>

#include "wizards/solver/wizard_solver.h"

// Introduction page asks for the case name and platform
ControlPage::ControlPage(const QString& caseName, const QStringList& cases,
    const std::vector<FlowCompute::SolverFamily>& families, QWidget *parent):
    QWizardPage(parent), m_caseName(caseName), m_families(families) {
    // Set title and style
    setTitle(tr("Control Configuration (controlDict)"));

    // Create a grid layout with two columns
    QFormLayout* layout = new QFormLayout(this);
    layout->setSpacing(10);

    // Get selected case
    m_caseCombo = new QComboBox(this);
    m_caseCombo->addItems(cases);
    m_caseCombo->setCurrentText(caseName);
    layout->addRow(tr("Select the OpenFOAM case:"), m_caseCombo);
    connect(m_caseCombo, &QComboBox::currentTextChanged, this,
            [this](const QString& text) {
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

    // Timing group
    QGroupBox* timingGroup = new QGroupBox(tr("Solution Timing"), this);
    layout->addRow(timingGroup);
    QFormLayout* timingLayout = new QFormLayout(timingGroup);
    timingLayout->setSpacing(10);

    m_startFromCombo = new QComboBox(this);
    QMetaEnum metaEnum = QMetaEnum::fromType<CaseIO::StartSolverType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_startFromCombo->addItem(metaEnum.key(i));
    }
    timingLayout->addRow(tr("Start from (startFrom):"), m_startFromCombo);
    connect(m_startFromCombo, &QComboBox::currentTextChanged, this,
    [this](const QString& text){
        m_startTimeSpin->setEnabled(text == "startTime");
    });

    m_startTimeSpin = new QDoubleSpinBox(this);
    m_startTimeSpin->setRange(0.0, 1e6);
    m_startTimeSpin->setDecimals(5);
    m_startTimeSpin->setEnabled(m_startFromCombo->currentText() == "startTime");
    timingLayout->addRow(tr("Start time (startTime): "), m_startTimeSpin);

    m_stopAtCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<CaseIO::EndSolverType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_stopAtCombo->addItem(metaEnum.key(i));
    }
    timingLayout->addRow(tr("Stop time source (stopAt): "), m_stopAtCombo);
    connect(m_stopAtCombo, &QComboBox::currentTextChanged, this,
    [this](const QString& text){
        m_endTimeSpin->setEnabled(text == "endTime");
    });

    m_endTimeSpin = new QDoubleSpinBox(this);
    m_endTimeSpin->setRange(0.0, 1e6);
    m_endTimeSpin->setDecimals(5);
    m_endTimeSpin->setValue(0.5);
    m_endTimeSpin->setEnabled(m_stopAtCombo->currentText() == "endTime");
    timingLayout->addRow(tr("End time: "), m_endTimeSpin);

    m_deltaTSpin = new QDoubleSpinBox(this);
    m_deltaTSpin->setRange(1e-8, 1e4);
    m_deltaTSpin->setDecimals(6);
    m_deltaTSpin->setValue(1.0);
    timingLayout->addRow(tr("Time step/Initial step (deltaT):"), m_deltaTSpin);

    // Output group
    QGroupBox* writeGroup = new QGroupBox(tr("Writing Output"), this);
    layout->addRow(writeGroup);
    QFormLayout* writeLayout = new QFormLayout(writeGroup);
    writeLayout->setSpacing(10);

    // Check boxes
    auto* checkLayout = new QHBoxLayout();
    m_modifiableCheck = new QCheckBox(tr("Enable runtime modification"), this);
    checkLayout->addWidget(m_modifiableCheck);
    m_compressCheck = new QCheckBox(tr("Compress output"), this);
    checkLayout->addWidget(m_compressCheck);
    writeLayout->addRow(checkLayout);

    // Set write format
    m_writeFormatCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<CaseIO::WriteFormatType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_writeFormatCombo->addItem(metaEnum.key(i));
    }
    writeLayout->addRow(tr("Write format: "), m_writeFormatCombo);

    // Write control
    m_writeControlCombo = new QComboBox(this);
    metaEnum = QMetaEnum::fromType<CaseIO::WriteControlType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_writeControlCombo->addItem(metaEnum.key(i));
    }
    writeLayout->addRow(tr("Write control: "), m_writeControlCombo);
    connect(m_writeControlCombo, &QComboBox::currentIndexChanged,
            this, &ControlPage::writeControlChanged);

    // Write interval
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

    // Transient configuration group
    m_transientGroup = new QGroupBox(tr("Transient Configuration"), this);
    layout->addRow(m_transientGroup);
    QFormLayout* transientLayout = new QFormLayout(m_transientGroup);
    transientLayout->setSpacing(10);

    // Adjust time step
    m_adjustTimeCheck =
        new QCheckBox(tr("Adjust time step (adjustTimeStep)"), this);
    transientLayout->addRow(m_adjustTimeCheck);
    connect(m_adjustTimeCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_maxCourantSpin->setEnabled(checked);
        if (checked) {
            QString currentWrite = m_writeControlCombo->currentText();
            if (currentWrite == "timeStep" || currentWrite == "runTime") {
                m_writeControlCombo->setCurrentText("adjustableRunTime");
            }
        }
    });

    // Configure maximum Courant number
    m_maxCourantSpin = new QDoubleSpinBox(this);
    m_maxCourantSpin->setRange(0.1, 20.0);
    m_maxCourantSpin->setDecimals(2);
    m_maxCourantSpin->setSingleStep(0.1);
    m_maxCourantSpin->setValue(1.0);
    transientLayout->addRow(tr("Max Courant number (maxCo): "),
                            m_maxCourantSpin);

    // Set initial UI state based on the checkbox default
    bool isAdjustable = m_adjustTimeCheck->isChecked();
    m_maxCourantSpin->setEnabled(isAdjustable);

    // Register case name
    registerField("caseName", m_caseCombo, "currentText");

    setLayout(layout);
}

void ControlPage::initializePage() {
    // Access wizard data
    m_solverWizard = qobject_cast<SolverWizard*>(this->wizard());
    m_cfg = &(m_solverWizard->getControlConfig());

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

    // Get category for the solver in controlDict
    m_isOpenCFD = !m_cfg->application.startsWith("foam");
    QString solverName = m_isOpenCFD ? m_cfg->application : m_cfg->solver;

    // Get category for the solver in controlDict
    bool familyMatch = false;
    FlowCompute::SolverFamily foundFamily;
    for (const auto& family : std::as_const(m_families)) {
        for (const auto& solver : family.solvers) {
            bool match = m_isOpenCFD ? (solver.name == solverName)
                                     : (solver.foundationName == solverName);
            if (match) {
                foundFamily = family;
                familyMatch = true;
                break;
            }
        }
        if (familyMatch)
            break;
    }

    // Update families and solvers without triggering signals
    m_cfg->solverFamily = foundFamily.name;
    {
        QSignalBlocker familyBlocker(m_familyCombo);
        QSignalBlocker solverBlocker(m_solverCombo);
        m_familyCombo->clear();
        for (const auto& family: std::as_const(m_families)) {
            m_familyCombo->addItem(family.name);
        }
        m_solverCombo->clear();
        for (const auto& solver : std::as_const(foundFamily.solvers)) {
            QString item = m_isOpenCFD ? solver.name : solver.foundationName;
            if (m_solverCombo->findText(item) == -1) {
                m_solverCombo->addItem(item);
            }
        }

        // Set family
        if (m_cfg->solverFamily.isEmpty()) {
            m_familyCombo->setCurrentIndex(0);
        } else {
            m_familyCombo->setCurrentText(m_cfg->solverFamily);
        }

        // Set solver
        if (!solverName.isEmpty()) {
            m_solverCombo->setCurrentText(solverName);
        }
    }

    solverChanged(m_solverCombo->currentIndex());
}

// Populate list of solvers based on family
void ControlPage::familyChanged(int index) {
    m_solverCombo->clear();
    if (index >= 0 && index < m_families.size()) {
        for (const auto& solver: std::as_const(m_families[index].solvers)) {
            QString item = m_isOpenCFD ? solver.name : solver.foundationName;
            if (m_solverCombo->findText(item) == -1) {
                m_solverCombo->addItem(item);
            }
        }
    }
}

// Update values when a new solver is chosen
void ControlPage::solverChanged(int index) {
    // Check combo box states
    int familyIndex = m_familyCombo->currentIndex();
    if (familyIndex < 0 || familyIndex >= m_families.size())
        return;
    if (index < 0 || index >= m_families[familyIndex].solvers.size())
        return;

    // Determine if the solver is steady state
    if (m_isOpenCFD) {
        m_isSteadyState = m_families[familyIndex].solvers[index].isSteadyState;
    } else {
        m_isSteadyState = m_solverWizard->isSteadyState();
    }

    // Lock timeStep field for steady-state simulation
    if (m_isSteadyState) {
        m_transientGroup->setVisible(false);

        // Lock write control to timeStep
        int timeStepIndex = m_writeControlCombo->findText("timeStep");
        if (timeStepIndex != -1)
            m_writeControlCombo->setCurrentIndex(timeStepIndex);
        m_writeControlCombo->setEnabled(false);
        m_deltaTSpin->setValue(1.0);
        m_deltaTSpin->setEnabled(false);
    } else {
        m_transientGroup->setVisible(true);
        m_writeControlCombo->setEnabled(true);
        m_deltaTSpin->setEnabled(true);

        // State restoration for transient widgets
        bool isAdjustable = m_adjustTimeCheck->isChecked();
        m_maxCourantSpin->setEnabled(isAdjustable);
    }
}

void ControlPage::writeControlChanged(int index) {
    // Validation and formatting
    QString controlText = m_writeControlCombo->itemText(index);
    if (controlText == "timeStep") {
        m_writeIntervalEdit->setValidator(m_intValidator);

        // Strip decimals if the user already typed them
        QString currentText = m_writeIntervalEdit->text();
        if (currentText.contains('.')) {
            m_writeIntervalEdit->setText(
                QString::number(currentText.toDouble(), 'f', 0));
        }
    } else {
        m_writeIntervalEdit->setValidator(m_doubleValidator);
    }

    // If 'none' is selected, writing is disabled
    m_writeIntervalEdit->setEnabled(controlText != "none");

    // If the user explicitly selects adjustableRunTime, check adjustTimeStep
    if (controlText == "adjustableRunTime" && !m_adjustTimeCheck->isChecked()) {
        QSignalBlocker blocker(m_adjustTimeCheck);
        m_adjustTimeCheck->setChecked(true);
        m_maxCourantSpin->setEnabled(true);
    }
}

bool ControlPage::validatePage() {
    // Update wizard's case name
    m_solverWizard->setCaseName(m_caseName);

    // Validate inputs mutating the struct
    if (m_writeIntervalEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Input"),
                             tr("Please enter a valid write interval."));
        m_writeIntervalEdit->setFocus();
        return false;
    }

    m_cfg->solverFamily = m_familyCombo->currentText();

    // Check if ESI or Foundation version
    m_cfg->solverFamily = m_familyCombo->currentText();
    if (m_cfg->application.startsWith("foam")) {
        m_cfg->solver = m_solverCombo->currentText();
    } else {
        m_cfg->application = m_solverCombo->currentText();
        m_cfg->solver = "";
    }

    // Timing fields
    QMetaEnum startEnum = QMetaEnum::fromType<CaseIO::StartSolverType>();
    m_cfg->startFrom = static_cast<CaseIO::StartSolverType>(
        startEnum.keyToValue(
            m_startFromCombo->currentText().toUtf8().constData()));

    m_cfg->startTime = m_startTimeSpin->value();

    QMetaEnum stopEnum = QMetaEnum::fromType<CaseIO::EndSolverType>();
    m_cfg->stopAt = static_cast<CaseIO::EndSolverType>(
        stopEnum.keyToValue(m_stopAtCombo->currentText().toUtf8().constData()));

    m_cfg->endTime = m_endTimeSpin->value();
    m_cfg->deltaT = m_deltaTSpin->value();

    // Transient fields
    m_cfg->adjustTimeStep = m_adjustTimeCheck->isChecked();
    m_cfg->maxCo = m_maxCourantSpin->value();

    // Write output fields
    m_cfg->writeCompression = m_compressCheck->isChecked();
    m_cfg->runTimeModifiable = m_modifiableCheck->isChecked();

    QMetaEnum formatEnum = QMetaEnum::fromType<CaseIO::WriteFormatType>();
    m_cfg->writeFormat = static_cast<CaseIO::WriteFormatType>(
        formatEnum.keyToValue(
            m_writeFormatCombo->currentText().toUtf8().constData()));

    QMetaEnum controlEnum = QMetaEnum::fromType<CaseIO::WriteControlType>();
    m_cfg->writeControl = static_cast<CaseIO::WriteControlType>(
        controlEnum.keyToValue(
            m_writeControlCombo->currentText().toUtf8().constData()));

    m_cfg->writeInterval = m_writeIntervalEdit->text().toDouble();
    m_cfg->purgeWrite = m_purgeWriteSpin->value();

    return true;
}
