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

#include "run_solver_dialog.h"

#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFormLayout>
#include <QStackedWidget>
#include <QString>

#include "parser/open_foam_dictionary.h"
#include "systems/system_manager.h"

QString getSolverName(const QByteArray &controlData, bool isFoundation);

// Create dialog to configure simulation process
RunSolverDialog::RunSolverDialog(const QString& selectedCase,
    bool isFoundation, const SystemManager& systemMgr, QWidget* parent):
    m_isFoundation(isFoundation), m_systemMgr(systemMgr), QDialog(parent) {
    // Set title and style
    setWindowTitle(tr("Solver Execution"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QFormLayout* mainLayout = new QFormLayout(this);
    mainLayout->setSpacing(20);

    // Select case
    m_caseCombo = new QComboBox(this);
    m_caseCombo->addItems(m_systemMgr.getCases());
    m_caseCombo->setCurrentText(selectedCase);
    mainLayout->addRow(tr("Select an OpenFOAM case:"), m_caseCombo);
    connect(m_caseCombo, &QComboBox::currentTextChanged,
        this, &RunSolverDialog::onCaseChanged);

    // Create stacked widget
    m_layoutStack = new QStackedWidget(this);
    m_layoutStack->addWidget(createKeySightWidget());
    m_layoutStack->addWidget(createFoundationWidget());
    mainLayout->addRow(m_layoutStack);
    m_layoutStack->setCurrentIndex(m_isFoundation ? 1 : 0);

    // Create OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                      QDialogButtonBox::Cancel, this);
    mainLayout->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this,
            &RunSolverDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Configure layout
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    // Update user interface
    onCaseChanged(selectedCase);
}

QWidget* RunSolverDialog::createFoundationWidget() {
    // foamRun group
    QGroupBox* foamRunGroup =
        new QGroupBox(tr("foamRun Configuration"), this);
    QFormLayout* foamRunLayout = new QFormLayout(foamRunGroup);
    foamRunLayout->setContentsMargins(10, 20, 10, 10);
    foamRunLayout->setSpacing(20);

    // Select solver
    m_selectSolverCombo = new QComboBox(foamRunGroup);
    QStringList solverList = { "incompressibleFluid", "compressibleFluid",
        "incompressibleVoF", "compressibleVoF", "multiphaseVoF", "solid" };
    m_selectSolverCombo->addItems(solverList);
    m_selectSolverCombo->setEditable(true);
    foamRunLayout->addRow(tr("Solver module:"), m_selectSolverCombo);

    // Set number of cores
    m_numCoresFoundationCombo = new QComboBox(foamRunGroup);
    foamRunLayout->addRow(tr("Number of cores:"), m_numCoresFoundationCombo);

    // Delete existing time files
    m_deleteFilesFoundationCheck =
        new QCheckBox(tr("Delete existing time files"), foamRunGroup);
    m_deleteFilesFoundationCheck->setChecked(true);
    foamRunLayout->addRow(m_deleteFilesFoundationCheck);

    // Allow reconstruction
    m_reconstructFoundationCheck =
        new QCheckBox(tr("Reconstruct results after simulation "
                         "(reconstructPar)"), foamRunGroup);
    m_reconstructFoundationCheck->setChecked(true);
    foamRunLayout->addRow(m_reconstructFoundationCheck);

    // Delete files
    m_deleteProcessorFoundationCheck = new QCheckBox(tr("Delete processor "
        "directories after reconstruction"), foamRunGroup);
    m_deleteProcessorFoundationCheck->setChecked(true);
    foamRunLayout->addRow(m_deleteProcessorFoundationCheck);

    // Enable deletion of processor directories
    connect(m_reconstructFoundationCheck, &QCheckBox::toggled,
            m_deleteProcessorFoundationCheck, &QCheckBox::setEnabled);

    // Enable function objects
    m_functionCheck =
        new QCheckBox(tr("Enable function objects"), foamRunGroup);
    m_functionCheck->setChecked(true);
    foamRunLayout->addRow(m_functionCheck);

    // Set file handling
    m_fileHandlingCombo = new QComboBox(foamRunGroup);
    m_fileHandlingCombo->addItems({ "default", "uncollated", "collated" });
    m_fileHandlingCombo->setCurrentText("default");
    foamRunLayout->addRow(tr("Parallel file handling:"), m_fileHandlingCombo);
    return foamRunGroup;
}

QWidget* RunSolverDialog::createKeySightWidget() {
    // Create new widget
    QWidget* keySightWidget = new QWidget(m_layoutStack);
    QFormLayout* mainLayout = new QFormLayout(keySightWidget);
    mainLayout->setSpacing(15);

    // potentialFoam group
    QGroupBox* potentialGroup =
        new QGroupBox(tr("Velocity Field Generation"), this);
    QVBoxLayout* potentialLayout = new QVBoxLayout(potentialGroup);
    mainLayout->addRow(potentialGroup);

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
    mainLayout->addRow(simulationGroup);

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
    m_numCoresKeySightCombo = new QComboBox(simulationOptionsWidget);
    simulationOptionsLayout->addRow(tr("Number of cores:"),
                                    m_numCoresKeySightCombo);

    // Delete existing time files
    m_deleteFilesKeySightCheck = new QCheckBox(tr("Delete existing time files"),
                                       simulationOptionsWidget);
    m_deleteFilesKeySightCheck->setChecked(true);
    simulationOptionsLayout->addRow(m_deleteFilesKeySightCheck);

    // Allow reconstruction
    m_reconstructKeySightCheck =
        new QCheckBox(tr("Reconstruct results after simulation "
                         "(reconstructPar)"), simulationOptionsWidget);
    m_reconstructKeySightCheck->setChecked(true);
    simulationOptionsLayout->addRow(m_reconstructKeySightCheck);

    // Delete files
    m_deleteProcessorKeySightCheck =
        new QCheckBox(tr("Delete processor directories after reconstruction"),
                      simulationOptionsWidget);
    m_deleteProcessorKeySightCheck->setChecked(true);
    simulationOptionsLayout->addRow(m_deleteProcessorKeySightCheck);

    // Enable deletion of processor directories
    connect(m_reconstructKeySightCheck, &QCheckBox::toggled,
            m_deleteProcessorKeySightCheck, &QCheckBox::setEnabled);
    return keySightWidget;
}

// Get solver name from controlDict
QString getSolverName(const QByteArray &controlData, bool isFoundation) {
    QString content = QString::fromUtf8(controlData);
    QRegularExpressionMatch match;
    if (isFoundation) {
        QRegularExpression regex(R"(^\s*solver\s+([^\s;]+)\s*;)",
            QRegularExpression::MultilineOption);
        match = regex.match(content);
        if (match.hasMatch()) return match.captured(1);
    }

    QRegularExpression regex(R"(^\s*application\s+([^\s;]+)\s*;)",
        QRegularExpression::MultilineOption);
    match = regex.match(content);
    return match.hasMatch() ? match.captured(1) : QString();
}

std::pair<int, bool> getRunData(const QByteArray& dictContent) {
    // Check input
    if (dictContent.isEmpty())
        return std::make_pair(1, false);

    int numCores = -1;
    bool allowChange = false;

    // Create AST from decomposeParDict
    OpenFoamDictionary dict(dictContent);
    if (dict.hasSyntaxErrors()) {
        return std::make_pair(numCores, allowChange);
    }

    // Parse numberOfSubdomains
    double numSubs = dict.getNumber("numberOfSubdomains");
    if (!std::isnan(numSubs) && numSubs > 0) {
        numCores = static_cast<int>(numSubs);
    }

    // Check the decomposition method
    QString methodStr = dict.getString("method").toLower();
    if ((methodStr == "scotch") || (methodStr == "metis")) {
        allowChange = true;
    }
    return std::make_pair(numCores, allowChange);
}

QByteArray addPhiBlock(const QByteArray& fvSolutionContent) {
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
    QByteArray phiBlock = R"(
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

void RunSolverDialog::onCaseChanged(QString caseName) {
    // Check if using Foundation release of OpenFOAM
    QString openFoamPath = m_systemMgr.getData(caseName).openFoamPath;
    QString dirName = QDir(openFoamPath).dirName();
    const QRegularExpression foundationRegex("^openfoam\\d{2}$",
        QRegularExpression::CaseInsensitiveOption);
    m_isFoundation = foundationRegex.match(dirName).hasMatch();
    m_layoutStack->setCurrentIndex(m_isFoundation ? 1 : 0);

    // Read controlDict
    QString casePath = m_systemMgr.getData(caseName).casePath;
    auto system = m_systemMgr.getSystem(caseName);
    QByteArray controlData = system->getFileContent(
        casePath + "/" + caseName + "/system/controlDict");
    m_solverName = getSolverName(controlData, m_isFoundation);

    // Update solver combo
    if (m_isFoundation) {
        if (!m_solverName.isEmpty()) {
            m_selectSolverCombo->setCurrentText(m_solverName);
        } else {
            m_selectSolverCombo->setCurrentIndex(0);
        }
    } else {
        m_runSolverCheck->setText(tr("Run %1").arg(m_solverName));
    }

    // Get the number of cores
    int numCores = 1;
    QString output;
    if (system->launchShortUtility("nproc", output) == 0) {
        output.remove("\n");
        numCores = output.toInt();
    }

    // Generate QStringList for number of cores
    QStringList coreVals = { "1" };
    for (int i=2; i<=numCores; i++) {
        coreVals.append(QString::number(i));
    }

    // Get the number of cores based on decomposeParDict
    QByteArray decomposeData = system->getFileContent(
        casePath + "/" + caseName + "/system/decomposeParDict");
    std::pair<int, bool> data = getRunData(decomposeData);

    // Update combo boxes
    for (QComboBox* combo :
         { m_numCoresFoundationCombo, m_numCoresKeySightCombo }) {
        combo->clear();
        combo->addItems(coreVals);
        combo->setCurrentText(
            numCores > 1 ? coreVals[coreVals.size() / 2] : "1");

        if ((data.first > 0) &&
            (coreVals.contains(QString::number(data.first)))) {
            combo->setCurrentText(QString::number(data.first));
            combo->setEnabled(data.second);
        }
    }

    // this->adjustSize();
}

void RunSolverDialog::onOkClicked() {
    // Get case name and number of cores
    QString caseName = m_caseCombo->currentText();
    int numCores = (m_isFoundation) ?
        m_numCoresFoundationCombo->currentText().toInt() :
        m_numCoresKeySightCombo->currentText().toInt();
    auto system = m_systemMgr.getSystem(caseName);

    // Access target system data
    QString casePath = m_systemMgr.getData(caseName).casePath;
    QString dictPath = casePath + "/" + caseName + "/system/decomposeParDict";

    // Update decomposeParDict
    if (numCores > 1) {
        QByteArray dictContent = system->getFileContent(dictPath);
        OpenFoamDictionary dict(dictContent);

        // Only attempt to mutate if the dictionary parsed successfully
        if (!dict.hasSyntaxErrors()) {
            dict.setValue("numberOfSubdomains", QString::number(numCores));

            // Write the updated text to decomposeParDict
            system->writeData(dict.getRawText(), dictPath);
        }
    }

    // Base command
    QStringList commands;
    commands << "rm -rf processor* processors*";
    commands << "if [ -d 0.orig ]; then rm -rf 0 && cp -rf 0.orig 0; fi";

    // Create command depending on release type
    if (m_isFoundation) {
        if (m_deleteFilesFoundationCheck->isChecked()) {
            commands << "foamListTimes -rm";
        }

        QString runCmd;
        if (numCores > 1) {
            commands << "decomposePar";
            runCmd = QString("mpirun -np %1 foamRun -parallel").arg(numCores);
        } else {
            runCmd = "foamRun";
        }

        runCmd +=
            QString(" -solver %1").arg(m_selectSolverCombo->currentText());
        if (m_fileHandlingCombo->currentText() == "collated") {
            runCmd += " -fileHandler collated";
        }
        if (!m_functionCheck->isChecked()) {
            runCmd += " -noFunctionObjects";
        }
        commands << runCmd;

        if (numCores > 1 && m_reconstructFoundationCheck->isChecked()) {
            QString reconCmd = "reconstructPar";
            if (m_fileHandlingCombo->currentText() == "collated") {
                reconCmd += " -fileHandler collated";
            }
            reconCmd += " -constant";
            commands << reconCmd;

            if (m_deleteProcessorFoundationCheck->isChecked()) {
                commands << "rm -rf processor* processors*";
            }
        }
    } else {
        // Delete time directories
        if (m_deleteFilesKeySightCheck->isChecked()) {
            commands << "foamListTimes -rm";
        }

        // Generate velocity field
        if (m_potentialCheck->isChecked()) {
            // Add Phi block to fvSolution
            dictPath = casePath + "/" + caseName + "/system/fvSolution";
            QByteArray solutionContent = system->getFileContent(dictPath);
            system->writeData(addPhiBlock(solutionContent), dictPath);

            // Create command for potentialFoam with proper leading spaces
            QString potentialCmd = "potentialFoam";
            if (m_updateVelocityCheck->isChecked()) {
                potentialCmd += " -initialiseUBCs";
            }
            if (m_writePressureCheck->isChecked()) {
                potentialCmd += " -writep";
            }
            commands << potentialCmd;
        }

        // Launch simulation using discrete list elements
        if (m_runSolverCheck->isChecked()) {
            if (numCores > 1) {
                commands << "decomposePar";
                commands << QString("mpirun -np %1 %2 -parallel").
                    arg(numCores).arg(m_solverName);

                if (m_reconstructKeySightCheck->isChecked()) {
                    commands << "reconstructPar -constant";
                    if (m_deleteProcessorKeySightCheck->isChecked()) {
                        commands << "rm -rf processor* processors*";
                    }
                }
            } else {
                commands << m_solverName;
            }
        }
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
    m_numCoresKeySightCombo->setEnabled(enabled);
    m_deleteFilesKeySightCheck->setEnabled(enabled);
    m_reconstructKeySightCheck->setEnabled(enabled);
    m_deleteProcessorKeySightCheck->setEnabled(enabled);
    if (!enabled) {
        m_deleteFilesKeySightCheck->setChecked(false);
        m_reconstructKeySightCheck->setChecked(false);
        m_deleteProcessorKeySightCheck->setChecked(false);
    }
}