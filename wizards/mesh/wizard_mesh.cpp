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

#include <QDir>

#include "mesh_io.h"
#include "utils.h"

// Function declarations
MeshWizard::MeshWizard(const QString& caseName, const SystemManager& systemMgr,
    QWidget *parent): m_caseName(caseName), m_systemMgr(systemMgr),
    QWizard(parent) {
    // Configure the wizard appearance
    setWizardStyle(QWizard::ClassicStyle);
    setWindowTitle(tr("Mesh Configuration Wizard"));

    // Add pages using setPage with their explicit IDs
    QStringList cases = m_systemMgr.getCases();
    setPage(Page_Geometry, new GeometryPage(m_caseName, m_systemMgr, this));
    setPage(Page_BlockMesh1, new BlockMeshPage1(m_systemMgr, this));
    setPage(Page_BlockMesh2, new BlockMeshPage2(this));
    setPage(Page_SurfaceExtraction, new SurfaceFeaturePage(this));
    setPage(Page_Castellation, new CastellationPage(this));
    setPage(Page_SnapControl, new SnapControlPage(this));
    setPage(Page_LayerControl, new LayerControlPage(this));
    setOption(QWizard::NoBackButtonOnStartPage);
}

bool MeshWizard::loadParseFiles() {

    // Access OpenFOAM path on server
    m_caseName = field("caseName").toString();
    m_casePath = m_systemMgr.getData(m_caseName).casePath;
    auto system = m_systemMgr.getSystem(m_caseName);

    // Declare variables
    QString fileName;
    QByteArray fileData;
    std::shared_ptr<OpenFoamDictionary> dict;

    // If blockMesh should be run
    if (m_runBlockMesh) {

        // Load data
        fileName = "system/blockMeshDict";
        fileData = system->getFileContent(
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
                    emit createEditor(EditorType::TEXT,
                        fileName.split('/').last(), m_caseName + "/system",
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

    // If surfaceFeatureExtract should be run
    if (m_runExtract) {
        // Load data
        fileName = "system/surfaceFeatureExtractDict";
        fileData = system->getFileContent(
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
                    emit createEditor(EditorType::TEXT,
                        fileName.split('/').last(), m_caseName + "/system",
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

    // If snappyHexMesh should be run
    if ((m_runCastellated) || (m_runSnap) || (m_runLayers)) {

        // Load data
        fileName = "system/snappyHexMeshDict";
        fileData = system->getFileContent(
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
                    emit createEditor(EditorType::TEXT,
                        fileName.split('/').last(), m_caseName + "/system",
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
    return true;
}

void MeshWizard::accept() {
    // Validate last page before continuing
    QWizard::accept();
    QString openFoamPath = m_systemMgr.getData(m_caseName).openFoamPath;
    auto system = m_systemMgr.getSystem(m_caseName);

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
        system->writeData(blockMeshDictText.toUtf8(),
            m_casePath + "/" + m_caseName + "/system/blockMeshDict");
    }

    // Update or create surfaceFeatureExtractDict
    if (m_runExtract) {

        /*
        QString surfaceFeatureDictText;
        if (m_dictMap.contains("system/surfaceFeatureExtractDict")) {
            surfaceFeatureDictText =
                MeshIO::updateSurfaceFeatureExtractDict(
                    m_dictMap["system/surfaceFeatureExtractDict"],
                        m_surfaceFeatureMap);
        } else {
            surfaceFeatureDictText =
                MeshIO::createSurfaceFeatureExtractDict(m_surfaceFeatureMap,
                    openFoamPath);
        }
        */

        // Determine which OpenFOAM release is used
        QString dirName = QDir(openFoamPath).dirName();
        const QRegularExpression foundationRegex("^openfoam\\d{2}$",
            QRegularExpression::CaseInsensitiveOption);
        bool isFoundation = foundationRegex.match(dirName).hasMatch();

        QString surfaceFeatureDictText =
            MeshIO::createSurfaceFeatureDict(m_surfaceFeatureMap,
                openFoamPath);

        // Update file
        QString fileName = (isFoundation) ? "/system/surfaceFeaturesDict" :
                               "/system/surfaceFeatureExtractDict";
        system->writeData(surfaceFeatureDictText.toUtf8(),
            m_casePath + "/" + m_caseName + fileName);
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
        system->writeData(snappyHexMeshDictText.toUtf8(),
            m_casePath + "/" + m_caseName + "/system/snappyHexMeshDict");
    }

    // Refresh the system folder
    emit updatePath(m_caseName, "system");
}
