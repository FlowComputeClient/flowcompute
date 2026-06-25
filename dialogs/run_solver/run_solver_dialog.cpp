#include "run_solver_dialog.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFormLayout>
#include <QString>

#include "../../../main_window.h"
#include "../../parser/open_foam_dictionary.h"

QString getSolverName(const QByteArray &controlData);

RunSolverDialog::RunSolverDialog(const QString& selectedCase,
                             const QStringList& caseList,
                             QWidget* parent): QDialog(parent) {

    // Set title and style
    setWindowTitle(tr("Solver Execution"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QFormLayout* mainLayout = new QFormLayout(this);
    mainLayout->setSpacing(15);

    // Select case
    m_caseCombo = new QComboBox(this);
    m_caseCombo->addItems(caseList);
    m_caseCombo->setCurrentText(selectedCase);
    mainLayout->addRow(tr("Select an OpenFOAM case:"), m_caseCombo);
    connect(m_caseCombo, &QComboBox::currentTextChanged, this, &RunSolverDialog::onCaseChanged);

    // potentialFoam group
    QGroupBox* potentialGroup = new QGroupBox(tr("Velocity Field Generation"), this);
    QVBoxLayout* potentialLayout = new QVBoxLayout(potentialGroup);
    mainLayout->addRow(potentialGroup);

    // Choose potentialFoam
    m_potentialCheck = new QCheckBox(tr("Run potentialFoam"), potentialGroup);
    m_potentialCheck->setChecked(true);
    connect(m_potentialCheck, &QCheckBox::toggled,
            this, &RunSolverDialog::potentialCheckToggled);
    potentialLayout->addWidget(m_potentialCheck);

    // potentialFoam options container
    QWidget* potentialOptionsWidget = new QWidget(potentialGroup);
    QFormLayout* potentialOptionsLayout = new QFormLayout(potentialOptionsWidget);
    potentialOptionsLayout->setContentsMargins(20, 5, 0, 0);
    potentialOptionsLayout->setSpacing(10);
    potentialLayout->addWidget(potentialOptionsWidget);

    // Update velocity boundaries
    m_updateVelocityCheck = new QCheckBox(tr("Update velocity boundaries (-initialiseUBCs)"), potentialOptionsWidget);
    m_updateVelocityCheck->setChecked(true);
    potentialOptionsLayout->addRow(m_updateVelocityCheck);

    // Write kinematic pressure
    m_writePressureCheck = new QCheckBox(tr("Write kinematic pressure (-writep)"), potentialOptionsWidget);
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
    QFormLayout* simulationOptionsLayout = new QFormLayout(simulationOptionsWidget);
    simulationOptionsLayout->setContentsMargins(20, 5, 0, 0);
    simulationOptionsLayout->setSpacing(10);
    simulationLayout->addWidget(simulationOptionsWidget);

    // Create combo box for number of cores
    m_numCoresCombo = new QComboBox(simulationOptionsWidget);
    simulationOptionsLayout->addRow(tr("Number of cores:"), m_numCoresCombo);

    // Delete existing time files
    m_deleteFilesCheck = new QCheckBox(tr("Delete existing time files"), simulationOptionsWidget);
    m_deleteFilesCheck->setChecked(true);
    simulationOptionsLayout->addRow(m_deleteFilesCheck);

    // Allow reconstruction
    m_reconstructCheck = new QCheckBox(tr("Reconstruct results after simulation (reconstructPar)"), simulationOptionsWidget);
    m_reconstructCheck->setChecked(true);
    simulationOptionsLayout->addRow(m_reconstructCheck);

    // Delete files
    m_deleteProcessorCheck = new QCheckBox(tr("Delete processor directories after reconstruction"), simulationOptionsWidget);
    m_deleteProcessorCheck->setChecked(true);
    simulationOptionsLayout->addRow(m_deleteProcessorCheck);

    // Create OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &RunSolverDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Configure layout
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    // Enable deletion of processor directories
    connect(m_reconstructCheck, &QCheckBox::toggled, m_deleteProcessorCheck, &QCheckBox::setEnabled);

    // Update user interface
    onCaseChanged(selectedCase);

    this->adjustSize();
}

// Get solver name from controlDict
QString getSolverName(const QByteArray &controlData) {
    QString content = QString::fromUtf8(controlData);
    QRegularExpression regex(R"(^\s*application\s+([^\s;]+)\s*;)");
    regex.setPatternOptions(QRegularExpression::MultilineOption);
        QRegularExpressionMatch match = regex.match(content);
        if (match.hasMatch()) {
        return match.captured(1);
    }
    return QString();
}

std::pair<int, bool> getRunData(const QByteArray& dictContent) {

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

    // Abort if the file is already corrupted to prevent further damage
    if (dict.hasSyntaxErrors()) {
        qWarning() << "fvSolution contains syntax errors. Aborting Phi block insertion.";
        return fvSolutionContent;
    }

    QStringList solverKeys = dict.getDictKeys("solvers");

    // Check if the solvers dictionary exists
    if (solverKeys.isEmpty() && dict.getRawText().indexOf("solvers") == -1) {
        qWarning() << "No 'solvers' dictionary found in fvSolution.";
        return fvSolutionContent;
    }

    // Check if Phi (or phi) is already configured
    for (const QString& key : std::as_const(solverKeys)) {
        if (key.compare("Phi", Qt::CaseInsensitive) == 0) {
            return fvSolutionContent; // Already present, no changes needed
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

    // Access case data
    m_mainWin = qobject_cast<MainWindow*>(this->parent());
    QString casePath = m_mainWin->m_caseMap[caseName].casePath;
    int targetId = m_mainWin->m_caseMap[caseName].targetSystemId;

    // Update solver name
    QByteArray controlData = m_mainWin->targetSystems[targetId]->getFileContent(
        casePath + "/" + caseName + "/system/controlDict");
    m_solverName = getSolverName(controlData);
    m_runSolverCheck->setText(tr("Run %1").arg(m_solverName));

    // Get the number of cores
    int numCores = 1;
    QString output;
    if (m_mainWin->targetSystems[targetId]->launchShortUtility("nproc", output) == 0) {
        output.remove("\n");
        numCores = output.toInt();
    }

    // Generate QStringList for number of cores
    QStringList coreVals = { "1" };
    for (int i=2; i<=numCores; i++) {
        coreVals.append(QString::number(i));
    }

    // Update combo box
    m_numCoresCombo->clear();
    m_numCoresCombo->addItems(coreVals);
    if (numCores > 1) {
        m_numCoresCombo->setCurrentText(coreVals[coreVals.size()/2]);
    } else {
        m_numCoresCombo->setCurrentText("1");
    }

    // Set the number of cores based on decomposeParDict
    QByteArray deomposeData = m_mainWin->targetSystems[targetId]->getFileContent(
        casePath + "/" + caseName + "/system/decomposeParDict");
    std::pair<int, bool> data = getRunData(deomposeData);
    if ((data.first > 0) && (coreVals.contains(QString::number(data.first)))) {
        m_numCoresCombo->setCurrentText(QString::number(data.first));
        m_numCoresCombo->setEnabled(data.second);
    }
}

void RunSolverDialog::onOkClicked() {

    // Get case name and number of cores
    QString caseName = m_caseCombo->currentText();
    int numCores = m_numCoresCombo->currentText().toInt();

    // Access target system data
    QString casePath = m_mainWin->m_caseMap[caseName].casePath;
    int targetId = m_mainWin->m_caseMap[caseName].targetSystemId;
    QString dictPath = casePath + "/" + caseName + "/system/decomposeParDict";

    // Update decomposeParDict
    if (numCores > 1) {
        QByteArray dictContent = m_mainWin->targetSystems[targetId]->getFileContent(dictPath);
        OpenFoamDictionary dict(dictContent);

        // Only attempt to mutate if the dictionary parsed successfully
        if (!dict.hasSyntaxErrors()) {
            dict.setValue("numberOfSubdomains", QString::number(numCores));

            // Write the updated text to decomposeParDict
            m_mainWin->targetSystems[targetId]->writeData(dict.getRawText(), dictPath);
        }
    }

    // Base command
    QString solverCmd = "rm -rf processor*; if [ -d 0.orig ]; then rm -rf 0 && cp -rf 0.orig 0; fi && ";

    // Clean time Directories
    if (m_deleteFilesCheck->isChecked()) {
        solverCmd += "foamListTimes -rm && ";
    }

    // Generate velocity field
    if (m_potentialCheck->isChecked()) {

        // Add Phi block to fvSolution
        dictPath = casePath + "/" + caseName + "/system/fvSolution";
        QByteArray solutionContent = m_mainWin->targetSystems[targetId]->getFileContent(dictPath);
        m_mainWin->targetSystems[targetId]->writeData(addPhiBlock(solutionContent), dictPath);

        solverCmd += "potentialFoam";
        if (m_updateVelocityCheck->isChecked()) {
            solverCmd += " -initialiseUBCs";
        }
        if (m_writePressureCheck->isChecked()) {
            solverCmd += " -writep";
        }
        solverCmd += " && ";
    }

    // Launch simulation
    if (m_runSolverCheck->isChecked()) {
        if (numCores > 1) {
            solverCmd += QString("decomposePar && mpirun -np %1 %2 -parallel").arg(
                QString::number(numCores), m_solverName);
        } else {
            solverCmd += m_solverName;
        }

        // Post-Processing
        if (numCores > 1 && m_reconstructCheck->isChecked()) {
            solverCmd += " && reconstructPar -constant";
            if (m_deleteProcessorCheck->isChecked()) {
                solverCmd += " && rm -rf processor*";
            }
        }
    }

    // Launch the solver
    m_mainWin->runSolver(caseName, solverCmd);

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