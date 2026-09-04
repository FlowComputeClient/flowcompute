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
#include <QClipboard>
#include <QtConcurrent>
#include <QFileDialog>
#include <QFuture>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QMessageBox>
#include <QProgressDialog>

#include <memory>
#include <utility>

#include "dialogs/run_mesh/run_mesh_dialog.h"
#include "dialogs/run_solver/run_solver_dialog.h"
#include "dialogs/selection/selection_dialog.h"
#include "editors/graphical/surface/surface_editor.h"
#include "editors/graphical/mesh/mesh_editor.h"
// #include "editors/graphical/result/result_editor.h"
#include "geometry/stl/stl_reader.h"
#include "geometry/obj/obj_reader.h"
#include "parser/boundary.h"
#include "parser/decompose_par_dict.h"
#include "wizards/mesh/wizard_mesh.h"
#include "wizards/new_case/wizard_new_case.h"
#include "wizards/open_case/wizard_open_case.h"
#include "wizards/post_processing/wizard_postprocessing.h"
#include "wizards/solver/wizard_solver.h"

// Launch new case wizard
void MainWindow::newCase() {
    NewCaseWizard wizard(m_systemMgr, this);

    // Connect case-creation signal
    connect(&wizard, &NewCaseWizard::requestCaseCreation,
            this, &MainWindow::createCase);

    // Open it modally
    wizard.exec();
}

// Open an existing case
void MainWindow::openCase() {
    // Define case location options
    QStringList caseLocations;
#if defined(Q_OS_WIN)
    caseLocations.append("Windows Subsystem for Linux (WSL)");
#elif defined(Q_OS_LINUX)
    caseLocations.append("Local");
#endif
    caseLocations.append("Remote");

    // Location selection dialog
    SelectionDialog selectionDialog(tr("Open Case"),
        tr("Select the location of the case:"), caseLocations, this);

    if (selectionDialog.exec() != QDialog::Accepted)
        return;

    int selection = selectionDialog.getSelectedIndex();
    if (selection < 0)
        return;

    // Convert index to enumerated type
    TargetType targetType = TargetType::REMOTE_LINUX;
    if (selection == 0) {
#if defined(Q_OS_WIN)
        targetType = TargetType::LOCAL_WINDOWS;
#elif defined(Q_OS_LINUX)
        targetType = TargetType::LOCAL_LINUX;
#endif
    }

    // Get OpenFOAM path
    QString openFoamPath = checkOpenFoam(static_cast<int>(targetType));

    // Create wizard to open the new case
    OpenCaseWizard wizard(targetType, m_systemMgr, openFoamPath, this);

    // Connect the wizard's signals
    connect(&wizard, &OpenCaseWizard::requestCaseCreation,
            this, &MainWindow::createCase);
    connect(&wizard, &OpenCaseWizard::logMessage,
            this, &MainWindow::log);

    // Launch the wizard
    wizard.exec();
}

// Respond when a file/folder is renamed
void MainWindow::renameFile(const QString& filePath, const QString& newName) {
    // Get filename
    int lastSlash = filePath.lastIndexOf('/');
    bool isCase = lastSlash == -1;

    // Update the caseMap if necessary   
    QString newPath;
    if (isCase) {
        m_systemMgr.renameCase(filePath, newName);
        newPath = newName;
    } else {
        newPath = filePath.left(lastSlash) + "/" + newName;
    }

    // Access QSettings
    QSettings settings;
    settings.beginGroup("Tabs");
    QStringList tabOrder = settings.value("tabOrder").toStringList();

    // Iterate through tabs
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        const QString tabPath = m_tabWidget->tabBar()->tabData(i).toString();

        // Match tabs belonging to the renamed case
        if (tabPath.startsWith(filePath + "/") || tabPath == filePath) {

            // Update path in tab widget
            QString newTabPath = newPath + tabPath.mid(filePath.size());
            m_tabWidget->tabBar()->setTabData(i, newTabPath);

            // Update tab that displays the case name and (mesh) or (result)
            QString tabText = m_tabWidget->tabText(i);
            if (isCase && (tabText.startsWith(filePath + " ("))) {
                QString newTabText = newName + tabText.mid(filePath.size());
                m_tabWidget->setTabText(i, newTabText);
            }

            // Update tab for a renamed file
            if (!isCase && (tabPath == filePath)) {
                m_tabWidget->setTabText(i, newName);
            }

            // Update tabMap
            auto it = m_tabMap.find(tabPath);
            if (it != m_tabMap.end()) {
                TabData tabData = it.value();
                m_tabMap.erase(it);
                m_tabMap.insert(newTabPath, tabData);

                // Write the data to the new subgroup
                settings.beginGroup(newTabPath);
                settings.setValue("type", static_cast<int>(tabData.type));
                settings.endGroup();
            }

            // Update the order list in memory
            int orderIndex = tabOrder.indexOf(tabPath);
            if (orderIndex != -1) {
                tabOrder.replace(orderIndex, newTabPath);
            }

            // Remove the old subgroup
            settings.remove(tabPath);
        }
    }

    // Save the tab order list to disk
    if (tabOrder.isEmpty()) {
        settings.remove("tabOrder");
    } else {
        settings.setValue("tabOrder", tabOrder);
    }
    settings.endGroup();

    // Update flags if needed
    if (!isCase) {
        m_systemMgr.updateFlags(filePath.split("/")[0]);
    }
}

// Respond when a file/folder is removed
void MainWindow::removeFile(const QString& filePath, bool isCase) {
    // Destroy tabs containing files in the deleted case
    for (int i = m_tabWidget->count() - 1; i >= 0; --i) {
        QString tabPath = m_tabWidget->tabBar()->tabData(i).toString();
        if (tabPath.startsWith(filePath + "/") || tabPath == filePath) {
            // Tells the tabwidget to destroy tab - updates settings
            m_tabWidget->destroyTab(i, true);
        }
    }

    // Remove case from system manager and QSettings
    if (isCase) {
        m_systemMgr.removeCase(filePath);
    } else {
        QString caseName = filePath.split("/")[0];
        m_systemMgr.updateFlags(caseName);
    }
}

void MainWindow::cutPasteFile(const QString& oldPath, const QString& newPath) {
    // Access QSettings
    QSettings settings;
    settings.beginGroup("Tabs");
    QStringList tabOrder = settings.value("tabOrder").toStringList();

    // Iterate through tabs
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        const QString tabPath = m_tabWidget->tabBar()->tabData(i).toString();

        // Match tabs belonging to the old path
        if (tabPath.startsWith(oldPath + "/") || tabPath == oldPath) {

            // Update path in tab widget
            QString newTabPath = newPath + tabPath.mid(oldPath.size());
            m_tabWidget->tabBar()->setTabData(i, newTabPath);

            // Update tabMap
            auto it = m_tabMap.find(tabPath);
            if (it != m_tabMap.end()) {
                TabData tabData = it.value();
                m_tabMap.erase(it);
                m_tabMap.insert(newTabPath, tabData);

                // Write the data to the new subgroup
                settings.beginGroup(newTabPath);
                settings.setValue("type", static_cast<int>(tabData.type));
                settings.endGroup();
            }

            // Update the order list in memory
            int orderIndex = tabOrder.indexOf(tabPath);
            if (orderIndex != -1) {
                tabOrder.replace(orderIndex, newTabPath);
            }

            // Remove the old subgroup
            settings.remove(tabPath);
        }
    }

    // Save the tab order list to disk
    if (tabOrder.isEmpty()) {
        settings.remove("tabOrder");
    } else {
        settings.setValue("tabOrder", tabOrder);
    }
    settings.endGroup();

    // Update flags for the old and new cases
    m_systemMgr.updateFlags(oldPath.split("/")[0]);
    m_systemMgr.updateFlags(newPath.split("/")[0]);
}

QString MainWindow::checkOpenFoam(int targetId) {
    // Determine OpenFOAM installation
    QStringList ofList = m_systemMgr.getSystem(targetId)->findOpenFoam();
    if(ofList.empty()) {
        QMessageBox::critical(this, tr("Missing OpenFOAM"),
                              tr("No OpenFOAM installations detected..."));
        return "";
    } else if (ofList.size() > 1) {
        SelectionDialog dlg(tr("Multiple OpenFOAM Installations Detected"),
                tr("Select one of the following:"), ofList, this);
        dlg.exec();
        return dlg.getSelectedItem();
    } else {
        return ofList[0];
    }
}

// Create new case folder
void MainWindow::createCase(const QString& caseName, const QString& casePath,
    const QStringList& caseFiles, int targetId, const QString& openFoamPath,
    CaseFlags flag, const QString& userName, const QString& hostName, int port)
    {
    // Add case to map
    m_systemMgr.addCase(caseName, CaseData{casePath, caseFiles, targetId,
        openFoamPath, flag, userName, hostName, port});

    // Update flags
    if (flag == CaseFlag::NotChecked) {
        flag = m_systemMgr.updateFlags(caseName, casePath);
    }

    // Update utility map if necessary
    if (!m_utilMap.contains(openFoamPath)) {
        QString path = casePath + "/" + caseName + "/";
        m_utilMap[openFoamPath] = checkUtilities(path, m_utilities);
    }

    // Display case in navigator
    m_navigator->addCase(caseName, caseFiles);
    m_navigator->expandCase(caseName);

    // Update QSettings
    QSettings settings;
    settings.beginGroup("Cases");

    // Get the order
    QStringList caseOrder = settings.value("caseOrder").toStringList();
    if (!caseOrder.contains(caseName)) {
        caseOrder.append(caseName);
        settings.setValue("caseOrder", caseOrder);
    }

    // Write the new case data into a new group
    settings.beginGroup(caseName);
    settings.setValue("casePath", casePath);
    settings.setValue("targetSystemId", targetId);
    settings.setValue("openFoamPath", openFoamPath);

    if (targetId == static_cast<int>(TargetType::REMOTE_LINUX)) {
        settings.setValue("userName", userName);
        settings.setValue("hostName", hostName);
        settings.setValue("port", port);
    }

    settings.endGroup();
    settings.endGroup();
}

// Respond to changing tab
void MainWindow::tabChanged(int index) {
    if (m_currentEditor) {
        m_currentEditor->disconnect(m_cutAction);
        m_currentEditor->disconnect(m_copyAction);
        m_currentEditor->document()->disconnect(m_undoAction);
        m_currentEditor->document()->disconnect(m_redoAction);
        m_currentEditor = nullptr;
    }

    // Default states for non-text tabs
    m_undoAction->setEnabled(false);
    m_redoAction->setEnabled(false);
    m_cutAction->setEnabled(false);
    m_copyAction->setEnabled(false);

    if (index < 0)
        return;

    QString tabPath = m_tabWidget->tabBar()->tabData(index).toString();
    if (!m_tabMap.contains(tabPath))
        return;

    QWidget* currentWidget = m_tabWidget->widget(index);
    if (!currentWidget) return;

    // Perform operations based on the type
    if (TextWidget* textWidget = qobject_cast<TextWidget*>(currentWidget)) {
        m_currentEditor = textWidget;

        // Access statistics
        QString caseName = tabPath.split("/")[0];
        QString fullPath =
            m_systemMgr.getData(caseName).casePath + "/" + tabPath;
        auto newStats = m_systemMgr.getSystem(caseName)->getFileStats(fullPath);

        // Compare new and old statistics
        auto oldStats = m_tabMap[tabPath].stats;
        if (newStats.has_value()) {
            if (oldStats.has_value()) {
                if ((newStats->mtime > oldStats->mtime) ||
                    (newStats->size != oldStats->size))
                    textWidget->showBanner();
            } else {
                m_tabMap[tabPath].stats = newStats;
            }
        }

        // Sync UI state for the text editor
        m_undoAction->setEnabled(textWidget->document()->isUndoAvailable());
        m_redoAction->setEnabled(textWidget->document()->isRedoAvailable());

        bool hasSelection = textWidget->editor()->textCursor().hasSelection();
        m_cutAction->setEnabled(hasSelection);
        m_copyAction->setEnabled(hasSelection);

        // Connect state-change signals
        connect(textWidget->document(), &QTextDocument::undoAvailable,
                m_undoAction, &QAction::setEnabled);
        connect(textWidget->document(), &QTextDocument::redoAvailable,
                m_redoAction, &QAction::setEnabled);

        connect(textWidget->editor(), &TextEditor::copyAvailable,
                m_cutAction, &QAction::setEnabled);
        connect(textWidget->editor(), &TextEditor::copyAvailable,
                m_copyAction, &QAction::setEnabled);
    }
}

// Respond when a tab is closed
void MainWindow::tabClosed(const QString& tabPath) {
    // Remove tab from tab map
    m_tabMap.remove(tabPath);

    // Update settings
    QSettings settings;
    settings.beginGroup("Tabs");

    // Remove tab from order list
    QStringList tabOrder = settings.value("tabOrder").toStringList();
    if (tabOrder.removeOne(tabPath)) {
        if (tabOrder.isEmpty()) {
            settings.remove("tabOrder");
        } else {
            settings.setValue("tabOrder", tabOrder);
        }
    }

    // Remove the tab subgroup
    settings.remove(tabPath);
    settings.endGroup();
}

// Respond when the user presses the Reload button to update editor
void MainWindow::updateTab() {
    // Determine the target widget
    TextWidget* targetWidget = qobject_cast<TextWidget*>(sender());

    // Default to the active tab
    if (!targetWidget)
        targetWidget = qobject_cast<TextWidget*>(m_tabWidget->currentWidget());
    if (!targetWidget)
        return;

    // Prevent silent overwriting of local changes
    if (targetWidget->editor()->document()->isModified()) {
        QMessageBox::StandardButton reply = QMessageBox::warning(this,
             tr("Unsaved Changes"),
             tr("This file has unsaved local changes. Reloading will discard"
                " them.\n\nDo you want to proceed?"),
             QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            return;
        }
    }

    // Find the widget in the tab map
    int tabIndex = m_tabWidget->indexOf(targetWidget);
    if (tabIndex == -1) {
        return;
    }

    // Extract the fullPath safely
    QString fullPath = m_tabWidget->tabBar()->tabData(tabIndex).toString();
    QString caseName = fullPath.split("/")[0];

    // Reconstruct the correct server path
    CaseData caseData = m_systemMgr.getData(caseName);
    QString serverPath = caseData.casePath + "/" + fullPath;

    // Fetch data and update UI
    auto newData =
        m_systemMgr.getSystem(caseName)->getFileContentAndStats(serverPath);
    if (newData.has_value()) {
        m_tabMap[fullPath].stats = newData->stats;
        targetWidget->editor()->setTextData(newData->content);

        // Reset modified state since it now matches the server perfectly
        targetWidget->editor()->document()->setModified(false);
        targetWidget->hideBanner();
    } else {
        log(tr("Failed to reload file: %1").arg(fullPath));
    }
}

// Update the QSettings with tab data
void updateTabSettings(const QString& tabPath, const TabData& tabData) {
    QSettings settings;
    settings.beginGroup("Tabs");

    // Update the tab list
    QStringList tabOrder = settings.value("tabOrder").toStringList();
    if (!tabOrder.contains(tabPath)) {
        tabOrder.append(tabPath);
        settings.setValue("tabOrder", tabOrder);
    }

    // Write tab data
    settings.beginGroup(tabPath);
    settings.setValue("type", static_cast<int>(tabData.type));
    settings.endGroup();
    settings.endGroup();
}

// Check if there's already an editor open
bool MainWindow::checkExistingEditor(const QString& fullPath) {
    QString tabPath;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        tabPath = m_tabWidget->tabBar()->tabData(i).toString();
        if (tabPath == fullPath) {
            m_tabWidget->setCurrentIndex(i);
            return true;
        }
    }
    return false;
}

// Create text editor
void MainWindow::createTextEditor(const QString& fileName,
                                  const QString& fullPath, bool logMessage) {
    // Check existing editor
    if (checkExistingEditor(fullPath))
        return;

    // Get case data
    QString caseName = fullPath.split('/').first();
    CaseData caseData = m_systemMgr.getData(caseName);
    QString casePath = caseData.casePath + "/" + caseName;
    QString path = caseData.casePath + "/" + fullPath;

    // Access data - remove tab if file isn't present
    std::optional<FileDataAndStats> data =
        m_systemMgr.getSystem(caseName)->getFileContentAndStats(path);
    if (!data) {
        log(tr("Failed to load file: %1").arg(fullPath));
        tabClosed(fullPath);
        return;
    }

    // Update tab map and settings
    TabData tabData;
    tabData.type = EditorType::TEXT;
    tabData.stats = data.value().stats;
    m_tabMap.insert(fullPath, tabData);
    updateTabSettings(fullPath, tabData);

    // Update log
    if (logMessage)
        m_console->appendPlainText(tr("Opening %1\n").arg(path));

    // Create editor
    TextWidget* textWidget = new TextWidget(this);
    textWidget->editor()->setFont(m_font);
    textWidget->editor()->setTextData(data.value().content);
    textWidget->editor()->applyTheme(m_textTheme);

    // Update tab widget
    int tabIndex = m_tabWidget->addTab(textWidget, fileName);
    m_tabWidget->setCurrentIndex(tabIndex);
    m_tabWidget->tabBar()->setTabData(tabIndex, fullPath);

    connect(textWidget, &TextWidget::reloadRequested, this,
            &MainWindow::updateTab);
    connect(textWidget->editor(), &TextEditor::dirtyStateChanged, this,
        [this, textWidget](bool isDirty) {
            onDirtyStateChanged(isDirty, textWidget);
        });
    connect(this, &MainWindow::textThemeChanged,
            textWidget->editor(), &TextEditor::applyTheme);
}

// Open editor to display surfaces
void MainWindow::createSurfaceEditor(const QString& fileName,
                                  const QString& fullPath, bool logMessage) {
    // Check existing editor
    if (checkExistingEditor(fileName))
        return;

    // Get case data
    QString caseName = fullPath.split('/').first();
    CaseData caseData = m_systemMgr.getData(caseName);
    QString casePath = caseData.casePath + "/" + caseName;
    QString path = caseData.casePath + "/" + fullPath;
    QString openFoamPath = caseData.openFoamPath;

    // Read model data
    bool isBinary = false;
    RenderData model;

    // Access data - remove tab if file isn't present
    std::optional<QByteArray> data =
        m_systemMgr.getSystem(caseName)->getFileContent(path);
    if (!data) {
        log(tr("Failed to load file: %1").arg(fullPath));
        tabClosed(fullPath);
        return;
    }

    // Display warning if geometry file is empty
    if (data.value().isEmpty()) {
        QMessageBox::warning(this, tr("Empty Geometry"), tr("The file '%1' "
            "contains no geometry data and cannot be rendered.").arg(fileName));
        log(tr("Failed to open empty geometry file: %1").arg(fullPath));
        return;
    }

    // Open geometry file
    if (fileName.endsWith(".stl", Qt::CaseInsensitive)) {
        std::pair<RenderData, bool> res =
            StlReader::readStlFile(fileName, data.value());
        model = res.first;
        isBinary = res.second;
    } else if (fileName.endsWith(".obj", Qt::CaseInsensitive)) {
        model = ObjReader::readObjFile(fileName, data.value());
    }
    std::shared_ptr<RenderData> modelData =
        std::make_shared<RenderData>(std::move(model));

    // Update tab map and settings
    TabData tabData;
    tabData.type = EditorType::SURFACE;
    m_tabMap.insert(fullPath, tabData);
    updateTabSettings(fullPath, tabData);

    // Update log
    if (logMessage)
        m_console->appendPlainText(tr("Reading %1\n").arg(path));

    // Create new surface editor
    SurfaceEditor* surfaceEditor = new SurfaceEditor(m_systemMgr, caseName,
        path, modelData, &m_vulkanInstance, isBinary, this);
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
    connect(this, &MainWindow::graphicalThemeChanged,
            surfaceEditor, &SurfaceEditor::applyTheme);
}

// Open editor to display meshes
void MainWindow::createMeshEditor(const QString& caseName, bool logMessage) {
    // Check existing editor
    QString fullPath = caseName + "/__mesh__";
    if (checkExistingEditor(fullPath))
        return;

    // Get case data
    CaseData caseData = m_systemMgr.getData(caseName);
    QString casePath = caseData.casePath + "/" + caseName;
    QString openFoamPath = caseData.openFoamPath;

    // Update tab map and settings
    TabData tabData;
    tabData.type = EditorType::MESH;
    m_tabMap.insert(fullPath, tabData);
    updateTabSettings(fullPath, tabData);

    // Update log
    if (logMessage)
        m_console->appendPlainText(tr("Opening %1 (mesh)\n").arg(caseName));

    // Create progress dialog
    auto* progress =
        new QProgressDialog("Loading mesh...", QString(), 0, 0, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumWidth(300);
    progress->setAttribute(Qt::WA_DeleteOnClose);
    progress->show();

    // Create future watcher
    using RenderDataPtr = std::shared_ptr<RenderData>;
    auto* watcher = new QFutureWatcher<RenderDataPtr>(this);
    connect(watcher, &QFutureWatcher<RenderDataPtr>::finished, this,
    [this, watcher, progress, casePath, caseName, fullPath]() {
        if (!progress)
            return;

        // Delete the watcher
        watcher->deleteLater();

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
        connect(this, &MainWindow::graphicalThemeChanged,
                meshEditor, &MeshEditor::applyTheme);

        QString tabTitle = caseName + " (mesh)";
        int tabIndex = m_tabWidget->addTab(meshEditor, tabTitle);
        m_tabWidget->setCurrentIndex(tabIndex);
        m_tabWidget->tabBar()->setTabData(tabIndex, fullPath);

        m_undoAction->setDisabled(true);
        m_redoAction->setDisabled(true);
        m_saveFileAction->setDisabled(true);
    });

    // Set future to get mesh data
    QFuture<std::shared_ptr<RenderData>> future =
        QtConcurrent::run([this, caseName, casePath]() {
        return std::make_shared<RenderData>(
            m_systemMgr.getSystem(caseName)->getMeshData(casePath)
        );
    });
    watcher->setFuture(future);
}

// Open editor to display results
void MainWindow::createResultEditor(const QString& caseName, bool logMessage) {
    // Check existing editor
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->tabBar()->tabData(i).toString() ==
            caseName + " (results)") {
            m_tabWidget->setCurrentIndex(i);
            return;
        }
    }

    // Get case data
    CaseData caseData = m_systemMgr.getData(caseName);
    QString casePath = caseData.casePath + "/" + caseName;
    QString openFoamPath = caseData.openFoamPath;

    // Update tab map and settings
    TabData tabData;
    tabData.type = EditorType::RESULT;
    m_tabMap.insert(caseName + " (result)", tabData);
    updateTabSettings(caseName + " (result)", tabData);

    // Update log
    if (logMessage)
        m_console->appendPlainText(tr("Opening %1 (result)\n").arg(caseName));

    // Read time folders and field files
    auto [timeFolders, fieldFiles] =
        m_systemMgr.getSystem(caseName)->getTimesAndFields(casePath);
    if (timeFolders.isEmpty()) {
        qWarning() << "Couldn't find any time folders";
        return;
    }
    if (fieldFiles.isEmpty()) {
        qWarning() << "Couldn't find any field files";
        return;
    }

    // Create dialog to select field
    SelectionDialog selectionDialog(tr("Field Selection"),
        tr("Select one of the following fields to be displayed:"),
        fieldFiles, this);
    if (selectionDialog.exec() != QDialog::Accepted) {
        return;
    }
    QString selectedField = selectionDialog.getSelectedItem();
    QString timeFolder;
    /*
    if (fullPath.isEmpty()) {
        timeFolder = timeFolders.last();
    } else {
        timeFolder = fullPath;
    }
    */

    // Create progress dialog
    auto* progress =
        new QProgressDialog("Loading mesh...", QString(), 0, 0, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumWidth(300);
    progress->setAttribute(Qt::WA_DeleteOnClose);
    progress->show();

    // Create future watcher
    using RenderDataPtr = std::shared_ptr<RenderData>;
    auto* watcher = new QFutureWatcher<RenderDataPtr>(this);
    connect(watcher, &QFutureWatcher<RenderDataPtr>::finished, this,
    [this, watcher, progress, casePath, caseName, timeFolders,
            timeFolder, tabData, selectedField]() {
        if (!progress)
            return;

        // Delete the watcher
        watcher->deleteLater();

        progress->close();
        RenderDataPtr renderData = watcher->result();
        if (!renderData)
            return;

        /*
        auto* resultEditor = new ResultEditor(timeFolders, timeFolder,
            renderData, casePath, &m_vulkanInstance, this);
        resultEditor->applyTheme(m_graphicalTheme);
        connect(resultEditor, &ResultEditor::timeChanged, this,
            &MainWindow::updateResult);

        QString tabTitle =
            caseName + QString(" (%1 @ %2)").arg(selectedField, timeFolder);
        int tabIndex = m_tabWidget->addTab(resultEditor, tabTitle);

        m_tabWidget->setCurrentIndex(tabIndex);
        m_tabWidget->tabBar()->setTabData(tabIndex, fullPath);

        m_undoAction->setDisabled(true);
        m_redoAction->setDisabled(true);
        m_saveFileAction->setDisabled(true);

        m_tabMap.insert(tabTitle, tabData);
        */
    });
    QFuture<std::shared_ptr<RenderData>> future =
        QtConcurrent::run([this, caseName, casePath, selectedField,
                                    timeFolder]() {
            return getResultData(caseName, casePath,
                             selectedField, timeFolder);
        });
    watcher->setFuture(future);
}

// Save file content to server
void MainWindow::saveFile() {
    int index = m_tabWidget->currentIndex();
    QString tabId = m_tabWidget->tabBar()->tabData(index).toString();

    // Look up data
    if (m_tabMap.contains(tabId)) {
        // Construct the remote path
        TabData tabData = m_tabMap[tabId];
        QString caseName = tabId.split("/")[0];
        QString fullPath =
            m_systemMgr.getData(caseName).casePath + "/" + tabId;

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
    }
}

// Apply theme to editor content
void MainWindow::applyTheme(const QString& themeFile) {
    // Access files in the themes folder
    m_themeFile = themeFile;
    QFile file(m_configDir.filePath("themes/" + m_themeFile));
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject rootObj = doc.object();

            // Associate style keys with string placeholders
            const QMap<QString, QString> styleMap = {
                {"background", "%BACKGROUND%"}, {"pane", "%PANE%"},
                {"border", "%BORDER%"}, {"text", "%TEXT%"},
                {"dialog", "%DIALOG%"}, {"tabbar", "%TAB_BAR%"},
                {"tab", "%TAB%"}, {"selectedTab", "%SELECTED_TAB%"},
                {"selection", "%SELECTION%"}, {"tabText", "%TAB_TEXT%"},
                {"selectedText", "%SELECTED_TEXT%"}, {"button", "%BUTTON%"},
                {"disabledText", "%DISABLED_TEXT%"}, {"input", "%INPUT%"},
                {"buttonHover", "%BUTTON_HOVER%"}, {"highlight", "%HIGHLIGHT%"},
                {"buttonPressed", "%BUTTON_PRESSED%"}
            };

            if (rootObj.contains("widgetPalette")) {
                QJsonObject styleObj = rootObj["widgetPalette"].toObject();

                // Associate placeholders with theme colors
                QMap<QString, QString> placeholders;
                for (auto it = styleMap.constBegin();
                     it != styleMap.constEnd(); ++it) {
                    const QString& jsonKey = it.key();
                    const QString& placeholderToken = it.value();

                    // Extract color, default to magenta
                    placeholders[placeholderToken] =
                        styleObj.value(jsonKey).toString("#FF00FF");
                }

                // Read style string from resource
                QFile file(":/styles/style_template.qss");
                QString styleText;
                if (file.open(QFile::ReadOnly | QFile::Text)) {
                    QTextStream in(&file);
                    styleText = in.readAll();
                    file.close();
                } else {
                    qWarning() << "Failed to open style_template.qss resource.";
                }

                // Use colors to update style text
                for (auto it = placeholders.constBegin();
                    it != placeholders.constEnd(); ++it) {
                    styleText.replace(it.key(), it.value());
                }

                // Apply updated style
                QApplication* app = qobject_cast<QApplication*>(qApp);
                if (app)
                    app->setStyleSheet(styleText);
            } else {
                qWarning() << QString("'widgetPalette' object is missing"
                                      "or invalid in %1.").arg(m_themeFile);
            }

            // Read text editor settings
            if (rootObj.contains("textEditor")) {
                QJsonObject textObj = rootObj["textEditor"].toObject();

                // Extract base editor colors
                if (textObj.contains("background"))
                    m_textTheme.background =
                        QColor(textObj["background"].toString());
                if (textObj.contains("gutterBackground"))
                    m_textTheme.gutterBackground =
                        QColor(textObj["gutterBackground"].toString());
                if (textObj.contains("lineNumberNormal"))
                    m_textTheme.lineNumberNormal =
                        QColor(textObj["lineNumberNormal"].toString());
                if (textObj.contains("lineNumberActive"))
                    m_textTheme.lineNumberActive =
                        QColor(textObj["lineNumberActive"].toString());
                if (textObj.contains("currentLineHighlight"))
                    m_textTheme.currentLineHighlight =
                        QColor(textObj["currentLineHighlight"].toString());

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
                qWarning() << QString("'textEditor' object is missing"
                                      "or invalid in %1.").arg(m_themeFile);
            }

            // Read graphical editor settings
            if (rootObj.contains("graphicalEditor")) {
                QJsonObject graphicalObj =
                    rootObj["graphicalEditor"].toObject();

                // Extract graphical editor color
                if (graphicalObj.contains("viewportClear")) {
                    m_graphicalTheme =
                        graphicalObj["viewportClear"].toString();
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

    // Apply theme to editors
    emit textThemeChanged(m_textTheme);
    emit graphicalThemeChanged(m_graphicalTheme);
}

// Launch mesh configuration wizard
void MainWindow::launchMeshConfigurationWizard() {
    // Check server connection
    QString caseName = getSelectedCase();
    auto system = m_systemMgr.getSystem(caseName);
    if (system == nullptr) {
        QMessageBox::critical(this, tr("Server access failure"),
                              tr("Couldn't reach server."));
        return;
    }

    // Create the wizard
    MeshWizard wizard(caseName, m_systemMgr, this);

    // Connect signals
    connect(&wizard, &MeshWizard::createTextEditor, this,
            &MainWindow::createTextEditor);
    connect(&wizard, &MeshWizard::updatePath, this, &MainWindow::updatePath);

    // Launch the wizard
    wizard.exec();
}

// Launch mesh execution dialog
void MainWindow::launchMeshExecutionDialog() {
    // Get path of currently-selected case
    QString caseName = getSelectedCase();
    CaseData caseData = m_systemMgr.getData(caseName);
    QString casePath = caseData.casePath + "/" + caseName;
    QString openFoamPath = caseData.openFoamPath;

    // Check server connection
    auto system = m_systemMgr.getSystem(caseName);
    if (system == nullptr) {
        QMessageBox::critical(this, tr("Server access failure"),
                              tr("Couldn't reach server."));
        return;
    }

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
    QStringList results = system->processPaths(meshConfigFileString,
                                               PathOperationType::CHECK);

    // Launch dialog if any config files are present
    if (results.contains("0")) {
        RunMeshDialog dialog(caseName, m_systemMgr, results,
                             isFoundation, this);

        // Connect signal
        connect(&dialog, &RunMeshDialog::requestRunMesh, this,
                &MainWindow::runMesh);
        dialog.exec();
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
                CaseIO::createDecomposeParDict(openFoamPath, numCores);
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

// Launch solver configuration wizard
void MainWindow::launchSolverConfigurationWizard() {
    // Check if server is available
    QString caseName = getSelectedCase();
    QString casePath = m_systemMgr.getData(caseName).casePath;
    auto system = m_systemMgr.getSystem(caseName);
    if (system == nullptr) {
        QMessageBox::critical(this, tr("Server access failure"),
                              tr("Couldn't reach server."));
        return;
    }

    // Get patch names
    QStringList patchNames;
    QString fileName = "constant/polyMesh/boundary";
    std::optional<QByteArray> fileData = system->getFileContent(
        casePath + "/" + caseName + "/" + fileName);
    if (fileData && !fileData.value().isEmpty()) {
        patchNames = CaseIO::getPatches(fileData.value());
    } else {
        QMessageBox::critical(this, tr("Boundary File Absent"),
            tr("Couldn't access constant/polyMesh/boundary file."));
        return;
    }

    // Create the wizard
    SolverWizard wizard(caseName, m_systemMgr, m_solverFamilies,
        m_turbulenceModels, m_transportProperties, m_fieldData,
        m_boundaryConditions, patchNames, this);

    // Parse case files
    if (wizard.parseFiles()) {
        connect(&wizard, &SolverWizard::createTextEditor, this,
            &MainWindow::createTextEditor);
        connect(&wizard, &SolverWizard::updatePath, this,
            &MainWindow::updatePath);
        wizard.exec();
    }
}

// Launch solver execution wizard
void MainWindow::launchSolverExecutionDialog() {
    // Get path of currently-selected case
    QString caseName = getSelectedCase();
    CaseData caseData = m_systemMgr.getData(caseName);
    QString casePath = caseData.casePath + "/" + caseName;
    QString openFoamPath = caseData.openFoamPath;

    // Check if server is available
    auto system = m_systemMgr.getSystem(caseName);
    if (system == nullptr) {
        QMessageBox::critical(this, tr("Server access failure"),
                              tr("Couldn't reach server."));
        return;
    }

    // Check if using Foundation release of OpenFOAM
    QString dirName = QDir(openFoamPath).dirName();
    const QRegularExpression foundationRegex("^openfoam\\d{2}$",
        QRegularExpression::CaseInsensitiveOption);
    bool isFoundation = foundationRegex.match(dirName).hasMatch();

    // Create dialog
    RunSolverDialog dialog(caseName, isFoundation, m_systemMgr, this);

    // Connect signal
    connect(&dialog, &RunSolverDialog::requestRunSolver, this,
            &MainWindow::runSolver);
    dialog.exec();
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

/*
std::shared_ptr<RenderData> MainWindow::getMeshData(const QString& caseName,
        const QString& casePath) {
    return std::make_shared<RenderData>(
        m_systemMgr.getSystem(caseName)->getMeshData(casePath));
}
*/

/*
                return std::make_shared<RenderData>(
                    m_systemMgr.getSystem(caseName)->getMeshData(casePath)
                    );
*/

// Access result data
std::shared_ptr<RenderData> MainWindow::getResultData(const QString& caseName,
    const QString& casePath, const QString& selectedField,
    const QString& timeFolder) {
    // Check communication
    auto system = m_systemMgr.getSystem(caseName);
    if (system == nullptr) {
        QMessageBox::critical(this, tr("Server access failure"),
                              tr("Couldn't reach server."));
        return nullptr;
    }

    // Get patches
    QStringList patchNames;
    std::optional<QByteArray> fileData =
        system->getFileContent(casePath + "/constant/polyMesh/boundary");
    if (fileData && !fileData.value().isEmpty()) {
        patchNames = CaseIO::getPatches(fileData.value());
    } else {
        qDebug() << "Couldn't access constant/polyMesh/boundary file.";
        return nullptr;
    }

    // Construct msg string
    QString fieldPath = casePath + "/" + timeFolder + "/" + selectedField;
    patchNames.prepend(fieldPath);
    QString msg = patchNames.join("|");

    // Get result data
    std::vector<FieldData> fieldData =
        m_systemMgr.getSystem(caseName)->getResultData(msg);
    return nullptr;
}

void MainWindow::updateResult(const QString& casePath,
                            const QString& timeFolder) {
    /*
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
    */
}

// Launch post-processing wizard
void MainWindow::launchPostProcessingWizard() {
    // Check server connection
    QString caseName = getSelectedCase();
    QString casePath = m_systemMgr.getData(caseName).casePath;
    auto system = m_systemMgr.getSystem(caseName);
    if (system == nullptr) {
        QMessageBox::critical(this, tr("Server access failure"),
            tr("Couldn't reach server."));
        return;
    }

    // Get patch names
    QStringList patchNames;
    QString fileName = "constant/polyMesh/boundary";
    std::optional<QByteArray> fileData = system->getFileContent(
        casePath + "/" + caseName + "/" + fileName);
    if (fileData && !fileData.value().isEmpty()) {
        patchNames = CaseIO::getPatches(fileData.value());
    } else {
        QMessageBox::critical(this, tr("Boundary File Absent"),
             tr("Couldn't access constant/polyMesh/boundary file."));
        return;
    }

    // Get field names
    QStringList fieldNames;
    // Check if field files are in 0
    fileName = casePath + "/" + caseName + "/0";
    fieldNames = m_systemMgr.getSystem(caseName)->processPaths(
        fileName, PathOperationType::LIST);
    if (fieldNames.isEmpty()) {
        QMessageBox::critical(this, tr("Field Files Absent"),
            tr("Couldn't find any field files in the case's 0 folder."));
        return;
    }

    // Remove pipe characters if present
    for (int i = fieldNames.size() - 1; i >= 0; --i) {
        if (fieldNames[i].endsWith('|')) {
            fieldNames[i].chop(1);
        }
        else {
            fieldNames.removeAt(i);
        }
    }

    // Create the wizard
    PostprocessingWizard wizard(getSelectedCase(), patchNames, fieldNames,
                                m_systemMgr, this);
    // Parse case files
    if (wizard.parseFile())
        wizard.exec();
}
