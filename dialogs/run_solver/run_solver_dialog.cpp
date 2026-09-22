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

#include "dialogs/run_solver/run_solver_dialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "parser/open_foam_dictionary.h"
#include "systems/system_manager.h"

// Create dialog to configure simulation process
RunSolverDialog::RunSolverDialog(const QString& selectedCase,
    bool isFoundation, const SystemManager& systemMgr, QWidget* parent):
    m_isFoundation(isFoundation), m_systemMgr(systemMgr), QDialog(parent) {
    // Set title and style
    setWindowTitle(tr("Solver Execution"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Create layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);

    // Select case
    m_caseCombo = new QComboBox(this);
    m_caseCombo->addItems(systemMgr.getCases());
    m_caseCombo->setCurrentText(selectedCase);

    QHBoxLayout* caseLayout = new QHBoxLayout();
    caseLayout->addWidget(new QLabel(tr("Select an OpenFOAM case:"), this));
    caseLayout->addWidget(m_caseCombo);
    mainLayout->addLayout(caseLayout);

    connect(m_caseCombo, &QComboBox::currentTextChanged, this,
            &RunSolverDialog::onCaseChanged);

    // potentialFoam group
    QGroupBox* potentialGroup =
        new QGroupBox(tr("Velocity Field Generation"), this);
    QVBoxLayout* potentialLayout = new QVBoxLayout(potentialGroup);
    mainLayout->addWidget(potentialGroup);

    // Choose potentialFoam
    m_potentialCheck = new QCheckBox(tr("Run potentialFoam"), potentialGroup);
    m_potentialCheck->setChecked(true);
    connect(m_potentialCheck, &QCheckBox::toggled,
            this, &RunSolverDialog::potentialCheckToggled);
    potentialLayout->addWidget(m_potentialCheck);

    // potentialFoam options
    QWidget* potentialOptionsWidget = new QWidget(potentialGroup);
    QFormLayout* potentialOptionsLayout =
        new QFormLayout(potentialOptionsWidget);
    potentialOptionsLayout->setContentsMargins(20, 5, 0, 0);
    potentialOptionsLayout->setSpacing(10);
    potentialLayout->addWidget(potentialOptionsWidget);

    // Update velocity boundaries
    m_updateVelocityCheck = new QCheckBox(tr("Update velocity boundaries "
        "(-initialiseUBCs)"), potentialOptionsWidget);
    m_updateVelocityCheck->setChecked(true);
    potentialOptionsLayout->addRow(m_updateVelocityCheck);

    // Write kinematic pressure
    m_writePressureCheck = new QCheckBox(tr("Write kinematic "
        "pressure (-writep)"), potentialOptionsWidget);
    m_writePressureCheck->setChecked(true);
    potentialOptionsLayout->addRow(m_writePressureCheck);

    // Solver group
    QGroupBox* simulationGroup = new QGroupBox(tr("Launch Simulation"), this);
    QVBoxLayout* simulationLayout = new QVBoxLayout(simulationGroup);
    mainLayout->addWidget(simulationGroup);

    // Choose solver
    m_runSolverCheck = new QCheckBox(simulationGroup);
    m_runSolverCheck->setChecked(true);
    simulationLayout->addWidget(m_runSolverCheck);
    connect(m_runSolverCheck, &QCheckBox::toggled,
            this, &RunSolverDialog::simulationCheckToggled);

    // Simulation options container
    QWidget* simulationOptionsWidget = new QWidget(simulationGroup);
    QFormLayout* simulationOptionsLayout =
        new QFormLayout(simulationOptionsWidget);
    simulationOptionsLayout->setContentsMargins(20, 5, 0, 0);
    simulationOptionsLayout->setSpacing(10);
    simulationLayout->addWidget(simulationOptionsWidget);

    // Create combo box for number of cores
    m_numCoresCombo = new QComboBox(simulationOptionsWidget);
    simulationOptionsLayout->addRow(tr("Number of cores:"), m_numCoresCombo);

    // Delete existing time files
    m_deleteFilesCheck = new QCheckBox(tr("Delete existing time files"),
                                               simulationOptionsWidget);
    m_deleteFilesCheck->setChecked(true);
    simulationOptionsLayout->addRow(m_deleteFilesCheck);

    // Allow reconstruction
    m_reconstructCheck = new QCheckBox(tr("Reconstruct results after "
                    "simulation (reconstructPar)"), simulationOptionsWidget);
    m_reconstructCheck->setChecked(true);
    simulationOptionsLayout->addRow(m_reconstructCheck);

    // Delete files
    m_deleteProcessorCheck = new QCheckBox(tr("Delete processor directories "
                    "after reconstruction"), simulationOptionsWidget);
    m_deleteProcessorCheck->setChecked(true);
    simulationOptionsLayout->addRow(m_deleteProcessorCheck);

    // Enable deletion of processor directories
    connect(m_reconstructCheck, &QCheckBox::toggled,
            m_deleteProcessorCheck, &QCheckBox::setEnabled);

    // Set file handling
    m_fileHandlingCombo = new QComboBox(simulationOptionsWidget);
    m_fileHandlingCombo->addItems({ "default", "uncollated", "collated" });
    m_fileHandlingCombo->setCurrentText("default");
    simulationOptionsLayout->addRow(tr("Parallel file handling:"),
                                    m_fileHandlingCombo);

    // Create OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                      QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this,
            &RunSolverDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Configure layout
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    // Update user interface
    onCaseChanged(selectedCase);
}

QByteArray RunSolverDialog::addPhiBlock(const QByteArray& fvSolutionContent) {
    OpenFoamDictionary dict(fvSolutionContent);
    // Abort if the file can't be read
    if (dict.hasSyntaxErrors()) {
        qWarning() << "fvSolution contains syntax errors. "
                      "Aborting Phi block insertion.";
        return fvSolutionContent;
    }

    // Check if the solvers dictionary exists
    QStringList solverKeys = dict.getDictKeys("solvers");
    if (solverKeys.isEmpty() && dict.getRawText().indexOf("solvers") == -1) {
        qWarning() << "No 'solvers' dictionary found in fvSolution.";
        return fvSolutionContent;
    }

    // Check if Phi (or phi) is already configured
    for (const QString& key : std::as_const(solverKeys)) {
        if (key.compare("Phi", Qt::CaseInsensitive) == 0) {
            return fvSolutionContent;
        }
    }

    // Standard potentialFoam parameters for the Phi equation
    QByteArray phiBlock;
    phiBlock = R"(
    Phi
    {
        solver          PCG;
        preconditioner  DIC;
        tolerance       1e-6;
        relTol          0;
    }
    )";

    // Inject the block using the AST
    dict.insertIntoDict("solvers", phiBlock);
    return dict.getRawText();
}

void RunSolverDialog::onCaseChanged(const QString& caseName) {
    // Check case properties
    m_isFoundation = !m_systemMgr.testCaseFlag(caseName, IsOpenCFD);
    m_isTransient = m_systemMgr.testCaseFlag(caseName, Transient);
    m_isCompressible = m_systemMgr.testCaseFlag(caseName, Compressible);
    m_isMultiPhase = m_systemMgr.testCaseFlag(caseName, Multiphase);
    m_isBuoyant = m_systemMgr.testCaseFlag(caseName, Buoyancy);
    QString casePath = m_systemMgr.getData(caseName).casePath;
    auto system = m_systemMgr.getSystem(caseName);

    // Control potentialFoam based on properties
    bool allowPotentialFoam =
        !m_isMultiPhase && !m_isBuoyant && !m_isCompressible;
    m_potentialCheck->setEnabled(allowPotentialFoam);
    m_updateVelocityCheck->setEnabled(allowPotentialFoam);
    m_writePressureCheck->setEnabled(allowPotentialFoam);
    if (allowPotentialFoam) {
        m_potentialCheck->setChecked(true);
        m_updateVelocityCheck->setChecked(true);
        m_writePressureCheck->setChecked(true);
    } else {
        m_potentialCheck->setChecked(false);
    }

    // Apply specific exclusionary overrides
    if (allowPotentialFoam && m_isTransient) {
        m_potentialCheck->setChecked(false);
    }

    // Get solver name from controlDict
    if (m_isFoundation) {
        m_solverName = "foamRun";
    } else {
        std::optional<QByteArray> controlData = system->getFileContent(
            casePath + "/" + caseName + "/system/controlDict");
        QString content = QString::fromUtf8(controlData.value());
        QRegularExpression regex(R"(^\s*application\s+([^\s;]+)\s*;)",
                                 QRegularExpression::MultilineOption);
        QRegularExpressionMatch match = regex.match(content);
        if (match.hasMatch()) {
            m_solverName = match.captured(1);
        }
    }

    m_runSolverCheck->setText(tr("Run %1").arg(m_solverName));

    // Initialize variables to prevent undefined behavior
    int totalCores = 1;
    QString output;
    if (system->launchShortUtility("nproc", output) == 0) {
        output.remove("\n");
        totalCores = output.toInt();
    }

    QStringList coreVals = { "1" };
    for (int i = 2; i <= totalCores; i++) {
        coreVals.append(QString::number(i));
    }

    // Read data from decomposeParDict
    std::optional<QByteArray> decomposeData = system->getFileContent(
        casePath + "/" + caseName + "/system/decomposeParDict");

    int numCores = 1;
    bool allowChange = false;
    if (decomposeData && !decomposeData->isEmpty()) {
        OpenFoamDictionary dict(decomposeData.value());
        if (!dict.hasSyntaxErrors()) {
            double numSubs = dict.getNumber("numberOfSubdomains");
            if (!std::isnan(numSubs) && numSubs > 0) {
                numCores = static_cast<int>(numSubs);
            }
            QString methodStr = dict.getString("method").toLower();
            if ((methodStr == "scotch") || (methodStr == "metis")) {
                allowChange = true;
            }
        }
    }

    // Update combo box
    m_numCoresCombo->clear();
    m_numCoresCombo->addItems(coreVals);
    m_numCoresCombo->setCurrentText(
        totalCores > 1 ? coreVals[coreVals.size() / 2] : "1");

    if ((numCores > 0) && (coreVals.contains(QString::number(numCores)))) {
        m_numCoresCombo->setCurrentText(QString::number(numCores));
        m_numCoresCombo->setEnabled(allowChange);
    }
}

void RunSolverDialog::onOkClicked() {
    // Get case name and number of cores
    QString caseName = m_caseCombo->currentText();
    int numCores = m_numCoresCombo->currentText().toInt();
    auto system = m_systemMgr.getSystem(caseName);

    // Access target system data
    QString casePath = m_systemMgr.getData(caseName).casePath;
    QString systemDir = casePath + "/" + caseName + "/system/";
    QString dictPath = systemDir + "decomposeParDict";

    // Update decomposeParDict
    if (numCores > 1) {
        std::optional<QByteArray> dictContent =
            system->getFileContent(dictPath);
        if (dictContent.has_value()) {
            OpenFoamDictionary dict(dictContent.value());
            if (!dict.hasSyntaxErrors()) {
                dict.setValue("numberOfSubdomains", QString::number(numCores));
                system->writeData(dict.getRawText(), dictPath);
            }
        }
    }

    QStringList commands;

    // Clean the case
    commands << "rm -rf processor* processors*";
    if (m_deleteFilesCheck->isChecked()) {
        commands << "foamListTimes -rm";
    }

    // Set Initial Conditions
    commands << "if [ -d 0.orig ]; then rm -rf 0 && cp -rf 0.orig 0; fi";

    // Run potentialFoam
    if (m_potentialCheck->isChecked()) {
        QString fvSolPath = systemDir + "fvSolution";
        std::optional<QByteArray> solutionContent =
            system->getFileContent(fvSolPath);
        if (solutionContent.has_value()) {            
            system->writeData(addPhiBlock(solutionContent.value()), fvSolPath);
        }

        // Create command for potentialFoam
        QString potentialCmd = "potentialFoam";
        if (m_updateVelocityCheck->isChecked()) {
            potentialCmd += " -initialiseUBCs";
        }
        if (m_writePressureCheck->isChecked()) {
            potentialCmd += " -writep";
        }
        commands << potentialCmd;
    }

    // Launch the solver
    if (numCores > 1) {
        // Determine file handling text
        QString fileHandlerStr = m_fileHandlingCombo->currentText();
        QString fileHandlerFlag = "";
        if (fileHandlerStr != "default") {
            fileHandlerFlag = " -fileHandler " + fileHandlerStr;
        }

        // Run decomposePar
        QString decompCmd = "decomposePar" + fileHandlerFlag;
        commands << decompCmd;

        // Create run command
        QString runCmd = QString("mpirun -np %1 %2 -parallel").
                         arg(numCores).arg(m_solverName);
        runCmd += fileHandlerFlag;
        commands << runCmd;

        // Create reconstruction command
        if (m_reconstructCheck->isChecked()) {
            QString reconCmd = "reconstructPar -constant" + fileHandlerFlag;
            commands << reconCmd;

            if (m_deleteProcessorCheck->isChecked()) {
                commands << "rm -rf processor* processors*";
            }
        }
    } else {
        commands << m_solverName;
    }

    // Launch the solver
    emit requestRunSolver(caseName, commands.join(" && "));
    QDialog::accept();
}

void RunSolverDialog::potentialCheckToggled(bool enabled) {
    m_updateVelocityCheck->setEnabled(enabled);
    m_writePressureCheck->setEnabled(enabled);
    if (!enabled) {
        m_updateVelocityCheck->setChecked(false);
        m_writePressureCheck->setChecked(false);
    }
}

void RunSolverDialog::simulationCheckToggled(bool enabled) {
    m_numCoresFoundationCombo->setEnabled(enabled);
    m_numCoresCombo->setEnabled(enabled);
    m_deleteFilesCheck->setEnabled(enabled);
    m_reconstructCheck->setEnabled(enabled);
    m_deleteProcessorCheck->setEnabled(enabled);
    if (!enabled) {
        m_deleteFilesCheck->setChecked(false);
        m_reconstructCheck->setChecked(false);
        m_deleteProcessorCheck->setChecked(false);
    }
}