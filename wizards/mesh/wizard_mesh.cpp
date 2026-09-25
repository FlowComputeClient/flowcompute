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

#include "wizards/mesh/wizard_mesh.h"

#include <QDir>

#include "wizards/mesh/page_10_geometry.h"
#include "wizards/mesh/page_20_blockmesh.h"
#include "wizards/mesh/page_30_blockmesh.h"
#include "wizards/mesh/page_40_surface_feature.h"
#include "wizards/mesh/page_50_castellation.h"
#include "wizards/mesh/page_60_snapcontrol.h"
#include "wizards/mesh/page_70_layercontrol.h"

// Function declarations
MeshWizard::MeshWizard(const QString& caseName, SystemManager& systemMgr,
    QWidget *parent): QWizard(parent), m_caseName(caseName),
    m_systemMgr(systemMgr) {
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

bool MeshWizard::parseFiles() {
    // Access OpenFOAM path on server
    m_caseName = field("caseName").toString();
    CaseData caseData = m_systemMgr.getData(m_caseName);
    m_casePath = caseData.casePath;
    m_isOpenCFD = caseData.caseType.testFlag(IsOpenCFD);
    auto system = m_systemMgr.getSystem(m_caseName);

    // Check if files are present
    QStringList meshFiles;
    meshFiles << "blockMeshDict"
        << (m_isOpenCFD ? "surfaceFeatureExtractDict" : "surfaceFeaturesDict")
        << "snappyHexMeshDict";
    QString meshFileString = meshFiles.join("\n");
    QStringList results =
        system->processPaths(meshFileString, PathOperationType::CHECK);

    // Declare variables
    QString fileName;
    std::optional<QByteArray> fileData;
    std::shared_ptr<OpenFoamDictionary> dict;

    // If blockMesh should be run
    if (m_runBlockMesh && (results[0] == "0")) {
        // Load data
        fileName = "system/" + meshFiles[0];
        fileData = system->getFileContent(
            m_casePath + "/" + m_caseName + "/" + fileName);
        if (fileData && !fileData.value().isEmpty()) {
            dict = std::make_shared<OpenFoamDictionary>(fileData.value());
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                m_blockMeshConfig = CaseIO::parseBlockMeshDict(dict);
            } else {
                auto action = CaseIO::showParsingErrorMessage(fileName, this);
                switch(action) {
                case CaseIO::ParseErrorAction::EditFile:
                    emit createTextEditor(fileName.split('/').last(),
                        m_caseName + "/system/" + fileName, false);
                    reject();
                    return false;
                case CaseIO::ParseErrorAction::Overwrite:
                    break;
                case CaseIO::ParseErrorAction::Cancel:
                    return false;
                }
            }
        }
    }

    // If surfaceFeatureExtract should be run
    if (m_runExtract && (results[1] == "0")) {
        // Load data
        fileName = "system/" + meshFiles[1];
        fileData = system->getFileContent(
            m_casePath + "/" + m_caseName + "/" + fileName);
        if (fileData && !fileData.value().isEmpty()) {
            dict = std::make_shared<OpenFoamDictionary>(fileData.value());
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                m_surfaceFeatureMap = CaseIO::parseSurfaceFeatureData(
                    dict, m_geometryMap.keys());
            } else {
                auto action = CaseIO::showParsingErrorMessage(fileName, this);
                switch(action) {
                case CaseIO::ParseErrorAction::EditFile:
                    emit createTextEditor(fileName.split('/').last(),
                                m_caseName + "/system/" + fileName, false);
                    reject();
                    return false;
                case CaseIO::ParseErrorAction::Overwrite:
                    break;
                case CaseIO::ParseErrorAction::Cancel:
                    return false;
                }
            }
        }
    }

    // If snappyHexMesh should be run
    if ((m_runCastellated || m_runSnap || m_runLayers) && (results[2] == "0")) {
        // Load data
        fileName = "system/" + meshFiles[2];
        fileData = system->getFileContent(
            m_casePath + "/" + m_caseName + "/" + fileName);
        if (fileData && !fileData.value().isEmpty()) {
            dict = std::make_shared<OpenFoamDictionary>(fileData.value());
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                if (m_runCastellated) m_castellatedMeshConfig =
                        CaseIO::parseCastellatedMesh(dict);
                if (m_runSnap) m_snapControlConfig =
                        CaseIO::parseSnapControlConfig(dict);
                if (m_runLayers) m_layerControlConfig =
                        CaseIO::parseLayerControlConfig(dict);
            }  else {
                auto action = CaseIO::showParsingErrorMessage(fileName, this);
                switch(action) {
                case CaseIO::ParseErrorAction::EditFile:
                    emit createTextEditor(fileName.split('/').last(),
                                m_caseName + "/system/" + fileName, false);
                    reject();
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
                CaseIO::updateBlockMeshDict(m_dictMap["system/blockMeshDict"],
                    m_blockMeshConfig);
        } else {
            blockMeshDictText =
                CaseIO::createBlockMeshDict(m_blockMeshConfig, openFoamPath);
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

        QString surfaceFeatureDictText =
            CaseIO::createSurfaceFeatureDict(m_surfaceFeatureMap,
                openFoamPath);

        // Update file
        QString fileName = (m_isOpenCFD) ? "/system/surfaceFeatureExtractDict" :
                               "/system/surfaceFeaturesDict";
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
            CaseIO::createSnappyHexMeshDict(m_surfaceFeatureMap,
                m_castellatedMeshConfig, m_snapControlConfig,
                m_layerControlConfig, openFoamPath);

        // Update file
        system->writeData(snappyHexMeshDictText.toUtf8(),
            m_casePath + "/" + m_caseName + "/system/snappyHexMeshDict");
    }

    // Update the case's flags
    m_systemMgr.updateFlags(m_caseName, m_casePath);

    // Refresh the system folder
    emit updatePath(m_caseName, "system");
}
