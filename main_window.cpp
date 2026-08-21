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

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QProgressDialog>
#include <QStatusBar>
#include <QToolBar>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "dialogs/utility_output/utility_output_dialog.h"
#include "editors/graphical/surface/surface_editor.h"
#include "editors/graphical/mesh/mesh_editor.h"
#include "geometry/stl/stl_reader.h"
#include "parser/surface_patch_dict.h"
#include "systems/local_system.h"
#include "systems/remote_system.h"
#include "systems/wsl_system.h"

// Create the main window
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
#ifdef Q_OS_WIN
    setWindowIcon(QIcon(":/images/flowcompute.ico"));
#else
    setWindowIcon(QIcon(":/images/flowcompute.png"));
#endif
    setWindowTitle("FlowCompute 0.9.0 - Visual CFD Made Simple");

    // Set font
    int regularId =
        QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Italic.ttf");
    if (regularId != -1) {
        QString fontFamily =
            QFontDatabase::applicationFontFamilies(regularId).at(0);
        m_font = QFont(fontFamily);
        m_font.setPointSize(12);
        m_font.setStyleHint(QFont::Monospace);
        m_font.setFixedPitch(true);
    }

    // Create communication systems
    auto wslSystem    = std::make_shared<WslSystem>();
    auto localSystem  = std::make_shared<LocalSystem>();
    auto remoteSystem = std::make_shared<RemoteSystem>();
    std::array<std::shared_ptr<TargetSystem>, 3> systems = {
        wslSystem, localSystem, remoteSystem };

    // Communication event handling
    for (const auto& system : systems) {
        connect(system.get(), &TargetSystem::logMessage,
                this, &MainWindow::log);
        connect(system.get(), &TargetSystem::longUtilityFinished,
                this, &MainWindow::longUtilityFinished);
        connect(system.get(), &TargetSystem::longUtilityError,
                this, &MainWindow::log);
    }

    // Update system manager
    m_systemMgr.setSystems(systems);

    // Create actions
    createActions();

    // Create tab widget
    m_tabWidget = new TabWidget(this);
    setCentralWidget(m_tabWidget);

    // Create the navigator
    m_navigatorWidget = new QDockWidget(tr("Case Navigator"), this);
    m_navigatorWidget->setTitleBarWidget(new QWidget(m_navigatorWidget));
    m_navigatorWidget->setAllowedAreas(Qt::LeftDockWidgetArea);
    m_navigator = new CaseNavigator(m_newCaseAction, m_openCaseAction,
        m_configureMeshAction, m_runMeshAction, m_viewMeshAction,
        m_configureSolverAction, m_runSolverAction, m_viewResultAction,
        m_cutAction, m_copyAction, m_pasteAction, m_uploadAction,
        m_downloadAction, m_systemMgr, this);
    m_navigatorWidget->setWidget(m_navigator);
    m_navigatorWidget->setMinimumWidth(200);
    addDockWidget(Qt::LeftDockWidgetArea, m_navigatorWidget);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

    // Connect navigator signals
    connect(m_navigator, &CaseNavigator::createEditor,
            this, &MainWindow::createEditor);
    connect(m_navigator, &CaseNavigator::logMessage,
            this, &MainWindow::log);
    connect(m_navigator, &CaseNavigator::requestUpdatePath,
            this, &MainWindow::updatePath);
    connect(m_navigator, &CaseNavigator::updateSettings,
            this, &MainWindow::saveCases);

    // Get actions from navigator
    QList<QAction*> actions = m_navigator->getActions();
    m_newFileAction = actions[0];
    m_newFolderAction = actions[1];
    m_newDictAction = actions[2];
    m_deleteAction = actions[3];

    // Create menus and toolbar
    createMenus();
    createToolBar();

    // Create the console
    m_consoleWidget = new QDockWidget(tr("Console"), this);
    QWidget *emptyTitleBar = new QWidget(m_consoleWidget);
    emptyTitleBar->setFixedHeight(0);
    m_consoleWidget->setTitleBarWidget(emptyTitleBar);
    m_consoleWidget->setAllowedAreas(Qt::BottomDockWidgetArea);
    m_console = new Console(m_consoleWidget);
    m_console->setMinimumHeight(150);
    m_consoleWidget->setWidget(m_console);
    addDockWidget(Qt::BottomDockWidgetArea, m_consoleWidget);
    m_console->appendPlainText("\nWelcome to FlowCompute!\n");

    // Access configuration directory
    QString configDirPath =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    m_configDir.setPath(configDirPath);
    if (!m_configDir.exists()) {
        m_configDir = QDir(".");
    }

    // Create themes directory if it doesn't exist
    QDir themesDir(m_configDir.filePath("themes"));
    if (!themesDir.exists()) {
        if (!m_configDir.mkpath("themes")) {
            throw std::runtime_error("Failed to create themes directory");
        }

        // Copy files from the resource system
        QDir resourceThemesDir(":/themes");
        const QFileInfoList files =
            resourceThemesDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
        for (const QFileInfo& fileInfo : files) {
            const QString sourcePath = fileInfo.filePath();
            const QString destPath = themesDir.filePath(fileInfo.fileName());
            if (QFile::copy(sourcePath, destPath)) {
                QFile newThemeFile(destPath);
                newThemeFile.setPermissions(
                    QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                    QFileDevice::ReadUser | QFileDevice::WriteUser);
            } else {
                qWarning() << "Failed to copy" <<
                    sourcePath << "to" << destPath;
            }
        }
    }

    // Read theme from settings
    QSettings settings;
    bool noAccess = false;
    applyTheme(settings.value("Theme/file", "dark.json").toString());

    // Populate caseMap and navigator from settings
    m_systemMgr.clear();
    int caseCount = settings.beginReadArray("Cases");
    bool wslServerCheck = false, wslServerAvailable = false;
    bool remoteServerCheck = false, remoteServerAvailable = false;

    // Read settings
    CaseData data;
    for (int i = 0; i < caseCount; ++i) {
        settings.setArrayIndex(i);
        QString caseName = settings.value("caseName").toString();
        data.casePath = settings.value("casePath").toString();
        data.caseFiles = settings.value("caseFiles").toStringList();
        data.targetId = settings.value("targetSystemId", 0).toInt();
        data.openFoamPath = settings.value("openFoamPath").toString();
        if (data.targetId == static_cast<int>(TargetType::REMOTE_LINUX)) {
            data.userName = settings.value("userName").toString();
            data.hostName = settings.value("hostName").toString();
            data.port = settings.value("port", 22).toInt();
            m_systemMgr.setDefaultHost(data.hostName);
            m_systemMgr.setDefaultUser(data.userName);
        }

        // Check server availability
        if ((data.targetId == static_cast<int>(TargetType::LOCAL_WINDOWS)) &&
            (!wslServerCheck)) {
            wslServerAvailable = m_systemMgr.checkWslServer();
            wslServerCheck = true;
        } else if
            ((data.targetId == static_cast<int>(TargetType::REMOTE_LINUX)) &&
                (!remoteServerCheck)) {
            remoteServerAvailable =
                m_systemMgr.checkRemoteServer(data.hostName, data.port);
            remoteServerCheck = true;
        }
        bool isServerAvailable =
            ((data.targetId == static_cast<int>(TargetType::LOCAL_WINDOWS)) &&
                wslServerAvailable) ||
            (data.targetId == static_cast<int>(TargetType::LOCAL_LINUX)) ||
            ((data.targetId == static_cast<int>(TargetType::REMOTE_LINUX)) &&
                remoteServerAvailable);

        bool casePresent = true;
        QStringList paths;
        QString casePath = data.casePath + "/" + caseName + "/";
        m_systemMgr.addCase(caseName, data);
        if (!isServerAvailable) {
            qDebug() << "SERVER NOT AVAILABLE for case " << caseName;
            if (!m_utilMap.contains(data.openFoamPath)) {
                m_utilMap[data.openFoamPath] = QMap<QString, bool>();
            }
            log(QString(tr("Server unreachable for case %1.").arg(caseName)));
        } else if (data.targetId ==
                   static_cast<int>(TargetType::REMOTE_LINUX)) {
            casePresent = true;
        } else {
            // Get the files/folders in the case's top-level folder
            paths = m_systemMgr.getSystem(data.targetId)->processPaths(
                casePath, PathOperationType::CHECK);

            if (!paths.isEmpty() && paths[0] == "0") {
                // Get files in case
                paths = m_systemMgr.getSystem(data.targetId)->processPaths(
                    casePath, PathOperationType::LIST);

                // Check utilities
                if (!m_utilMap.contains(data.openFoamPath)) {
                    m_utilMap[data.openFoamPath] =
                        checkUtilities(casePath, m_utilities);
                }

                // Log if server is up but OpenFOAM is missing
                if (m_utilMap[data.openFoamPath].empty()) {
                    log(QString(tr("Failed to open case %1. "
                                   "Cannot access OpenFOAM at %2.")
                                    .arg(caseName, data.openFoamPath)));
                }
            } else {
                casePresent = false;
            }
        }

        // Update navigator
        if (casePresent) {
            bool isDisabled =
                ((data.targetId != static_cast<int>(TargetType::REMOTE_LINUX)
                && (m_utilMap[data.openFoamPath].empty())
                || !isServerAvailable));
            m_navigator->addCase(caseName, paths, isDisabled);
            if (!isDisabled) {
                m_navigator->expandCase(caseName);
            }
        } else {
            log(QString(tr("Couldn't find case %1 at path %2. "
                "Please import the case if it has moved.")
                    .arg(caseName, casePath)));
        }
    }
    settings.endArray();

    // Populate tabMap and tabs from settings
    m_tabMap.clear();
    int tabCount = settings.beginReadArray("Tabs");
    for (int i = 0; i < tabCount; ++i) {
        settings.setArrayIndex(i);

        // Update map
        QString tabName = settings.value("tabName").toString();
        TabData data;
        data.type = static_cast<EditorType>(settings.value("type").toInt());
        data.fullPath = settings.value("fullPath").toString();

        // Check if the case can be accessed
        QString caseName = data.fullPath.split("/")[0];
        CaseData caseData = m_systemMgr.getData(caseName);
        if (m_utilMap[caseData.openFoamPath].empty()) {
            continue;
        }

        // Create tab and editor
        m_tabMap.insert(tabName, data);
        createEditor(data.type, tabName, data.fullPath, false);
    }
    settings.endArray();

    // Create Vulkan instance
    m_vulkanInstance.setLayers({ "VK_LAYER_KHRONOS_validation" });
    m_vulkanInstance.setApiVersion(QVersionNumber(1, 2));
    if (!m_vulkanInstance.create()) {
        qFatal("Failed to create Vulkan instance: %d",
               m_vulkanInstance.errorCode());
    }

    // Configure undo/redo actions
    connect(m_tabWidget, &QTabWidget::currentChanged, this,
        [=, this](int index) {
        TextEditor* editor =
            qobject_cast<TextEditor*>(m_tabWidget->widget(index));
        if (editor) {
            m_undoAction->setEnabled(editor->document()->isUndoAvailable());
            m_redoAction->setEnabled(editor->document()->isRedoAvailable());
        } else {
            m_undoAction->setEnabled(false);
            m_redoAction->setEnabled(false);
        }
    });

    // Create status bar
    statusBar()->showMessage("Ready");

    // Configure tab save
    connect(m_tabWidget, &TabWidget::saveTab, this, &MainWindow::saveFile);

    // Load data from JSON configuration files
    loadSolverFamilies();
    loadTurbulenceModels();
    loadFieldData();
    loadBoundaryConditions();
    loadMaterialProperties();
}

MainWindow::~MainWindow() = default;

/* Create QActions for file operations */
void MainWindow::createActions() {
    // Create new case
    m_newCaseAction = new QAction(QIcon(":/images/new_case.png"),
                                tr("New Case..."), this);
    m_newCaseAction->setShortcuts(QKeySequence::New);
    m_newCaseAction->setStatusTip(tr("Create a new case folder"));
    connect(m_newCaseAction, &QAction::triggered, this,
            &MainWindow::newCase);

    // Open existing case
    m_openCaseAction = new QAction(QIcon(":/images/open_case.png"),
                                  tr("Open Case..."), this);
    m_openCaseAction->setShortcuts(QKeySequence::Open);
    m_openCaseAction->setStatusTip(tr("Create a new case folder"));
    connect(m_openCaseAction, &QAction::triggered, this,
            &MainWindow::openCase);

    // Save file
    m_saveFileAction =
        new QAction(QIcon(":/images/save.png"), tr("Save"), this);
    m_saveFileAction->setShortcuts(QKeySequence::Save);
    m_saveFileAction->setStatusTip(tr("Save the file"));
    connect(m_saveFileAction, &QAction::triggered, this, &MainWindow::saveFile);
    m_saveFileAction->setDisabled(true);

    // Upload
    m_uploadAction = new QAction(QIcon(":/images/upload.png"),
                                  tr("Uplo&ad..."), this);
    m_uploadAction->setShortcut(QKeySequence("Ctrl+U"));
    m_uploadAction->setStatusTip(tr("Upload a file or folder"));
    m_uploadAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_uploadAction, &QAction::triggered, this,
            &MainWindow::upload);

    // Download
    m_downloadAction = new QAction(QIcon(":/images/download.png"),
                                 tr("Do&wnload..."), this);
    m_downloadAction->setShortcut(QKeySequence("Ctrl+D"));
    m_downloadAction->setStatusTip(tr("download a file or folder"));
    m_downloadAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_downloadAction, &QAction::triggered, this,
            &MainWindow::download);

    // Exit
    m_exitAction = new QAction(tr("E&xit"), this);
    m_exitAction->setShortcuts(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("Exit the application"));
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::close);

    // Cut
    m_cutAction = new QAction(QIcon(":/images/cut.png"), tr("Cu&t"), this);
    m_cutAction->setShortcuts(QKeySequence::Cut);
    m_cutAction->setStatusTip(tr("Cut"));
    connect(m_cutAction, &QAction::triggered, this, [this]() {
        QWidget* focused = QApplication::focusWidget();
        if (!focused || qobject_cast<QMenu*>(focused)) {
            if (QWidget* activeWindow = QApplication::activeWindow()) {
                focused = activeWindow->focusWidget();
            }
        }

        if (!focused) return;

        // Check the Case Navigator
        if (m_navigator == focused || m_navigator->isAncestorOf(focused)) {
            m_navigator->cutCopySelection(true);
            return;
        }

        // Check the TextEditor
        QWidget* current = focused;
        while (current) {
            if (auto* editor = qobject_cast<TextEditor*>(current)) {
                editor->cut();
                return;
            }
            current = current->parentWidget();
        }
        QMetaObject::invokeMethod(focused, "cut");
    });

    // Copy
    m_copyAction = new QAction(QIcon(":/images/copy.png"), tr("&Copy"), this);
    m_copyAction->setShortcuts(QKeySequence::Copy);
    m_copyAction->setStatusTip(tr("Copy"));
    connect(m_copyAction, &QAction::triggered, this, [this]() {
        QWidget* focused = QApplication::focusWidget();
        if (!focused) return;

        // Check the Case Navigator
        if (m_navigator == focused || m_navigator->isAncestorOf(focused)) {
            m_navigator->cutCopySelection(false);
            return;
        }

        // Check the TextEditor
        QWidget* current = focused;
        while (current) {
            if (auto* editor = qobject_cast<TextEditor*>(current)) {
                editor->copy();
                return;
            }
            current = current->parentWidget();
        }
        QMetaObject::invokeMethod(focused, "copy");
    });

    // Paste
    m_pasteAction =
        new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
    m_pasteAction->setShortcuts(QKeySequence::Paste);
    m_pasteAction->setStatusTip(tr("Paste"));
    connect(m_pasteAction, &QAction::triggered, this, [this]() {
        QWidget* focused = QApplication::focusWidget();
        if (!focused) return;

        // Check the Case Navigator
        if (m_navigator == focused || m_navigator->isAncestorOf(focused)) {
            m_navigator->pasteSelection();
            return;
        }

        // Check the TextEditor
        QWidget* current = focused;
        while (current) {
            if (auto* editor = qobject_cast<TextEditor*>(current)) {
                editor->paste();
                return;
            }
            current = current->parentWidget();
        }
        QMetaObject::invokeMethod(focused, "paste");
    });

    // Preferences
    m_preferencesAction = new QAction(QIcon(":/images/dict.png"),
            tr("Set preferences..."), this);
    m_preferencesAction->setShortcuts(QKeySequence::Preferences);
    m_preferencesAction->setStatusTip(tr("Set preferences"));
    connect(m_preferencesAction, &QAction::triggered, this,
            &MainWindow::launchPreferencesDialog);

    // Undo
    m_undoAction = new QAction(QIcon(":/images/undo.png"), tr("Undo"), this);
    m_undoAction->setShortcuts(QKeySequence::Undo);
    m_undoAction->setStatusTip(tr("Zoom in"));
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::undo);
    m_undoAction->setDisabled(true);

    // Redo
    m_redoAction = new QAction(QIcon(":/images/redo.png"), tr("Redo"), this);
    m_redoAction->setShortcuts(QKeySequence::Redo);
    m_redoAction->setStatusTip(tr("Zoom out"));
    connect(m_redoAction, &QAction::triggered, this, &MainWindow::redo);
    m_redoAction->setDisabled(true);

    // Zoom in
    m_zoomInAction =
        new QAction(QIcon(":/images/zoom_in.png"), tr("Zoom in"), this);
    m_zoomInAction->setShortcuts(QKeySequence::ZoomIn);
    m_zoomInAction->setStatusTip(tr("Zoom in"));

    // Zoom out
    m_zoomOutAction =
        new QAction(QIcon(":/images/zoom_out.png"), tr("Zoom out"), this);
    m_zoomOutAction->setShortcuts(QKeySequence::ZoomOut);
    m_zoomOutAction->setStatusTip(tr("Zoom out"));

    // Launch mesh configuration dialog
    m_configureMeshAction =
        new QAction(QIcon(":/images/mesh.png"), tr("Configure &Mesh"), this);
    m_configureMeshAction->setStatusTip(tr("Configure mesh process"));
    connect(m_configureMeshAction, &QAction::triggered, this,
            &MainWindow::launchMeshConfigurationWizard);

    // Launch mesh execution dialog
    m_runMeshAction = new QAction(QIcon(":/images/run_mesh.png"),
                                  tr("Run Mesh &Utilities"), this);
    m_runMeshAction->setStatusTip(tr("Run mesh utilities"));
    connect(m_runMeshAction, &QAction::triggered, this,
            &MainWindow::launchMeshExecutionDialog);

    // View mesh
    m_viewMeshAction =
        new QAction(QIcon(":/images/view_mesh.png"), tr("&View Mesh"), this);
    m_viewMeshAction->setStatusTip(tr("View mesh"));
    connect(m_viewMeshAction, &QAction::triggered, this,
            &MainWindow::viewMesh);

    // Launch solver configuration dialog
    m_configureSolverAction = new QAction(QIcon(":/images/solver.png"),
                               tr("Configure &Simulation"), this);
    m_configureSolverAction->setStatusTip(tr("Configure simulation"));
    connect(m_configureSolverAction, &QAction::triggered, this,
            &MainWindow::launchSolverConfigurationWizard);

    // Launch solver execution dialog
    m_runSolverAction = new QAction(QIcon(":/images/run_solver.png"),
                                  tr("Ru&n Simulation"), this);
    m_configureSolverAction->setStatusTip(tr("Launch solver"));
    connect(m_runSolverAction, &QAction::triggered, this,
            &MainWindow::launchSolverExecutionDialog);

    // Stop solver
    m_stopSolverAction = new QAction(QIcon(":/images/stop_solver.png"),
                                   tr("&Halt Simulation"), this);
    m_stopSolverAction->setStatusTip(tr("Stop solver"));
    m_stopSolverAction->setEnabled(false);
    connect(m_stopSolverAction, &QAction::triggered, this,
            &MainWindow::stopSolver);

    // View result
    m_viewResultAction = new QAction(
        QIcon(":/images/view_result.png"), tr("View R&esult"), this);
    m_viewResultAction->setStatusTip(tr("View result"));
    connect(m_viewResultAction, &QAction::triggered, this,
            &MainWindow::viewResult);

    // Post-processing
    m_postProcessAction = new QAction(QIcon(":/images/postprocess.png"),
         tr("&Post-process"), this);
    m_postProcessAction->setStatusTip(tr("Launch post-processing"));
    connect(m_postProcessAction, &QAction::triggered, this,
            &MainWindow::launchPostProcessingWizard);

    // Configure the About action in the help menu
    m_aboutAction = new QAction(QIcon(":/images/help.png"), tr("&Help"), this);
    m_aboutAction->setStatusTip(tr("Provide assistance"));
    // connect(aboutAction, &QAction::triggered, this, SLOT(About()));
}

// Assemble actions within main menu
void MainWindow::createMenus() {
    // Create file menu
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_newCaseAction);
    m_fileMenu->addAction(m_openCaseAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_newFileAction);
    m_fileMenu->addAction(m_newFolderAction);
    m_fileMenu->addAction(m_newDictAction);
    m_fileMenu->addAction(m_saveFileAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_uploadAction);
    m_fileMenu->addAction(m_downloadAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);

    // Create edit menu
    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_undoAction);
    m_editMenu->addAction(m_redoAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_cutAction);
    m_editMenu->addAction(m_copyAction);
    m_editMenu->addAction(m_pasteAction);
    m_editMenu->addAction(m_deleteAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_preferencesAction);

    // Create view menu
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(m_zoomInAction);
    m_viewMenu->addAction(m_zoomOutAction);

    // Create mesh menu
    m_meshMenu = menuBar()->addMenu(tr("&Mesh"));
    m_meshMenu->addAction(m_configureMeshAction);
    m_meshMenu->addAction(m_runMeshAction);

    // Create simulation menu
    m_simMenu = menuBar()->addMenu(tr("&Simulation"));
    m_simMenu->addAction(m_postProcessAction);

    // Create help menu
    helpMenu = menuBar()->addMenu(tr("&Help"));
    // helpMenu->addAction(aboutAction);
}

// Add entries to toolbars
void MainWindow::createToolBar() {
    // Create tool bar
    toolBar = addToolBar(tr("Toolbar"));
    toolBar->addAction(m_newCaseAction);
    toolBar->addAction(m_saveFileAction);
    /*
    toolBar->addAction(m_newFileAction);
    toolBar->addAction(open_fileAction);

    toolBar->addAction(printAction);
    */

    // Create tool bar with view actions
    toolBar->addSeparator();
    toolBar->addAction(m_undoAction);
    toolBar->addAction(m_redoAction);
    toolBar->addSeparator();
    toolBar->addAction(m_zoomInAction);
    toolBar->addAction(m_zoomOutAction);
    toolBar->addSeparator();

    // Create tool bar with mesh actions
    toolBar->addAction(m_configureMeshAction);
    toolBar->addAction(m_runMeshAction);
    toolBar->addSeparator();

    // Create tool bar with solver actions
    toolBar->addAction(m_configureSolverAction);
    toolBar->addAction(m_runSolverAction);
    toolBar->addAction(m_stopSolverAction);
    toolBar->addSeparator();

    // Create tool bar with view actions
    toolBar->addAction(m_postProcessAction);

    // Add help actions
    toolBar->addAction(m_aboutAction);
}

// Undo action in text editor
void MainWindow::undo() {
    TextEditor* editor =
        qobject_cast<TextEditor*>(m_tabWidget->currentWidget());
    if (editor)
        editor->undo();
}

// Redo action in text editor
void MainWindow::redo() {
    TextEditor* editor =
        qobject_cast<TextEditor*>(m_tabWidget->currentWidget());
    if (editor)
        editor->redo();
}

// Upload file or folder
void MainWindow::upload() {
    // Access selected node (always a folder or case folder)
    NodeData* node = m_navigator->nodeFromIndex(m_navigator->currentIndex());
    if (!node)
        return;

    // Construct path
    QString caseName, targetPath, nodePath;
    if (node->fullPath.isEmpty()) {
        caseName = node->name;
        nodePath = "";
        targetPath = m_systemMgr.getData(caseName).casePath + "/" + caseName;
    } else {
        int pos = node->fullPath.indexOf("/");
        caseName = node->fullPath.left(pos);
        nodePath = node->fullPath.mid(pos + 1);
        targetPath = m_systemMgr.getData(caseName).casePath + "/" +
                   node->fullPath + "/" +  node->name;
    }

    // Create file selection dialog
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select files to upload"));
    dialog.setDirectory(QDir::homePath());
    dialog.setFileMode(QFileDialog::ExistingFiles);

    // Launch dialog and upload files
    if (dialog.exec() == QDialog::Accepted) {
        auto system = m_systemMgr.getSystem(caseName);
        QStringList selections = dialog.selectedFiles();
        for (const QString& path : std::as_const(selections)) {
            QFileInfo fileInfo(path);
            QString fileName = fileInfo.fileName();
            system->writeData(path, targetPath + "/" + fileName);
        }
    }

    // Update folder
    updatePath(caseName, nodePath);
}

// Download file or folder
void MainWindow::download() {
    // Access selected node (always a folder or case folder)
    NodeData* node = m_navigator->nodeFromIndex(m_navigator->currentIndex());
    if (!node)
        return;

    // Get download
    QString localPath = QFileDialog::getExistingDirectory(this,
        QString(tr("Select folder to receive download")),
        QDir::homePath());
    if (localPath.isEmpty())
        return;

    // Construct path
    QString caseName, targetPath, basePath;
    if (node->fullPath.isEmpty()) {
        caseName = node->name;
        basePath = m_systemMgr.getData(caseName).casePath;
        targetPath = basePath + "/" + caseName;
    } else {
        int pos = node->fullPath.indexOf("/");
        caseName = node->fullPath.left(pos);
        basePath = m_systemMgr.getData(caseName).casePath + "/" +
                   node->fullPath;
        targetPath = basePath + "/" + node->name;
    }

    // Get the base file name
    QFileInfo fileInfo(targetPath);
    QString targetFileName = fileInfo.fileName();

    // Perform download depending on node type
    std::shared_ptr<TargetSystem> system = m_systemMgr.getSystem(caseName);
    if ((node->nodeType == NodeType::CaseFolder) ||
        (node->nodeType == NodeType::Folder)) {
        // Create progress dialog
        QProgressDialog* progressDialog = new QProgressDialog(
            tr("Downloading folder..."), QString(), 0, 0, this);
        progressDialog->setWindowTitle(tr("Operation in progress"));
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setAttribute(Qt::WA_DeleteOnClose);
        progressDialog->show();

        // Create and configure watcher
        QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
        connect(watcher, &QFutureWatcher<void>::finished, progressDialog,
                &QProgressDialog::accept);
        connect(watcher, &QFutureWatcher<void>::finished, watcher,
                &QFutureWatcher<void>::deleteLater);

        // Run function concurrently
        QFuture<void> future = QtConcurrent::run([=, this]() {
            downloadFolder(system, basePath, node->name, localPath);
        });
        watcher->setFuture(future);
    } else {
        // Get content of remote file
        QByteArray fileData = system->getFileContent(targetPath);

        // Write data to local file
        if (!fileData.isEmpty()) {
            QFile file(localPath + "/" + targetFileName);
            if (!file.open(QIODevice::WriteOnly))
                return;
            if (file.write(fileData) != fileData.size())
                return;
            file.close();
        }
    }
}

void MainWindow::downloadFolder(std::shared_ptr<TargetSystem> system,
                QString basePath, QString nodeName, QString localPath) {
    // Compress folder
    QString cmd =
        QString("cd %1; tar -czf %2.tar.gz %2/").arg(basePath, nodeName);
    QString output;
    if (system->launchShortUtility(cmd, output) == 0) {
        QString remoteFileName = basePath + "/" + nodeName + ".tar.gz";
        QString localFilePath =
            QDir::cleanPath(localPath + "/" + nodeName + ".tar.gz");

        // Download the compressed file
        QByteArray fileData = system->getFileContent(remoteFileName);

        // Save to local disk
        if (!fileData.isEmpty()) {
            QFile file(localFilePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(fileData);
                file.close();
            }
        }

        // Decompress the downloaded archive
        QProcess tarProcess;
        QStringList arguments;
        arguments << "-xzf" << localFilePath << "-C" << localPath;
        tarProcess.start("tar", arguments);

        // Wait for the extraction to complete in this background thread
        if (tarProcess.waitForFinished()) {
            // Only delete the archive if the extraction was successful
            if (tarProcess.exitStatus() ==
                    QProcess::NormalExit && tarProcess.exitCode() == 0) {
                QFile::remove(localFilePath);
            } else {
                qWarning() << tr("Failed to extract tar archive.");
            }
        }

        // Remove the archive from the remote server
        system->launchShortUtility(
            QString("rm %1").arg(remoteFileName), output);
    }
}

// Write text to the log
void MainWindow::log(const QString& text) {
    m_console->appendPlainText(text);
}

void MainWindow::updatePath(const QString& caseName, const QString& subDir) {
    // Get case path
    QString casePath = caseName;
    if (!subDir.isEmpty()) {
        casePath += "/" + subDir;
    }

    QString fullPath = m_systemMgr.getData(caseName).casePath + "/" + casePath;
    QStringList files = m_systemMgr.getSystem(caseName)->processPaths(fullPath,
        PathOperationType::LIST);

    if (!files.isEmpty()) {
        m_navigator->updatePath(casePath, files);
    }
}

void MainWindow::onDirtyStateChanged(bool isDirty, QWidget* widget) {
    // Get the index of the editor
    if (!widget) return;
    int index = m_tabWidget->indexOf(widget);

    if (index != -1) {
        // Get the current tab text
        QString tabText = m_tabWidget->tabText(index);

        // Remove trailing " *"
        if (tabText.endsWith(" *")) {
            tabText.chop(2);
        }

        // Append the asterisk if the state is dirty
        if (isDirty) {
            tabText += " *";
        }

        m_tabWidget->setTabText(index, tabText);

        // Update the save action if the modified file is being viewed
        if (m_tabWidget->currentIndex() == index) {
            m_saveFileAction->setEnabled(isDirty);
        }
    }
}

// Check if utility is available
QMap<QString, bool> MainWindow::checkUtilities(const QString& fullPath,
        const QStringList& utilities) {
    // Get case path
    QMap<QString, bool> utilMap;
    QString casePath = fullPath.left(fullPath.lastIndexOf('/'));

    // Get OpenFOAM path
    QString openFoamPath;
    QString caseName = QFileInfo(casePath).fileName();
    if (m_systemMgr.contains(caseName)) {
        openFoamPath = m_systemMgr.getData(caseName).openFoamPath;
    } else {
        qWarning() << "Case " << caseName << " is not available.";
        return {};
    }

    QString utilityList = utilities.join(" ");
    QString cmd =
        QString("cd %1; source %2/etc/bashrc; out=\"\"; for u in %3; "
                "do command -v $u >/dev/null 2>&1 && out+=\"$u:true,\""
                "|| out+=\"$u:false,\"; done; echo \"${out%,}\"").
                  arg(casePath, openFoamPath, utilityList);

    // Get result from checking utilities
    QString output;
    if (m_systemMgr.getSystem(caseName)->launchShortUtility(cmd, output) == 0) {
        output.remove("\n");
        QStringList res;
        QStringList utils = output.split(",");
        for (const auto& util : std::as_const(utils)) {
            res = util.split(":");
            utilMap[res[0]] = (res[1] == "true");
        }
    }

    // Clear map if no utilities are detected
    bool allFalse = std::none_of(utilMap.cbegin(), utilMap.cend(),
        [](bool value) { return value; });
    if (allFalse)
        utilMap.clear();
    /*
    else {
        for (const auto& util : utilities) {
            utilMap[util] = false;
        }
    }
    */
    return utilMap;
}

// Run autoPatch
void MainWindow::runMeshPatch(double angle, const QString& casePath) {
    // Get OpenFOAM path
    QString openFoamPath;
    QString caseName = QFileInfo(casePath).fileName();
    if (m_systemMgr.contains(caseName)) {
        openFoamPath = m_systemMgr.getData(caseName).openFoamPath;
    } else {
        qWarning() << "Case" << caseName << "is not in case map.";
        return;
    }

    // Make sure autoPatch is present
    QMap<QString, bool> utilMap = m_utilMap[openFoamPath];
    if (!utilMap.value("autoPatch", false)) {
        QMessageBox::warning(this, tr("Utility Not Found"),
            tr("The autoPatch utility could not be found."));
        return;
    }

    // Run autoPatch
    QString result;
    QString cmd =
        QString("cd \"%1\"; source %2/etc/bashrc; autoPatch -overwrite %3")
            .arg(casePath, openFoamPath, QString::number(angle));
    m_systemMgr.getSystem(caseName)->launchShortUtility(cmd, result);

    // Reload mesh
    QProgressDialog* progress = new QProgressDialog("Loading Mesh Data...",
                                                    QString(), 0, 0, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumWidth(300);
    progress->setAttribute(Qt::WA_DeleteOnClose);
    progress->show();

    // Setup the Future Watcher
    using RenderDataPtr = std::shared_ptr<RenderData>;
    QFutureWatcher<RenderDataPtr>* watcher =
        new QFutureWatcher<RenderDataPtr>(this);

    // Connect the watcher's finished signal to handle the result
    connect(watcher, &QFutureWatcher<RenderDataPtr>::finished, this,
    [this, watcher, progress, casePath]() {
        progress->close();

        // Retrieve the result from the background thread
        RenderDataPtr renderData = watcher->result();

        if (renderData) {
            MeshEditor* editor =
                qobject_cast<MeshEditor*>(m_tabWidget->currentWidget());
            editor->updateMesh(renderData);
        }

        // Clean up the watcher
        watcher->deleteLater();
    });

    // Start the background thread
    QFuture<RenderDataPtr> future =
        QtConcurrent::run(&MainWindow::getMeshData, this,
                          caseName, casePath);
    watcher->setFuture(future);

    // Refresh polyMesh directory in navigator
    updatePath(caseName, "constant/polyMesh");
}

void MainWindow::runMeshCheck(const QString& casePath) {
    // Get OpenFoam path
    QString openFoamPath;
    QString caseName = QFileInfo(casePath).fileName();
    if (m_systemMgr.contains(caseName)) {
        openFoamPath = m_systemMgr.getData(caseName).openFoamPath;
    } else {
        qWarning() << "Case" << caseName << "is not in case map.";
        return;
    }

    // Make sure checkMesh is present
    QMap<QString, bool> utilMap = m_utilMap[openFoamPath];
    if (!utilMap.value("checkMesh", false)) {
        QMessageBox::warning(this, tr("Utility Not Found"),
            tr("The checkMesh utility could not be found."));
        return;
    }

    // Run checkMesh
    QString cmd =
        QString("cd \"%1\"; source %2/etc/bashrc; checkMesh -constant")
            .arg(casePath, openFoamPath);

    // Get output
    QString logText;
    m_systemMgr.getSystem(caseName)->launchShortUtility(cmd, logText);
    if (logText.isEmpty())
        return;

    // Parse output
    QString status = "Unknown";
    QString cells = "?", faces = "?", points = "?";
    QString boundingBox = "Unknown";
    QString maxAspectRatio = "Unknown";
    QString nonOrthoMax = "?", nonOrthoAvg = "?";
    QString maxSkewness = "Unknown";
    QString minVolume = "Unknown";
    QString minFaceArea = "Unknown";

    // Overall Status
    QRegularExpression statusRegex("(Mesh OK\\.|Failed \\d+ mesh checks\\.)");
    auto statusMatch = statusRegex.match(logText);
    if (statusMatch.hasMatch()) status = statusMatch.captured(1);

    // Topology Counts
    QRegularExpression pointsRegex("points:\\s+(\\d+)");
    if (auto match = pointsRegex.match(logText); match.hasMatch())
        points = match.captured(1);

    QRegularExpression facesRegex("faces:\\s+(\\d+)");
    if (auto match = facesRegex.match(logText); match.hasMatch())
        faces = match.captured(1);

    QRegularExpression cellsRegex("cells:\\s+(\\d+)");
    if (auto match = cellsRegex.match(logText); match.hasMatch())
        cells = match.captured(1);

    QRegularExpression bboxRegex("Overall domain bounding box\\s+(.*)");
    if (auto match = bboxRegex.match(logText); match.hasMatch())
        boundingBox = match.captured(1);

    QRegularExpression aspectRegex("Max aspect ratio = ([\\d\\.]+)");
    if (auto match = aspectRegex.match(logText); match.hasMatch())
        maxAspectRatio = match.captured(1);

    QRegularExpression orthoRegex("Mesh non-orthogonality Max:\\s+([\\d\\.]+)"
                                  "\\s+average:\\s+([\\d\\.]+)");
    if (auto match = orthoRegex.match(logText); match.hasMatch()) {
        nonOrthoMax = match.captured(1);
        nonOrthoAvg = match.captured(2);
    }

    QRegularExpression skewRegex("Max skewness = ([\\d\\.]+)");
    if (auto match = skewRegex.match(logText); match.hasMatch())
        maxSkewness = match.captured(1);

    QRegularExpression volRegex("Min volume = ([\\d\\.\\-eE\\+]+)");
    if (auto match = volRegex.match(logText); match.hasMatch())
        minVolume = match.captured(1);

    QRegularExpression areaRegex("Minimum face area = ([\\d\\.\\-eE\\+]+)");
    if (auto match = areaRegex.match(logText); match.hasMatch())
        minFaceArea = match.captured(1);

    // Construct the formatted string using chained .arg() calls for safety
    std::vector<QString> messageStrings;
    messageStrings.push_back(QString("Overall Status: %1\n").arg(status));
    messageStrings.push_back(
        QString("Topology: %1 Cells, %2 Faces, %3 Points\n").
            arg(cells, faces, points));
    messageStrings.push_back(
        QString("Bounding Box: %1\n").arg(boundingBox));
    messageStrings.push_back(
        QString("Max Aspect Ratio: %1\n").arg(maxAspectRatio));
    messageStrings.push_back(
        QString("Non-Orthogonality: Max %1, Average %2\n").
            arg(nonOrthoMax, nonOrthoAvg));
    messageStrings.push_back(
        QString("Max Face Skewness: %1\n").arg(maxSkewness));
    messageStrings.push_back(QString("Min Cell Volume: %1\n").arg(minVolume));
    messageStrings.push_back(QString("Min Face Area: %1\n").arg(minFaceArea));

    // Display dialog
    UtilityOutputDialog dlg(tr("Mesh Check Results"),
        tr("Output of checkMesh:"), messageStrings, logText, this);
    dlg.exec();
}

// Run renumberMesh
void MainWindow::runMeshRenumber(const QString& casePath) {
    // Get OpenFoam path
    QString openFoamPath;
    QString caseName = QFileInfo(casePath).fileName();
    if (m_systemMgr.contains(caseName)) {
        openFoamPath = m_systemMgr.getData(caseName).openFoamPath;
    } else {
        qWarning() << "Case" << caseName << "is not in case map.";
        return;
    }

    // Make sure renumberMesh is present
    QMap<QString, bool> utilMap = m_utilMap[openFoamPath];
    if (!utilMap.value("renumberMesh", false)) {
        QMessageBox::warning(this, tr("Utility Not Found"),
                        tr("The renumberMesh utility could not be found."));
        return;
    }

    // Run renumberMesh
    QString cmd = QString("cd \"%1\"; source %2/etc/bashrc; "
                          "renumberMesh -constant -overwrite")
                      .arg(casePath, openFoamPath);

    // Execute the tool
    QString result;
    m_systemMgr.getSystem(caseName)->launchShortUtility(cmd, result);

    // Parse and display the result
    QString meshSize = "Unknown";
    QString beforeBand = "Unknown";
    QString beforeProfile = "Unknown";
    QString afterBand = "Unknown";
    QString afterProfile = "Unknown";

    // Extract Mesh Size
    QRegularExpression sizeRegex("Mesh\\s+.*?size:\\s+(\\d+)");
    QRegularExpressionMatch sizeMatch = sizeRegex.match(result);
    if (sizeMatch.hasMatch()) {
        meshSize = sizeMatch.captured(1).trimmed();
    }

    // Extract "Before renumbering" statistics
    QRegularExpression beforeRegex("Before renumbering\\s+band\\s+:\\s+(\\S+)"
                                   "\\s+profile\\s+:\\s+(\\S+)");
    QRegularExpressionMatch beforeMatch = beforeRegex.match(result);
    if (beforeMatch.hasMatch()) {
        beforeBand = beforeMatch.captured(1).trimmed();
        beforeProfile = beforeMatch.captured(2).trimmed();
    }

    // Extract "After renumbering" statistics
    QRegularExpression afterRegex("After renumbering\\s+band\\s+:\\s+(\\S+)"
                                  "\\s+profile\\s+:\\s+(\\S+)");
    QRegularExpressionMatch afterMatch = afterRegex.match(result);
    if (afterMatch.hasMatch()) {
        afterBand = afterMatch.captured(1).trimmed();
        afterProfile = afterMatch.captured(2).trimmed();
    }

    // Construct the three strings
    std::vector<QString> messageStrings;
    messageStrings.push_back(QString("Mesh size: %1\n").arg(meshSize));
    messageStrings.push_back(QString("Before renumbering:"));
    messageStrings.push_back(QString("     band: %1").arg(beforeBand));
    messageStrings.push_back(QString("     profile: %1\n").arg(beforeProfile));
    messageStrings.push_back(QString("After renumbering:"));
    messageStrings.push_back(QString("     band: %1").arg(afterBand));
    messageStrings.push_back(QString("     profile: %1\n").arg(afterProfile));

    // Display dialog
    UtilityOutputDialog dlg(tr("Renumber Results"),
                            tr("Output of renumberMesh:"),
                            messageStrings, result, this);
    dlg.exec();
}

// Run surfacePatch
void MainWindow::runSurfacePatch(double angle, const QString& fullPath,
                                 bool isBinary) {
    // Variables for command string
    QFileInfo info(fullPath);
    QString path = info.path();
    QString fileName = info.fileName();
    QString stem = info.completeBaseName();
    QString tmpName = info.completeBaseName() + "_tmp." + info.suffix();
    QString cmd;

    // Get the OpenFOAM path
    QString casePath, caseName, openFoamPath;
    int index = fullPath.lastIndexOf("/constant");
    if (index != -1) {
        casePath = fullPath.left(index);
    } else {
        qWarning() << "Geometry file is not in a valid directory.";
        return;
    }

    caseName = QFileInfo(casePath).fileName();
    if (m_systemMgr.contains(caseName)) {
        openFoamPath = m_systemMgr.getData(caseName).openFoamPath;
    } else {
        qWarning() << "Case" << caseName << "is not in case map.";
        return;
    }
    QMap<QString, bool> utilMap = m_utilMap[openFoamPath];

    // Run surfaceAutoPatch if present
    if (utilMap.value("surfaceAutoPatch", false)) {
        /*
        if (isBinary) {
            QString symlinkName = info.completeBaseName() + ".stlb";
            cmd = QString("cd \"%1\" && ln -s \"%2\" \"%3\" && "
                          "surfaceAutoPatch \"%3\" %4 \"%5\"; rm -f \"%3\"")
            .arg(path, fileName, symlinkName, QString::number(angle), tmpName);
        } else {
            cmd = QString("cd \"%1\" && surfaceAutoPatch \"%2\" %3 \"%4\"")
            .arg(path, fileName, QString::number(angle), tmpName);
        }
        */
        // Run surfaceAutoPatch
        cmd = QString("cd \"%1\"; source %2/etc/bashrc; "
                "surfaceAutoPatch %3 %4 %5").arg(casePath, openFoamPath,
                       fullPath, fullPath, QString::number(angle));
        QString result;
        m_systemMgr.getSystem(caseName)->launchShortUtility(cmd, result);

        // Display dialog containing result
        QString title;
        std::vector<QString> messages;
        if (result.contains("Feature set:")) {
            title = tr("Output of the surfaceAutoPatch utiilty:");
            for (auto line : QStringTokenizer(result, u'\n')) {
                line = line.trimmed();
                if ((line.startsWith(u"feature")) ||
                    (line.startsWith(u"region")) ||
                    (line.startsWith(u"external")) ||
                    (line.startsWith(u"internal"))) {
                    messages.push_back(line.toString() + "\n");
                }
            }
        } else {
            title = tr("The surfaceAutoPatch utility failed to produce "
                       "acceptable results");
        }

        // Display dialog
        UtilityOutputDialog dlg(tr("Surface Patch Results"), title, messages,
                                result, this);
        dlg.exec();
    } else if (utilMap.value("surfacePatch", false)) {
        // Create surfacePatchDict
        QString dictText =
            CaseIO::createSurfacePatchDict(openFoamPath, fileName, angle);
        m_systemMgr.getSystem(caseName)->writeData(
            dictText.toUtf8(), casePath + "/system/surfacePatchDict");

        // Run surfacePatch
        cmd = QString("cd \"%1\"; source %2/etc/bashrc; surfacePatch").
              arg(casePath, openFoamPath);
        QString result;
        m_systemMgr.getSystem(caseName)->launchShortUtility(cmd, result);

        // Check if a new file has been created
        if (result.contains("Writing repatched surface")) {
            // Read patched file content
            QString newPath = path + "/" + stem + "_patched.stl";
            QByteArray newData =
                m_systemMgr.getSystem(caseName)->getFileContent(newPath);

            if (newData.size() > 0) {
                // Create new RenderData
                std::pair<RenderData, bool> res =
                    StlReader::readStlFile(fileName, newData);
                RenderData mesh = res.first;
                std::shared_ptr<RenderData> meshData =
                    std::make_shared<RenderData>(std::move(mesh));

                // Pass data to SurfaceEditor
                SurfaceEditor* editor =
                    qobject_cast<SurfaceEditor*>(m_tabWidget->currentWidget());
                editor->updateModel(meshData);
            }
        } else if (result.contains("unchanged")) {
            QMessageBox::warning(this, tr("No Patches Generated"),
             tr("The surfacePatch utility didn't generate any patches.\n\n"
            "You may want to reduce the angle or update surfacePatchDict."));
        }
    }
}

void MainWindow::runSurfaceCheck(const QString& fullPath, bool isBinary) {
    // Variables for command string
    QFileInfo info(fullPath);
    QString path = info.path();
    QString fileName = info.fileName();

    // Get the OpenFOAM path
    QString casePath, caseName, openFoamPath;
    int index = fullPath.lastIndexOf("/constant");
    if (index != -1) {
        casePath = fullPath.left(index);
    } else {
        qWarning() << "Geometry file is not in a valid directory.";
        return;
    }

    caseName = QFileInfo(casePath).fileName();
    if (m_systemMgr.contains(caseName)) {
        openFoamPath = m_systemMgr.getData(caseName).openFoamPath;
    } else {
        qWarning() << "Case" << caseName << "is not in case map.";
        return;
    }

    // Access utility map
    QMap<QString, bool> utilMap = m_utilMap[openFoamPath];
    if (!utilMap.value("surfaceCheck", true)) {
        qDebug() << "The surfaceCheck utility can't be found";
        return;
    }

    // Surround path in double quotes
    QString cmd;
    QString safePath = QString("\"%1\"").arg(fullPath);
    if (isBinary) {
        QString symlinkPath = QString("\"%1b\"").arg(fullPath);
        cmd = QString("ln -s %1 %2 && surfaceCheck %2 ; rm -f %2")
                  .arg(safePath, symlinkPath);
    } else {
        cmd = QString("source %1/etc/bashrc; surfaceCheck %2").
              arg(openFoamPath, safePath);
    }

    // Get output
    QString result;
    m_systemMgr.getSystem(caseName)->launchShortUtility(cmd, result);
    if (result.isEmpty())
        return;

    // Display dialog containing result
    std::vector<QString> messageStrings;
    for (auto line : QStringTokenizer(result, u'\n')) {
        if ((line.startsWith(u"Surface")) || (line.startsWith(u"Number"))) {
            messageStrings.push_back(line.toString() + "\n");
        }
    }

    // Display dialog
    UtilityOutputDialog dlg(tr("Surface Check Results"),
    tr("Output of the surfaceCheck utiilty:"), messageStrings, result, this);
    dlg.exec();
}

// Run surfacePatch
void MainWindow::runSurfaceScale(double scaleFactor, const QString& fullPath) {
    // Variables for command string
    QFileInfo info(fullPath);
    QString path = info.path();
    QString fileName = info.fileName();
    QString stem = info.completeBaseName();
    QString tmpName = info.completeBaseName() + "_tmp." + info.suffix();

    // Get the OpenFOAM path
    QString casePath, caseName, openFoamPath;
    int index = fullPath.lastIndexOf("/constant");
    if (index != -1) {
        casePath = fullPath.left(index);
    } else {
        qWarning() << "Geometry file is not in a valid directory.";
        return;
    }
    caseName = QFileInfo(casePath).fileName();
    if (m_systemMgr.contains(caseName)) {
        openFoamPath = m_systemMgr.getData(caseName).openFoamPath;
    }

    // Determine which OpenFOAM release is used
    QString dirName = QDir(openFoamPath).dirName();
    const QRegularExpression foundationRegex("^openfoam\\d{2}$",
        QRegularExpression::CaseInsensitiveOption);
    bool isFoundation = foundationRegex.match(dirName).hasMatch();

    // Create command
    QString cmd;
    if (isFoundation) {
        cmd = QString("source %1/etc/bashrc; cd \"%2\"; "
        "surfaceTransformPoints \"scale=(%3 %3 %3)\" \"%4\" \"%4\"")
        .arg(openFoamPath, path, QString::number(scaleFactor), fileName);
    } else {
        cmd = QString("source %1/etc/bashrc; cd \"%2\"; "
        "surfaceTransformPoints -scale \"(%3 %3 %3)\" \"%4\" \"%4\"")
        .arg(openFoamPath, path, QString::number(scaleFactor), fileName);
    }

    // Execute command
    QString result;
    m_systemMgr.getSystem(caseName)->launchShortUtility(cmd, result);

    // Display message
    if ((isFoundation && result.contains("Writing surf")) ||
        (!isFoundation && result.contains("uniformly"))) {
        // Update bounds in editor
        SurfaceEditor* editor =
            qobject_cast<SurfaceEditor*>(m_tabWidget->currentWidget());
        editor->changeBounds(scaleFactor);

        QMessageBox::information(this, tr("Operation Successful"),
            tr("The scale operation completed successfully."));
    } else {
        QMessageBox::information(this, tr("Operation Unsuccessful"),
            tr("The scale operation did not complete successfully."));
    }
    return;
}

QString MainWindow::getSelectedCase() {
    // Get selected case from the navigator
    QString selectedCase = m_navigator->getSelectedCase();
    if (selectedCase.isEmpty()) {
        // Choose the case associated with the current tab
        if (m_tabWidget->count() > 0) {
            int index = m_tabWidget->currentIndex();
            QString tabPath = m_tabWidget->tabBar()->tabData(index).toString();
            selectedCase = tabPath.split("/")[0];
        } else {
            QStringList cases = m_navigator->getCases();
            selectedCase = cases.back();
        }
    }
    return selectedCase;
}

void MainWindow::longUtilityFinished(const QString& status,
                    const QString& caseName, UtilityType utilityType) {
    // Refresh navigator
    switch (utilityType) {
    case UtilityType::MESH:
        updatePath(caseName, "constant/polyMesh");
        updatePath(caseName, "system");
        break;
    case UtilityType::SOLVER:
        updatePath(caseName, "");
    case UtilityType::POSTPROCESS:
        updatePath(caseName, "");
        break;
    }
}

void MainWindow::loadSolverFamilies() {
    // Load data from solvers.json
    QFile file(m_configDir.filePath("solvers.json"));
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        // Check if the root is an array
        if (doc.isArray()) {
            QJsonArray rootArray = doc.array();
            for (const QJsonValue& value : std::as_const(rootArray)) {
                if (value.isObject()) {
                    QJsonObject obj = value.toObject();
                    FlowCompute::SolverFamily family;
                    family.name = obj["category"].toString();
                    QJsonArray solverArray = obj["solvers"].toArray();
                    for (const QJsonValue& solverVal :
                        std::as_const(solverArray)) {
                        // Read object
                        if (solverVal.isObject()) {
                            QJsonObject solverObj = solverVal.toObject();
                            FlowCompute::SolverDef solverDef;
                            solverDef.name = solverObj["name"].toString();
                            QString algoStr = solverObj["algorithm"].toString();
                            solverDef.algorithm = FlowCompute::Algorithm(
                                QMetaEnum::fromType<FlowCompute::Algorithm>().
                                    keyToValue(algoStr.toUtf8().constData()));
                            solverDef.fields =
                                solverObj["fields"].toVariant().toStringList();
                            solverDef.transportProperties =
                                solverObj["transportProperties"].toVariant().
                                                            toStringList();
                            if (solverObj.contains("thermalProperties")) {
                                solverDef.thermalProperties =
                                    solverObj["thermalProperties"].toVariant().
                                                              toStringList();
                            } else {
                                solverDef.thermalProperties = {};
                            }
                            solverDef.isSteadyState =
                                solverObj["is_steady_state"].toBool(
                                    solverDef.isSteadyState);
                            family.solvers.append(solverDef);
                        }
                    }
                    m_solverFamilies.push_back(family);
                }
            }
        }
        file.close();
    }
}

void MainWindow::loadMaterialProperties() {
    // Load data from material_properties.json
    QFile file(m_configDir.filePath("material_properties.json"));
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        // Ensure the root of the JSON document is an object
        if (doc.isObject()) {
            QJsonObject rootObj = doc.object();

            // Clear existing data
            m_transportProperties.clear();

            // Check the transportProperties object
            if (rootObj.contains("transportProperties") &&
                rootObj["transportProperties"].isObject()) {
                QJsonObject transportObj =
                    rootObj["transportProperties"].toObject();

                // Iterate through items
                for (auto it = transportObj.constBegin();
                     it != transportObj.constEnd(); ++it) {
                    QString transportPropertyName = it.key();

                    if (it.value().isObject()) {
                        QJsonObject propDetails = it.value().toObject();
                        FlowCompute::TransportPropertyDef propertyDef;

                        // Extract fields
                        propertyDef.name = propDetails["name"].toString();
                        propertyDef.dimensions =
                            propDetails["dimensions"].toString();
                        propertyDef.defaultVal =
                            propDetails["default"].toString();
                        m_transportProperties[transportPropertyName] =
                            propertyDef;
                    } else {
                        qWarning() << "Value for property" <<
                            transportPropertyName << "is not a valid object.";
                    }
                }
            } else {
                qWarning() << "'transportProperties' object is missing or "
                              "invalid in material_properties.json.";
            }

        } else {
            qWarning() << "Failed to parse material_properties.json: "
                          "Root is not a JSON Object.";
        }
        file.close();
    } else {
        qWarning() << "Failed to open material_properties.json at:" <<
            file.fileName();
    }
}

void MainWindow::loadTurbulenceModels() {
    // Load data from turbulence.json
    QFile file(m_configDir.filePath("turbulence.json"));
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        // Ensure the root of the JSON document is an object
        if (doc.isObject()) {
            QJsonObject rootObj = doc.object();

            // Clear any existing data before repopulating
            m_turbulenceModels.clear();

            // Iterate through categories
            for (auto catIt = rootObj.constBegin();
                 catIt != rootObj.constEnd(); ++catIt) {
                QString categoryName = catIt.key();
                QJsonObject subCatObj = catIt.value().toObject();

                QMap<QString, std::vector<FlowCompute::TurbulenceModel>> subMap;

                // Iterate through subcategories
                for (auto subCatIt = subCatObj.constBegin();
                     subCatIt != subCatObj.constEnd(); ++subCatIt) {
                    QString subCategoryName = subCatIt.key();
                    QJsonObject modelsObj = subCatIt.value().toObject();

                    std::vector<FlowCompute::TurbulenceModel> modelList;

                    // Iterate through models
                    for (auto modelIt = modelsObj.constBegin();
                           modelIt != modelsObj.constEnd(); ++modelIt) {
                        QString modelName = modelIt.key();
                        QJsonObject modelDataObj = modelIt.value().toObject();

                        FlowCompute::TurbulenceModel model;
                        model.name = modelName;
                        model.description =
                            modelDataObj["description"].toString();
                        model.fields =
                            modelDataObj["fields"].toVariant().toStringList();
                        modelList.push_back(model);
                    }

                    // Insert the populated list into the subcategory map
                    subMap.insert(subCategoryName, modelList);
                }

                // Insert the subcategory map into the main category map
                m_turbulenceModels.insert(categoryName, subMap);
            }
        } else {
            qWarning() << "Failed to parse turbulence.json: root not object.";
        }
        file.close();
    } else {
        qWarning() << "Failed to open turbulence.json at:" << file.fileName();
    }
}

void MainWindow::loadFieldData() {
    // Load data from fields.json
    QFile file(m_configDir.filePath("fields.json"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    // Parse the JSON document
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return;
    }

    if (!jsonDoc.isObject()) {
        qWarning() << "Root JSON element is not an object.";
        return;
    }

    // Iterate through the root object and populate the QHash
    QJsonObject rootObj = jsonDoc.object();
    for (auto it = rootObj.constBegin(); it != rootObj.constEnd(); ++it) {
        QString fieldName = it.key();
        QJsonObject fieldObj = it.value().toObject();

        FlowCompute::FieldDef data;
        QString classStr = fieldObj.value("class").toString();
        data.fieldClass =
            FlowCompute::FieldClass(
                QMetaEnum::fromType<FlowCompute::FieldClass>().
                    keyToValue(classStr.toUtf8().constData()));
        data.dimensions = fieldObj.value("dimensions").toString();

        // Handle default value conversion
        QJsonValue defValue = fieldObj.value("default");
        if (defValue.isArray()) {
            QJsonArray arr = defValue.toArray();
            QStringList components;
            for (const QJsonValue& val : std::as_const(arr)) {
                components << QString::number(val.toDouble());
            }
            data.defaultValue = "(" + components.join(" ") + ")";
        } else {
            data.defaultValue = QString::number(defValue.toDouble());
        }

        m_fieldData.insert(fieldName, data);
    }
}

void MainWindow::loadBoundaryConditions() {
    // Load data from boundary_conditions.json
    QFile file(m_configDir.filePath("boundary_conditions.json"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    // 2. Parse the JSON document
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return;
    }

    // 3. Ensure the root is an array
    if (!jsonDoc.isArray()) {
        qWarning() << "Root JSON element is not an array.";
        return;
    }

    // A helper lambda to keep the array conversion DRY
    auto arrayToStringList = [](const QJsonArray& jsonArray) {
        QStringList list;
        for (const QJsonValue& val : jsonArray) {
            list.append(val.toString());
        }
        return list;
    };

    // 4. Iterate through the array and populate the structs
    QJsonArray rootArray = jsonDoc.array();
    for (const QJsonValue& itemVal : std::as_const(rootArray)) {
        if (!itemVal.isObject()) continue;

        QJsonObject obj = itemVal.toObject();
        FlowCompute::BoundaryConditionDef bc;

        bc.name = obj.value("name").toString();

        // Convert the nested JSON arrays into QStringLists
        bc.categories = arrayToStringList(obj.value("categories").toArray());
        bc.types      = arrayToStringList(obj.value("types").toArray());
        bc.patchTypes = arrayToStringList(obj.value("patchTypes").toArray());
        bc.parameters = arrayToStringList(obj.value("parameters").toArray());
        m_boundaryConditions.push_back(bc);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Check tabs
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (!m_tabWidget->promptToSave(i)) {
            return;
        }
    }

    // Access settings
    QSettings settings;
    settings.setValue("Theme/file", m_themeFile);

    // Save open tabs to settings
    settings.remove("Tabs");
    settings.beginWriteArray("Tabs");

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        settings.setArrayIndex(i);
        QString fileName = m_tabWidget->tabText(i);
        TabData data = m_tabMap.value(fileName);

        // Save values to settings
        settings.setValue("tabName", fileName);
        settings.setValue("fullPath", data.fullPath);
        settings.setValue("type", static_cast<int>(data.type));
    }
    settings.endArray();
}
