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

#include "wizard_solver.h"

#include "utils.h"
#include "solver_io.h"

SolverWizard::SolverWizard(const QString& caseName,
    const SystemManager& systemMgr,
    const std::vector<FlowCompute::SolverFamily>& families,
    const FlowCompute::TurbulenceDatabase& turbModels,
    const std::map<QString, FlowCompute::TransportPropertyDef>&
        transportProperties,
    const QHash<QString, FlowCompute::FieldDef>& fieldData,
    const std::vector<FlowCompute::BoundaryConditionDef>& boundaryConditions,
    QWidget *parent): QWizard(parent), m_caseName(caseName),
        m_systemMgr(systemMgr), m_families(families), m_turbModels(turbModels),
        m_fieldData(fieldData), m_boundaryConditions(boundaryConditions) {

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
    setPage(Page_Transient, new TransientPage(families, this));
    setPage(Page_Physics, new PhysicsPage(families, turbModels,
                                          transportProperties, this));
    setPage(Page_Boundary, new BoundaryPage(fieldData, boundaryConditions,
                                            this));
    setPage(Page_Algorithm, new AlgorithmPage(this));
    setPage(Page_Simple, new SimplePage(this));
    setPage(Page_Pimple, new PimplePage(this));
    setPage(Page_Piso, new PisoPage(this));
    setPage(Page_Parallel, new ParallelPage(this));
    setPage(Page_Visualization1, new VisualizationPage(this));

    setOption(QWizard::NoBackButtonOnStartPage);
}

bool SolverWizard::parseFiles() {
    // Access OpenFOAM path on server
    QString casePath = m_systemMgr.getData(m_caseName).casePath;
    auto system = m_systemMgr.getSystem(m_caseName);

    // Declare variables
    QString fileName;
    QByteArray fileData;
    std::shared_ptr<OpenFoamDictionary> dict;

    // boundary file
    fileName = "constant/polyMesh/boundary";
    fileData = system->getFileContent(
        casePath + "/" + m_caseName + "/" + fileName);
    if (!fileData.isEmpty()) {
        m_boundaries = SolverIO::parseBoundaryPatches(fileData);
    }
    if(m_boundaries.empty()) {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle(tr("Missing Boundary Error"));
        msgBox.setText(tr("Missing or incomplete boundary file "
                          "(constant/polyMesh/boundary)"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        int result = msgBox.exec();
        if (result == QMessageBox::Ok) {
            return false;
        }
    }

    // controlDict
    fileName = "system/controlDict";
    fileData = system->getFileContent(
        casePath + "/" + m_caseName + "/" + fileName);
    if (!fileData.isEmpty()) {
        dict = std::make_shared<OpenFoamDictionary>(fileData);
        if(!dict->hasSyntaxErrors()) {
            m_dictMap.insert(fileName, dict);
            m_controlConfig = SolverIO::parseControlConfig(dict);
        } else {
            auto action = Utils::showParsingErrorMessage(fileName, this);
            switch(action) {
            case Utils::ParseErrorAction::EditFile:
                emit createEditor(EditorType::TEXT, fileName.split('/').last(),
                                  m_caseName + "/0.orig", false);
                reject();
                return false;
            case Utils::ParseErrorAction::Overwrite:
                break;
            case Utils::ParseErrorAction::Cancel:
                return false;
            }
        }
    }

    // turbulenceProperties
    fileName = "constant/turbulenceProperties";
    fileData = system->getFileContent(
        casePath + "/" + m_caseName + "/" + fileName);
    if (!fileData.isEmpty()) {
        dict = std::make_shared<OpenFoamDictionary>(fileData);
        if(!dict->hasSyntaxErrors()) {
            m_dictMap.insert(fileName, dict);
            m_physicsConfig = SolverIO::parseTurbulenceProperties(dict);
        } else {
            auto action = Utils::showParsingErrorMessage(fileName, this);
            switch(action) {
            case Utils::ParseErrorAction::EditFile:
                emit createEditor(EditorType::TEXT,
                    fileName.split('/').last(), m_caseName + "/0.orig", false);
                reject();
                return false;
            case Utils::ParseErrorAction::Overwrite:
                break;
            case Utils::ParseErrorAction::Cancel:
                return false;
            }
        }
    }

    // transportProperties
    fileName = "constant/transportProperties";
    fileData = system->getFileContent(
        casePath + "/" + m_caseName + "/" + fileName);
    if (!fileData.isEmpty()) {
        dict = std::make_shared<OpenFoamDictionary>(fileData);
        if(!dict->hasSyntaxErrors()) {
            m_dictMap.insert(fileName, dict);
            SolverIO::parseTransportProperties(dict, m_physicsConfig);
        } else {
            auto action = Utils::showParsingErrorMessage(fileName, this);
            switch(action) {
            case Utils::ParseErrorAction::EditFile:
                emit createEditor(EditorType::TEXT,
                    fileName.split('/').last(), m_caseName + "/0.orig", false);
                reject();
                return false;
            case Utils::ParseErrorAction::Overwrite:
                break;
            case Utils::ParseErrorAction::Cancel:
                return false;
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
        QByteArray fileData = system->getFileContent(
            casePath + "/" + m_caseName + "/0.orig/" + fileName);
        if (!fileData.isEmpty()) {
            auto dict = std::make_shared<OpenFoamDictionary>(fileData);
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                FlowCompute::FieldData fieldData;
                SolverIO::parseFieldFile(dict, fieldData);
                m_boundaryConfig[fileName] = fieldData;
            } else {
                auto action = Utils::showParsingErrorMessage(fileName, this);
                switch(action) {
                case Utils::ParseErrorAction::EditFile:
                    emit createEditor(EditorType::TEXT,
                        fileName.split('/').last(), m_caseName + "/0.orig",
                            false);
                    reject();
                    return false;
                case Utils::ParseErrorAction::Overwrite:
                    break;
                case Utils::ParseErrorAction::Cancel:
                    return false;
                }
            }
        }
    }

    // fvSolution
    fileName = "system/fvSolution";
    fileData = system->getFileContent(
        casePath + "/" + m_caseName + "/" + fileName);
    if (!fileData.isEmpty()) {
        dict = std::make_shared<OpenFoamDictionary>(fileData);
        if(!dict->hasSyntaxErrors()) {
            m_dictMap.insert(fileName, dict);
            SolverIO::parseSolutionFile(dict, m_mathConfig);
        } else {

            /*
            QList<SyntaxError> errors = dict->getSyntaxErrors();
            for(auto const& error: errors) {
                qDebug() << error.message << ": " << error.text << ", Line " << error.line << ", Column " << error.column;
            }
            */

            auto action = Utils::showParsingErrorMessage(fileName, this);
            switch(action) {
            case Utils::ParseErrorAction::EditFile:
                emit createEditor(EditorType::TEXT, fileName.split('/').last(),
                                  m_caseName + "/system", false);
                reject();
                return false;
            case Utils::ParseErrorAction::Overwrite:
                break;
            case Utils::ParseErrorAction::Cancel:
                return false;
            }
        }
    }

    // decomposeParDict
    fileName = "system/decomposeParDict";
    fileData = system->getFileContent(
        casePath + "/" + m_caseName + "/" + fileName);
    if (!fileData.isEmpty()) {
        dict = std::make_shared<OpenFoamDictionary>(fileData);
        if(!dict->hasSyntaxErrors()) {
            m_dictMap.insert(fileName, dict);
            SolverIO::parseParallelFile(dict, m_parallelConfig);
        } else {

            /*
            QList<SyntaxError> errors = dict->getSyntaxErrors();
            for(auto const& error: errors) {
                qDebug() << error.message << ": " << error.text << ", Line " << error.line << ", Column " << error.column;
            }
            */

            auto action = Utils::showParsingErrorMessage(fileName, this);
            switch(action) {
            case Utils::ParseErrorAction::EditFile:
                emit createEditor(EditorType::TEXT, fileName.split('/').last(),
                                  m_caseName + "/system", false);
                reject();
                return false;
            case Utils::ParseErrorAction::Overwrite:
                break;
            case Utils::ParseErrorAction::Cancel:
                return false;
            }
        }
    }
    return true;
}

QStringList SolverWizard::getSolverFields() {
    // Access solver category and names
    QString solverFamily = m_controlConfig.solverCategory;
    QString solverName = m_controlConfig.solver;

    // Iterate through families
    for (const auto& family : std::as_const(m_families)) {
        if (family.name == solverFamily) {
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

    // Access turbulence category and names
    QString turbulenceCategory = m_physicsConfig.simulationType;
    QString turbulenceModel = m_physicsConfig.model;

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

    // Get the name of the selected solver
    QString solverName = m_controlConfig.solver;

    // Ensure the solver exists in our map to prevent crashes
    if (!m_solverAlgorithmMap.contains(solverName)) {
        return FlowCompute::Algorithm::UNKNOWN;
    }

    // Determine the algorithm
    return m_solverAlgorithmMap.value(solverName);
}

void SolverWizard::accept() {

    QWizard::accept();

    CaseData caseData = m_systemMgr.getData(m_caseName);
    QString openFoamPath = caseData.openFoamPath;
    QString casePath = caseData.casePath;
    auto system = m_systemMgr.getSystem(m_caseName);

    // Update boundary file
    QByteArray fileData = system->getFileContent(
        casePath + "/" + m_caseName + "/constant/polyMesh/boundary");
    QByteArray newData = SolverIO::removeEmptyPatches(fileData);
    system->writeData(newData,
        casePath + "/" + m_caseName + "/constant/polyMesh/boundary");

    // Update/create controlDict
    QString fileName = "system/controlDict";
    /*
    if (m_dictMap.contains(fileName)) {
        controlDictText = SolverIO::updateControlDict(m_dictMap[fileName], m_controlConfig);
    } else {
        controlDictText = SolverIO::createControlDict(m_controlConfig, openFoamPath);
    }
    */
    QString dictText = SolverIO::createControlDict(m_controlConfig,
        m_visualizationConfig, openFoamPath);
    system->writeData(dictText.toUtf8(),
        casePath + "/" + m_caseName + "/" + fileName);

    // Update/create turbulenceProperties
    fileName = "constant/turbulenceProperties";
    dictText = SolverIO::createTurbulenceProperties(m_physicsConfig,
                                                    openFoamPath);
    system->writeData(dictText.toUtf8(),
        casePath + "/" + m_caseName + "/" + fileName);

    // Update/create transportProperties
    fileName = "constant/transportProperties";
    dictText = SolverIO::createTransportProperties(m_physicsConfig,
                                                   openFoamPath);
    system->writeData(dictText.toUtf8(),
        casePath + "/" + m_caseName + "/" + fileName);

    // Update/create field files
    for (auto it = m_boundaryConfig.constBegin();
         it != m_boundaryConfig.constEnd(); ++it) {
        const QString& fieldName = it.key();
        FlowCompute::FieldData fieldData = it.value();
        fileName = "0.orig/" + fieldName;
        dictText = SolverIO::createFieldFile(fieldName,
            m_boundaryConfig[fieldName], openFoamPath);
        system->writeData(dictText.toUtf8(),
            casePath + "/" + m_caseName + "/" + fileName);
    }

    // Update/create fvSolution
    fileName = "system/fvSolution";
    dictText = SolverIO::createSolutionFile(m_mathConfig, openFoamPath);
    system->writeData(dictText.toUtf8(),
        casePath + "/" + m_caseName + "/" + fileName);

    // Update/create decomposeParDict
    fileName = "system/decomposeParDict";
    dictText = SolverIO::createParallelFile(m_parallelConfig, openFoamPath);
    system->writeData(dictText.toUtf8(),
        casePath + "/" + m_caseName + "/" + fileName);

    emit updatePath(m_caseName, "0.orig");
    emit updatePath(m_caseName, "constant");
    emit updatePath(m_caseName, "system");
}
