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

#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStandardPaths>

#include "page_10_intro.h"
#include "page_15_remote.h"
#include "page_20_tutorial.h"
#include "page_30_interactive.h"
#include "page_40_project.h"

#include "dialogs/selection/selection_dialog.h"
#include "template_strings.h"

NewCaseWizard::NewCaseWizard(SystemManager& systemMgr, QWidget *parent):
    m_systemMgr(systemMgr), QWizard(parent) {
    // Configure wizard appearance
    setWizardStyle(QWizard::ClassicStyle);
    setWindowTitle("New Case Wizard");

    // Check if WSL is available
    bool isWslAvailable = !QStandardPaths::findExecutable("wsl.exe").isEmpty();

    // Add pages
    setPage(static_cast<int>(NewCasePage::Page_Intro),
            new IntroPage(systemMgr, isWslAvailable, this));
    setPage(static_cast<int>(NewCasePage::Page_Remote),
            new RemotePage(systemMgr, this));
    setPage(static_cast<int>(NewCasePage::Page_Tutorial),
            new TutorialPage(this));
    setPage(static_cast<int>(NewCasePage::Page_Interactive),
            new InteractivePage(this));
    setPage(static_cast<int>(NewCasePage::Page_Project),
            new ProjectPage(this));
    setOption(QWizard::NoBackButtonOnStartPage);

    // Adjust size for each page
    connect(this, &QWizard::currentIdChanged, this, [this](int) {
        adjustSize();
    });
}

QStringList NewCaseWizard::processPaths(const QString& path) {
    return m_system->processPaths(path, PathOperationType::LIST);
}

QStringList NewCaseWizard::getTutorials() {
    QStringList results = m_system->getTutorials(m_openFoamPath);
    return results;
}

bool NewCaseWizard::validateCurrentPage() {
    // Validate first page
    if (currentId() == static_cast<int>(NewCasePage::Page_Intro)) {
        // Read registered fields
        m_caseName = field("caseName").toString();
        m_targetId = static_cast<TargetType>(field("targetSystemId").toInt());
        m_system = m_systemMgr.getSystem(m_targetId);

        // For WSL access, check if server is installed
        if (m_targetId == static_cast<int>(TargetType::LOCAL_WINDOWS)) {
            if(!m_systemMgr.checkWslServer()) {
                QString title = tr("Server Installation Failure");
                QString msg = tr("Failed to install the WSL server in "
                     "~/config/flowcompute.\n"
                     "Please make sure this directory is accessible.");
                QMessageBox::critical(nullptr, title, msg);
                return false;
            }
        }

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

        // For remote access, move to the next page
        if (m_targetId == static_cast<int>(TargetType::REMOTE_LINUX)) {
            return true;
        }
        return checkOpenFoam();
    } else if (currentId() == static_cast<int>(NewCasePage::Page_Remote)) {
        return checkOpenFoam();
    }

    return QWizard::validateCurrentPage();
}

bool NewCaseWizard::checkOpenFoam() {
    // Determine OpenFOAM installation
    QStringList ofList = m_systemMgr.getSystem(m_targetId)->findOpenFoam();
    if(ofList.empty()) {
        QMessageBox::critical(this, tr("Missing OpenFOAM"),
            tr("No OpenFOAM installations detected..."));
        m_openFoamPath = "";
        return false;
    } else if (ofList.size() > 1) {
        // Create selection dialog
        SelectionDialog selectionDialog(
            tr("Multiple OpenFOAM Installations Detected"),
            tr("Select one of the following:"), ofList, this);
        if (selectionDialog.exec() != QDialog::Accepted) {
            return false;
        }
        m_openFoamPath = selectionDialog.getSelectedItem();
    } else {
        m_openFoamPath = ofList[0];
    }

    // Check OpenFOAM release and version
    QRegularExpression re("openfoam-?v?(\\d+)",
                          QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(m_openFoamPath.split("/").last());

    if (match.hasMatch()) {
        QString digits = match.captured(1);
        int versionNumber = digits.toInt();

        // OpenCFD uses YYMM, Foundation uses NN
        if (versionNumber > 100) {
            m_isOpenCFD = true;
            m_openFoamVersion = "v" + digits;
        } else {
            m_isOpenCFD = false;
            m_openFoamVersion = digits;
        }
        m_cfg.isOpenCFD = m_isOpenCFD;
    } else {
        qWarning() << "Warning: Could not parse OpenFOAM version from path: "
                   << m_openFoamPath;
    }
    return true;
}

void NewCaseWizard::accept() {
    // Create the new case
    CaseCreationType caseCreationType =
        static_cast<CaseCreationType>(field("caseCreationType").toInt());
    QString casePath = field("casePath").toString();
    m_geometryFile = field("geometryFile").toString();

    // Check if case folder already exists
    QString checkPath = casePath + "/" + m_caseName;
    QStringList results =
        m_system->processPaths(checkPath, PathOperationType::CHECK);
    QString result = results[0];

    // Handle existing case
    if (result == "0") {
        bool uniqueCase = false;
        int count = 1;
        while (!uniqueCase) {
            QStringList testList;
            for (int i = 0; i < 5; ++i) {
                testList << checkPath + "_" + QString::number(count + i);
            }

            QString testCase = testList.join('\n');
            results =
                m_system->processPaths(testCase, PathOperationType::CHECK);
            for (int i = 0; i < results.size(); ++i) {
                if (results[i] == "-1") {
                    result = QString::number(count + i);
                    uniqueCase = true;
                    break;
                }
            }
            if (!uniqueCase)
                count += 5;
        }

        // Create message box
        QString msg =
            tr("The folder '%1' already exists in the selected location.\n"
            "Create '%2' instead?").arg(m_caseName, m_caseName + "_" + result);
        if (QMessageBox::question(this, tr("Existing Case Detected"), msg,
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
            return;
        }

        m_caseName = m_caseName + "_" + result;
        checkPath = QDir(casePath).filePath(m_caseName);
    }

    // Determine the path of the new case
    QStringList openFolders;
    CaseFlags flag = CaseFlag::Initial;
    CaseType type;
    if (caseCreationType == CaseCreationType::TUTORIAL) {
        // Copy files from tutorial to new case folder
        QString tutorialPath = field("tutorialPath").toString();
        m_system->copyTutorialFolders(tutorialPath, checkPath);
        flag = CaseFlag::NotChecked;
        type = m_systemMgr.updateType(m_targetId, checkPath, m_isOpenCFD);
    }
    else if (caseCreationType == CaseCreationType::INTERACTIVE) {
        if (createCase(checkPath)) {
            // Set case type
            type.setFlag(Transient,
                         m_cfg.timeConfig == TimeConfig::Transient);
            type.setFlag(Compressible,
                         m_cfg.flowConfig == FlowConfig::Compressible);
            type.setFlag(Multiphase,
                         m_cfg.phaseConfig == PhaseConfig::MultiPhase);
            type.setFlag(TurbulenceRAS,
                         m_cfg.turbulenceConfig == TurbulenceConfig::RAS);
            type.setFlag(TurbulenceLES,
                         m_cfg.turbulenceConfig == TurbulenceConfig::LES);
            type.setFlag(FluidHeat,
                         m_cfg.heatConfig == HeatConfig::FluidHeat);
            type.setFlag(ConjugateHeat,
                         m_cfg.heatConfig == HeatConfig::ConjugateHeat);
            type.setFlag(MeshMRF,
                         m_cfg.meshConfig == MeshConfig::DynamicMRF);
            type.setFlag(MeshAMI,
                         m_cfg.meshConfig == MeshConfig::DynamicAMI);
            type.setFlag(MeshOverset,
                         m_cfg.meshConfig == MeshConfig::DynamicOverset);
            type.setFlag(MeshDeforming,
                         m_cfg.meshConfig == MeshConfig::Deformable);
            type.setFlag(Radiation, m_cfg.radiationConfig);
            type.setFlag(Combustion, m_cfg.combustionConfig);
            type.setFlag(Buoyancy, m_cfg.buoyancyConfig);
            type.setFlag(Lagrangian, m_cfg.particlesConfig);
            type.setFlag(IsOpenCFD, m_cfg.isOpenCFD);
            openFolders = { m_caseName };
        } else {
            QMessageBox::critical(this, tr("Case Creation Issue"),
                                  tr("Failed to create case folder"));
            return;
        }
    }

    // Copy geometry file if given
    if (!m_geometryFile.isEmpty()) {
        // Write geometry file to new case
        QFileInfo info(m_geometryFile);
        if (info.exists() && info.isFile()) {
            QString subDir = (m_isOpenCFD) ? "/constant/triSurface/" :
                                 "/constant/geometry/";
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

    // Get credentials for remote cases
    int port = 0;
    QString userName, hostName;
    if (m_targetId == static_cast<int>(TargetType::REMOTE_LINUX)) {
        userName = field("userName").toString();
        hostName = field("hostName").toString();
        port = field("port").toInt();
    }

    // Request case creation
    emit requestCaseCreation(m_caseName, casePath, openFolders, m_targetId,
        m_openFoamPath, flag, type, userName, hostName, port);
    QWizard::accept();
}

bool NewCaseWizard::createCase(const QString& newCasePath) {
    // Set the website based on the distribution
    QString websiteText = m_isOpenCFD ? "www.openfoam.com" : "www.openfoam.org";

    // Create directories
    QStringList newDirs = { newCasePath, newCasePath + "/constant",
                           newCasePath + "/system", newCasePath + "/0.orig" };
    QString newDirStr = newDirs.join("\n");
    QStringList results = m_system->processPaths(newDirStr,
                                                 PathOperationType::CREATE);

    if (!results.contains("-1")) {
        createCaseFiles(newCasePath, m_openFoamVersion, websiteText);
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
    QString turbName, turbDict, cleanCommand, meshText, fieldsText;
    QString endTimeText, deltaText, writeControlText, writeIntervalText;
    QString ddtScheme = "Euler", divPhiU, divPhik, schemeText, orthoScheme;
    QString pText, alphaText, algoText, relaxText, fluxField = "p;";
    QString executableName, controlDictSolverBlock;

    // Solver selection
    if (m_cfg.heatConfig == HeatConfig::ConjugateHeat) {
        // Multi-region conjugate heat transfer
        pText = pTextCompressible;
        schemeText = schemeTextCompressible;
        fluxField = "p;\n    rho;";
        if (m_cfg.timeConfig == TimeConfig::SteadyState) {
            executableName = m_cfg.isOpenCFD
                ? "chtMultiRegionSimpleFoam" : "foamRun";
            controlDictSolverBlock = m_cfg.isOpenCFD
                ? "chtMultiRegionSimpleFoam;"
                : "foamRun;\nsolver          fluidSolid;";
        } else {
            executableName = m_cfg.isOpenCFD ? "chtMultiRegionFoam" : "foamRun";
            controlDictSolverBlock = m_cfg.isOpenCFD ? "chtMultiRegionFoam;"
                : "foamRun;\nsolver          fluidSolid;";
        }
    }
    else if (m_cfg.phaseConfig == PhaseConfig::MultiPhase) {
        // Multiphase logic
        pText = pTextMultiphase;
        fieldsText = fieldsTextMultiphase;
        schemeText = schemeTextMultiphase;
        alphaText = alphaTextMultiphase;
        fluxField = "p_rgh;";

        if (m_cfg.flowConfig == FlowConfig::Incompressible) {
            executableName = m_cfg.isOpenCFD ? "interFoam" : "foamRun";
            controlDictSolverBlock = m_cfg.isOpenCFD ? "interFoam;"
                : "foamRun;\nsolver          incompressibleVoF;";
        } else {
            executableName = m_cfg.isOpenCFD ?
                "compressibleInterFoam" : "foamRun";
            controlDictSolverBlock = m_cfg.isOpenCFD ? "compressibleInterFoam;"
                : "foamRun;\nsolver          compressibleVoF;";
        }
    }
    else if (m_cfg.heatConfig == HeatConfig::FluidHeat) {
        // Single-region fluid heat transfer
        pText = pTextCompressible;
        if (m_cfg.flowConfig == FlowConfig::Incompressible) {
            // Uses Boussinesq approximation for incompressible thermal flows
            alphaText = alphaTextBoussinesq;
            executableName = (m_cfg.timeConfig == TimeConfig::SteadyState)
             ? (m_cfg.isOpenCFD ? "buoyantBoussinesqSimpleFoam" : "foamRun")
             : (m_cfg.isOpenCFD ? "buoyantBoussinesqPimpleFoam" : "foamRun");
            controlDictSolverBlock = m_cfg.isOpenCFD ? executableName + ";"
                : "foamRun;\nsolver          fluid;";
        } else {
            fluxField = "p;\n    rho;";
            alphaText = alphaTextCompressible;
            schemeText = schemeTextCompressible;
            pText = pTextCompressible;
            executableName = (m_cfg.timeConfig == TimeConfig::SteadyState)
            ? (m_cfg.isOpenCFD ? "rhoSimpleFoam" : "foamRun")
            : (m_cfg.isOpenCFD ? "rhoPimpleFoam" : "foamRun");
            controlDictSolverBlock = m_cfg.isOpenCFD ? executableName + ";"
                : "foamRun;\nsolver          fluid;";
        }
    }
    else {
        // Isothermal, single-phase logic (NoHeat)
        if (m_cfg.flowConfig == FlowConfig::Compressible) {
            pText = pTextCompressible;
            alphaText = alphaTextCompressible;
            schemeText = schemeTextCompressible;
            fluxField = "p;\n    rho;";
            executableName = (m_cfg.timeConfig == TimeConfig::SteadyState)
             ? (m_cfg.isOpenCFD ? "rhoSimpleFoam" : "foamRun")
             : (m_cfg.isOpenCFD ? "rhoPimpleFoam" : "foamRun");
            controlDictSolverBlock = m_cfg.isOpenCFD ? executableName + ";"
                : "foamRun;\nsolver          fluid;";
        } else {
            pText = pTextIncompressible;
            executableName = (m_cfg.timeConfig == TimeConfig::SteadyState)
             ? (m_cfg.isOpenCFD ? "simpleFoam" : "foamRun")
             : (m_cfg.isOpenCFD ? "pimpleFoam" : "foamRun");
            controlDictSolverBlock = m_cfg.isOpenCFD ? executableName + ";"
                : "foamRun;\nsolver          incompressibleFluid;";
        }
    }
    if ((m_cfg.isOpenCFD) && (m_cfg.flowConfig == FlowConfig::Compressible)) {
        schemeText += "\n    div(((rho*nuEff)*dev2(T(grad(U))))) Gauss linear;";
    }

    // Turbulence check
    bool isSteady = (m_cfg.timeConfig == TimeConfig::SteadyState);
    switch(m_cfg.turbulenceConfig) {
    case TurbulenceConfig::Laminar:
        turbName = "laminar";
        divPhiU = isSteady ? "bounded Gauss limitedLinearV 1" :
                      "Gauss limitedLinearV 1";
        break;
    case TurbulenceConfig::RAS:
        turbName = "RAS";
        divPhiU = isSteady ? "bounded Gauss upwind" : "Gauss upwind";
        divPhik = isSteady ? "bounded Gauss upwind" : "Gauss upwind";
        turbDict = turbDictRAS;
        break;
    case TurbulenceConfig::LES:
        turbName = "LES";
        ddtScheme = "backward";
        divPhiU = "Gauss LUST grad(U)";
        divPhik = "Gauss upwind";
        turbDict = turbDictLES;
        break;
    }

    // Time check
    if (isSteady) {
        ddtScheme = "steadyState";
        endTimeText = "500";
        deltaText = "1";
        writeControlText = "timeStep";
        writeIntervalText = "50";
        algoText = algoTextSimple;
        relaxText = relaxTextSimple;
    } else {
        endTimeText = "10.0";
        deltaText = "0.001";
        writeControlText = "adjustableRunTime";
        writeIntervalText = "0.1";
        algoText = algoTextPimple;
    }

    // Geometry check
    cleanCommand = cleanCommandGeneral;
    if (!m_geometryFile.isEmpty()) {
        orthoScheme = "limited 0.33";
        meshText = meshTextGeometry;
    } else {
        orthoScheme = "orthogonal";
        if ((m_cfg.flowConfig == FlowConfig::Compressible) ||
            (m_cfg.flowConfig == FlowConfig::Incompressible)) {
            cleanCommand = "cleanCase0";
        }
    }

    // Check if thermophysicalProperties file is necessary
    bool thermoFile = (m_cfg.flowConfig == FlowConfig::Compressible) ||
                      (m_cfg.heatConfig == HeatConfig::FluidHeat) ||
                      (m_cfg.heatConfig == HeatConfig::ConjugateHeat);

    // Initialize map of files
    QMap<QString, QStringList> configMap;    
    if (!thermoFile) {
        QString transportName = (m_cfg.isOpenCFD) ? "transportProperties" :
                                    "physicalProperties";
        configMap[transportName] = { "/constant/", versionPadded };
    }
    configMap["turbulenceProperties"] = { "/constant/", versionPadded,
                                         websitePadded, turbName, turbDict };
    configMap["Allclean"] = { "/", cleanCommand };
    configMap["Allrun"] = { "/", meshText, fieldsText, executableName };
    configMap["controlDict"] = { "/system/", versionPadded, websitePadded,
                                controlDictSolverBlock, endTimeText, deltaText,
                                writeControlText, writeIntervalText };
    configMap["fvSchemes"] = { "/system/", versionPadded, websitePadded,
            ddtScheme, fluxField, divPhiU, divPhik, schemeText, orthoScheme };
    configMap["fvSolution"] = { "/system/", versionPadded, websitePadded,
                               pText, alphaText, algoText, relaxText };

    // Update template files
    QFile templateFile;
    QString fileText, filePath;
    for( auto& fileName: configMap.keys() ) {
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
            QString placeholder = QString("%%1").arg(i);
            fileText.replace(placeholder, cfgList[i]);
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