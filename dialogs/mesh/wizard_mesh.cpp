#include "wizard_mesh.h"

#include "../../main_window.h"

// Function declarations
MeshWizard::MeshWizard(QWidget *parent): QWizard(parent) {

    setWizardStyle(QWizard::ClassicStyle);
    setStyleSheet("color: black;");
    setWindowTitle(tr("Mesh Configuration Wizard"));

    // Access the main window, get cases
    mainWin = qobject_cast<MainWindow*>(this->parentWidget());

    // Add pages using setPage with their explicit IDs
    setPage(Page_Geometry, new GeometryPage(this));
    setPage(Page_BlockMesh1, new BlockMeshPage1(this));
    setPage(Page_BlockMesh2, new BlockMeshPage2(this));
    setPage(Page_SurfaceExtraction, new SurfaceExtractionPage(this));
    setPage(Page_Castellation, new CastellationPage(this));
    setPage(Page_SnapControl, new SnapControlPage(this));
    setPage(Page_LayerControl, new LayerControlPage(this));
    setPage(Page_Execution, new ExecutionPage(this));
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
        fileData = mainWin->targetSystems[m_targetId]->getFileContent(m_casePath + "/" + m_caseName + "/" + fileName);
        if (!fileData.isEmpty()) {
            dict = std::make_shared<OpenFoamDictionary>(fileData);
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                m_blockMeshConfig = MeshIO::parseBlockMesh(dict);
            } else {
                if(!showParsingErrorMessage(fileName)) {
                    return false;
                }
            }
        }
    }

    // If surfaceFeatureExtract should be run
    if (m_runExtract) {

        // Load data
        fileName = "system/surfaceFeatureExtractDict";
        fileData = mainWin->targetSystems[m_targetId]->getFileContent(m_casePath + "/" + m_caseName + "/" + fileName);
        if (!fileData.isEmpty()) {
            dict = std::make_shared<OpenFoamDictionary>(fileData);
            if(!dict->hasSyntaxErrors()) {
                m_dictMap.insert(fileName, dict);
                m_surfaceFeatureMap = MeshIO::parseSurfaceFeatureData(dict, m_geometryMap.keys());
            } else {
                if(!showParsingErrorMessage(fileName)) {
                    return false;
                }
            }
        }
    }

    // If snappyHexMesh should be run
    if ((m_runCastellated) || (m_runSnap) || (m_runLayers)) {

        // Load data
        fileName = "system/snappyHexMeshDict";
        fileData = mainWin->targetSystems[m_targetId]->getFileContent(m_casePath + "/" + m_caseName + "/" + fileName);
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
                if(!showParsingErrorMessage(fileName)) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool MeshWizard::showParsingErrorMessage(QString fileName) {

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
        mainWin->createEditor(EditorType::TEXT, fileName.split('/').last(), m_caseName + "/system");
        reject();
        return false;
    } else if (errorDialog.clickedButton() == overwriteBtn) {
        return true;
    } else if (errorDialog.clickedButton() == cancelBtn) {
        return false;
    }
    return false;
}

void MeshWizard::accept() {

    QString openFoamPath = mainWin->m_caseMap[m_caseName].openFoamPath;

    // Update or create blockMeshDict
    if (m_runBlockMesh) {
        QString blockMeshDictText;
        if (m_dictMap.contains("system/blockMeshDict")) {
            blockMeshDictText = MeshIO::updateBlockMeshDict(m_dictMap["system/blockMeshDict"], m_blockMeshConfig);
        } else {
            blockMeshDictText = MeshIO::createBlockMeshDict(m_blockMeshConfig, openFoamPath);
        }

        // Update file
        mainWin->targetSystems[m_targetId]->writeData(blockMeshDictText.toUtf8(),
            m_casePath + "/" + m_caseName + "/system/blockMeshDict");

        // Launch utility
        QString cmd = QString("cd %1; source %2/etc/bashrc && blockMesh").arg(m_casePath + "/" + m_caseName, openFoamPath);
        QString output;
        if (mainWin->targetSystems[m_targetId]->launchShortUtility(cmd, output) == 0) {
            mainWin->log(output);
        }
    }

    // Update or create surfaceFeatureExtractDict
    if (m_runExtract) {
        QString surfaceFeatureExtractDictText;
        if (m_dictMap.contains("system/surfaceFeatureExtractDict")) {
            surfaceFeatureExtractDictText =
                MeshIO::updateSurfaceFeatureExtractDict(m_dictMap["system/surfaceFeatureExtractDict"], m_surfaceFeatureMap);
        } else {
            surfaceFeatureExtractDictText =
                MeshIO::createSurfaceFeatureExtractDict(m_surfaceFeatureMap, openFoamPath);
        }

        // Update file
        mainWin->targetSystems[m_targetId]->writeData(surfaceFeatureExtractDictText.toUtf8(),
            m_casePath + "/" + m_caseName + "/system/surfaceFeatureExtractDict");

        // Launch utility
        QString cmd = QString("cd %1; source %2/etc/bashrc && surfaceFeatureExtract").arg(m_casePath + "/" + m_caseName, openFoamPath);
        QString output;
        if (mainWin->targetSystems[m_targetId]->launchShortUtility(cmd, output) == 0) {
            mainWin->log(output);
        }
    }

    // Update or create snappyHexMeshDict
    if ((m_runCastellated) || (m_runSnap) || (m_runLayers)) {
        QString snappyHexMeshDictText;
        if (m_dictMap.contains("system/snappyHexMeshDict")) {
            snappyHexMeshDictText =
                MeshIO::updateSnappyHexMeshDict(m_dictMap["system/snappyHexMeshDict"],
                    m_castellatedMeshConfig, m_snapControlConfig, m_layerControlConfig);
        } else {
            snappyHexMeshDictText =
                MeshIO::createSnappyHexMeshDict(m_castellatedMeshConfig,
                    m_snapControlConfig, m_layerControlConfig, openFoamPath);
        }

        // Update file
        mainWin->targetSystems[m_targetId]->writeData(snappyHexMeshDictText.toUtf8(),
            m_casePath + "/" + m_caseName + "/system/snappyHexMeshDict");

        // Launch utility
        QString cmd = QString("cd %1; source %2/etc/bashrc && snappyHexMesh").arg(m_casePath + "/" + m_caseName, openFoamPath);
        QString output;
        if (mainWin->targetSystems[m_targetId]->launchShortUtility(cmd, output) == 0) {
            mainWin->log(output);
        }
    }

    // Launch mesh utilities
    // mainWin->runMesh(m_casePath + "/" + m_caseName, openFoamPath, m_runBlockMesh, m_runExtract,
    //                 m_runCastellated || m_runSnap || m_runLayers, m_targetId);

    QWizard::accept();
}
