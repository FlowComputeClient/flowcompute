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

#include "wizard_new_case.h"

#include "../../main_window.h"

NewCaseWizard::NewCaseWizard(QWidget *parent): QWizard(parent) {

    setWizardStyle(QWizard::ClassicStyle);
    setWindowTitle("New Case Wizard");

    // Access the main window
    mainWin = qobject_cast<MainWindow*>(this->parentWidget());

    // Add pages
    setPage(static_cast<int>(WizardPage::Page_Intro), new IntroPage(this));
    setPage(static_cast<int>(WizardPage::Page_Tutorial), new TutorialPage(this));
    setPage(static_cast<int>(WizardPage::Page_Interactive), new InteractivePage(this));
    setPage(static_cast<int>(WizardPage::Page_Project), new ProjectPage(this));

    setOption(QWizard::NoBackButtonOnStartPage);
}

// Set the target system
void NewCaseWizard::setTargetSystemId(int id) {
    m_targetId = static_cast<TargetType>(id);
}

QStringList NewCaseWizard::getHomeFolders() {
    QStringList results = mainWin->targetSystems[m_targetId]->getHomeFolders();
    return results;
}

QStringList NewCaseWizard::getTutorials() {
    QStringList results = mainWin->targetSystems[m_targetId]->getTutorials(m_openFoamPath);
    return results;
}

bool NewCaseWizard::validateCurrentPage() {

    // Validate first page
    if (currentId() == static_cast<int>(WizardPage::Page_Intro)) {

        // Read registered fields
        m_caseName = field("caseName").toString();
        m_targetId = static_cast<TargetType>(field("targetSystemId").toInt());

        // Check if case name is in the case map
        int count = 0;
        QString newName;
        if (mainWin->m_caseMap.contains(m_caseName)) {
            count = 1;
            while (true) {
                newName = m_caseName + "_" + QString::number(count++);
                if (!mainWin->m_caseMap.contains(newName)) {
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
        QStringList ofList = mainWin->targetSystems[m_targetId]->findOpenFoam();
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
    return QWizard::validateCurrentPage();
}

QString NewCaseWizard::createSelectionDialog(const QStringList& paths) {

    // Create the dialog
    QDialog dialog(this);
    dialog.setWindowTitle("Multiple OpenFOAM Installations Detected");

    // Layout for the dialog
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(15);
    layout->setContentsMargins(10, 10, 10, 10);

    // Add label
    layout->addWidget(new QLabel(tr("Select one of the following OpenFOAM installations:"), &dialog));

    // Create sub-layout
    QVBoxLayout *subLayout = new QVBoxLayout();
    subLayout->setContentsMargins(12, 5, 0, 0);
    subLayout->setSpacing(15);
    layout->addLayout(subLayout);

    // Button group to manage exclusivity and retrieval
    QButtonGroup buttonGroup(&dialog);

    // Create radio buttons
    for (int i = 0; i < paths.size(); ++i) {
        QRadioButton *rb = new QRadioButton(paths[i], &dialog);
        subLayout->addWidget(rb);
        buttonGroup.addButton(rb, i);
        if (i == 0) {
            rb->setChecked(true);
        }
    }

    layout->addSpacing(5);

    // OK/Cancel buttons
    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttonBox->setCenterButtons(true);
    layout->addWidget(buttonBox);

    layout->addSpacing(5);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // Execute the dialog
    if (dialog.exec() == QDialog::Accepted) {
        QAbstractButton *selected = buttonGroup.checkedButton();
        if (selected)
            return selected->text();
    }
    return "";
}

void NewCaseWizard::accept() {

    CaseCreationType caseCreationType =
        static_cast<CaseCreationType>(field("caseCreationType").toInt());
    QString casePath = field("casePath").toString();
    m_geometryFile = field("geometryFile").toString();

    // Check if case folder already exists
    QString newCasePath;
    QString result = mainWin->targetSystems[m_targetId]->checkPath(newCasePath);
    if (result != "0") {
        QMessageBox::StandardButton reply;
        QString msg =
            tr("The folder '%1' already exists in the selected directory.\nCreate '%2' instead?").arg(m_caseName, m_caseName + "_" + result);

        reply = QMessageBox::question(this, tr("Existing Case Detected"), msg,
            QMessageBox::Yes | QMessageBox::No);

        // End wizard if the response is no
        if (reply == QMessageBox::No) {
            QWizard::accept();
        } else {
            m_caseName = m_caseName + "_" + result;
        }
    }

    // Determine the path of the new case
    newCasePath = casePath + "/" + m_caseName;

    QStringList files;
    if (caseCreationType == CaseCreationType::TUTORIAL) {

        // Copy files from tutorial to new case folder
        QString tutorialPath = field("tutorialPath").toString();
        files = mainWin->targetSystems[m_targetId]->copyTutorialFolders(tutorialPath, newCasePath);
    }
    else if (caseCreationType == CaseCreationType::INTERACTIVE) {
        if (createCase(newCasePath)) {
            files = { "0.orig", "constant", "system", "Allclean|", "Allrun|" };
        } else {
            QMessageBox::critical(this, tr("Case Creation Issue"), tr("Failed to create case folder"));
            return;
        }
    }

    // Copy geometry file if given
    if (!m_geometryFile.isEmpty()) {

        QFileInfo info(m_geometryFile);
        if (info.exists() && info.isFile()) {
            QString remotePath = newCasePath + "/constant/triSurface/" + info.fileName();
            bool success = mainWin->targetSystems[m_targetId]->writeData(m_geometryFile, remotePath);
            if (!success) {
                qWarning() << "Failed to transfer geometry file:" << info.fileName();
            }
        } else {
            qWarning() << "Geometry file does not exist or is invalid:" << m_geometryFile;
        }
    }

    // Create case
    mainWin->createCase(m_caseName, casePath, files, m_targetId, m_openFoamPath);

    QWizard::accept();
}

bool NewCaseWizard::createCase(QString newCasePath) {

    // Check OpenFOAM version
    bool isESI = false;
    QString versionText = "unknown";
    QRegularExpression re("openfoam-?v?(\\d+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(m_openFoamPath.split("/").last());

    if (match.hasMatch()) {
        QString digits = match.captured(1);
        int versionNumber = digits.toInt();

        // ESI/Keysight releases use YYMM, so the number is always > 100
        if (versionNumber > 100) {
            isESI = true;
            versionText = "v" + digits;
        } else {
            // Foundation releases use sequential major versions (e.g., 11, 12, 13)
            isESI = false;
            versionText = digits;
        }
    } else {
        qWarning() << "Warning: Could not parse OpenFOAM version from path:" << m_openFoamPath;
    }

    // Determine the appropriate website based on the fork
    QString websiteText = isESI ? "www.openfoam.com" : "www.openfoam.org";

    // Create directories
    QStringList newDirs = { newCasePath, newCasePath + "/constant", newCasePath + "/system", newCasePath + "/0.orig" };
    if (mainWin->targetSystems[m_targetId]->createDirectories(newDirs)) {

        createCaseFiles(newCasePath, versionText, websiteText);

        return true;
    } else {
        QMessageBox::critical(this, tr("Case Creation Issue"), tr("Failed to create case folder"));
        return false;
    }
}

void NewCaseWizard::createCaseFiles(const QString& newCasePath, const QString& versionText,
                                    const QString& websiteText) {

    // Configuration strings
    QString versionPadded = QString("%1").arg(versionText, -38);
    QString websitePadded = QString("%1").arg(websiteText, -38);
    QString turbName, turbDict, cleanCommand, meshText, solverText, fieldsText;
    QString endTimeText, deltaText, writeControlText, writeIntervalText;
    QString ddtScheme = "Euler", divUScheme = "bounded Gauss upwind", schemeText;
    QString orthoScheme, pText, alphaText, algoText, rhoText, eqText, dimText, fileText, filePath;

    // Flow type check
    switch(m_caseConfig.flowConfig) {
    case FlowConfig::Incompressible:
        pText = pTextIncompressible;
        dimText = "[0 2 -2 0 0 0 0]";
        switch(m_caseConfig.timeConfig) {
        case TimeConfig::SteadyState:
            solverText = "simpleFoam";
            break;
        case TimeConfig::Transient:
            solverText = "pimpleFoam";
            break;
        }
        break;
    case FlowConfig::Compressible:
        pText = pTextCompressible;
        dimText = "[1 -1 -2 0 0 0 0]";
        rhoText = "        rho             0.1;";
        eqText = "        \"(e|h)\"         0.7;";
        switch(m_caseConfig.timeConfig) {
        case TimeConfig::SteadyState:
            solverText = "rhoSimpleFoam";
            break;
        case TimeConfig::Transient:
            solverText = "rhoPimpleFoam";
            break;
        }
        break;
    case FlowConfig::Multiphase:
        pText = pTextMultiphase;
        dimText = "[1 -1 -2 0 0 0 0]";
        fieldsText = fieldsTextMultiphase;
        solverText = "interFoam";
        schemeText = schemeTextMultiphase;
        alphaText = alphaTextMultiphase;
        break;
    }

    // Turbulence check
    switch(m_caseConfig.turbulenceConfig) {
    case TurbulenceConfig::Laminar:
        turbName = "laminar";
        break;
    case TurbulenceConfig::RAS:
        turbName = "RAS";
        turbDict = turbDictRAS;
        break;
    case TurbulenceConfig::LES:
        ddtScheme = "backward";
        divUScheme = "Gauss LUST grad(U)";
        turbName = "LES";
        turbDict = turbDictLES;
        break;
    }

    // Time check
    switch(m_caseConfig.timeConfig) {
    case TimeConfig::SteadyState:
        ddtScheme = "steadyState";
        endTimeText = "500";
        deltaText = "1";
        writeControlText = "timeStep";
        writeIntervalText = "50";
        algoText = algoTextSimple;
        break;
    case TimeConfig::Transient:
        endTimeText = "10.0";
        deltaText = "0.001";
        writeControlText = "adjustableRunTime";
        writeIntervalText = "0.1";
        algoText = algoTextPimple;
        break;
    }

    // Geometry check
    cleanCommand = cleanCommandGeneral;
    if (!m_geometryFile.isEmpty()) {
        orthoScheme = "limited 0.33";
        meshText = meshTextGeometry;
    } else {
        orthoScheme = "orthogonal";
        if ((m_caseConfig.flowConfig == FlowConfig::Compressible) ||
            (m_caseConfig.flowConfig == FlowConfig::Incompressible)) {
            cleanCommand = "cleanCase0";
        }
    }

    // Initialize map of files
    QMap<QString, QStringList> configMap;
    configMap["transportProperties"] = { "/constant/", versionPadded, websitePadded };
    configMap["turbulenceProperties"] = { "/constant/", versionPadded, websitePadded, turbName, turbDict };
    configMap["Allclean"] = { "/", cleanCommand };
    configMap["Allrun"] = { "/", meshText, fieldsText, solverText };
    configMap["controlDict"] = { "/system/", versionPadded, websitePadded, solverText, endTimeText,
                                deltaText, writeControlText, writeIntervalText };
    configMap["fvSchemes"] = { "/system/", versionPadded, websitePadded, ddtScheme, divUScheme,
                              schemeText, orthoScheme };
    configMap["fvSolution"] = { "/system/", versionPadded, websitePadded, pText, alphaText,
                               algoText, rhoText, eqText };
    configMap["blockMeshDict"] = { "/system/", versionPadded, websitePadded };
    configMap["p"] = { "/0.orig/", versionPadded, websitePadded, dimText };
    configMap["U"] = { "/0.orig/", versionPadded, websitePadded };
    if (m_caseConfig.flowConfig == FlowConfig::Multiphase) {
        configMap["p_rgh"] = { "/0.orig/", versionPadded, websitePadded, dimText };
    }

    // Update template files
    QFile templateFile;
    for( auto const& fileName: configMap.keys() ) {

        // Read template file to string
        templateFile.setFileName(":/template/" + fileName);
        if (!templateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::critical(this, tr("Resource Error"), tr("Failed to load %1 file.").arg(fileName));
            continue;
        }
        fileText = QString::fromUtf8(templateFile.readAll());
        templateFile.close();

        // Update string
        QStringList cfgList = configMap[fileName];
        for (int i = 1; i < cfgList.size(); ++i) {
            fileText = fileText.arg(cfgList[i]);
        }

        // Write string to file on server
        filePath = newCasePath + cfgList[0] + fileName;
        if ((fileName == "Allclean") || (fileName == "Allrun")) {
            filePath += "|";
        }
        if (!mainWin->targetSystems[m_targetId]->writeData(fileText.toUtf8(), filePath)) {
            QMessageBox::critical(this, tr("File Write Error"), tr("Failed to write %1 to target.").arg(fileName));
            return;
        }
    }
}