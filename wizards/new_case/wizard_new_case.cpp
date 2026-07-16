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

#include "template_strings.h"

NewCaseWizard::NewCaseWizard(bool isWindows, bool isWslAvailable,
    SystemManager& systemMgr, QWidget *parent):
    m_systemMgr(systemMgr), QWizard(parent) {
    // Configure wizard appearance
    setWizardStyle(QWizard::ClassicStyle);
    setWindowTitle("New Case Wizard");

    // Add pages
    setPage(static_cast<int>(WizardPage::Page_Intro),
            new IntroPage(isWindows, isWslAvailable, this));
    setPage(static_cast<int>(WizardPage::Page_Tutorial),
            new TutorialPage(this));
    setPage(static_cast<int>(WizardPage::Page_Interactive),
            new InteractivePage(this));
    setPage(static_cast<int>(WizardPage::Page_Project),
            new ProjectPage(this));
    setOption(QWizard::NoBackButtonOnStartPage);
}

QStringList NewCaseWizard::processPaths(QString path) {
    return m_system->processPaths(path, PathOperationType::LIST);
}

QStringList NewCaseWizard::getTutorials() {
    QStringList results = m_system->getTutorials(m_openFoamPath);
    return results;
}

bool NewCaseWizard::validateCurrentPage() {

    // Validate first page
    if (currentId() == static_cast<int>(WizardPage::Page_Intro)) {

        // Read registered fields
        m_caseName = field("caseName").toString();
        m_targetId = static_cast<TargetType>(field("targetSystemId").toInt());
        m_system = m_systemMgr.getSystem(m_targetId);

        // Check if case name is in the case map
        int count = 0;
        QString newName;
        if (m_systemMgr.contains(m_caseName)) {
            count = 1;
            while (true) {
                newName = m_caseName + "_" + QString::number(count++);
                if (!m_systemMgr.contains(newName)) {
                    break;
                }
            }
        }

        // If name is in case map, ask user
        if (count > 0) {
            QMessageBox::StandardButton reply;
            QString msg = tr("There is already a case named '%1'.\n"
                             "Create '%2' instead?").arg(m_caseName, newName);
            reply = QMessageBox::question(this, tr("Existing Case Detected"),
                msg, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

            // Fail validation if the response is no
            if (reply == QMessageBox::No) {
                return false;
            } else {
                m_caseName = newName;
            }
        }

        // Determine OpenFOAM installation
        QStringList ofList = m_system->findOpenFoam();
        if(ofList.empty()) {
            QMessageBox::critical(this, tr("Missing OpenFOAM"),
                tr("No OpenFOAM installations detected..."));
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
    layout->addWidget(new QLabel(tr("Select one of the following "
                                    "OpenFOAM installations:"), &dialog));

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
        new QDialogButtonBox(QDialogButtonBox::Ok |
            QDialogButtonBox::Cancel, &dialog);
    buttonBox->setCenterButtons(true);
    layout->addWidget(buttonBox);

    layout->addSpacing(5);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog,
        &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog,
        &QDialog::reject);

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
    QString checkPath = casePath + "/" + m_caseName;
    QStringList results = m_system->processPaths(checkPath,
        PathOperationType::CHECK);
    QString result = results[0];

    // Handle existing case
    if (result == "0") {

        // Search for unique case name
        bool uniqueCase = false;
        int count = 1;
        while (!uniqueCase) {
            QStringList testList = {
                checkPath + "_" + QString::number(count),
                checkPath + "_" + QString::number(count + 1),
                checkPath + "_" + QString::number(count + 2),
                checkPath + "_" + QString::number(count + 3),
                checkPath + "_" + QString::number(count + 4)};
            QString testCase = testList.join('\n');

            // Check if any paths already exist
            results = m_system->processPaths(testCase,
                PathOperationType::CHECK);
            for (int i=0; i<5; i++) {
                if (results[i] == "-1") {
                    result = QString::number(count + i);
                    uniqueCase = true;
                    break;
                }
            }
            count += 5;
        }

        QMessageBox::StandardButton reply;
        QString msg =
            tr("The folder '%1' already exists in the selected directory.\n"
               "Create '%2' instead?").arg(m_caseName,
                    m_caseName + "_" + result);

        reply = QMessageBox::question(this, tr("Existing Case Detected"), msg,
            QMessageBox::Yes | QMessageBox::No);

        // End wizard if the response is no
        if (reply == QMessageBox::No) {
            return;
        } else {
            m_caseName = m_caseName + "_" + result;
        }
    }

    // Determine the path of the new case
    checkPath = casePath + "/" + m_caseName;

    QStringList files;
    if (caseCreationType == CaseCreationType::TUTORIAL) {

        // Copy files from tutorial to new case folder
        QString tutorialPath = field("tutorialPath").toString();
        files = m_system->copyTutorialFolders(tutorialPath, checkPath);
    }
    else if (caseCreationType == CaseCreationType::INTERACTIVE) {
        if (createCase(checkPath)) {
            files = { "0.orig", "constant", "system", "Allclean|", "Allrun|" };
        } else {
            QMessageBox::critical(this, tr("Case Creation Issue"),
                                  tr("Failed to create case folder"));
            return;
        }
    }

    // Copy geometry file if given
    if (!m_geometryFile.isEmpty()) {

        // Determine which OpenFOAM release is used
        QString dirName = QDir(m_openFoamPath).dirName();
        const QRegularExpression foundationRegex("^openfoam\\d{2}$",
            QRegularExpression::CaseInsensitiveOption);
        bool isFoundation = foundationRegex.match(dirName).hasMatch();

        // Write geometry file to new case
        QFileInfo info(m_geometryFile);
        if (info.exists() && info.isFile()) {

            QString subDir = (isFoundation) ? "/constant/geometry/" :
                                 "/constant/triSurface/";
            QString remotePath = checkPath + subDir + info.fileName();
            bool success = m_system->writeData(m_geometryFile, remotePath);
            if (!success) {
                qWarning() << "Failed to transfer geometry file:"
                           << info.fileName();
            }
        } else {
            qWarning() << "Geometry file does not exist or is invalid:"
                       << m_geometryFile;
        }
    }

    // Request case creation
    emit requestCaseCreation(m_caseName, casePath, files, m_targetId,
                             m_openFoamPath);

    QWizard::accept();
}

bool NewCaseWizard::createCase(QString newCasePath) {

    // Check OpenFOAM version
    bool isESI = false;
    QString versionText = "unknown";
    QRegularExpression re("openfoam-?v?(\\d+)",
                          QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(m_openFoamPath.split("/").last());

    if (match.hasMatch()) {
        QString digits = match.captured(1);
        int versionNumber = digits.toInt();

        // ESI/Keysight uses YYMM, so the number is always > 100
        if (versionNumber > 100) {
            isESI = true;
            versionText = "v" + digits;
        } else {
            // Foundation uses sequential major versions (e.g., 11, 12, 13)
            isESI = false;
            versionText = digits;
        }
    } else {
        qWarning() << "Warning: Could not parse OpenFOAM version from path: "
                   << m_openFoamPath;
    }

    // Determine the appropriate website based on the fork
    QString websiteText = isESI ? "www.openfoam.com" : "www.openfoam.org";

    // Create directories
    QStringList newDirs = { newCasePath, newCasePath + "/constant",
                           newCasePath + "/system", newCasePath + "/0.orig" };
    QString newDirStr = newDirs.join("\n");
    QStringList results = m_system->processPaths(newDirStr,
        PathOperationType::CREATE);

    if (!results.contains("-1")) {
        createCaseFiles(newCasePath, versionText, websiteText);
        return true;
    } else {
        QMessageBox::critical(this, tr("Case Creation Issue"),
                              tr("Failed to create case folder"));
        return false;
    }
}

void NewCaseWizard::createCaseFiles(const QString& newCasePath,
        const QString& versionText, const QString& websiteText) {

    // Configuration strings
    QString versionPadded = QString("%1").arg(versionText, -38);
    QString websitePadded = QString("%1").arg(websiteText, -38);
    QString turbName, turbDict, cleanCommand, meshText, solverText, fieldsText;
    QString endTimeText, deltaText, writeControlText, writeIntervalText;
    QString ddtScheme = "Euler",
        divUScheme = "bounded Gauss upwind", schemeText;
    QString orthoScheme, pText, alphaText, algoText, rhoText, eqText,
        fileText, filePath;

    // Flow type check
    switch(m_caseConfig.flowConfig) {
    case FlowConfig::Incompressible:
        pText = pTextIncompressible;
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
    configMap["transportProperties"] = { "/constant/", versionPadded,
                                        websitePadded };
    configMap["turbulenceProperties"] = { "/constant/", versionPadded,
                                         websitePadded, turbName, turbDict };
    configMap["Allclean"] = { "/", cleanCommand };
    configMap["Allrun"] = { "/", meshText, fieldsText, solverText };
    configMap["controlDict"] = { "/system/", versionPadded, websitePadded,
                                solverText, endTimeText, deltaText,
                                writeControlText, writeIntervalText };
    configMap["fvSchemes"] = { "/system/", versionPadded, websitePadded,
                              ddtScheme, divUScheme, schemeText, orthoScheme };
    configMap["fvSolution"] = { "/system/", versionPadded, websitePadded,
                               pText, alphaText,algoText, rhoText, eqText };
    configMap["blockMeshDict"] = { "/system/", versionPadded, websitePadded };

    // Update template files
    QFile templateFile;
    for( auto const& fileName: configMap.keys() ) {

        // Read template file to string
        templateFile.setFileName(":/template/" + fileName);
        if (!templateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::critical(this, tr("Resource Error"),
                tr("Failed to load %1 file.").arg(fileName));
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
        if (!m_system->writeData(fileText.toUtf8(), filePath)) {
            QMessageBox::critical(this, tr("File Write Error"),
                tr("Failed to write %1 to target.").arg(fileName));
            return;
        }
    }
}