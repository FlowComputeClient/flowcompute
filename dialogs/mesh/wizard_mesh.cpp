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

#include "wizard_mesh.h"

#include "../../main_window.h"
#include "../../utils.h"
#include "mesh_io.h"

// Function declarations
MeshWizard::MeshWizard(const QString& caseName, const QStringList& cases,
                       QWidget *parent):
    m_caseName(caseName), QWizard(parent) {

    setWizardStyle(QWizard::ClassicStyle);
    setWindowTitle(tr("Mesh Configuration Wizard"));

    // Access the main window, get cases
    mainWin = qobject_cast<MainWindow*>(this->parentWidget());

    // Add pages using setPage with their explicit IDs
    setPage(Page_Geometry, new GeometryPage(m_caseName, cases, this));
    setPage(Page_BlockMesh1, new BlockMeshPage1(this));
    setPage(Page_BlockMesh2, new BlockMeshPage2(this));
    setPage(Page_SurfaceExtraction, new SurfaceExtractionPage(this));
    setPage(Page_Castellation, new CastellationPage(this));
    setPage(Page_SnapControl, new SnapControlPage(this));
    setPage(Page_LayerControl, new LayerControlPage(this));
    setOption(QWizard::NoBackButtonOnStartPage);
}

bool MeshWizard::loadParseFiles() {

    // Access OpenFOAM path on server
    m_caseName = field("caseName").toString();
    m_targetId = mainWin->m_caseMap[m_caseName].targetSystemId;
    m_casePath = mainWin->m_caseMap[m_caseName].casePath;

    // Declare variables
    QString fileName;
    QByteArray fileData;
    std::shared_ptr<OpenFoamDictionary> dict;

    // If blockMesh should be run
    if (m_runBlockMesh) {

        // Load data
        fileName = "system/blockMeshDict";
        fileData = mainWin->targetSystems[m_targetId]->getFileContent(
            m_casePath + "/" + m_caseName + "/" + fileName);
        if (!fileData.isEmpty()) {
            dict = std::make_shared<OpenFoamDictionary>(fileData);
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                m_blockMeshConfig = MeshIO::parseBlockMesh(dict);
            } else {
                auto action = Utils::showParsingErrorMessage(fileName, this);
                switch(action) {
                case Utils::ParseErrorAction::EditFile:
                    mainWin->createEditor(EditorType::TEXT,
                        fileName.split('/').last(), m_caseName + "/system");
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

    // If surfaceFeatureExtract should be run
    if (m_runExtract) {

        // Load data
        fileName = "system/surfaceFeatureExtractDict";
        fileData = mainWin->targetSystems[m_targetId]->getFileContent(
            m_casePath + "/" + m_caseName + "/" + fileName);
        if (!fileData.isEmpty()) {
            dict = std::make_shared<OpenFoamDictionary>(fileData);
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                m_surfaceFeatureMap = MeshIO::parseSurfaceFeatureData(
                    dict, m_geometryMap.keys());
            } else {
                auto action = Utils::showParsingErrorMessage(fileName, this);
                switch(action) {
                case Utils::ParseErrorAction::EditFile:
                    mainWin->createEditor(EditorType::TEXT,
                        fileName.split('/').last(), m_caseName + "/system");
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

    // If snappyHexMesh should be run
    if ((m_runCastellated) || (m_runSnap) || (m_runLayers)) {

        // Load data
        fileName = "system/snappyHexMeshDict";
        fileData = mainWin->targetSystems[m_targetId]->getFileContent(
            m_casePath + "/" + m_caseName + "/" + fileName);
        if (!fileData.isEmpty()) {
            dict = std::make_shared<OpenFoamDictionary>(fileData);
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                if (m_runCastellated) m_castellatedMeshConfig =
                        MeshIO::parseCastellatedMesh(dict);
                if (m_runSnap) m_snapControlConfig =
                        MeshIO::parseSnapControlConfig(dict);
                if (m_runLayers) m_layerControlConfig =
                        MeshIO::parseLayerControlConfig(dict);
            }  else {
                auto action = Utils::showParsingErrorMessage(fileName, this);
                switch(action) {
                case Utils::ParseErrorAction::EditFile:
                    mainWin->createEditor(EditorType::TEXT,
                        fileName.split('/').last(), m_caseName + "/system");
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
    return true;
}

void MeshWizard::accept() {

    QWizard::accept();

    QString openFoamPath = mainWin->m_caseMap[m_caseName].openFoamPath;

    // Update or create blockMeshDict
    if (m_runBlockMesh) {

        QString blockMeshDictText;
        if (m_dictMap.contains("system/blockMeshDict")) {
            blockMeshDictText =
                MeshIO::updateBlockMeshDict(m_dictMap["system/blockMeshDict"],
                    m_blockMeshConfig);
        } else {
            blockMeshDictText =
                MeshIO::createBlockMeshDict(m_blockMeshConfig, openFoamPath);
        }

        // Update file
        mainWin->targetSystems[m_targetId]->writeData(
            blockMeshDictText.toUtf8(),
            m_casePath + "/" + m_caseName + "/system/blockMeshDict");
    }

    // Update or create surfaceFeatureExtractDict
    if (m_runExtract) {

        /*
        QString surfaceFeatureExtractDictText;
        if (m_dictMap.contains("system/surfaceFeatureExtractDict")) {
            surfaceFeatureExtractDictText =
                MeshIO::updateSurfaceFeatureExtractDict(
                    m_dictMap["system/surfaceFeatureExtractDict"],
                        m_surfaceFeatureMap);
        } else {
            surfaceFeatureExtractDictText =
                MeshIO::createSurfaceFeatureExtractDict(m_surfaceFeatureMap,
                    openFoamPath);
        }
        */

        QString surfaceFeatureExtractDictText =
            MeshIO::createSurfaceFeatureExtractDict(m_surfaceFeatureMap,
                openFoamPath);

        // Update file
        mainWin->targetSystems[m_targetId]->writeData(
            surfaceFeatureExtractDictText.toUtf8(),
            m_casePath + "/" + m_caseName +
                "/system/surfaceFeatureExtractDict");
    }

    // Update or create snappyHexMeshDict
    if ((m_runCastellated) || (m_runSnap) || (m_runLayers)) {

        QString snappyHexMeshDictText;
        /*
        if (m_dictMap.contains("system/snappyHexMeshDict")) {
            snappyHexMeshDictText =
                MeshIO::updateSnappyHexMeshDict(
                    m_dictMap["system/snappyHexMeshDict"],
                    m_castellatedMeshConfig, m_snapControlConfig,
                    m_layerControlConfig);
        } else {
            snappyHexMeshDictText =
                MeshIO::createSnappyHexMeshDict(m_surfaceFeatureMap,
                    m_castellatedMeshConfig, m_snapControlConfig,
                    m_layerControlConfig, openFoamPath);
        }
        */

        snappyHexMeshDictText =
            MeshIO::createSnappyHexMeshDict(m_surfaceFeatureMap,
                m_castellatedMeshConfig, m_snapControlConfig,
                m_layerControlConfig, openFoamPath);

        // Update file
        mainWin->targetSystems[m_targetId]->writeData(
            snappyHexMeshDictText.toUtf8(),
            m_casePath + "/" + m_caseName + "/system/snappyHexMeshDict");
    }

    mainWin->updatePath(m_caseName, "system", m_targetId);
}
