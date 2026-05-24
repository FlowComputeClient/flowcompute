#include "wizard_solver.h"

#include "../../main_window.h"

// Page IDs
enum {
    Page_Control = 0,
    Page_Physics = 1,
    Page_Boundary = 2,
    Page_Algorithm = 3,
    Page_Parallel = 4
};

QList<FlowCompute::BoundaryPatch> parseBoundaryPatches(const QByteArray& fileData);

SolverWizard::SolverWizard(const QString& caseName,
                           const QList<FlowCompute::SolverFamily>& families,
                           const FlowCompute::TurbulenceDatabase& turbModels,
                           const QHash<QString, FlowCompute::FieldData>& fieldData,
                           const QList<FlowCompute::BoundaryCondition>& boundaryConditions,
                           QWidget *parent): QWizard(parent), m_caseName(caseName),
    m_families(families), m_turbModels(turbModels), m_fieldData(fieldData),
    m_boundaryConditions(boundaryConditions) {

    // Configure the wizard's appearance
    setWizardStyle(QWizard::ClassicStyle);
    setStyleSheet("color: black;");
    setWindowTitle("Solver Configuration Wizard");

    // Create map to look up solver algorithms
    for (const auto& family : std::as_const(m_families)) {
        for (const auto& solver : std::as_const(family.solvers)) {
            m_solverAlgorithmMap.insert(solver.name, solver.algorithm);
        }
    }

    // Access the main window
    mainWin = qobject_cast<MainWindow*>(this->parentWidget());

    // Add pages
    setPage(Page_Control, new ControlPage(families, this));
    setPage(Page_Physics, new PhysicsPage(turbModels, this));
    setPage(Page_Boundary, new BoundaryPage(fieldData, boundaryConditions, this));
    setPage(Page_Algorithm, new AlgorithmPage(this));
    setPage(Page_Parallel, new ParallelPage(this));

    setOption(QWizard::NoBackButtonOnStartPage);

    // Set the finish button to "Run Simulation"
    setButtonText(QWizard::FinishButton, tr("Run Simulation"));
}

bool SolverWizard::parseFiles() {

    // Access OpenFOAM path on server
    int targetId = mainWin->caseMap[m_caseName].targetSystemId;
    QString casePath = mainWin->caseMap[m_caseName].casePath;

    // Declare variables
    QString fileName;
    QByteArray fileData;
    std::shared_ptr<OpenFoamDictionary> dict;

    // Load and parse boundary file
    fileName = "constant/polyMesh/boundary";
    fileData = mainWin->targetSystems[targetId]->getFileContent(casePath + "/" + m_caseName + "/" + fileName);
    if (!fileData.isEmpty()) {
        m_boundaries = parseBoundaryPatches(fileData);
    }
    if(m_boundaries.isEmpty()) {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle(tr("Missing Boundary Error"));
        msgBox.setText(tr("Missing or incomplete boundary file (constant/polyMesh/boundary)"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        int result = msgBox.exec();
        if (result == QMessageBox::Ok) {
            return false;
        }
    }

    // Load and parse controlDict
    fileName = "system/controlDict";
    fileData = mainWin->targetSystems[targetId]->getFileContent(casePath + "/" + m_caseName + "/" + fileName);
    if (!fileData.isEmpty()) {
        dict = std::make_shared<OpenFoamDictionary>(fileData);
        if(!dict->hasSyntaxErrors()) {
            m_dictMap.insert(fileName, dict);
            m_controlConfig = SolverIO::parseControlConfig(dict);
        } else {
            if(!showParsingErrorMessage(fileName)) {
                return false;
            }
            return false;
        }
    }

    // Load and parse turbulenceProperties
    fileName = "constant/turbulenceProperties";
    fileData = mainWin->targetSystems[targetId]->getFileContent(casePath + "/" + m_caseName + "/" + fileName);
    if (!fileData.isEmpty()) {
        dict = std::make_shared<OpenFoamDictionary>(fileData);
        if(!dict->hasSyntaxErrors()) {
            m_dictMap.insert(fileName, dict);
            m_physicsConfig = SolverIO::parseTurbulenceProperties(dict);
        } else {
            if(!showParsingErrorMessage(fileName)) {
                return false;
            }
            return false;
        }
    }

    // Load and parse transportProperties
    fileName = "constant/transportProperties";
    fileData = mainWin->targetSystems[targetId]->getFileContent(casePath + "/" + m_caseName + "/" + fileName);
    if (!fileData.isEmpty()) {
        dict = std::make_shared<OpenFoamDictionary>(fileData);
        if(!dict->hasSyntaxErrors()) {
            m_dictMap.insert(fileName, dict);
            SolverIO::parseTransportProperties(dict, m_physicsConfig);
        } else {
            if(!showParsingErrorMessage(fileName)) {
                return false;
            }
            return false;
        }
    }

    return true;
}

QList<FlowCompute::BoundaryPatch> parseBoundaryPatches(const QByteArray& fileData) {
    QList<FlowCompute::BoundaryPatch> patches;

    // Convert the raw byte array from the WSL socket into a QString.
    QString text = QString::fromUtf8(fileData);

    // Remove comments
    text.replace(QRegularExpression("/\\*.*?\\*/", QRegularExpression::DotMatchesEverythingOption), "");
    text.replace(QRegularExpression("//.*"), "");

    // Look through top-level parentheses
    int startIdx = text.indexOf('(');
    int endIdx = text.lastIndexOf(')');
    if (startIdx == -1 || endIdx == -1 || startIdx >= endIdx) {
        return patches;
    }

    QString listContent = text.mid(startIdx + 1, endIdx - startIdx - 1);

    // Step 1: Regex to capture the patch name and everything inside its { } block
    // [^}]* matches anything that is NOT a closing brace, preventing bleed into the next block
    QRegularExpression reBlock("([A-Za-z0-9_\\-]+)\\s*\\{([^}]*)\\}");

    // Step 2: Regex to capture the patch type inside the block
    QRegularExpression reType("type\\s+([A-Za-z0-9_\\-]+)\\s*;");

    QRegularExpressionMatchIterator i = reBlock.globalMatch(listContent);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        FlowCompute::BoundaryPatch bp;
        bp.name = match.captured(1); // The patch name

        QString blockContent = match.captured(2); // The text inside the { }
        QRegularExpressionMatch typeMatch = reType.match(blockContent);
        bp.type = (typeMatch.hasMatch()) ? typeMatch.captured(1) : "unknown";
        patches.append(bp);
    }
    return patches;
}

QStringList SolverWizard::getSolverFields() {

    // Access family and solver names
    QString solverFamily = field("solverFamily").toString();
    QString solverName = field("solverName").toString();

    // Iterate through families
    for (const auto& family : std::as_const(m_families)) {
        if (family.familyName == solverFamily) {
            for (const auto& solver : family.solvers) {
                if (solver.name == solverName) {
                    return solver.fields;
                }
            }
            break;
        }
    }
    return QStringList();
}

QStringList SolverWizard::getTurbulenceFields() {

    // Access family and solver names
    QString turbulenceModel = field("turbulenceModel").toString();
    QString turbulenceCategory = field("turbulenceCategory").toString();
    QString turbulenceSubCategory = field("turbulenceSubCategory").toString();

    if (turbulenceModel.toLower() == "laminar") {
        return QStringList();
    }

    if (m_turbModels.contains(turbulenceCategory)) {
        if(m_turbModels[turbulenceCategory].contains(turbulenceSubCategory)) {
            for(const auto& model: m_turbModels[turbulenceCategory][turbulenceSubCategory]) {
                if (model.name == turbulenceModel) {
                    return model.fields;
                }
            }
        }
    }
    return QStringList();
};

bool SolverWizard::showParsingErrorMessage(QString fileName) {

    QMessageBox errorDialog(this);
    errorDialog.setWindowTitle("Parse Error");
    errorDialog.setText(QString("<b>Failed to parse %1.</b>").arg(fileName));
    errorDialog.setInformativeText("The file may contain syntax errors or unsupported keywords.");
    errorDialog.setIcon(QMessageBox::Warning);

    // Add the custom choices and assign them roles
    QPushButton *editBtn = errorDialog.addButton("Edit File", QMessageBox::ActionRole);
    QPushButton *overwriteBtn = errorDialog.addButton("Overwrite with Defaults", QMessageBox::DestructiveRole);
    QPushButton *cancelBtn = errorDialog.addButton("Cancel", QMessageBox::RejectRole);
    errorDialog.setDefaultButton(editBtn);

    // Execute the dialog modally
    errorDialog.exec();

    // Determine which choice the user made
    if (errorDialog.clickedButton() == editBtn) {
        mainWin->displayText(fileName.split('/').last(), m_caseName + "/system");
        reject();
        return false;
    } else if (errorDialog.clickedButton() == overwriteBtn) {
        return true;
    } else if (errorDialog.clickedButton() == cancelBtn) {
        return false;
    }
    return false;
}

// Access the algorithm for the selected solver
FlowCompute::Algorithm SolverWizard::getSolverAlgorithm() {

    // Get the name of the selected solver
    QString solverName = field("solverName").toString();

    // Ensure the solver exists in our map to prevent crashes
    if (!m_solverAlgorithmMap.contains(solverName)) {
        return FlowCompute::Algorithm::UNKNOWN;
    }

    // Determine the algorithm
    return m_solverAlgorithmMap.value(solverName);
}

bool SolverWizard::validateCurrentPage() {

    /*
    // Validate first page
    if (currentId() == Page_Control) {

        // Read registered fields
        m_caseName = field("caseName").toString();
        m_targetSystemId = field("targetSystemId").toInt();

        // Check if case name is in the case map
        int count = 0;
        QString newName;
        if (mainWin->caseMap.contains(m_caseName)) {
            count = 1;
            while (true) {
                newName = m_caseName + "_" + QString::number(count++);
                if (!mainWin->caseMap.contains(newName)) {
                    break;
                }
            }
        }

        // If name is in case map, ask user
        if (count > 0) {
            QMessageBox::StandardButton reply;
            QString msg = tr("There is already a case named '%1'.\nCreate '%2' instead?")
                              .arg(m_caseName, newName);
            reply = QMessageBox::question(this, tr("Existing Case Detected"), msg,
                                          QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

            // Fail validation if the response is no
            if (reply == QMessageBox::No) {
                return false;
            } else {
                m_caseName = newName;
            }
        }

        // Determine OpenFOAM installation
        QStringList ofList = mainWin->targetSystems[m_targetSystemId]->findOpenFoam();
        if(ofList.empty()) {
            QMessageBox::critical(this, tr("Missing OpenFOAM"), tr("No OpenFOAM installations detected..."));
            m_openFoamPath = "";
            return false;
        } else if (ofList.size() > 1) {
            m_openFoamPath = createSelectionDialog(ofList);
            return !m_openFoamPath.isEmpty();
        } else {
            m_openFoamPath = ofList[0];
        }
        return true;
    }
    */
    return QWizard::validateCurrentPage();
}

void SolverWizard::accept() {

    QWizard::accept();
}
