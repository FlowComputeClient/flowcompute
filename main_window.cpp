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
#include <QGuiApplication>
#include <QProgressBar>
#include <QProgressDialog>
#include <QStatusBar>

#include <string>
#include <memory>
#include <utility>
#include <vector>

#include "dialogs/mesh/wizard_mesh.h"
#include "dialogs/preferences/preferences_dialog.h"
#include "dialogs/run_mesh/run_mesh_dialog.h"
#include "dialogs/run_solver/run_solver_dialog.h"
#include "dialogs/solver/wizard_solver.h"
#include "dialogs/utility_output/utility_output_dialog.h"

#include "editors/graphical/surface/surface_editor.h"
#include "editors/graphical/mesh/mesh_editor.h"
#include "editors/graphical/result/result_editor.h"
#include "geometry/mesh/mesh_reader.h"
#include "geometry/stl/stl_reader.h"
#include "./utils.h"

bool MainWindow::s_isWindows = false;
bool MainWindow::s_isWslAvailable = false;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
// Check environment
#ifdef Q_OS_WIN
    s_isWindows = true;

    // Check for wsl.exe
    QString wslPath = QStandardPaths::findExecutable("wsl.exe");
    s_isWslAvailable = !wslPath.isEmpty();
    setWindowIcon(QIcon(":/images/flowcompute.ico"));
#else
    setWindowIcon(QIcon(":/images/flowcompute.png"));
#endif

    /*
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
    */
    setWindowTitle("FlowCompute 0.8.0 - Visual CFD Made Simple");

    // Configure font
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

    // Create WSL communication system
    targetSystems[0] = &wslSystem;
    connect(&wslSystem, &WslSystem::longUtilityOutputReceived,
            this, &MainWindow::log);
    connect(&wslSystem, &WslSystem::longUtilityFinished,
            this, &MainWindow::longUtilityFinished);
    connect(&wslSystem, &WslSystem::longUtilityError,
            this, &MainWindow::log);

    // Create Local Linux communication system
    targetSystems[1] = &localSystem;
    connect(&localSystem, &LocalSystem::longUtilityOutputReceived,
            this, &MainWindow::log);
    connect(&localSystem, &LocalSystem::longUtilityFinished,
            this, &MainWindow::longUtilityFinished);
    connect(&localSystem, &LocalSystem::longUtilityError,
            this, &MainWindow::log);

    // Create actions, menus, and toolbars
    createActions();
    createMenus();
    createToolBar();

    // Create tab widget
    m_tabWidget = new TabWidget(this);
    setCentralWidget(m_tabWidget);

    // Create the navigator
    navigatorWidget = new QDockWidget(tr("Case Navigator"), this);
    navigatorWidget->setTitleBarWidget(new QWidget(navigatorWidget));
    navigatorWidget->setAllowedAreas(Qt::LeftDockWidgetArea);
    m_navigator = new CaseNavigator(this);
    navigatorWidget->setWidget(m_navigator);
    navigatorWidget->setMinimumWidth(200);
    addDockWidget(Qt::LeftDockWidgetArea, navigatorWidget);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

    // Create the console
    consoleWidget = new QDockWidget(tr("Console"), this);
    consoleWidget->setTitleBarWidget(new QWidget(consoleWidget));
    consoleWidget->setAllowedAreas(Qt::BottomDockWidgetArea);
    m_console = new Console(this);
    consoleWidget->setWidget(m_console);
    consoleWidget->setMinimumHeight(150);
    addDockWidget(Qt::BottomDockWidgetArea, consoleWidget);
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
    applyTheme(settings.value("Theme/file", "dark.json").toString());

    // Populate caseMap and navigator from settings
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
            m_utilMap[data.openFoamPath] =
                checkUtilities(path, data.targetSystemId, m_utilities);
        }

        // Update navigator
        m_navigator->addCase(caseName, data.caseFiles);
        m_navigator->expandCase(caseName);
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
        m_tabMap.insert(tabName, data);

        // Create editor
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
    connect(m_tabWidget, &TabWidget::saveTab, this, &MainWindow::saveFile);

    // Configure logging from the navigator
    // connect(m_navigator, &CaseNavigator::createEditor, this, &MainWindow::createEditor);

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
    newCaseAction = new QAction(QIcon(":/images/new_case.png"),
                                tr("&New Case Folder"), this);
    newCaseAction->setShortcuts(QKeySequence::New);
    newCaseAction->setStatusTip(tr("Create a new case folder"));
    connect(newCaseAction, &QAction::triggered, this,
            &MainWindow::runNewCaseWizard);

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

    // Preferences
    preferencesAction = new QAction(QIcon(":/images/dict.png"),
            tr("Set preferences..."), this);
    preferencesAction->setShortcuts(QKeySequence::Preferences);
    preferencesAction->setStatusTip(tr("Set preferences"));
    connect(preferencesAction, &QAction::triggered, this,
            &MainWindow::setPreferences);

    // Undo
    undoAction = new QAction(QIcon(":/images/undo.png"), tr("Undo"), this);
    undoAction->setShortcuts(QKeySequence::Undo);
    undoAction->setStatusTip(tr("Zoom in"));
    connect(undoAction, &QAction::triggered, this, &MainWindow::undo);
    undoAction->setDisabled(true);

    // Redo
    redoAction = new QAction(QIcon(":/images/redo.png"), tr("Redo"), this);
    redoAction->setShortcuts(QKeySequence::Redo);
    redoAction->setStatusTip(tr("Zoom out"));
    connect(redoAction, &QAction::triggered, this, &MainWindow::redo);
    redoAction->setDisabled(true);

    // Zoom in
    zoomInAction =
        new QAction(QIcon(":/images/zoom_in.png"), tr("Zoom in"), this);
    zoomInAction->setShortcuts(QKeySequence::ZoomIn);
    zoomInAction->setStatusTip(tr("Zoom in"));

    // Zoom out
    zoomOutAction =
        new QAction(QIcon(":/images/zoom_out.png"), tr("Zoom out"), this);
    zoomOutAction->setShortcuts(QKeySequence::ZoomOut);
    zoomOutAction->setStatusTip(tr("Zoom out"));

    // Mesh configuration
    meshAction =
        new QAction(QIcon(":/images/mesh.png"), tr("Configure &Mesh"), this);
    meshAction->setStatusTip(tr("Configure mesh process"));
    connect(meshAction, &QAction::triggered, this,
            &MainWindow::runMeshConfiguration);

    // Launch mesh utilities
    runMeshAction =
        new QAction(QIcon(":/images/run_mesh.png"), tr("Run M&esh"), this);
    runMeshAction->setStatusTip(tr("Run mesh utilities"));
    connect(runMeshAction, &QAction::triggered, this,
            &MainWindow::runMeshExecution);

    // Solver configuration
    solverAction = new QAction(QIcon(":/images/solver.png"),
                               tr("&Solver Configuration"), this);
    solverAction->setStatusTip(tr("Configure simulation"));
    connect(solverAction, &QAction::triggered, this,
            &MainWindow::runSolverConfiguration);

    // Run solver
    runSolverAction = new QAction(QIcon(":/images/run_solver.png"),
                                  tr("&Run solver"), this);
    solverAction->setStatusTip(tr("Launch solver"));
    connect(runSolverAction, &QAction::triggered, this,
            &MainWindow::runSolverExecution);

    // Stop solver
    stopSolverAction = new QAction(QIcon(":/images/stop_solver.png"),
                                   tr("&Halt solver execution"), this);
    stopSolverAction->setStatusTip(tr("Stop solver"));
    stopSolverAction->setEnabled(false);
    connect(stopSolverAction, &QAction::triggered, this,
            &MainWindow::stopSolverExecution);

    // Post-process
    // postProcessAction = new QAction(QIcon(":/images/post_process.png"), tr("&Post-process"), this);
    // postProcessAction->setStatusTip(tr("Launch post-processing"));
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
    editMenu->addAction(preferencesAction);
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
    toolBar->addAction(solverAction);
    toolBar->addAction(runSolverAction);
    toolBar->addAction(stopSolverAction);
    toolBar->addSeparator();

    // Create tool bar with view actions
    // toolBar->addAction(postProcessAction);

    // Add help actions
    toolBar->addAction(aboutAction);
}

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
                    m_textTheme.syntaxConfig.string =
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

void MainWindow::createEditor(EditorType type, QString& fileName,
                              const QString& fullPath, bool logMessage) {
    // Get case name
    QString caseName;
    if ((type != EditorType::MESH) && (type != EditorType::RESULT)) {
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

    // Update log
    if (logMessage) m_console->appendPlainText(tr("Reading %1\n").arg(path));

    // Read data
    if ((type != EditorType::MESH) && (type != EditorType::RESULT)) {
        data = targetSystems[targetId]->getFileContent(path);
    }

    // Check to see if there's already an editor
    if (m_tabMap.contains(fileName)) {
        tabData = m_tabMap[fileName];
        if (tabData.fullPath == fullPath) {

            // Iterate through tabs
            for (int i = 0; i < m_tabWidget->count(); ++i) {
                QString tabName = m_tabWidget->tabText(i);
                QString tabPath = m_tabWidget->tabBar()->tabData(i).toString();
                if (tabName == fileName && tabPath == fullPath) {
                    m_tabWidget->setCurrentIndex(i);
                    return;
                }
            }
        }
    }

    // Update tabMap
    tabData.fullPath = fullPath;
    tabData.type = type;
    m_tabMap.insert(fileName, tabData);

    // Create text editor
    if (type == EditorType::TEXT) {
        // Create new tab
        TextEditor* textEditor = new TextEditor(this);
        textEditor->setFont(m_font);
        textEditor->setTextData(data);
        textEditor->applyTheme(m_textTheme);
        tabIndex = m_tabWidget->addTab(textEditor, fileName);
        m_tabWidget->setCurrentIndex(tabIndex);
        m_tabWidget->tabBar()->setTabData(tabIndex, fullPath);

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
        connect(m_tabWidget, &QTabWidget::currentChanged,
            this, [=, this](int index) {
                TextEditor* currentEditor =
                    qobject_cast<TextEditor*>(m_tabWidget->widget(index));
                if (currentEditor) {
                    saveFileAction->setEnabled(
                        currentEditor->document()->isModified());
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
        if (fileName.endsWith(".stl", Qt::CaseInsensitive)) {
            std::pair<RenderData, bool> res =
                StlReader::readStlFile(fileName, data);
            model = res.first;
            isBinary = res.second;
        }
        std::shared_ptr<RenderData> modelData =
            std::make_shared<RenderData>(std::move(model));

        // Create editor
        SurfaceEditor* surfaceEditor = new SurfaceEditor(modelData, path,
            targetId, &m_vulkanInstance, isBinary, this);
        tabIndex = m_tabWidget->addTab(surfaceEditor, fileName);
        connect(surfaceEditor, &SurfaceEditor::surfacePatchRequested,
                this, &MainWindow::runSurfacePatch);
        connect(surfaceEditor, &SurfaceEditor::surfaceCheckRequested,
                this, &MainWindow::runSurfaceCheck);
        connect(surfaceEditor, &SurfaceEditor::surfaceScaleRequested,
                this, &MainWindow::runSurfaceScale);
        connect(surfaceEditor, &SurfaceEditor::dirtyStateChanged,
                this, [this, surfaceEditor](bool isDirty) {
                    onDirtyStateChanged(isDirty, surfaceEditor);
                });
    }
    if (type == EditorType::MESH) {
        // Create mesh editor
        QProgressDialog* progress =
            new QProgressDialog("Loading Mesh Data...", QString(), 0, 0, this);
        progress->setWindowModality(Qt::WindowModal);
        progress->setMinimumWidth(300);
        progress->setAttribute(Qt::WA_DeleteOnClose);
        progress->show();

        // Setup the Future Watcher
        using RenderDataPtr = std::shared_ptr<RenderData>;
        QFutureWatcher<RenderDataPtr>* watcher =
            new QFutureWatcher<RenderDataPtr>(this);

        // Connect the finished signal to handle the result
        connect(watcher, &QFutureWatcher<RenderDataPtr>::finished, this,
        [this, watcher, progress, casePath, targetId, fileName, fullPath]() {
            progress->close();

            // Retrieve the result from the background thread
            RenderDataPtr renderData = watcher->result();

            if (renderData) {
                MeshEditor* meshEditor = new MeshEditor(renderData,
                    casePath, targetId, m_solverFamilies, m_turbulenceModels,
                    m_fieldData, m_boundaryConditions, &m_vulkanInstance, this);
                connect(meshEditor, &MeshEditor::meshPatchRequested,
                        this, &MainWindow::runMeshPatch);
                connect(meshEditor, &MeshEditor::meshCheckRequested,
                        this, &MainWindow::runMeshCheck);
                connect(meshEditor, &MeshEditor::meshRenumberRequested,
                        this, &MainWindow::runMeshRenumber);

                int tabIndex =
                    m_tabWidget->addTab(meshEditor, fileName + " (mesh)");
                m_tabWidget->setCurrentIndex(tabIndex);
                m_tabWidget->tabBar()->setTabData(tabIndex, fullPath);
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

    if (type == EditorType::RESULT) {
        // Get list of time folders
        QString resultPath = casePath + "/postProcessing/surfaces";
        QString res = targetSystems[targetId]->getResultFolders(resultPath);
        if (!res.isEmpty()) {
            QStringList timeFolders = res.split(',');
            QString lastTime = timeFolders.last();
            resultPath += "/" + lastTime;
            RenderData renderData =
                targetSystems[targetId]->getResultData(resultPath);
            std::shared_ptr<RenderData> resultData =
                std::make_shared<RenderData>(std::move(renderData));
            if (resultData) {
                ResultEditor* resultEditor = new ResultEditor(timeFolders,
                    resultData, casePath, targetId, &m_vulkanInstance, this);
                connect(resultEditor, &ResultEditor::timeChanged,
                        this, &MainWindow::updateResultEditor);
                int tabIndex =
                    m_tabWidget->addTab(resultEditor,
                        fileName + QString(" (result@%1)").arg(lastTime));
                m_tabWidget->setCurrentIndex(tabIndex);
                m_tabWidget->tabBar()->setTabData(tabIndex, fullPath);
                undoAction->setDisabled(true);
                redoAction->setDisabled(true);
                saveFileAction->setDisabled(true);
            }
        }
        return;
    }

    if (type != EditorType::MESH) {
        // Update tab widget
        m_tabWidget->setCurrentIndex(tabIndex);
        m_tabWidget->tabBar()->setTabData(tabIndex, fullPath);
        undoAction->setDisabled(true);
        redoAction->setDisabled(true);
        saveFileAction->setDisabled(true);
    }
}

void MainWindow::deleteFile(const QString& fileName, const QString& fullPath) {
    // Construct path to delete
    QString caseName, filePath, casePath;
    if (fullPath.isEmpty()) {
        caseName = filePath;
        filePath = m_caseMap[caseName].casePath + "/" + caseName;
        casePath = caseName;
    } else {
        caseName = fullPath.split('/').first();
        filePath = m_caseMap[caseName].casePath + "/" +
                   fullPath + "/" + fileName;
        casePath = fullPath;
    }

    // Delete file
    int targetId = m_caseMap[caseName].targetSystemId;
    if (!targetSystems[targetId]->deleteFile(filePath)) {
        qDebug() << "Failed to delete " << filePath;
    }

    // Update case navigator

}

std::shared_ptr<RenderData> MainWindow::getMeshData(QString caseName,
        QString casePath, QString openFoamPath, int targetId) {
    // Convert mesh to STL file
    QByteArray data;
    QString meshFile = caseName + "_tmp.stl";
    QString cmd = QString("cd %1; source %2/etc/bashrc && foamToSurface %3")
                      .arg(casePath, openFoamPath, meshFile);

    // Read new STL file
    QString output;
    if (targetSystems[targetId]->launchShortUtility(cmd, output) == 0) {
        data =
            targetSystems[targetId]->getFileContent(casePath + "/" + meshFile);
    }

    // Convert data to RenderData structure
    return std::make_shared<RenderData>(MeshReader::readMesh(caseName, data));
}

void MainWindow::runMesh(const QString& caseName, bool runBlockMesh,
                         bool runSurfaceFeatureExtract, bool runSnappyHexMesh,
                         const QString& snappyCmd, int numCores) {
    // Get OpenFoam path
    CaseData caseData = m_caseMap[caseName];
    int targetId = caseData.targetSystemId;
    QString casePath = caseData.casePath + "/" + caseName;
    QString openFoamPath = caseData.openFoamPath;
    QString cmd, output;

    // Clear console
    m_console->clear();

    // Launch blockMesh
    if (runBlockMesh) {
        cmd = QString("cd %1; source %2/etc/bashrc; blockMesh").arg(
            casePath, openFoamPath);
        if (targetSystems[targetId]->launchShortUtility(cmd, output) == 0) {
            log(output);
        }
    }

    // Launch surfaceFeatureExtract
    if (runSurfaceFeatureExtract) {
        cmd = QString("cd %1; source %2/etc/bashrc; surfaceFeatureExtract").
              arg(casePath, openFoamPath);
        if (targetSystems[targetId]->launchShortUtility(cmd, output) == 0) {
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
            targetSystems[targetId]->writeData(dictText.toUtf8(),
                casePath + "/system/decomposeParDict");
        }

        // Create command
        cmd = QString("cd %1; source %2/etc/bashrc; " + snappyCmd).
                arg(casePath, openFoamPath);
        targetSystems[targetId]->launchLongUtility(cmd,
                caseName, UtilityType::MESH);
    }
}

void MainWindow::runSolver(const QString& caseName, const QString& cmd) {
    // Get OpenFoam path
    CaseData caseData = m_caseMap[caseName];
    int targetId = caseData.targetSystemId;
    QString casePath = caseData.casePath + "/" + caseName;
    QString openFoamPath = caseData.openFoamPath;

    // Clear console
    m_console->clear();

    // Execute command
    QString command = QString("cd %1; source %2/etc/bashrc; " + cmd).arg(casePath, openFoamPath);
    targetSystems[targetId]->launchLongUtility(command, caseName, UtilityType::SOLVER);
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
    m_navigator->addCase(caseName, caseFiles);
    m_navigator->expandCase(caseName);
}

// Create new case folder
void MainWindow::saveFile() {
    // Remove the " *" to get the map key
    QString rawTabText = m_tabWidget->tabText(m_tabWidget->currentIndex());
    QString fileName = rawTabText.remove(" *");

    // Look up data
    if (m_tabMap.contains(fileName)) {

        // Construct the remote path
        TabData tabData = m_tabMap[fileName];
        QString caseName = tabData.fullPath.split("/")[0];
        int targetId = m_caseMap[caseName].targetSystemId;
        QString fullPath = m_caseMap[caseName].casePath + "/" +
                           tabData.fullPath + "/" + fileName;

        // Save data for text editor
        if (tabData.type == EditorType::TEXT) {
            TextEditor* editor =
                qobject_cast<TextEditor*>(m_tabWidget->currentWidget());
            if (editor) {
                bool save =
                    targetSystems[targetId]->writeData(
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
                    if (targetSystems[targetId]->
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
                    if (targetSystems[targetId]->
                            launchShortUtility(cmd, output) == 0) {
                        onDirtyStateChanged(false, editor);
                    }
                }
            }
            return;
        }
    }
}

// Set preferences
void MainWindow::setPreferences() {
    // Create preferences dialog
    PreferencesDialog dlg(this);
    dlg.exec();

    // Get desired theme
    QString theme = dlg.getTheme();
    applyTheme(theme + ".json");
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

// Write text to the log
void MainWindow::log(const QString& text) {
    m_console->appendPlainText(text);
}

void MainWindow::updatePath(const QString& caseName, const QString& subDir,
                            int targetId) {
    // Get case path
    QString casePath = caseName;
    if (!subDir.isEmpty()) {
        casePath += "/" + subDir;
    }

    QString fullPath = m_caseMap[caseName].casePath + "/" + casePath;
    QStringList files = targetSystems[targetId]->getFiles(fullPath);

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

        // 4. Update the save action if the modified file is currently being viewed
        if (m_tabWidget->currentIndex() == index) {
            saveFileAction->setEnabled(isDirty);
        }
    }
}

void MainWindow::updateResultEditor(const QString& casePath, int targetId,
                                    const QString& timeFolder) {
    // Access data in time folder
    QString resultPath = casePath + "/postProcessing/surfaces/" + timeFolder;
    RenderData renderData =
        targetSystems[targetId]->getResultData(resultPath);
    std::shared_ptr<RenderData> newData =
        std::make_shared<RenderData>(std::move(renderData));
    if (newData) {

        // Access current editor
        ResultEditor* resultEditor =
            qobject_cast<ResultEditor*>(m_tabWidget->currentWidget());
        resultEditor->updateResult(newData);

        // Update tab text
        QString tabText = m_tabWidget->tabText(m_tabWidget->currentIndex());
        tabText.replace(
            QRegularExpression(R"(\(result@\d+\)$)"),
            QString("(result@%1)").arg(timeFolder));
        m_tabWidget->setTabText(m_tabWidget->currentIndex(), tabText);
    }
}

// Launch new case wizard
void MainWindow::runNewCaseWizard() {
    NewCaseWizard wizard(this);
    wizard.exec();
}

// Launch mesh configuration wizard
void MainWindow::runMeshConfiguration() {
    QString caseName = getSelectedCase();
    QStringList cases = m_caseMap.keys();
    MeshWizard wizard(caseName, cases, this);
    wizard.exec();
}

// Launch mesh execution wizard
void MainWindow::runMeshExecution() {
    QString caseName = getSelectedCase();
    QStringList cases = m_caseMap.keys();
    RunMeshDialog dialog(caseName, cases, this);
    dialog.exec();
}

// Check if utility is available
QMap<QString, bool> MainWindow::checkUtilities(const QString& fullPath,
                                int targetId, const QStringList& utilities) {
    // Get case path
    QMap<QString, bool> utilMap;
    QString casePath = fullPath.left(fullPath.lastIndexOf('/'));

    // Get OpenFOAM path
    QString openFoamPath;
    QString caseName = QFileInfo(casePath).fileName();
    if (m_caseMap.contains(caseName)) {
        openFoamPath = m_caseMap[caseName].openFoamPath;
    } else {
        qWarning() << "Case " << caseName << " is not in case map.";
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
    if (targetSystems[targetId]->launchShortUtility(cmd, output) == 0) {
        output.remove("\n");
        QStringList res;
        QStringList utils = output.split(",");
        for (const auto& util : std::as_const(utils)) {
            res = util.split(":");
            utilMap[res[0]] = (res[1] == "true");
        }
    } else {
        for (const auto& util : utilities) {
            utilMap[util] = false;
        }
    }
    return utilMap;
}

// Run autoPatch
void MainWindow::runMeshPatch(double angle, const QString& casePath,
                              int targetId) {
    // Get OpenFOAM path
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
        QMessageBox::warning(this, tr("Utility Not Found"),
                             tr("The autoPatch utility could not be found."));
        return;
    }

    // Run autoPatch
    QString result;
    QString cmd =
        QString("cd \"%1\"; source %2/etc/bashrc; autoPatch -overwrite %3")
            .arg(casePath, openFoamPath, QString::number(angle));
    targetSystems[targetId]->launchShortUtility(cmd, result);

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
            [this, watcher, progress, casePath, targetId]() {

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
                          caseName, casePath, openFoamPath, targetId);
    watcher->setFuture(future);

    updatePath(caseName, "constant/polyMesh", targetId);
}

void MainWindow::runMeshCheck(const QString& casePath, int targetId) {
    // Get OpenFoam path
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
    targetSystems[targetId]->launchShortUtility(cmd, logText);
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

    QRegularExpression orthoRegex("Mesh non-orthogonality Max:\\s+([\\d\\.]+)\\s+average:\\s+([\\d\\.]+)");
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
void MainWindow::runMeshRenumber(const QString& casePath, int targetId) {
    // Get OpenFoam path
    QString openFoamPath;
    QString caseName = QFileInfo(casePath).fileName();
    if (m_caseMap.contains(caseName)) {
        openFoamPath = m_caseMap[caseName].openFoamPath;
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
    targetSystems[targetId]->launchShortUtility(cmd, result);

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
    QRegularExpression beforeRegex("Before renumbering\\s+band\\s+:\\s+(\\S+)\\s+profile\\s+:\\s+(\\S+)");
    QRegularExpressionMatch beforeMatch = beforeRegex.match(result);
    if (beforeMatch.hasMatch()) {
        beforeBand = beforeMatch.captured(1).trimmed();
        beforeProfile = beforeMatch.captured(2).trimmed();
    }

    // Extract "After renumbering" statistics
    QRegularExpression afterRegex("After renumbering\\s+band\\s+:\\s+(\\S+)\\s+profile\\s+:\\s+(\\S+)");
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
        qWarning() << "STL file is not in a valid triSurface directory.";
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
            cmd = QString("cd \"%1\" && ln -s \"%2\" \"%3\" && "
                          "surfaceAutoPatch \"%3\" %4 \"%5\"; rm -f \"%3\"")
            .arg(path, fileName, symlinkName, QString::number(angle), tmpName);
        } else {
            cmd = QString("cd \"%1\" && surfaceAutoPatch \"%2\" %3 \"%4\"")
            .arg(path, fileName, QString::number(angle), tmpName);
        }

        // Run surfacePatch
        QString result;
        targetSystems[targetId]->launchShortUtility(cmd, result);
        qDebug() << result;
    } else if (utilMap.value("surfacePatch", false)) {
        // Create surfacePatchDict
        QString dictText =
            Utils::createSurfacePatchDict(openFoamPath, fileName, angle);
        targetSystems[targetId]->writeData(
            dictText.toUtf8(), casePath + "/system/surfacePatchDict");

        // Run surfacePatch
        cmd = QString("cd \"%1\"; source %2/etc/bashrc; surfacePatch").
              arg(casePath, openFoamPath);
        QString result;
        targetSystems[targetId]->launchShortUtility(cmd, result);

        // Check if a new file has been created
        if (result.contains("Writing repatched surface")) {

            // Read patched file content
            QString newPath = path + "/" + stem + "_patched.stl";
            QByteArray newData =
                targetSystems[targetId]->getFileContent(newPath);

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

void MainWindow::runSurfaceCheck(const QString& fullPath, int targetId,
                                 bool isBinary) {
    // Variables for command string
    QFileInfo info(fullPath);
    QString path = info.path();
    QString fileName = info.fileName();

    // Get the OpenFOAM path
    QString casePath, caseName, openFoamPath;
    int index = fullPath.lastIndexOf("/constant/triSurface");
    if (index != -1) {
        casePath = fullPath.left(index);
    } else {
        qWarning() << "STL file is not in a valid triSurface directory.";
        return;
    }

    caseName = QFileInfo(casePath).fileName();
    if (m_caseMap.contains(caseName)) {
        openFoamPath = m_caseMap[caseName].openFoamPath;
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
    targetSystems[targetId]->launchShortUtility(cmd, result);
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
        qWarning() << "STL file is not in a valid triSurface directory.";
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

void MainWindow::runSolverConfiguration() {
    // Get the selected case
    QString caseName = getSelectedCase();
    QStringList cases = m_caseMap.keys();

    // Create solver wizard if parsing succeeds
    SolverWizard* wizard = new SolverWizard(caseName, cases, m_solverFamilies,
        m_turbulenceModels, m_transportProperties, m_fieldData,
        m_boundaryConditions, this);
    if (wizard->parseFiles()) {
        wizard->exec();
    } else {
        wizard->deleteLater();
    }
}

// Launch solver execution wizard
void MainWindow::runSolverExecution() {
    // Launch the dialog
    QString caseName = getSelectedCase();
    QStringList cases = m_caseMap.keys();
    RunSolverDialog dialog(caseName, cases, this);
    dialog.exec();
}

// Stop solver execution
void MainWindow::stopSolverExecution() {}

void MainWindow::longUtilityFinished(const QString& status,
                    const QString& caseName, UtilityType utilityType) {
    // Get the ID of the target system
    int targetId = m_caseMap[caseName].targetSystemId;

    // Refresh navigator
    switch (utilityType) {
    case UtilityType::MESH:
        updatePath(caseName, "constant/polyMesh", targetId);
        updatePath(caseName, "system", targetId);
        break;
    case UtilityType::SOLVER:
        updatePath(caseName, "", targetId);
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

    // Save cases to settings
    settings.remove("Cases");
    settings.beginWriteArray("Cases");
    QStringList cases = m_navigator->getCases();

    // Iterate through cases
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

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        settings.setArrayIndex(i);
        QString fileName = m_tabWidget->tabText(i);
        if (fileName.endsWith(suffix)) {
            fileName.chop(suffix.length());
        }
        TabData data = m_tabMap.value(fileName);

        // Save values to settings
        settings.setValue("tabName", fileName);
        settings.setValue("fullPath", data.fullPath);
        settings.setValue("type", static_cast<int>(data.type));
    }
    settings.endArray();
}
