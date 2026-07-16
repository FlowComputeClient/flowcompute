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

#include "./main_window.h"

#include <QApplication>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QProgressDialog>

#include <memory>
#include <string>
#include <vector>
#include <utility>

#include "dialogs/preferences/preferences_dialog.h"
#include "dialogs/run_mesh/run_mesh_dialog.h"
#include "dialogs/run_solver/run_solver_dialog.h"
#include "editors/graphical/surface/surface_editor.h"
#include "editors/graphical/mesh/mesh_editor.h"
#include "editors/graphical/result/result_editor.h"
#include "geometry/mesh/mesh_reader.h"
#include "geometry/stl/stl_reader.h"
#include "geometry/obj/obj_reader.h"
#include "./utils.h"
#include "wizards/mesh/wizard_mesh.h"
#include "wizards/new_case/wizard_new_case.h"
#include "wizards/solver/wizard_solver.h"

// Launch new case wizard
void MainWindow::launchNewCaseWizard() {
    // Create the wizard
    auto* wizard = new NewCaseWizard(m_isWindows, m_isWslAvailable,
                                     m_systemMgr, this);
    wizard->setAttribute(Qt::WA_DeleteOnClose);

    // Connect the signal to your slot
    connect(wizard, &NewCaseWizard::requestCaseCreation,
            this, &MainWindow::createCase);

    // Open it modally
    wizard->exec();
}

// Create new case folder
void MainWindow::createCase(QString caseName, QString casePath,
        QStringList caseFiles, int targetId, QString openFoamPath) {
    // Add case to map
    m_systemMgr.addCase(caseName, CaseData{casePath, caseFiles, targetId,
                                        openFoamPath});

    // Update utility map if necessary
    if (!m_utilMap.contains(openFoamPath)) {
        QString path = casePath + "/" + caseName + "/";
        m_utilMap[openFoamPath] = checkUtilities(path, m_utilities);
    }

    // Display case in navigator
    m_navigator->addCase(caseName, caseFiles);
    m_navigator->expandCase(caseName);
}

// Create an editor for the given file
void MainWindow::createEditor(EditorType type, QString& fileName,
        const QString& fullPath, bool logMessage) {
    // Check for existing editor
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        QString tabName = m_tabWidget->tabText(i);
        EditorType tabType = m_tabMap[tabName].type;
        QString tabPath = m_tabWidget->tabBar()->tabData(i).toString();
        if (tabName.startsWith(fileName) && tabPath == fullPath &&
            type == tabType) {
            m_tabWidget->setCurrentIndex(i);
            return;
        }
    }

    // Get case name
    QString caseName, timeFolder;
    if ((type != EditorType::MESH) && (type != EditorType::RESULT)) {
        caseName = fullPath.split('/').first();
    } else {
        int pos = fileName.indexOf(" (");
        if (pos != -1) {
            if (type == EditorType::RESULT) {
                QString number = fileName.mid(pos + 9);
                number.chop(1);
                bool ok;
                int timeVal = number.toInt(&ok);
                timeFolder = (ok) ? QString::number(timeVal) : "";
            }
            fileName.truncate(pos);
        }
        caseName = fileName;
    }

    // Access case data
    CaseData caseData = m_systemMgr.getData(caseName);
    int targetId = caseData.targetId;
    QString casePath = caseData.casePath + "/" + caseName;
    QString path = caseData.casePath + "/" + fullPath + "/" + fileName;
    QString openFoamPath = caseData.openFoamPath;

    TabData tabData;
    tabData.fullPath = fullPath;
    tabData.type = type;

    // Update log
    if (logMessage)
        m_console->appendPlainText(tr("Reading %1\n").arg(path));

    // Text editor
    if (type == EditorType::TEXT) {
        TextEditor* textEditor = new TextEditor(this);
        textEditor->setFont(m_font);

        QByteArray data = m_systemMgr.getSystem(caseName)->getFileContent(path);
        textEditor->setTextData(data);
        textEditor->applyTheme(m_textTheme);

        int tabIndex = m_tabWidget->addTab(textEditor, fileName);
        m_tabWidget->setCurrentIndex(tabIndex);
        m_tabWidget->tabBar()->setTabData(tabIndex, fullPath);

        connect(textEditor->document(), &QTextDocument::undoAvailable,
            m_undoAction, &QAction::setEnabled);
        connect(textEditor->document(), &QTextDocument::redoAvailable,
            m_redoAction, &QAction::setEnabled);
        connect(textEditor, &TextEditor::dirtyStateChanged, this,
            [this, textEditor](bool isDirty) {
                onDirtyStateChanged(isDirty, textEditor);
            });

        // Track immediately for synchronous tabs
        m_tabMap.insert(fileName, tabData);
        return;
    }

    // Surface editor
    if (type == EditorType::SURFACE) {
        bool isBinary = false;
        RenderData model;
        QByteArray data;
        if (fileName.endsWith(".stl", Qt::CaseInsensitive)) {
            data = m_systemMgr.getSystem(caseName)->getFileContent(path);
            std::pair<RenderData, bool> res =
                StlReader::readStlFile(fileName, data);
            model = res.first;
            isBinary = res.second;
        } else if (fileName.endsWith(".obj", Qt::CaseInsensitive)) {
            data = m_systemMgr.getSystem(caseName)->getFileContent(path);
            model = ObjReader::readObjFile(fileName, data);
        }
        std::shared_ptr<RenderData> modelData =
            std::make_shared<RenderData>(std::move(model));

        SurfaceEditor* surfaceEditor = new SurfaceEditor(modelData, path,
                targetId, &m_vulkanInstance, isBinary, this);
        surfaceEditor->applyTheme(m_graphicalTheme);
        int tabIndex = m_tabWidget->addTab(surfaceEditor, fileName);
        m_tabWidget->setCurrentIndex(tabIndex);
        m_tabWidget->tabBar()->setTabData(tabIndex, fullPath);

        // Action default configurations
        m_undoAction->setDisabled(true);
        m_redoAction->setDisabled(true);
        m_saveFileAction->setDisabled(true);

        connect(surfaceEditor, &SurfaceEditor::surfacePatchRequested,
                this, &MainWindow::runSurfacePatch);
        connect(surfaceEditor, &SurfaceEditor::surfaceCheckRequested,
                this, &MainWindow::runSurfaceCheck);
        connect(surfaceEditor, &SurfaceEditor::surfaceScaleRequested,
                this, &MainWindow::runSurfaceScale);
        connect(surfaceEditor, &SurfaceEditor::dirtyStateChanged, this,
            [this, surfaceEditor](bool isDirty) {
                onDirtyStateChanged(isDirty, surfaceEditor);
            });

        m_tabMap.insert(fileName, tabData);
        return;
    }

    // Create progress dialog
    auto* progress = new QProgressDialog(
        type == EditorType::MESH ? "Loading mesh..." : "Loading Results...",
        QString(), 0, 0, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumWidth(300);
    progress->setAttribute(Qt::WA_DeleteOnClose);

    using RenderDataPtr = std::shared_ptr<RenderData>;

    // Mesh editor
    if (type == EditorType::MESH) {
        progress->show();
        auto* watcher = new QFutureWatcher<RenderDataPtr>(this);

        // Connect watcher to delete itself when finished
        connect(watcher, &QFutureWatcher<RenderDataPtr>::finished, watcher,
                &QObject::deleteLater);

        connect(watcher, &QFutureWatcher<RenderDataPtr>::finished, this,
        [this, watcher, progress, casePath, caseName, fileName, fullPath,
            tabData]() {
            if (!progress)
                return;

            // Get mesh data
            progress->close();
            RenderDataPtr renderData = watcher->result();
            if (!renderData)
                return;

            // Create mesh editor
            auto* meshEditor = new MeshEditor(
                renderData, casePath, m_systemMgr.getSystem(caseName),
                m_solverFamilies, m_turbulenceModels, m_fieldData,
                m_boundaryConditions, &m_vulkanInstance, this);
            meshEditor->applyTheme(m_graphicalTheme);

            connect(meshEditor, &MeshEditor::updatePath, this,
                    &MainWindow::updatePath);
            connect(meshEditor, &MeshEditor::meshPatchRequested, this,
                    &MainWindow::runMeshPatch);
            connect(meshEditor, &MeshEditor::meshCheckRequested, this,
                    &MainWindow::runMeshCheck);
            connect(meshEditor, &MeshEditor::meshRenumberRequested, this,
                    &MainWindow::runMeshRenumber);

            QString tabTitle = fileName + " (mesh)";
            int tabIndex = m_tabWidget->addTab(meshEditor, tabTitle);
            m_tabWidget->setCurrentIndex(tabIndex);
            m_tabWidget->tabBar()->setTabData(tabIndex, fullPath);

            m_undoAction->setDisabled(true);
            m_redoAction->setDisabled(true);
            m_saveFileAction->setDisabled(true);

            m_tabMap.insert(tabTitle, tabData);
        });

        watcher->setFuture(QtConcurrent::run(&MainWindow::getMeshData,
            this, caseName, casePath, openFoamPath));
    }

    // Result editor
    if (type == EditorType::RESULT) {
        QString resultPath = casePath + "/postProcessing/surfaces";
        QString res =
            m_systemMgr.getSystem(caseName)->getResultFolders(resultPath);
        if (res.isEmpty()) {
            qWarning() << "Couldn't find result folders";
            progress->deleteLater();
            return;
        }

        // Set time folders
        QStringList timeFolders = res.split(',');
        if (timeFolder.isEmpty())
            timeFolder = timeFolders.back();

        progress->show();
        auto* watcher = new QFutureWatcher<RenderDataPtr>(this);

        // Connect watcher to delete itself when finished
        connect(watcher, &QFutureWatcher<RenderDataPtr>::finished, watcher,
                &QObject::deleteLater);

        connect(watcher, &QFutureWatcher<RenderDataPtr>::finished, this,
        [this, watcher, progress, casePath, fileName, fullPath,
            timeFolders, timeFolder, tabData]() {
            if (!progress)
                return;

            progress->close();
            RenderDataPtr renderData = watcher->result();
            if (!renderData)
                return;

            auto* resultEditor = new ResultEditor(timeFolders, timeFolder,
                renderData, casePath, &m_vulkanInstance, this);
            resultEditor->applyTheme(m_graphicalTheme);
            connect(resultEditor, &ResultEditor::timeChanged, this,
                &MainWindow::updateResult);

            QString tabTitle =
                fileName + QString(" (result=%1)").arg(timeFolder);
            int tabIndex = m_tabWidget->addTab(resultEditor, tabTitle);

            m_tabWidget->setCurrentIndex(tabIndex);
            m_tabWidget->tabBar()->setTabData(tabIndex, fullPath);

            m_undoAction->setDisabled(true);
            m_redoAction->setDisabled(true);
            m_saveFileAction->setDisabled(true);

            m_tabMap.insert(tabTitle, tabData);
        });

        resultPath = resultPath + "/" + timeFolder;
        watcher->setFuture(QtConcurrent::run(
            &MainWindow::getResultData, this, caseName, resultPath));
    }
}

std::shared_ptr<RenderData> MainWindow::getMeshData(QString caseName,
        QString casePath, QString openFoamPath) {
    // Convert mesh to STL file
    QByteArray data;
    QString meshFile = caseName + "_tmp.stl";
    QString cmd = QString("cd %1; source %2/etc/bashrc && foamToSurface %3")
                      .arg(casePath, openFoamPath, meshFile);

    // Read new STL file
    QString output;
    if (m_systemMgr.getSystem(caseName)->launchShortUtility(cmd, output) == 0) {
        data = m_systemMgr.getSystem(caseName)->getFileContent(
            casePath + "/" + meshFile);
    }

    // Convert data to RenderData structure
    return std::make_shared<RenderData>(MeshReader::readMesh(caseName, data));
}

// Access result data
std::shared_ptr<RenderData> MainWindow::getResultData(QString caseName,
                                                      QString resultPath) {
    return std::make_shared<RenderData>(
        m_systemMgr.getSystem(caseName)->getResultData(resultPath));
}

// Save file content to server
void MainWindow::saveFile() {
    // Remove the " *" to get the map key
    QString rawTabText = m_tabWidget->tabText(m_tabWidget->currentIndex());
    QString fileName = rawTabText.remove(" *");

    // Look up data
    if (m_tabMap.contains(fileName)) {
        // Construct the remote path
        TabData tabData = m_tabMap[fileName];
        QString caseName = tabData.fullPath.split("/")[0];
        QString fullPath = m_systemMgr.getData(caseName).casePath + "/" +
                           tabData.fullPath + "/" + fileName;

        // Save data for text editor
        if (tabData.type == EditorType::TEXT) {
            TextEditor* editor =
                qobject_cast<TextEditor*>(m_tabWidget->currentWidget());
            if (editor) {
                bool save =
                    m_systemMgr.getSystem(caseName)->writeData(
                        editor->toPlainText().toUtf8(), fullPath);
                if (save) editor->document()->setModified(false);
            }
            return;
        }

        // Save data for model editor
        if (tabData.type == EditorType::SURFACE) {
            SurfaceEditor* editor =
                qobject_cast<SurfaceEditor*>(m_tabWidget->currentWidget());
            if (editor) {
                QString output;
                QFileInfo info(fullPath);
                QString path = info.path();
                QString fileName = info.fileName();

                // Delete patched file when saved
                if (editor->isSurfacePatched()) {
                    // Delete xyz.stl and rename xyz_patched.stl to xyz.stl
                    QString patchName =
                        info.completeBaseName() + "_patched." + info.suffix();
                    QString cmd =
                        QString("cd %1; rm %2; mv %3 %2; ").
                        arg(path, fileName, patchName);
                    if (m_systemMgr.getSystem(caseName)->
                        launchShortUtility(cmd, output) == 0) {
                        editor->setSurfaceChanged(false);
                        onDirtyStateChanged(false, editor);
                    }
                }

                // Check if patch names have changed
                std::vector<std::pair<std::string, std::string>> vec =
                    editor->getPatchChanges();
                if (!vec.empty()) {
                    // Replace old patch names with new patch names
                    QString cmd = QString("cd %1; sed -i ").arg(path);
                    for (const auto& change : vec) {
                        QString oldStr = QString::fromStdString(change.first);
                        QString newStr = QString::fromStdString(change.second);
                        cmd += QString("-e 's#%1#%2#g' ").arg(oldStr, newStr);
                    }
                    cmd += fileName;

                    // Perform text replacement
                    if (m_systemMgr.getSystem(caseName)->
                        launchShortUtility(cmd, output) == 0) {
                        onDirtyStateChanged(false, editor);
                    }
                }
            }
            return;
        }
    }
}

void MainWindow::deleteFile() {
    // Access node
    QVariant data = m_deleteAction->data();
    NodeData* node = data.value<NodeData*>();
    if (!node)
        return;

    // Construct path to delete
    QString caseName, filePath, casePath;
    if (node->fullPath.isEmpty()) {
        caseName = filePath;
        filePath = m_systemMgr.getData(caseName).casePath + "/" + caseName;
        casePath = caseName;
    } else {
        caseName = node->fullPath.split('/').first();
        filePath = m_systemMgr.getData(caseName).casePath + "/" +
                   node->fullPath + "/" +  node->name;
        casePath = node->fullPath;
    }

    // Delete file
    QStringList result = m_systemMgr.getSystem(caseName)->processPaths(filePath,
        PathOperationType::DELETE);
    m_deleteAction->setData(QVariant());
    m_navigator->removeNode(node);
}

// Set preferences
void MainWindow::launchPreferencesDialog() {
    // Create preferences dialog
    PreferencesDialog dlg(this);
    dlg.exec();

    // Get desired theme
    QString theme = dlg.getTheme();
    applyTheme(theme + ".json");
}

// Apply theme to editor content
void MainWindow::applyTheme(const QString& themeFile) {
    // Access files in the themes folder
    QString styleText;
    m_themeFile = themeFile;
    QFile file(m_configDir.filePath("themes/" + m_themeFile));
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject rootObj = doc.object();

            // Read stylesheet settings
            if (rootObj.contains("qt_stylesheet")) {
                styleText = rootObj["qt_stylesheet"].toString();
            } else {
                qWarning() << QString("'qt_stylesheet' object is missing"
                                      "or invalid in %1.").arg(m_themeFile);
            }

            // Read text editor settings
            if (rootObj.contains("text_editor")) {
                QJsonObject textObj = rootObj["text_editor"].toObject();

                // Extract base editor colors
                if (textObj.contains("background"))
                    m_textTheme.background =
                        QColor(textObj["background"].toString());
                if (textObj.contains("gutter_background"))
                    m_textTheme.gutterBackground =
                        QColor(textObj["gutter_background"].toString());
                if (textObj.contains("line_number_normal"))
                    m_textTheme.lineNumberNormal =
                        QColor(textObj["line_number_normal"].toString());
                if (textObj.contains("line_number_active"))
                    m_textTheme.lineNumberActive =
                        QColor(textObj["line_number_active"].toString());
                if (textObj.contains("current_line_highlight"))
                    m_textTheme.currentLineHighlight =
                        QColor(textObj["current_line_highlight"].toString());

                // Extract syntax highlighting rules
                if (textObj.contains("syntax")) {
                    QJsonObject syntaxObj = textObj["syntax"].toObject();

                    // Lambda helper to parse the syntax items
                    auto parseSyntaxItem =
                        [](const QJsonValue& val) -> SyntaxItem {
                        SyntaxItem item;
                        if (val.isObject()) {
                            QJsonObject obj = val.toObject();
                            if (obj.contains("color"))
                                item.color = (obj["color"].toString());
                            if (obj.contains("bold"))
                                item.bold = obj["bold"].toBool(false);
                            if (obj.contains("italic"))
                                item.italic = obj["italic"].toBool(false);
                        }
                        return item;
                    };

                    // Map the JSON objects to your struct fields
                    m_textTheme.syntaxConfig.keyword =
                        parseSyntaxItem(syntaxObj["keyword"]);
                    m_textTheme.syntaxConfig.number =
                        parseSyntaxItem(syntaxObj["number"]);
                    m_textTheme.syntaxConfig.stringItem =
                        parseSyntaxItem(syntaxObj["string"]);
                    m_textTheme.syntaxConfig.enumItem =
                        parseSyntaxItem(syntaxObj["enum"]);
                    m_textTheme.syntaxConfig.comment =
                        parseSyntaxItem(syntaxObj["comment"]);
                    m_textTheme.syntaxConfig.punctuation =
                        parseSyntaxItem(syntaxObj["punctuation"]);
                    m_textTheme.syntaxConfig.macro =
                        parseSyntaxItem(syntaxObj["macro"]);
                }
            } else {
                qWarning() << QString("'text_editor' object is missing"
                                      "or invalid in %1.").arg(m_themeFile);
            }

            // Read graphical editor settings
            if (rootObj.contains("graphical_editor")) {
                QJsonObject graphicalObj =
                    rootObj["graphical_editor"].toObject();

                // Extract graphical editor color
                if (graphicalObj.contains("viewport_clear")) {
                    m_graphicalTheme =
                        graphicalObj["viewport_clear"].toString();
                }
            }
        } else {
            qWarning() << QString("Failed to parse %1: "
                "Root not a JSON Object.").arg(m_themeFile);
        }
        file.close();
    } else {
        qWarning() << QString("Failed to open %1 at:" +
                              file.fileName()).arg(m_themeFile);
    }
    if (!styleText.isEmpty()) {
        QApplication* app = qobject_cast<QApplication*>(qApp);
        if (app)
            app->setStyleSheet(styleText);
    }

    // Apply theme to editors
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        QString tabName = m_tabWidget->tabText(i);
        qDebug() << tabName << ": " << static_cast<int>(m_tabMap[tabName].type);
        if (m_tabMap.contains(tabName)) {
            switch (m_tabMap[tabName].type) {
            case EditorType::TEXT: {
                TextEditor* textEditor =
                    qobject_cast<TextEditor*>(m_tabWidget->widget(i));
                textEditor->applyTheme(m_textTheme);
                break;
            }
            case EditorType::SURFACE: {
                SurfaceEditor* surfaceEditor =
                    qobject_cast<SurfaceEditor*>(m_tabWidget->widget(i));
                surfaceEditor->applyTheme(m_graphicalTheme);
                break;
            }
            case EditorType::MESH: {
                MeshEditor* meshEditor =
                    qobject_cast<MeshEditor*>(m_tabWidget->widget(i));
                meshEditor->applyTheme(m_graphicalTheme);
                break;
            }
            case EditorType::RESULT: {
                ResultEditor* resultEditor =
                    qobject_cast<ResultEditor*>(m_tabWidget->widget(i));
                resultEditor->applyTheme(m_graphicalTheme);
                break;
            }
            }
        }
    }
}

// Launch mesh configuration wizard
void MainWindow::launchMeshConfigurationWizard() {
    // Create the wizard
    auto* wizard = new MeshWizard(getSelectedCase(), m_systemMgr, this);
    wizard->setAttribute(Qt::WA_DeleteOnClose);

    // Connect the signal to your slot
    connect(wizard, &MeshWizard::createEditor, this, &MainWindow::createEditor);
    connect(wizard, &MeshWizard::updatePath, this, &MainWindow::updatePath);
    wizard->exec();
}

// Launch mesh execution dialog
void MainWindow::launchMeshExecutionDialog() {
    // Get path of currently-selected case
    QString caseName = getSelectedCase();
    CaseData caseData = m_systemMgr.getData(caseName);
    QString casePath = caseData.casePath + "/" + caseName;
    QString openFoamPath = caseData.openFoamPath;

    // Check if using Foundation release of OpenFOAM
    QString dirName = QDir(openFoamPath).dirName();
    const QRegularExpression foundationRegex("^openfoam\\d{2}$",
        QRegularExpression::CaseInsensitiveOption);
    bool isFoundation = foundationRegex.match(dirName).hasMatch();

    // Check if mesh configuration files are present
    QStringList meshConfigFiles;
    if (isFoundation) {
        meshConfigFiles = {casePath + "/system/blockMeshDict",
                            casePath + "/system/surfaceFeaturesDict",
                            casePath + "/system/snappyHexMeshDict"};
    } else {
        meshConfigFiles = {casePath + "/system/blockMeshDict",
                           casePath + "/system/surfaceFeatureExtractDict",
                           casePath + "/system/snappyHexMeshDict"};
    }
    QString meshConfigFileString = meshConfigFiles.join("\n");
    QStringList results = m_systemMgr.getSystem(caseName)->
        processPaths(meshConfigFileString, PathOperationType::CHECK);

    // Launch dialog if any config files are present
    if (results.contains("0")) {
        auto* dialog = new RunMeshDialog(getSelectedCase(), m_systemMgr,
                                         results, isFoundation, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);

        // Connect signal
        connect(dialog, &RunMeshDialog::requestRunMesh, this,
                &MainWindow::runMesh);
        dialog->exec();
    } else {
        QMessageBox::information(this, tr("Mesh Configuration Files Absent"),
            tr("The selected case doesn't have files for blockMesh, "
                "surface feature analysis, or snappyHexMesh."));
    }
}

// Run mesh utilities
void MainWindow::runMesh(const QString& caseName, bool runBlockMesh,
    bool runSurfaceFeature, bool runSnappyHexMesh,
    const QString& snappyCmd, int numCores) {
    // Get OpenFoam path
    CaseData caseData = m_systemMgr.getData(caseName);
    QString casePath = caseData.casePath + "/" + caseName;
    QString openFoamPath = caseData.openFoamPath;
    QString cmd, output;

    // Clear console
    m_console->clear();

    // Launch blockMesh
    if (runBlockMesh) {
        cmd = QString("cd %1; source %2/etc/bashrc; blockMesh").arg(
            casePath, openFoamPath);
        if (m_systemMgr.getSystem(caseName)->launchShortUtility(
                cmd, output) == 0) {
            log(output);
            if (!runSnappyHexMesh) {
                updatePath(caseName, "constant/polyMesh");
            }
        }
    }

    // Launch surfaceFeatures or surfaceFeatureExtract
    if (runSurfaceFeature) {

        // Determine type of installation
        QString dirName = QDir(openFoamPath).dirName();
        const QRegularExpression foundationRegex("^openfoam\\d{2}$",
            QRegularExpression::CaseInsensitiveOption);
        bool isFoundation = foundationRegex.match(dirName).hasMatch();
        QString utility =
            (isFoundation) ? "surfaceFeatures" : "surfaceFeatureExtract";

        // Run the appropriate command
        cmd = QString("cd %1; source %2/etc/bashrc; %3").
              arg(casePath, openFoamPath, utility);
        if (m_systemMgr.getSystem(caseName)->launchShortUtility(
                cmd, output) == 0) {
            log(output);
        }
    }

    // Launch snappyHexMesh
    if (runSnappyHexMesh) {
        // Configure multicore operation
        if (numCores > 1) {
            // Create surfacePatchDict
            QString dictText =
                Utils::createDecomposeParDict(openFoamPath, numCores);
            m_systemMgr.getSystem(caseName)->writeData(dictText.toUtf8(),
                casePath + "/system/decomposeParDict");
        }

        // Create command
        cmd = QString("cd %1; source %2/etc/bashrc; " + snappyCmd).
              arg(casePath, openFoamPath);
        m_systemMgr.getSystem(caseName)->launchLongUtility(cmd,
            caseName, UtilityType::MESH);
    }
}

void MainWindow::viewMesh() {
    // Access node
    QVariant data = m_viewMeshAction->data();
    NodeData* node = data.value<NodeData*>();
    if (!node)
        return;

    // Create editor for mesh
    createEditor(EditorType::MESH, node->name, node->fullPath, true);

    // Clear data
    m_viewMeshAction->setData(QVariant());
}

// Launch solver configuration wizard
void MainWindow::launchSolverConfigurationWizard() {
    // Create the wizard
    auto* wizard = new SolverWizard(getSelectedCase(), m_systemMgr,
        m_solverFamilies, m_turbulenceModels, m_transportProperties,
        m_fieldData, m_boundaryConditions, this);
    wizard->setAttribute(Qt::WA_DeleteOnClose);

    // Parse case files
    if (wizard->parseFiles()) {
        connect(wizard, &SolverWizard::createEditor, this,
            &MainWindow::createEditor);
        connect(wizard, &SolverWizard::updatePath, this,
            &MainWindow::updatePath);
        wizard->exec();
    } else {
        wizard->deleteLater();
    }
}

// Launch solver execution wizard
void MainWindow::launchSolverExecutionDialog() {
    // Get path of currently-selected case
    QString caseName = getSelectedCase();
    CaseData caseData = m_systemMgr.getData(caseName);
    QString casePath = caseData.casePath + "/" + caseName;
    QString openFoamPath = caseData.openFoamPath;

    // Check if using Foundation release of OpenFOAM
    QString dirName = QDir(openFoamPath).dirName();
    const QRegularExpression foundationRegex("^openfoam\\d{2}$",
        QRegularExpression::CaseInsensitiveOption);
    bool isFoundation = foundationRegex.match(dirName).hasMatch();

    // Create dialog
    auto* dialog = new RunSolverDialog(caseName, isFoundation,
                                       m_systemMgr, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    // Connect signal
    connect(dialog, &RunSolverDialog::requestRunSolver, this,
            &MainWindow::runSolver);
    dialog->exec();
}

// Run the simulation
void MainWindow::runSolver(const QString& caseName, const QString& cmd) {
    // Get OpenFoam path
    CaseData caseData = m_systemMgr.getData(caseName);
    QString casePath = caseData.casePath + "/" + caseName;
    QString openFoamPath = caseData.openFoamPath;

    // Clear console
    m_console->clear();

    // Execute command
    QString command =
        QString("cd %1; source %2/etc/bashrc; " + cmd).
            arg(casePath, openFoamPath);
    m_systemMgr.getSystem(caseName)->launchLongUtility(
        command, caseName, UtilityType::SOLVER);
}

// Stop solver execution
void MainWindow::stopSolver() {}

void MainWindow::viewResult() {
    // Access node
    QVariant data = m_viewResultAction->data();
    NodeData* node = data.value<NodeData*>();
    if (!node)
        return;

    // Create editor for mesh
    createEditor(EditorType::RESULT, node->name, node->fullPath, true);

    // Clear data
    m_viewResultAction->setData(QVariant());
}

void MainWindow::updateResult(const QString& casePath,
                                const QString& timeFolder) {
    // Access data in time folder
    QString caseName = QFileInfo(casePath).fileName();
    QString resultPath = casePath + "/postProcessing/surfaces/" + timeFolder;
    RenderData renderData =
        m_systemMgr.getSystem(caseName)->getResultData(resultPath);
    std::shared_ptr<RenderData> newData =
        std::make_shared<RenderData>(std::move(renderData));
    if (newData) {
        // Access current editor
        ResultEditor* resultEditor =
            qobject_cast<ResultEditor*>(m_tabWidget->currentWidget());
        resultEditor->updateResult(newData);

        // Update tab text
        QString tabText = m_tabWidget->tabText(m_tabWidget->currentIndex());
        tabText.replace(QRegularExpression(R"(\(result@\d+\)$)"),
            QString("(result@%1)").arg(timeFolder));
        m_tabWidget->setTabText(m_tabWidget->currentIndex(), tabText);
    }
}
