#include "main_window.h"

#include "dialogs/mesh/wizard_mesh.h"
#include "dialogs/run_mesh/run_mesh_dialog.h"
#include "dialogs/selection/selection_dialog.h"
#include "dialogs/solver/wizard_solver.h"
#include "dialogs/utility_output/utility_output_dialog.h"

#include "editors/text/text_editor.h"
#include "editors/graphical/model_editor/model_editor.h"
#include "editors/graphical/mesh_editor/mesh_editor.h"
#include "geometry/mesh/mesh_reader.h"
#include "geometry/stl/stl_reader.h"
#include "utils.h"

#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QProgressBar>
#include <QProgressDialog>
#include <QStatusBar>

bool MainWindow::s_isWindows = false;
bool MainWindow::s_isWslAvailable = false;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {

    // Check environment
#ifdef Q_OS_WIN
    s_isWindows = true;

    // Check for wsl.exe
    QString wslPath = QStandardPaths::findExecutable("wsl.exe");
    s_isWslAvailable = !wslPath.isEmpty();
#endif

    // Set title and icon
    setStyleSheet(
        "QWidget { background-color: #F4F1EA; }"
        "QComboBox, QLineEdit, QTextEdit, QPlainTextEdit { background-color: white; }"
        "QComboBox QAbstractItemView { background-color: white; }"
        "QListWidget { background-color: white; }"
        "QDoubleSpinBox { background-color: white; }"
        "QSpinBox { background-color: white; }"
        "QGroupBox { border: 1px solid gray; margin-top: 8px; padding-top: 10px; padding-bottom: 10px; padding-left: 15px; padding-right: 15px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 5px; font: bold; }"
        "QTreeWidget { background-color: white; }"
        "QTreeView { background-color: white; }"
        );
    menuBar()->setStyleSheet("QMenuBar { color: black; }");
    setWindowTitle("FlowCompute 1.0.0");

    // Configure font
    font = QFont();
    font.setPointSize(13);
    font.setFamily("Courier New");

    // Create communication systems
    targetSystems[0] = &wslSystem;
    connect(&wslSystem, &WslSystem::longUtilityOutputReceived,
            this, &MainWindow::log);
    connect(&wslSystem, &WslSystem::longUtilityFinished,
            this, &MainWindow::log);
    connect(&wslSystem, &WslSystem::longUtilityError,
            this, &MainWindow::log);

    // Create actions, menus, and toolbars
    createActions();
    createMenus();
    createToolBar();

    // Create tab widget
    tabWidget = new TabWidget(this);
    setCentralWidget(tabWidget);

    // Create the navigator
    navigatorWidget = new QDockWidget(tr("Case Navigator"), this);
    navigatorWidget->setTitleBarWidget(new QWidget(navigatorWidget));
    navigatorWidget->setAllowedAreas(Qt::LeftDockWidgetArea);
    navigator = new CaseNavigator(this);
    navigatorWidget->setWidget(navigator);
    navigatorWidget->setMinimumWidth(200);
    addDockWidget(Qt::LeftDockWidgetArea, navigatorWidget);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

    // Populate caseMap and navigator from settings
    QSettings settings;
    m_caseMap.clear();
    int caseCount = settings.beginReadArray("Cases");
    for (int i = 0; i < caseCount; ++i) {
        settings.setArrayIndex(i);

        // Read settings
        QString caseName = settings.value("caseName").toString();
        CaseData data;
        data.casePath = settings.value("casePath").toString();
        data.caseFiles = settings.value("caseFiles").toStringList();
        data.targetSystemId = settings.value("targetSystemId", 0).toInt();
        data.openFoamPath = settings.value("openFoamPath").toString();
        m_caseMap.insert(caseName, data);

        // Update utility availability
        if (!m_utilMap.contains(data.openFoamPath)) {
            QString path = data.casePath + "/" + caseName + "/";
            m_utilMap[data.openFoamPath] = checkUtilities(path, data.targetSystemId, m_utilities);
        }

        // Update navigator
        navigator->addCase(caseName, data.caseFiles);
        navigator->expandCase(caseName);
    }
    settings.endArray();

    // Populate tabMap and tabs from settings
    tabMap.clear();
    int tabCount = settings.beginReadArray("Tabs");
    for (int i = 0; i < tabCount; ++i) {
        settings.setArrayIndex(i);

        // Update map
        QString tabName = settings.value("tabName").toString();
        TabData data;
        data.type = static_cast<EditorType>(settings.value("type").toInt());
        data.fullPath = settings.value("fullPath").toString();
        tabMap.insert(tabName, data);

        // Create editor
        createEditor(data.type, tabName, data.fullPath);
    }
    settings.endArray();

    // Create the console
    consoleWidget = new QDockWidget(tr("Console"), this);
    consoleWidget->setTitleBarWidget(new QWidget(consoleWidget));
    consoleWidget->setAllowedAreas(Qt::BottomDockWidgetArea);
    console = new Console(this);
    consoleWidget->setWidget(console);
    consoleWidget->setMinimumHeight(150);
    addDockWidget(Qt::BottomDockWidgetArea, consoleWidget);

    // Create Vulkan instance
    m_vulkanInstance.setLayers({ "VK_LAYER_KHRONOS_validation" });
    m_vulkanInstance.setApiVersion(QVersionNumber(1, 2));
    if (!m_vulkanInstance.create()) {
        qFatal("Failed to create Vulkan instance: %d", m_vulkanInstance.errorCode());
    }

    // Configure undo/redo actions
    connect(tabWidget, &QTabWidget::currentChanged, this, [=, this](int index) {
        TextEditor* editor = qobject_cast<TextEditor*>(tabWidget->widget(index));
        if (editor) {
            undoAction->setEnabled(editor->document()->isUndoAvailable());
            redoAction->setEnabled(editor->document()->isRedoAvailable());
        } else {
            undoAction->setEnabled(false);
            redoAction->setEnabled(false);
        }
    });

    // Create status bar
    statusBar()->showMessage("Ready");

    // Configure tab save
    connect(tabWidget, &TabWidget::saveTab, this, &MainWindow::saveFile);

    // Load data from JSON configuration files
    loadSolverFamilies();
    loadTurbulenceModels();
    loadFieldData();
    loadBoundaryConditions();
}

MainWindow::~MainWindow() = default;

/* Create QActions for file operations */
void MainWindow::createActions() {

    // Create new case
    newCaseAction = new QAction(QIcon(":/images/new_case.png"), tr("&New Case Folder"), this);
    newCaseAction->setShortcuts(QKeySequence::New);
    newCaseAction->setStatusTip(tr("Create a new case folder"));
    connect(newCaseAction, &QAction::triggered, this, &MainWindow::runNewCaseWizard);

    // Save file
    saveFileAction = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveFileAction->setShortcuts(QKeySequence::Save);
    saveFileAction->setStatusTip(tr("Save the file"));
    connect(saveFileAction, &QAction::triggered, this, &MainWindow::saveFile);
    saveFileAction->setDisabled(true);

    /*
    // Create new file
    new_fileAction = new QAction(QIcon(image_dir + "new.png"), tr("New &File"), this);
    new_fileAction->setShortcuts(QKeySequence::New);
    new_fileAction->setStatusTip(tr("Create a new file"));
    connect(new_fileAction, &QAction::triggered, this, SLOT(NewFile()));

    // Open file
    open_fileAction = new QAction(QIcon(image_dir + "open.png"), tr("&Open..."), this);
    open_fileAction->setShortcuts(QKeySequence::Open);
    open_fileAction->setStatusTip(tr("Open an existing project"));
    connect(open_fileAction, &QAction::triggered, this, SLOT(Open()));

    // Save as
    save_asAction = new QAction(tr("Save &As..."), this);
    save_asAction->setShortcuts(QKeySequence::SaveAs);
    save_asAction->setStatusTip(tr("Save the document under a new name"));
    connect(save_asAction, &QAction::triggered, this, SLOT(SaveAs()));

    // Print
    printAction = new QAction(QIcon(image_dir + "print.png"), tr("&Print"), this);
    printAction->setShortcuts(QKeySequence::Print);
    printAction->setStatusTip(tr("Send the document to a printer"));
    connect(printAction, &QAction::triggered, this, SLOT(Print()));

    // Exit
    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcuts(QKeySequence::Quit);
    exitAction->setStatusTip(tr("Exit the application"));
    connect(exitAction, &QAction::triggered, this, SLOT(close()));

    // Cut
    cutAction = new QAction(QIcon(image_dir + "cut.png"), tr("Cut"), this);
    cutAction->setStatusTip(tr("Cut"));
    connect(cutAction, &QAction::triggered, this, SLOT(Cut()));

    // Copy
    copyAction = new QAction(QIcon(image_dir + "copy.png"), tr("Copy"), this);
    copyAction->setStatusTip(tr("Copy"));
    //connect(copyAction, &QAction::triggered, this, SLOT(copy()));

    // Paste
    pasteAction = new QAction(QIcon(image_dir + "paste.png"), tr("Paste"), this);
    pasteAction->setStatusTip(tr("Paste"));
    connect(pasteAction, &QAction::triggered, this, SLOT(Paste()));
    */

    // Create undo action
    undoAction = new QAction(QIcon(":/images/undo.png"), tr("Undo"), this);
    undoAction->setShortcuts(QKeySequence::Undo);
    undoAction->setStatusTip(tr("Zoom in"));
    connect(undoAction, &QAction::triggered, this, &MainWindow::undo);
    undoAction->setDisabled(true);

    // Create redo action
    redoAction = new QAction(QIcon(":/images/redo.png"), tr("Redo"), this);
    redoAction->setShortcuts(QKeySequence::Redo);
    redoAction->setStatusTip(tr("Zoom out"));
    connect(redoAction, &QAction::triggered, this, &MainWindow::redo);
    redoAction->setDisabled(true);

    // Create zoom in action
    zoomInAction = new QAction(QIcon(":/images/zoom_in.png"), tr("Zoom in"), this);
    zoomInAction->setShortcuts(QKeySequence::ZoomIn);
    zoomInAction->setStatusTip(tr("Zoom in"));

    // Create zoom out action
    zoomOutAction = new QAction(QIcon(":/images/zoom_out.png"), tr("Zoom out"), this);
    zoomOutAction->setShortcuts(QKeySequence::ZoomOut);
    zoomOutAction->setStatusTip(tr("Zoom out"));

    // Configure the mesh action in the Mesh menu
    meshAction = new QAction(QIcon(":/images/mesh.png"), tr("&Mesh Configuration"), this);
    meshAction->setStatusTip(tr("Configure mesh process"));
    connect(meshAction, &QAction::triggered, this, &MainWindow::runMeshConfiguration);

    // Configure the mesh action in the Mesh menu
    runMeshAction = new QAction(QIcon(":/images/run_mesh.png"), tr("Mesh &Execution"), this);
    runMeshAction->setStatusTip(tr("Run mesh utilities"));
    connect(runMeshAction, &QAction::triggered, this, &MainWindow::runMeshExecution);

    // Configure the run action in the Solver menu
    runAction = new QAction(QIcon(":/images/play.png"), tr("&Launch solver"), this);
    runAction->setStatusTip(tr("Launch solver"));
    connect(runAction, &QAction::triggered, this, &MainWindow::runSolverWizard);

    // Configure the stop action in the Solver menu
    stopAction = new QAction(QIcon(":/images/stop.png"), tr("&Stop solver"), this);
    stopAction->setStatusTip(tr("Stop solver"));
    stopAction->setEnabled(false);
    // connect(meshAction, &QAction::triggered, this, SLOT(About()));

    // Configure the mesh action in the Mesh menu
    postProcessAction = new QAction(QIcon(":/images/post_process.png"), tr("&Post-process"), this);
    postProcessAction->setStatusTip(tr("Launch post-processing"));
    // connect(meshAction, &QAction::triggered, this, SLOT(About()));

    // Configure the About action in the help menu
    aboutAction = new QAction(QIcon(":/images/help.png"), tr("&Help"), this);
    aboutAction->setStatusTip(tr("Provide assistance"));
    // connect(aboutAction, &QAction::triggered, this, SLOT(About()));
}

// Assemble actions within main menu
void MainWindow::createMenus() {

    // Create file menu
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newCaseAction);
    fileMenu->addAction(saveFileAction);
    /*
    fileMenu->addAction(new_fileAction);
    fileMenu->addAction(open_fileAction);
    fileMenu->addAction(save_asAction);
    fileMenu->addSeparator();
    fileMenu->addAction(printAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);
    */
    menuBar()->addSeparator();

    // Create edit menu
    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);
    /*
    editMenu->addAction(cutAction);
    editMenu->addAction(copyAction);
    editMenu->addAction(pasteAction);
    */
    menuBar()->addSeparator();

    // Create view menu
    viewMenu = menuBar()->addMenu(tr("&View"));
    /*
    viewMenu->addAction(zoom_inAction);
    viewMenu->addAction(zoom_outAction);
    */
    menuBar()->addSeparator();

    // Create mesh menu
    meshMenu = menuBar()->addMenu(tr("&Mesh"));
    meshMenu->addAction(meshAction);
    meshMenu->addAction(runMeshAction);
    menuBar()->addSeparator();

    // Create solve menu
    solveMenu = menuBar()->addMenu(tr("&Solve"));
    menuBar()->addSeparator();

    // Create post-process menu
    postProcessMenu = menuBar()->addMenu(tr("&PostProcess"));
    menuBar()->addSeparator();

    // Create help menu
    helpMenu = menuBar()->addMenu(tr("&Help"));
    // helpMenu->addAction(aboutAction);
}

// Add entries to toolbars
void MainWindow::createToolBar() {

    // Create tool bar
    toolBar = addToolBar(tr("Toolbar"));
    toolBar->addAction(newCaseAction);
    toolBar->addAction(saveFileAction);
    /*
    toolBar->addAction(new_fileAction);
    toolBar->addAction(open_fileAction);

    toolBar->addAction(printAction);
    */

    // Create tool bar with view actions
    toolBar->addSeparator();
    toolBar->addAction(undoAction);
    toolBar->addAction(redoAction);
    toolBar->addSeparator();
    toolBar->addAction(zoomInAction);
    toolBar->addAction(zoomOutAction);
    toolBar->addSeparator();

    // Create tool bar with mesh actions
    toolBar->addAction(meshAction);
    toolBar->addAction(runMeshAction);
    toolBar->addSeparator();

    // Create tool bar with solver actions
    toolBar->addAction(runAction);
    toolBar->addAction(stopAction);
    toolBar->addSeparator();

    // Create tool bar with view actions
    toolBar->addAction(postProcessAction);

    // Add help actions
    toolBar->addAction(aboutAction);
}

void MainWindow::createEditor(EditorType type, QString& fileName,
                              const QString& fullPath) {

    // Get case name
    QString caseName;
    if (type != EditorType::MESH) {
        caseName = fullPath.split('/').first();
    } else {
        caseName = fileName;
    }
    CaseData caseData = m_caseMap[caseName];
    int targetId = caseData.targetSystemId;
    QString casePath = caseData.casePath + "/" + caseName;
    QString path = caseData.casePath + "/" + fullPath + "/" + fileName;
    QString openFoamPath = caseData.openFoamPath;
    int tabIndex;
    TabData tabData;
    QByteArray data;

    // Read data
    if (type != EditorType::MESH) {
        data = targetSystems[targetId]->getFileContent(path);
    }

    // Check to see if there's already an editor
    if (tabMap.contains(fileName)) {
        tabData = tabMap[fileName];
        if (tabData.fullPath == fullPath) {

            // Iterate through tabs
            for (int i = 0; i < tabWidget->count(); ++i) {
                QString tabName = tabWidget->tabText(i);
                QString tabPath = tabWidget->tabBar()->tabData(i).toString();
                if (tabName == fileName && tabPath == fullPath) {
                    tabWidget->setCurrentIndex(i);
                    return;
                }
            }
        }
    }

    // Update tabMap
    tabData.fullPath = fullPath;
    tabData.type = type;
    tabMap.insert(fileName, tabData);

    // Create text editor
    if (type == EditorType::TEXT) {

        // Create new tab
        TextEditor* textEditor = new TextEditor(this);
        textEditor->setFont(font);
        textEditor->setTextData(data);
        tabIndex = tabWidget->addTab(textEditor, fileName);
        tabWidget->setCurrentIndex(tabIndex);
        tabWidget->tabBar()->setTabData(tabIndex, fullPath);

        // Enable undo and redo actions according to the editor
        connect(textEditor->document(), &QTextDocument::undoAvailable,
                undoAction, &QAction::setEnabled);
        connect(textEditor->document(), &QTextDocument::redoAvailable,
                redoAction, &QAction::setEnabled);

        // Configure event handling
        connect(textEditor, &TextEditor::dirtyStateChanged,
                this, [this, textEditor](bool isDirty) {
            onDirtyStateChanged(isDirty, textEditor);
        });

        // Configure the save action
        connect(tabWidget, &QTabWidget::currentChanged, this, [=, this](int index) {
            TextEditor* currentEditor = qobject_cast<TextEditor*>(tabWidget->widget(index));
            if (currentEditor) {
                saveFileAction->setEnabled(currentEditor->document()->isModified());
            } else {
                saveFileAction->setEnabled(false);
            }
        });
        return;
    }

    // Create editor
    if (type == EditorType::SURFACE) {

        // Convert data to RenderData structure
        bool isBinary = false;
        RenderData model;
        if(fileName.endsWith(".stl", Qt::CaseInsensitive)) {
            std::pair<RenderData, bool> res = StlReader::readStlFile(fileName, data);
            model = res.first;
            isBinary = res.second;
        }
        std::shared_ptr<RenderData> modelData = std::make_shared<RenderData>(std::move(model));

        // Create editor
        ModelEditor* modelEditor = new ModelEditor(modelData, path, targetId,
                                              &m_vulkanInstance, isBinary, this);
        tabIndex = tabWidget->addTab(modelEditor, fileName);
        connect(modelEditor, &ModelEditor::surfacePatchRequested,
                this, &MainWindow::runSurfacePatch);
        connect(modelEditor, &ModelEditor::surfaceCheckRequested,
                this, &MainWindow::runSurfaceCheck);
        connect(modelEditor, &ModelEditor::surfaceScaleRequested,
                this, &MainWindow::runSurfaceScale);
        connect(modelEditor, &ModelEditor::dirtyStateChanged,
                this, [this, modelEditor](bool isDirty) {
                    onDirtyStateChanged(isDirty, modelEditor);
        });
    }
    if (type == EditorType::MESH) {

        QProgressDialog* progress = new QProgressDialog("Loading Mesh Data...", QString(), 0, 0, this);
        progress->setWindowModality(Qt::WindowModal);
        progress->setMinimumWidth(300);
        progress->setAttribute(Qt::WA_DeleteOnClose);
        progress->show();

        // Setup the Future Watcher
        using RenderDataPtr = std::shared_ptr<RenderData>;
        QFutureWatcher<RenderDataPtr>* watcher = new QFutureWatcher<RenderDataPtr>(this);

        // Connect the watcher's finished signal to handle the result on the MAIN thread
        connect(watcher, &QFutureWatcher<RenderDataPtr>::finished, this,
                [this, watcher, progress, casePath, targetId, fileName, fullPath]() {
            progress->close();

            // Retrieve the result from the background thread
            RenderDataPtr renderData = watcher->result();

            if (renderData) {
                MeshEditor* meshEditor = new MeshEditor(renderData,
                    casePath, targetId, m_solverFamilies,
                    m_turbulenceModels, m_fieldData, m_boundaryConditions,
                    &m_vulkanInstance, this);
                connect(meshEditor, &MeshEditor::meshPatchRequested,
                        this, &MainWindow::runMeshPatch);
                connect(meshEditor, &MeshEditor::meshCheckRequested,
                        this, &MainWindow::runMeshCheck);
                connect(meshEditor, &MeshEditor::meshRenumberRequested,
                        this, &MainWindow::runMeshRenumber);

                int tabIndex = tabWidget->addTab(meshEditor, fileName + " (mesh)");
                tabWidget->setCurrentIndex(tabIndex);
                tabWidget->tabBar()->setTabData(tabIndex, fullPath);
                undoAction->setDisabled(true);
                redoAction->setDisabled(true);
                saveFileAction->setDisabled(true);
            }

            // Clean up the watcher
            watcher->deleteLater();
        });

        // Start the background thread
        QFuture<RenderDataPtr> future =
            QtConcurrent::run(&MainWindow::getMeshData, this,
            caseName, casePath, openFoamPath, targetId);
        watcher->setFuture(future);
    }

    if (type != EditorType::MESH) {

        // Update tab widget
        tabWidget->setCurrentIndex(tabIndex);
        tabWidget->tabBar()->setTabData(tabIndex, fullPath);
        undoAction->setDisabled(true);
        redoAction->setDisabled(true);
        saveFileAction->setDisabled(true);
    }
}

std::shared_ptr<RenderData> MainWindow::getMeshData(QString caseName, QString casePath,
                             QString openFoamPath, int targetId) {

    // Convert mesh to STL file
    QByteArray data;
    QString meshFile = caseName + "_tmp.stl";
    QString cmd = QString("cd %1; source %2/etc/bashrc && foamToSurface %3")
                      .arg(casePath, openFoamPath, meshFile);

    // Read new STL file
    QString output;
    if (targetSystems[targetId]->launchShortUtility(cmd, output) == 0) {
        data = targetSystems[targetId]->getFileContent(casePath + "/" + meshFile);
    }

    // Convert data to RenderData structure
    return std::make_shared<RenderData>(MeshReader::readMesh(caseName, data));
}

void MainWindow::runMesh(const QString& caseName, bool runBlockMesh,
                         bool runSurfaceFeatureExtract, bool runSnappyHexMesh,
                         const QString& snappyCmd, int numCores, bool display) {

    CaseData caseData = m_caseMap[caseName];
    int targetId = caseData.targetSystemId;
    QString casePath = caseData.casePath + "/" + caseName;
    QString openFoamPath = caseData.openFoamPath;
    QString cmd, output;

    // Launch blockMesh
    if (runBlockMesh) {
        cmd = QString("cd %1; source %2/etc/bashrc; blockMesh").arg(casePath, openFoamPath);
        if (targetSystems[targetId]->launchShortUtility(cmd, output) == 0) {
            log(output);
        }
    }

    // Launch surfaceFeatureExtract
    if (runSurfaceFeatureExtract) {
        cmd = QString("cd %1; source %2/etc/bashrc; surfaceFeatureExtract").arg(casePath, openFoamPath);
        if (targetSystems[targetId]->launchShortUtility(cmd, output) == 0) {
            log(output);
        }
    }

    // Launch snappyHexMesh
    if (runSnappyHexMesh) {

        // Configure multicore operation
        if(numCores > 1) {

            // Create surfacePatchDict
            QString dictText = Utils::createDecomposeParDict(openFoamPath, numCores);
            targetSystems[targetId]->writeData(dictText.toUtf8(), casePath + "/system/decomposeParDict");
        }

        // Create command
        cmd = QString("cd %1; source %2/etc/bashrc; " + snappyCmd).arg(casePath, openFoamPath);
        qDebug() << cmd;
        targetSystems[targetId]->launchLongUtility(cmd);
    }

    updatePath(caseName, "constant/polyMesh", targetId);
    updatePath(caseName, "system", targetId);
}

void MainWindow::createCase(QString caseName, QString casePath, QStringList caseFiles,
                            int targetId, QString openFoamPath) {

    // Add case to map
    m_caseMap.insert(caseName, CaseData{casePath, caseFiles, targetId, openFoamPath});

    // Update utility map if necessary
    if (!m_utilMap.contains(openFoamPath)) {
        QString path = casePath + "/" + caseName + "/";
        m_utilMap[openFoamPath] = checkUtilities(path, targetId, m_utilities);
    }

    // Display case in navigator
    navigator->addCase(caseName, caseFiles);
    navigator->expandCase(caseName);
}

// Create new case folder
void MainWindow::saveFile() {

    // Remove the " *" to get the real map key
    QString rawTabText = tabWidget->tabText(tabWidget->currentIndex());
    QString fileName = rawTabText.remove(" *");

    // Look up data
    if (tabMap.contains(fileName)) {

        // Construct the remote path
        TabData tabData = tabMap[fileName];
        QString caseName = tabData.fullPath.split("/")[0];
        int targetId = m_caseMap[caseName].targetSystemId;
        QString fullPath = m_caseMap[caseName].casePath + "/" + tabData.fullPath + "/" + fileName;

        // Save data for text editor
        if (tabData.type == EditorType::TEXT) {
            TextEditor* editor = qobject_cast<TextEditor*>(tabWidget->currentWidget());
            if (editor) {
                bool save = targetSystems[targetId]->writeData(editor->toPlainText().toUtf8(),
                                                                     fullPath);
                if (save) editor->document()->setModified(false);
            }
            return;
        }

        // Save data for model editor
        if (tabData.type == EditorType::SURFACE) {
            ModelEditor* editor = qobject_cast<ModelEditor*>(tabWidget->currentWidget());
            if (editor) {

                QString output;
                QFileInfo info(fullPath);
                QString path = info.path();
                QString fileName = info.fileName();

                // Delete patched file when saved
                if(editor->isSurfacePatched()) {

                    // Delete xyz.stl and rename xyz_patched.stl to xyz.stl
                    QString patchName = info.completeBaseName() + "_patched." + info.suffix();
                    QString cmd = QString("cd %1; rm %2; mv %3 %2; ").arg(path, fileName, patchName);
                    if (targetSystems[targetId]->launchShortUtility(cmd, output) == 0) {
                        editor->setSurfaceChanged(false);
                        onDirtyStateChanged(false, editor);
                    }
                }

                // Check if patch names have changed
                std::vector<std::pair<std::string, std::string>> vec = editor->getPatchChanges();
                if (!vec.empty()) {

                    // Create command to replace old patch names with new patch names
                    QString cmd = QString("cd %1; sed -i ").arg(path);
                    for(const auto& change: vec) {
                        QString oldStr = QString::fromStdString(change.first);
                        QString newStr = QString::fromStdString(change.second);
                        cmd += QString("-e 's#%1#%2#g' ").arg(oldStr, newStr);
                    }
                    cmd += fileName;

                    // Perform text replacement
                    if (targetSystems[targetId]->launchShortUtility(cmd, output) == 0) {
                        onDirtyStateChanged(false, editor);
                    }
                }
            }
            return;
        }
    }
}

// Undo action in text editor
void MainWindow::undo() {
    TextEditor* editor = qobject_cast<TextEditor*>(tabWidget->currentWidget());
    if (editor) { editor->undo(); }
}

// Redo action in text editor
void MainWindow::redo() {
    TextEditor* editor = qobject_cast<TextEditor*>(tabWidget->currentWidget());
    if (editor) { editor->redo(); }
}

// Write text to the log
void MainWindow::log(const QString& text) {
    console->appendPlainText(text);
}

void MainWindow::updatePath(const QString& caseName, const QString& subDir, int targetId) {

    QString casePath = caseName + "/" + subDir;
    QString fullPath = m_caseMap[caseName].casePath + "/" + casePath;
    QStringList files = targetSystems[targetId]->getFiles(fullPath);
    if(!files.isEmpty()) {
        navigator->updatePath(casePath, files);
    }
}

void MainWindow::onDirtyStateChanged(bool isDirty, QWidget* widget) {

    // Get the index of the editor
    // QWidget* senderWidget = qobject_cast<QWidget*>(sender());
    if (!widget) return;
    int index = tabWidget->indexOf(widget);

    if (index != -1) {

        // Get the current tab text
        QString tabText = tabWidget->tabText(index);

        // Remove trailing " *"
        if (tabText.endsWith(" *")) {
            tabText.chop(2);
        }

        // Append the asterisk if the state is dirty
        if (isDirty) {
            tabText += " *";
        }

        tabWidget->setTabText(index, tabText);

        // 4. Update the save action if the modified file is currently being viewed
        if (tabWidget->currentIndex() == index) {
            saveFileAction->setEnabled(isDirty);
        }
    }
}

// Launch new case wizard
void MainWindow::runNewCaseWizard() {
    NewCaseWizard wizard(this);
    wizard.exec();
}

// Launch mesh configuration wizard
void MainWindow::runMeshConfiguration() {
    MeshWizard wizard(this);
    wizard.exec();
}

// Launch mesh execution wizard
void MainWindow::runMeshExecution() {
    QStringList availableCases = navigator->getCases();
    QString selectedCase = navigator->getSelectedCase();
    RunMeshDialog dialog(selectedCase, availableCases, this);
    dialog.exec();
}

// Check if utility is available
QMap<QString, bool> MainWindow::checkUtilities(const QString& fullPath, int targetId,
                                               const QStringList& utilities) {

    QMap<QString, bool> utilMap;
    QString path = fullPath.left(fullPath.lastIndexOf('/'));
    QString utilityList = utilities.join(" ");
    QString cmd = QString("cd %1; out=\"\"; for u in %2; do command -v $u >/dev/null 2>&1 && out+=\"$u:true,\""
                  "|| out+=\"$u:false,\"; done; echo \"${out%,}\"").arg(path, utilityList);

    // Get result from checking utilities
    QString output;
    if (targetSystems[targetId]->launchShortUtility(cmd, output) == 0) {
        output.remove("\n");
        QStringList res;
        QStringList utils = output.split(",");
        for (const auto& util: std::as_const(utils)) {
            res = util.split(":");
            utilMap[res[0]] = (res[1] == "true");
        }
    } else {
        for (const auto& util: utilities) {
            utilMap[util] = false;
        }
    }
    return utilMap;
}

// Run autoPatch
void MainWindow::runMeshPatch(double angle, const QString& casePath,
                                 int targetId) {

    QString openFoamPath;
    QString caseName = QFileInfo(casePath).fileName();
    if (m_caseMap.contains(caseName)) {
        openFoamPath = m_caseMap[caseName].openFoamPath;
    } else {
        qWarning() << "Case" << caseName << "is not in case map.";
        return;
    }

    // Make sure autoPatch is present
    QMap<QString, bool> utilMap = m_utilMap[openFoamPath];
    if (!utilMap.value("autoPatch", false)) {
        QMessageBox::warning(this, tr("Utility Not Found"), tr("The autoPatch utility could not be found."));
        return;
    }

    // Run autoPatch
    QString result;
    QString cmd = QString("cd \"%1\"; source %2/etc/bashrc; autoPatch -overwrite %3")
        .arg(casePath, openFoamPath, QString::number(angle));
    targetSystems[targetId]->launchShortUtility(cmd, result);

    // Reload mesh
    QProgressDialog* progress = new QProgressDialog("Loading Mesh Data...", QString(), 0, 0, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumWidth(300);
    progress->setAttribute(Qt::WA_DeleteOnClose);
    progress->show();

    // Setup the Future Watcher
    using RenderDataPtr = std::shared_ptr<RenderData>;
    QFutureWatcher<RenderDataPtr>* watcher = new QFutureWatcher<RenderDataPtr>(this);

    // Connect the watcher's finished signal to handle the result on the MAIN thread
    connect(watcher, &QFutureWatcher<RenderDataPtr>::finished, this,
        [this, watcher, progress, casePath, targetId]() {

        progress->close();

        // Retrieve the result from the background thread
        RenderDataPtr renderData = watcher->result();

        if (renderData) {
            MeshEditor* editor = qobject_cast<MeshEditor*>(tabWidget->currentWidget());
            editor->updateModel(renderData);
        }

        // Clean up the watcher
        watcher->deleteLater();
    });

    // Start the background thread
    QFuture<RenderDataPtr> future =
        QtConcurrent::run(&MainWindow::getMeshData, this,
                          caseName, casePath, openFoamPath, targetId);
    watcher->setFuture(future);
}

void MainWindow::runMeshCheck(const QString& casePath, int targetId) {

    QString openFoamPath;
    QString caseName = QFileInfo(casePath).fileName();
    if (m_caseMap.contains(caseName)) {
        openFoamPath = m_caseMap[caseName].openFoamPath;
    } else {
        qWarning() << "Case" << caseName << "is not in case map.";
        return;
    }

    // Make sure checkMesh is present
    QMap<QString, bool> utilMap = m_utilMap[openFoamPath];
    if (!utilMap.value("checkMesh", false)) {
        QMessageBox::warning(this, tr("Utility Not Found"), tr("The checkMesh utility could not be found."));
        return;
    }

    // Run checkMesh
    QString cmd = QString("cd \"%1\"; source %2/etc/bashrc; checkMesh -constant")
                      .arg(casePath, openFoamPath);

    // Get output
    QString result;
    targetSystems[targetId]->launchShortUtility(cmd, result);
    if(result.isEmpty()) { return; }

    // Display dialog containing result
    std::vector<QString> messageStrings;
    if (result.contains("Mesh OK")) {
        messageStrings.push_back("Mesh OK");
    } else if (result.contains("Failed")) {
        messageStrings.push_back("Failed");
    }

    /*
    for (auto line : QStringTokenizer(result, u'\n')) {
        if((line.startsWith(u"Surface")) || (line.startsWith(u"Number"))) {
            messageStrings.push_back(line.toString());
        }
    }
    */

    // Display dialog
    UtilityOutputDialog dlg(tr("Mesh Check Results"),
                            tr("Output of the checkMesh utiilty:"),
                            messageStrings, result, this);
    dlg.exec();
}

// Run autoPatch
void MainWindow::runMeshRenumber(const QString& casePath, int targetId) {

    QString openFoamPath;
    QString caseName = QFileInfo(casePath).fileName();
    if (m_caseMap.contains(caseName)) {
        openFoamPath = m_caseMap[caseName].openFoamPath;
    } else {
        qWarning() << "Case" << caseName << "is not in case map.";
        return;
    }

    qDebug() << "casePath: " << casePath;
    qDebug() << "caseName: " << caseName;
    qDebug() << "openFoamPath: " << openFoamPath;

    // Make sure autoPatch is present
    QMap<QString, bool> utilMap = m_utilMap[openFoamPath];
    if (!utilMap.value("renumberMesh", false)) {
        QMessageBox::warning(this, tr("Utility Not Found"), tr("The renumberMesh utility could not be found."));
        return;
    }

    // Run renumberMesh
    QString cmd = QString("cd \"%1\"; source %2/etc/bashrc; renumberMesh -constant -overwrite")
                      .arg(casePath, openFoamPath);

    qDebug() << cmd;

    QString result;
    targetSystems[targetId]->launchShortUtility(cmd, result);
    qDebug() << result;
}

// Run surfacePatch
void MainWindow::runSurfacePatch(double angle, const QString& fullPath,
                                 int targetId, bool isBinary) {

    // Variables for command string
    QFileInfo info(fullPath);
    QString path = info.path();
    QString fileName = info.fileName();
    QString stem = info.completeBaseName();
    QString tmpName = info.completeBaseName() + "_tmp." + info.suffix();
    QString cmd;

    // Get the OpenFOAM path
    QString casePath, caseName, openFoamPath;
    int index = fullPath.lastIndexOf("/constant/triSurface");
    if (index != -1) {
        casePath = fullPath.left(index);
    } else {
        qWarning() << "STL file is not located in a valid OpenFOAM triSurface directory.";
        return;
    }
    caseName = QFileInfo(casePath).fileName();
    if (m_caseMap.contains(caseName)) {
        openFoamPath = m_caseMap[caseName].openFoamPath;
    } else {
        qWarning() << "Case" << caseName << "is not in case map.";
        return;
    }
    QMap<QString, bool> utilMap = m_utilMap[openFoamPath];

    // Run surfaceAutoPatch if present
    if (utilMap.value("surfaceAutoPatch", false)) {
        if (isBinary) {
            QString symlinkName = info.completeBaseName() + ".stlb";
            cmd = QString("cd \"%1\" && ln -s \"%2\" \"%3\" && surfaceAutoPatch \"%3\" %4 \"%5\"; rm -f \"%3\"")
                      .arg(path, fileName, symlinkName, QString::number(angle), tmpName);
        } else {
            cmd = QString("cd \"%1\" && surfaceAutoPatch \"%2\" %3 \"%4\"")
            .arg(path, fileName, QString::number(angle), tmpName);
        }

        // Run surfacePatch
        QString result;
        targetSystems[targetId]->launchShortUtility(cmd, result);
        qDebug() << result;
    }
    else if (utilMap.value("surfacePatch", false)) {

        // Create surfacePatchDict
        QString dictText = Utils::createSurfacePatchDict(openFoamPath, fileName, angle);
        targetSystems[targetId]->writeData(dictText.toUtf8(), casePath + "/system/surfacePatchDict");

        // Run surfacePatch
        cmd = QString("cd \"%1\" && surfacePatch").arg(casePath);
        QString result;
        targetSystems[targetId]->launchShortUtility(cmd, result);

        // Check if a new file has been created
        if (result.contains("Writing repatched surface")) {

            // Read patched file content
            QString newPath = path + "/" + stem + "_patched.stl";
            QByteArray newData = targetSystems[targetId]->getFileContent(newPath);

            if (newData.size() > 0) {

                // Create new RenderData
                std::pair<RenderData, bool> res = StlReader::readStlFile(fileName, newData);
                RenderData mesh = res.first;
                std::shared_ptr<RenderData> meshData = std::make_shared<RenderData>(std::move(mesh));

                // Pass data to ModelEditor
                ModelEditor* editor = qobject_cast<ModelEditor*>(tabWidget->currentWidget());
                editor->updateModel(meshData);
            }
        } else if (result.contains("unchanged")) {
            QMessageBox::warning(this, tr("No Patches Generated"),
                tr("The surfacePatch utility didn't generate any patches.\n\n"
                   "You may want to reduce the featureAngle or update surfacePatchDict."));
        }
    }
}

void MainWindow::runSurfaceCheck(const QString& fullPath, int targetId, bool isBinary) {

    // Check if surfaceCheck is present
    QStringList utils = { "surfaceCheck" };
    QMap<QString, bool> utilMap = checkUtilities(fullPath, targetId, utils);
    if (!utilMap["surfaceCheck"]) {
        QMessageBox::warning(this, tr("Utility Not Found"), tr("The surfaceCheck utility could not be found."));
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
        cmd = QString("surfaceCheck %1").arg(safePath);
    }

    // Get output
    QString result;
    targetSystems[targetId]->launchShortUtility(cmd, result);

    if(result.isEmpty()) {
        return;
    }

    // Display dialog containing result
    std::vector<QString> messageStrings;
    for (auto line : QStringTokenizer(result, u'\n')) {
        if((line.startsWith(u"Surface")) || (line.startsWith(u"Number"))) {
            messageStrings.push_back(line.toString());
        }
    }

    // Display dialog
    UtilityOutputDialog dlg(tr("Surface Check Results"),
                            tr("Output of the surfaceCheck utiilty:"),
                            messageStrings, result, this);
    dlg.exec();
}

// Run surfacePatch
void MainWindow::runSurfaceScale(double scaleFactor, const QString& fullPath,
                                 int targetId) {

    // Variables for command string
    QFileInfo info(fullPath);
    QString path = info.path();
    QString fileName = info.fileName();
    QString stem = info.completeBaseName();
    QString tmpName = info.completeBaseName() + "_tmp." + info.suffix();

    // Get the OpenFOAM path
    QString casePath, caseName, openFoamPath;
    int index = fullPath.lastIndexOf("/constant/triSurface");
    if (index != -1) {
        casePath = fullPath.left(index);
    } else {
        qWarning() << "STL file is not located in a valid OpenFOAM triSurface directory.";
        return;
    }
    caseName = QFileInfo(casePath).fileName();
    if (m_caseMap.contains(caseName)) {
        openFoamPath = m_caseMap[caseName].openFoamPath;
    }

    // Create command
    QString cmd = QString("cd \"%1\"; source %2/etc/bashrc; "
                          "surfaceTransformPoints -scale \"(%3 %3 %3)\" \"%4\" \"%4\"")
                          .arg(path, openFoamPath, QString::number(scaleFactor), fileName);

    // Execute command
    QString result;
    targetSystems[targetId]->launchShortUtility(cmd, result);

    // Display message
    if (result.contains("uniformly")) {

        /*
        // Update model
        QByteArray newData = targetSystems[targetId]->getFileContent(fullPath);

        qDebug() << "fullPath = " << fullPath;
        qDebug() << "newData.size() = " << newData.size();

        if (newData.size() > 0) {

            // Create new RenderData
            std::pair<RenderData, bool> res = StlReader::readStlFile(fileName, newData);
            RenderData mesh = res.first;
            std::shared_ptr<RenderData> meshData = std::make_shared<RenderData>(std::move(mesh));

            // Pass data to ModelEditor
            ModelEditor* editor = qobject_cast<ModelEditor*>(tabWidget->currentWidget());
            editor->updateModel(meshData);
        }
        */

        ModelEditor* editor = qobject_cast<ModelEditor*>(tabWidget->currentWidget());
        editor->changeBounds(scaleFactor);

        QMessageBox::information(this, tr("Operation Successful"), tr("The scale operation completed successfully."));
    } else {
        QMessageBox::information(this, tr("Operation Unsuccessful"), tr("The scale operation did not complete successfully."));
    }
    return;
}

void MainWindow::runSolverWizard() {

    // Get the selected case
    QString selectedCase = navigator->getSelectedCase();

    // Respond if there are no cases
    if (selectedCase.isEmpty()) {

        // Launch selection dialog
        QStringList availableCases = navigator->getCases();
        SelectionDialog selectionDialog(tr("Case Selection Dialog"), tr("Select the case to be processed:"),
                                        availableCases, this);
        if (selectionDialog.exec() == QDialog::Accepted) {
            selectedCase = selectionDialog.getSelectedItem();
        } else {
            return;
        }
    }

    // Create solver wizard if parsing succeeds
    SolverWizard* wizard = new SolverWizard(selectedCase, m_solverFamilies,
                                            m_turbulenceModels, m_fieldData,
                                            m_boundaryConditions, this);
    if (wizard->parseFiles()) {
        wizard->exec();
    } else {
        wizard->deleteLater();
    }
}

void MainWindow::loadSolverFamilies() {

    static const QMap<QString, FlowCompute::Algorithm> algoMap = {
        {"SIMPLE", FlowCompute::Algorithm::SIMPLE},
        {"PIMPLE", FlowCompute::Algorithm::PIMPLE},
        {"PISO", FlowCompute::Algorithm::PISO},
        {"CENTRAL_UPWIND", FlowCompute::Algorithm::CENTRAL_UPWIND},
        {"NONE", FlowCompute::Algorithm::NONE}
    };

    // Update families and categories
    QString configDirPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir configDir(configDirPath);
    if (!configDir.exists()) {
        configDir = QDir(".");
    }

    QFile file(configDir.filePath("solvers.json"));
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
                    family.familyName = obj["category"].toString();
                    QJsonArray solverArray = obj["solvers"].toArray();
                    for (const QJsonValue& solverVal : std::as_const(solverArray)) {

                        // Handle the case where it is an object with potential overrides
                        if (solverVal.isObject()) {
                            QJsonObject solverObj = solverVal.toObject();
                            FlowCompute::SolverDetails details;
                            details.name = solverObj["name"].toString();
                            details.algorithm = algoMap[solverObj["algorithm"].toString()];
                            details.fields = solverObj["fields"].toVariant().toStringList();
                            details.isSteadyState = solverObj["is_steady_state"].toBool(details.isSteadyState);
                            family.solvers.append(details);
                        }
                    }
                    m_solverFamilies.push_back(family);
                }
            }
        }
        file.close();
    }
}

void MainWindow::loadTurbulenceModels() {

    // Update families and categories
    QString configDirPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir configDir(configDirPath);
    if (!configDir.exists()) {
        configDir = QDir(".");
    }

    QFile file(configDir.filePath("turbulence.json"));
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        // Ensure the root of the JSON document is an object
        if (doc.isObject()) {
            QJsonObject rootObj = doc.object();

            // Clear any existing data before repopulating
            m_turbulenceModels.clear();

            // Iterate through categories
            for (auto catIt = rootObj.constBegin(); catIt != rootObj.constEnd(); ++catIt) {
                QString categoryName = catIt.key();
                QJsonObject subCatObj = catIt.value().toObject();

                QMap<QString, std::vector<FlowCompute::TurbulenceModel>> subCatMap;

                // Iterate through subcategories
                for (auto subCatIt = subCatObj.constBegin(); subCatIt != subCatObj.constEnd(); ++subCatIt) {
                    QString subCategoryName = subCatIt.key();
                    QJsonObject modelsObj = subCatIt.value().toObject();

                    std::vector<FlowCompute::TurbulenceModel> modelList;

                    // Iterate through models
                    for (auto modelIt = modelsObj.constBegin(); modelIt != modelsObj.constEnd(); ++modelIt) {
                        QString modelName = modelIt.key();
                        QJsonObject modelDataObj = modelIt.value().toObject();

                        FlowCompute::TurbulenceModel model;
                        model.name = modelName;
                        model.description = modelDataObj["description"].toString();
                        model.fields = modelDataObj["fields"].toVariant().toStringList();
                        modelList.push_back(model);
                    }

                    // Insert the populated list into the subcategory map
                    subCatMap.insert(subCategoryName, modelList);
                }

                // Insert the subcategory map into the main category map
                m_turbulenceModels.insert(categoryName, subCatMap);
            }
        } else {
            qWarning() << "Failed to parse turbulence.json: Root is not a JSON Object.";
        }
        file.close();
    } else {
        qWarning() << "Failed to open turbulence.json at:" << file.fileName();
    }
}

void MainWindow::loadFieldData() {

    // Update families and categories
    QString configDirPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir configDir(configDirPath);
    if (!configDir.exists()) {
        configDir = QDir(".");
    }

    QFile file(configDir.filePath("fields.json"));
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

    if (!jsonDoc.isObject()) {
        qWarning() << "Root JSON element is not an object.";
        return;
    }

    // 3. Iterate through the root object and populate the QHash
    QJsonObject rootObj = jsonDoc.object();
    for (auto it = rootObj.constBegin(); it != rootObj.constEnd(); ++it) {
        QString fieldName = it.key();
        QJsonObject fieldObj = it.value().toObject();

        static const QHash<QString, FlowCompute::FieldClass> classMap = {
            {"volScalarField", FlowCompute::FieldClass::VolScalarField},
            {"volVectorField", FlowCompute::FieldClass::VolVectorField},
            {"volSphericalTensorField", FlowCompute::FieldClass::VolSphericalTensorField},
            {"volSymmTensorField", FlowCompute::FieldClass::VolSymmTensorField},
            {"volTensorField", FlowCompute::FieldClass::VolTensorField},
            {"surfaceScalarField", FlowCompute::FieldClass::SurfaceScalarField},
            {"surfaceVectorField", FlowCompute::FieldClass::SurfaceVectorField},
            {"surfaceSphericalTensorField", FlowCompute::FieldClass::SurfaceSphericalTensorField},
            {"surfaceSymmTensorField", FlowCompute::FieldClass::SurfaceSymmTensorField},
            {"surfaceTensorField", FlowCompute::FieldClass::SurfaceTensorField},
            {"pointScalarField", FlowCompute::FieldClass::PointScalarField},
            {"pointVectorField", FlowCompute::FieldClass::PointVectorField},
            {"pointSphericalTensorField", FlowCompute::FieldClass::PointSphericalTensorField},
            {"pointSymmTensorField", FlowCompute::FieldClass::PointSymmTensorField},
            {"pointTensorField", FlowCompute::FieldClass::PointTensorField}
        };

        FlowCompute::FieldDef data;
        data.fieldClass = classMap[fieldObj.value("class").toString()];
        data.dimensions = fieldObj.value("dimensions").toString();

        // 4. Handle default value conversion (Scalar vs OpenFOAM Vector Array)
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

    // Update families and categories
    QString configDirPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir configDir(configDirPath);
    if (!configDir.exists()) {
        configDir = QDir(".");
    }

    // Read file content
    QFile file(configDir.filePath("boundary_conditions.json"));
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
    auto jsonArrayToStringList = [](const QJsonArray& jsonArray) {
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
        bc.categories = jsonArrayToStringList(obj.value("categories").toArray());
        bc.types      = jsonArrayToStringList(obj.value("types").toArray());
        bc.patchTypes = jsonArrayToStringList(obj.value("patchTypes").toArray());
        bc.parameters = jsonArrayToStringList(obj.value("parameters").toArray());
        m_boundaryConditions.push_back(bc);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {

    // Check tabs
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (!tabWidget->promptToSave(i)) {
            return;
        }
    }

    // Access settings
    QSettings settings;

    // Save cases to settings
    settings.remove("Cases");
    settings.beginWriteArray("Cases");
    QStringList cases = navigator->getCases();

    // We need an integer index for the array, so we use a traditional for-loop
    for (int i = 0; i < cases.size(); ++i) {
        settings.setArrayIndex(i);
        QString caseName = cases.at(i);
        CaseData data = m_caseMap.value(caseName);

        // Save values to settings
        settings.setValue("caseName", caseName);
        settings.setValue("casePath", data.casePath);
        settings.setValue("caseFiles", data.caseFiles);
        settings.setValue("targetSystemId", data.targetSystemId);
        settings.setValue("openFoamPath", data.openFoamPath);
    }
    settings.endArray();

    // Save open tabs to settings
    settings.remove("Tabs");
    settings.beginWriteArray("Tabs");
    const QString suffix = " (mesh)";

    for (int i = 0; i < tabWidget->count(); ++i) {
        settings.setArrayIndex(i);
        QString fileName = tabWidget->tabText(i);
        if (fileName.endsWith(suffix)) {
            fileName.chop(suffix.length());
        }
        TabData data = tabMap.value(fileName);

        // Save values to settings
        settings.setValue("tabName", fileName);
        settings.setValue("fullPath", data.fullPath);
        settings.setValue("type", static_cast<int>(data.type));
    }
    settings.endArray();
}
