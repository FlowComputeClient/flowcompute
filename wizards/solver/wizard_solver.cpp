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

#include "wizards/solver/wizard_solver.h"

#include <QFile>
#include <QList>
#include <QMessageBox>
#include <QRegularExpression>

#include "parser/boundary.h"
#include "parser/transport_properties.h"
#include "parser/turbulence_properties.h"

#include "wizards/solver/page_10_control.h"
#include "wizards/solver/page_20_physics.h"
#include "wizards/solver/page_30_thermo.h"
#include "wizards/solver/page_50_algorithm.h"
#include "wizards/solver/page_40_boundary.h"
#include "wizards/solver/page_60_simple.h"
#include "wizards/solver/page_70_pimple.h"
#include "wizards/solver/page_80_piso.h"
#include "wizards/solver/page_90_parallel.h"
#include "wizards/post_processing/page_10_tasks.h"

SolverWizard::SolverWizard(const QString& caseName,
    SystemManager& systemMgr,
    const std::vector<FlowCompute::SolverFamily>& families,
    const FlowCompute::TurbulenceDatabase& turbModels,
    const std::map<QString, FlowCompute::TransportPropertyDef>&
        transportProperties,
    const QHash<QString, FlowCompute::FieldDef>& fieldData,
    const std::vector<FlowCompute::BoundaryConditionDef>& boundaryConditions,
    const QStringList& patchNames, QWidget *parent): QWizard(parent),
        m_caseName(caseName), m_systemMgr(systemMgr), m_families(families),
        m_turbModels(turbModels), m_fieldData(fieldData),
        m_boundaryConditions(boundaryConditions), m_patchNames(patchNames) {
    // Configure the wizard's appearance
    setWizardStyle(QWizard::ClassicStyle);
    setWindowTitle("Solver Configuration Wizard");

    // Create map to look up solver algorithms
    for (const auto& family : std::as_const(m_families)) {
        for (const auto& solver : std::as_const(family.solvers)) {
            m_solverAlgorithmMap.insert(solver.name, solver.algorithm);
        }
    }

    // Add pages
    QStringList cases = m_systemMgr.getCases();
    setPage(Page_Control, new ControlPage(caseName, cases, families, this));
    setPage(Page_Physics,
        new PhysicsPage(families, turbModels, transportProperties, this));
    setPage(Page_Thermo, new ThermoPage(this));
    setPage(Page_Boundary,
        new BoundaryPage(fieldData, boundaryConditions, this));
    setPage(Page_Algorithm, new AlgorithmPage(fieldData, this));
    setPage(Page_Simple, new SimplePage(this));
    setPage(Page_Pimple, new PimplePage(this));
    setPage(Page_Piso, new PisoPage(this));
    setPage(Page_Parallel, new ParallelPage(this));
    setPage(Page_Tasks, new TasksPage(m_patchNames, m_fieldNames,
                                        m_functionObjects, this));
    setOption(QWizard::NoBackButtonOnStartPage);

    // Allow the user to finish the wizard at Page_Parallel
    connect(this, &QWizard::currentIdChanged, this, [this](int id) {
        if (id == Page_Parallel) {
            this->setOption(QWizard::HaveFinishButtonOnEarlyPages, true);
        } else {
            this->setOption(QWizard::HaveFinishButtonOnEarlyPages, false);
        }
    });
}

bool SolverWizard::parseFiles() {
    // Access OpenFOAM path on server
    CaseData caseData = m_systemMgr.getData(m_caseName);
    QString casePath = caseData.casePath;
    m_isOpenCFD = caseData.caseType.testFlag(IsOpenCFD);
    m_isSteadyState = !caseData.caseType.testFlag(Transient);
    m_isCompressible = caseData.caseType.testFlag(Compressible);
    auto system = m_systemMgr.getSystem(m_caseName);

    // Declare variables
    QString fileName;
    std::optional<QByteArray> fileData;
    std::shared_ptr<OpenFoamDictionary> dict;

    // Create list of files
    QStringList solverFiles;
    solverFiles = { "constant/polyMesh/boundary", "system/controlDict",
        "constant/turbulenceProperties" };
    solverFiles << (m_isOpenCFD ? "constant/transportProperties" :
                        "constant/physicalProperties");
    solverFiles << "system/fvSolution" << "system/decomposeParDict";

    // Create list of paths and check if files are present
    QStringList solverPaths;
    solverPaths.reserve(solverFiles.size());
    for (const QString& file : std::as_const(solverFiles))
        solverPaths.append(casePath + "/" + m_caseName + "/" + file);
    QString solverPathString = solverPaths.join("\n");
    QStringList results =
        system->processPaths(solverPathString, PathOperationType::CHECK);

    // boundary file
    int fileIndex = 0;
    fileName = solverFiles[fileIndex];
    if (results[fileIndex] == "0") {
        fileData = system->getFileContent(
            casePath + "/" + m_caseName + "/" + fileName);
        if (fileData && !fileData->isEmpty()) {
            m_boundaries = CaseIO::parseBoundary(fileData.value());
        }
    }
    if((results[fileIndex] != "0") || m_boundaries.empty()) {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle(tr("Missing Boundary Error"));
        msgBox.setText(tr("Missing or incomplete boundary file "
                          "(%1)").arg(solverFiles[fileIndex]));
        msgBox.setStandardButtons(QMessageBox::Ok);
        int result = msgBox.exec();
        if (result == QMessageBox::Ok) {
            return false;
        }
    }

    // controlDict
    fileIndex++;
    fileName = solverFiles[fileIndex];
    if (results[fileIndex] == "0") {
        fileData = system->getFileContent(
            casePath + "/" + m_caseName + "/" + fileName);
        if (fileData && !fileData->isEmpty()) {
            dict = std::make_shared<OpenFoamDictionary>(fileData.value());
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                m_controlConfig = CaseIO::parseControlDict(dict);
            } else {
                auto action = CaseIO::showParsingErrorMessage(fileName, this);
                switch(action) {
                case CaseIO::ParseErrorAction::EditFile:
                    emit createTextEditor( fileName.split('/').last(),
                                      m_caseName + "/0.orig", false);
                    return false;
                case CaseIO::ParseErrorAction::Overwrite:
                    break;
                case CaseIO::ParseErrorAction::Cancel:
                    return false;
                }
            }
        }
    }

    // turbulenceProperties
    fileIndex++;
    fileName = solverFiles[fileIndex];
    if (results[fileIndex] == "0") {
        fileData = system->getFileContent(
            casePath + "/" + m_caseName + "/" + fileName);
        if (fileData && !fileData->isEmpty()) {
            dict = std::make_shared<OpenFoamDictionary>(fileData.value());
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                m_turbulenceConfig = CaseIO::parseTurbulenceProperties(dict);
            } else {
                auto action = CaseIO::showParsingErrorMessage(fileName, this);
                switch(action) {
                case CaseIO::ParseErrorAction::EditFile:
                    emit createTextEditor(fileName.split('/').last(),
                                          m_caseName + "/" + fileName, false);
                    return false;
                case CaseIO::ParseErrorAction::Overwrite:
                    break;
                case CaseIO::ParseErrorAction::Cancel:
                    return false;
                }
            }
        }
    }

    // transportProperties
    fileIndex++;
    fileName = solverFiles[fileIndex];
    if (results[fileIndex] == "0") {
        fileData = system->getFileContent(
            casePath + "/" + m_caseName + "/" + fileName);
        if (fileData && !fileData->isEmpty()) {
            dict = std::make_shared<OpenFoamDictionary>(fileData.value());
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                CaseIO::parseTransportProperties(dict, m_transportConfig);
            } else {
                auto action = CaseIO::showParsingErrorMessage(fileName, this);
                switch(action) {
                case CaseIO::ParseErrorAction::EditFile:
                    emit createTextEditor(fileName.split('/').last(),
                                          m_caseName + "/" + fileName, false);
                    return false;
                case CaseIO::ParseErrorAction::Overwrite:
                    break;
                case CaseIO::ParseErrorAction::Cancel:
                    return false;
                }
            }
        }
    }

    // field files
    QStringList fieldFiles = system->processPaths(
        casePath + "/" + m_caseName + "/0.orig", PathOperationType::LIST);

    for(auto& fileName: fieldFiles) {
        if (fileName.endsWith('|')) {
            fileName.chop(1);
        }
        fileData = system->getFileContent(
            casePath + "/" + m_caseName + "/0.orig/" + fileName);
        if (fileData && !fileData->isEmpty()) {
            auto dict = std::make_shared<OpenFoamDictionary>(fileData.value());
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                CaseIO::FieldData fieldData;
                CaseIO::parseFieldFile(dict, fieldData);
                m_boundaryConfig[fileName] = fieldData;
            } else {
                auto action = CaseIO::showParsingErrorMessage(fileName, this);
                switch(action) {
                case CaseIO::ParseErrorAction::EditFile:
                    emit createTextEditor(fileName.split('/').last(),
                            m_caseName + "/0.orig/" + fileName, false);
                    return false;
                case CaseIO::ParseErrorAction::Overwrite:
                    break;
                case CaseIO::ParseErrorAction::Cancel:
                    return false;
                }
            }
        }
    }

    // fvSolution
    fileIndex++;
    fileName = solverFiles[fileIndex];
    if (results[fileIndex] == "0") {
        fileData = system->getFileContent(
            casePath + "/" + m_caseName + "/" + fileName);
        if (fileData && !fileData->isEmpty()) {
            dict = std::make_shared<OpenFoamDictionary>(fileData.value());
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                CaseIO::parseFvSolution(dict, m_mathConfig);
            } else {

                /*
                QList<SyntaxError> errors = dict->getSyntaxErrors();
                for(auto const& error: errors) {
                    qDebug() << error.message << ": " << error.text << ", Line " << error.line << ", Column " << error.column;
                }
                */

                auto action = CaseIO::showParsingErrorMessage(fileName, this);
                switch(action) {
                case CaseIO::ParseErrorAction::EditFile:
                    emit createTextEditor(fileName.split('/').last(),
                                          m_caseName + "/" + fileName, false);
                    return false;
                case CaseIO::ParseErrorAction::Overwrite:
                    break;
                case CaseIO::ParseErrorAction::Cancel:
                    return false;
                }
            }
        }
    }

    // decomposeParDict
    fileIndex++;
    fileName = solverFiles[fileIndex];
    if (results[fileIndex] == "0") {
        fileData = system->getFileContent(
            casePath + "/" + m_caseName + "/" + fileName);
        if (fileData && !fileData->isEmpty()) {
            dict = std::make_shared<OpenFoamDictionary>(fileData.value());
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                CaseIO::parseDecomposeParDict(dict, m_parallelConfig);
            } else {

                /*
                QList<SyntaxError> errors = dict->getSyntaxErrors();
                for(auto const& error: errors) {
                    qDebug() << error.message << ": " << error.text << ", Line " << error.line << ", Column " << error.column;
                }
                */

                auto action = CaseIO::showParsingErrorMessage(fileName, this);
                switch(action) {
                case CaseIO::ParseErrorAction::EditFile:
                    emit createTextEditor(fileName.split('/').last(),
                                          m_caseName + "/" + fileName, false);
                    return false;
                case CaseIO::ParseErrorAction::Overwrite:
                    break;
                case CaseIO::ParseErrorAction::Cancel:
                    return false;
                }
            }
        }
    }
    return true;
}

QStringList SolverWizard::getSolverFields() {
    // Access solver category and names
    QString solverFamily = m_controlConfig.solverFamily;
    QString solverName = (m_isOpenCFD) ? m_controlConfig.application :
                             m_controlConfig.solver;

    // Iterate through families
    QString testString;
    for (const auto& family : std::as_const(m_families)) {
        if (family.name == solverFamily) {
            for (const auto& solver : family.solvers) {
                testString = (m_isOpenCFD) ? solver.name :
                    solver.foundationName;
                if (testString == solverName) {
                    return solver.fields;
                }
            }
            break;
        }
    }
    return QStringList();
}

QStringList SolverWizard::getTurbulenceFields() {
    // Access turbulence category and names
    QString turbulenceCategory = m_turbulenceConfig.simulationType;
    QString turbulenceModel = m_turbulenceConfig.model;

    if (turbulenceModel.toLower() == "laminar") {
        return QStringList();
    }

    if (m_turbModels.contains(turbulenceCategory)) {
        const auto& subCategoryMap = m_turbModels[turbulenceCategory];
        for(const auto& modelVector : subCategoryMap) {
            for(const auto& model : modelVector) {
                if (model.name == turbulenceModel) {
                    return model.fields;
                }
            }
        }
    }
    return QStringList();
}

// Access the algorithm for the selected solver
FlowCompute::Algorithm SolverWizard::getSolverAlgorithm() {
    // Get the name of the selected application
    QString applicationName = m_controlConfig.application;

    // Check for Foundation or OpenCFD
    if (!m_isOpenCFD) {
        // Use result from checking ddtSchemes in fvSchemes
        if (m_isSteadyState) {
            return FlowCompute::Algorithm::SIMPLE;
        } else {
            return FlowCompute::Algorithm::PIMPLE;
        }
    } else {
        // Verify that the solver is in the map
        if (m_solverAlgorithmMap.contains(applicationName)) {
            return m_solverAlgorithmMap.value(applicationName);
        }
    }
    return FlowCompute::Algorithm::UNKNOWN;
}

// Check if a thermophysicalProperties file is needed
bool SolverWizard::isThermoRequired() {
    CaseData caseData = m_systemMgr.getData(m_caseName);
    // Identify the Boussinesq condition:
    bool isBoussinesq = !caseData.caseType.testFlag(Compressible) &&
                        !caseData.caseType.testFlag(Multiphase) &&
                        !caseData.caseType.testFlag(Combustion) &&
                        caseData.caseType.testFlag(FluidHeat) &&
                        caseData.caseType.testFlag(Buoyancy);
    if (isBoussinesq) {
        m_isThermoRequired = false;
    } else {
        m_isThermoRequired = caseData.caseType.testAnyFlags(
            Compressible | FluidHeat | ConjugateHeat | Combustion);
    }

    return m_isThermoRequired;
}

void SolverWizard::accept() {
    // Complete validation
    QWizard::accept();

    // Access communication
    CaseData caseData = m_systemMgr.getData(m_caseName);
    QString openFoamPath = caseData.openFoamPath;
    QString casePath = caseData.casePath;
    auto system = m_systemMgr.getSystem(m_caseName);

    // Update pressure dimensions if compressible
    if (m_isCompressible) {
        if (m_boundaryConfig.contains("p")) {
            m_boundaryConfig["p"].dimension = "[1 -1 -2 0 0 0 0]";
        }
        if (m_boundaryConfig.contains("p_rgh")) {
            m_boundaryConfig["p_rgh"].dimension = "[1 -1 -2 0 0 0 0]";
        }
    }

    // Update boundary file
    std::optional<QByteArray> fileData = system->getFileContent(
        casePath + "/" + m_caseName + "/constant/polyMesh/boundary");
    QByteArray newData = CaseIO::removeEmptyPatches(fileData.value());
    system->writeData(newData,
        casePath + "/" + m_caseName + "/constant/polyMesh/boundary");

    // Access the tasks page
    TasksPage* tasksPage = qobject_cast<TasksPage*>(page(Page_Tasks));
    if (!tasksPage) {
        qWarning() << "Error: Could not resolve TasksPage";
        return;
    }

    // Create the text for function objects
    QString funcText =
        CaseIO::createFunctionsBlock(m_isOpenCFD, m_functionObjects);

    // Add functions block for Foundation version
    if (!m_isOpenCFD)
        funcText = "functions\n{\n" + funcText + "}\n";

    // Update/create controlDict
    QString dictText, fileName = "system/controlDict";
    /*
    if (m_dictMap.contains(fileName)) {
        dictText = CaseIO::updateControlDict(
            m_dictMap[fileName], m_controlConfig, funcText);
    } else {
        dictText = CaseIO::createControlDict(
            m_controlConfig, openFoamPath, funcText);
    }
    */
    dictText = CaseIO::createControlDict(
        m_controlConfig, openFoamPath, funcText);
    system->writeData(dictText.toUtf8(),
        casePath + "/" + m_caseName + "/" + fileName);

    // Update/create turbulenceProperties
    fileName = "constant/turbulenceProperties";
    dictText = CaseIO::createTurbulenceProperties(m_turbulenceConfig,
                                                    openFoamPath);
    system->writeData(dictText.toUtf8(),
        casePath + "/" + m_caseName + "/" + fileName);

    // Update/create transportProperties if thermal properties aren't needed
    if (!m_isThermoRequired) {
        if (m_isOpenCFD) {
            fileName = "constant/transportProperties";
        } else {
            fileName = "constant/physicalProperties";
        }
        dictText = CaseIO::createTransportProperties(m_transportConfig,
                                                openFoamPath, m_isOpenCFD);
    } else {
        if (m_isOpenCFD) {
            fileName = "constant/thermophysicalProperties";
        } else {
            fileName = "constant/physicalProperties";
        }
        dictText = CaseIO::createThermophysicalProperties(m_thermoConfig,
                                                openFoamPath, m_isOpenCFD);
    }
    system->writeData(dictText.toUtf8(),
        casePath + "/" + m_caseName + "/" + fileName);

    // Update/create field files
    bool createField = false;
    QStringList fieldNames;
    for (auto it = m_boundaryConfig.constBegin();
         it != m_boundaryConfig.constEnd(); ++it) {
        const QString& fieldName = it.key();
        fieldNames.append(fieldNames);
        CaseIO::FieldData fieldData = it.value();
        fileName = "0.orig/" + fieldName;
        dictText = CaseIO::createFieldFile(fieldName,
            m_boundaryConfig[fieldName], openFoamPath);
        createField |= system->writeData(dictText.toUtf8(),
            casePath + "/" + m_caseName + "/" + fileName);
    }
    m_systemMgr.setFlag(m_caseName, CaseFlag::HasFieldFiles, createField);

    // Rename T to h or e as needed
    if (fieldNames.contains("T") && isThermoRequired()) {
        int index = fieldNames.indexOf("T");
        auto& thermoCfg = getThermoConfig();
        if (thermoCfg.thermoType.energy == "sensibleInternalEnergy") {
            fieldNames[index] = "e";
        } else if (thermoCfg.thermoType.energy == "sensibleEnthalpy") {
            fieldNames[index] = "h";
        }
    }

    // Update/create fvSolution
    fileName = "system/fvSolution";

    dictText = CaseIO::createFvSolution(m_mathConfig, openFoamPath,
                    m_isCompressible, !m_isSteadyState, m_isOpenCFD);
    system->writeData(dictText.toUtf8(),
        casePath + "/" + m_caseName + "/" + fileName);

    // Update/create decomposeParDict
    fileName = "system/decomposeParDict";
    dictText = CaseIO::createDecomposeParDict(m_parallelConfig, openFoamPath);
    system->writeData(dictText.toUtf8(),
        casePath + "/" + m_caseName + "/" + fileName);

    emit updatePath(m_caseName, "0.orig");
    emit updatePath(m_caseName, "constant");
    emit updatePath(m_caseName, "system");
}
