#include "main_window.h"

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
    connect(&wslSystem, &WslSystem::newLogLineReceived, this, &MainWindow::log);

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
    caseMap.clear();
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
        caseMap.insert(caseName, data);

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

        // Update tab widget
        if (data.type == EditorType::TEXT) {
            displayText(tabName, data.fullPath);
        } else if (data.type == EditorType::GEOMETRY) {
            displayGraphics(tabName, data.fullPath);
        }
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

    // Load data from solvers.json
    loadSolverFamilies();
    loadTurbulenceModels();
    loadFieldData();
    loadBoundaryConditions();
}

MainWindow::~MainWindow() = default;

/* Create QActions for file operations */
void MainWindow::createActions() {

    // Create new project
    newCaseAction = new QAction(QIcon(":/images/new_case.png"), tr("&New Case Folder"), this);
    newCaseAction->setShortcuts(QKeySequence::New);
    newCaseAction->setStatusTip(tr("Create a new case folder"));
    connect(newCaseAction, SIGNAL(triggered()), this, SLOT(newCaseFolder()));

    // Save file
    saveFileAction = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveFileAction->setShortcuts(QKeySequence::Save);
    saveFileAction->setStatusTip(tr("Save the file"));
    connect(saveFileAction, SIGNAL(triggered()), this, SLOT(saveFile()));
    saveFileAction->setDisabled(true);

    /*
    // Create new file
    new_fileAction = new QAction(QIcon(image_dir + "new.png"), tr("New &File"), this);
    new_fileAction->setShortcuts(QKeySequence::New);
    new_fileAction->setStatusTip(tr("Create a new file"));
    connect(new_fileAction, SIGNAL(triggered()), this, SLOT(NewFile()));

    // Open file
    open_fileAction = new QAction(QIcon(image_dir + "open.png"), tr("&Open..."), this);
    open_fileAction->setShortcuts(QKeySequence::Open);
    open_fileAction->setStatusTip(tr("Open an existing project"));
    connect(open_fileAction, SIGNAL(triggered()), this, SLOT(Open()));

    // Save as
    save_asAction = new QAction(tr("Save &As..."), this);
    save_asAction->setShortcuts(QKeySequence::SaveAs);
    save_asAction->setStatusTip(tr("Save the document under a new name"));
    connect(save_asAction, SIGNAL(triggered()), this, SLOT(SaveAs()));

    // Print
    printAction = new QAction(QIcon(image_dir + "print.png"), tr("&Print"), this);
    printAction->setShortcuts(QKeySequence::Print);
    printAction->setStatusTip(tr("Send the document to a printer"));
    connect(printAction, SIGNAL(triggered()), this, SLOT(Print()));

    // Exit
    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcuts(QKeySequence::Quit);
    exitAction->setStatusTip(tr("Exit the application"));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    // Cut
    cutAction = new QAction(QIcon(image_dir + "cut.png"), tr("Cut"), this);
    cutAction->setStatusTip(tr("Cut"));
    connect(cutAction, SIGNAL(triggered()), this, SLOT(Cut()));

    // Copy
    copyAction = new QAction(QIcon(image_dir + "copy.png"), tr("Copy"), this);
    copyAction->setStatusTip(tr("Copy"));
    //connect(copyAction, SIGNAL(triggered()), this, SLOT(copy()));

    // Paste
    pasteAction = new QAction(QIcon(image_dir + "paste.png"), tr("Paste"), this);
    pasteAction->setStatusTip(tr("Paste"));
    connect(pasteAction, SIGNAL(triggered()), this, SLOT(Paste()));
    */

    // Create undo action
    undoAction = new QAction(QIcon(":/images/undo.png"), tr("Undo"), this);
    undoAction->setShortcuts(QKeySequence::Undo);
    undoAction->setStatusTip(tr("Zoom in"));
    connect(undoAction, SIGNAL(triggered()), this, SLOT(undo()));
    undoAction->setDisabled(true);

    // Create redo action
    redoAction = new QAction(QIcon(":/images/redo.png"), tr("Redo"), this);
    redoAction->setShortcuts(QKeySequence::Redo);
    redoAction->setStatusTip(tr("Zoom out"));
    connect(redoAction, SIGNAL(triggered()), this, SLOT(redo()));
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
    meshAction = new QAction(QIcon(":/images/mesh.png"), tr("Create new &mesh"), this);
    meshAction->setStatusTip(tr("Create new mesh"));
    connect(meshAction, SIGNAL(triggered()), this, SLOT(createMesh()));

    // Configure the run action in the Solver menu
    runAction = new QAction(QIcon(":/images/play.png"), tr("&Launch solver"), this);
    runAction->setStatusTip(tr("Launch solver"));
    connect(runAction, SIGNAL(triggered()), this, SLOT(runSolverWizard()));

    // Configure the stop action in the Solver menu
    stopAction = new QAction(QIcon(":/images/stop.png"), tr("&Stop solver"), this);
    stopAction->setStatusTip(tr("Stop solver"));
    stopAction->setEnabled(false);
    // connect(meshAction, SIGNAL(triggered()), this, SLOT(About()));

    // Configure the mesh action in the Mesh menu
    postProcessAction = new QAction(QIcon(":/images/post_process.png"), tr("&Post-process"), this);
    postProcessAction->setStatusTip(tr("Launch post-processing"));
    // connect(meshAction, SIGNAL(triggered()), this, SLOT(About()));

    // Configure the About action in the help menu
    aboutAction = new QAction(QIcon(":/images/help.png"), tr("&Help"), this);
    aboutAction->setStatusTip(tr("Provide assistance"));
    // connect(aboutAction, SIGNAL(triggered()), this, SLOT(About()));
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

    // Create tool bar with solver actions
    toolBar->addAction(runAction);
    toolBar->addAction(stopAction);
    toolBar->addSeparator();

    // Create tool bar with view actions
    toolBar->addAction(postProcessAction);

    // Add help actions
    toolBar->addAction(aboutAction);
}

void MainWindow::displayText(QString fileName, QString fullPath) {

    // Get text
    QString caseName = fullPath.split('/').first();
    CaseData caseData = caseMap[caseName];
    int targetSystemId = caseData.targetSystemId;
    QString path = caseData.casePath + "/" + fullPath + "/" + fileName;
    QByteArray textData = targetSystems[targetSystemId]->getFileContent(path);

    // Create new tab
    TextEditor* textEditor = new TextEditor(this);
    textEditor->setFont(font);
    textEditor->setTextData(textData);
    int tabIndex = tabWidget->addTab(textEditor, fileName);
    tabWidget->setCurrentIndex(tabIndex);

    // Enable undo and redo actions according to the editor
    connect(textEditor->document(), &QTextDocument::undoAvailable,
            undoAction, &QAction::setEnabled);
    connect(textEditor->document(), &QTextDocument::redoAvailable,
            redoAction, &QAction::setEnabled);

    // Configure the text editor's event handling
    connect(textEditor, &TextEditor::dirtyStateChanged, this, [=, this](bool isDirty) {
        int index = tabWidget->indexOf(textEditor);
        if (index != -1) {
            QString tabText = fileName;
            if (isDirty) tabText += " *";
            tabWidget->setTabText(index, tabText);
            if (tabWidget->currentIndex() == index) {
                saveFileAction->setEnabled(isDirty);
            }
        }
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
}

// Create new tab
void MainWindow::displayGraphics(const QString& fileName, const QString& fullPath) {

    // Get data
    QString caseName = fullPath.split('/').first();
    CaseData caseData = caseMap[caseName];
    int targetSystemId = caseData.targetSystemId;
    QString path = caseData.casePath + "/" + fullPath + "/" + fileName;
    QByteArray data = targetSystems[targetSystemId]->getFileContent(path);

    // Access mesh data
    MeshData mesh;
    if(fileName.endsWith(".stl", Qt::CaseInsensitive)) {
        mesh = StlReader::readStlFile(fileName, data);
    }
    std::shared_ptr<MeshData> meshData = std::make_shared<MeshData>(std::move(mesh));

    // Create Vulkan window
    VulkanWindow* window = new VulkanWindow(meshData);
    window->setVulkanInstance(&m_vulkanInstance);

    // Create widget for the window
    QWidget* wrapper = QWidget::createWindowContainer(window);
    wrapper->setAttribute(Qt::WA_OpaquePaintEvent, true);
    wrapper->setAttribute(Qt::WA_NoSystemBackground, true);
    wrapper->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    wrapper->setFocusPolicy(Qt::StrongFocus);
    wrapper->setAttribute(Qt::WA_DeleteOnClose);

    // Add widget to the QTabWidget
    QFileInfo fileInfo(fileName);
    int tabIndex = tabWidget->addTab(wrapper, fileInfo.fileName());
    tabWidget->setCurrentIndex(tabIndex);

    // Disable text editor actions
    undoAction->setDisabled(true);
    redoAction->setDisabled(true);
    saveFileAction->setDisabled(true);
}

void MainWindow::runMesh(const QString& casePath, const QString& openFoamPath, bool runBlockMesh,
                         bool runSurfaceFeatureExtract, bool runSnappyHexMesh, const int& targetId) {

    // 1. Build the base environment setup and the sed replacement
    QStringList commands;
    commands << QString("source %1/etc/bashrc").arg(openFoamPath);
    commands << QString("cd %1").arg(casePath);

    // Replace the "ascii" in controlDict to "binary" - make sure mesh files are binary
    commands << "sed -i '0,/ascii/s//binary/' system/controlDict";

    // Append the requested utilities
    if (runBlockMesh) {
        commands << "blockMesh";
    }
    if (runSurfaceFeatureExtract) {
        commands << "surfaceFeatureExtract";
    }
    if (runSnappyHexMesh) {
        commands << "snappyHexMesh";
    }

    // Join the commands with the shell logical AND operator
    QString finalCmd = commands.join(" && ");

    // Fire and forget - let the remote Linux shell handle the sequence
    targetSystems[targetId]->launchUtility(finalCmd);
}

void MainWindow::storeTab(QString fileName, QString fullPath, EditorType type) {

    // Add tab to tabmap
    TabData data;
    data.fullPath = fullPath;
    data.type = type;
    tabMap.insert(fileName, data);
}

void MainWindow::createCase(QString caseName, QString casePath, QStringList caseFiles,
                            int targetSystemId, QString openFoamPath) {

    // Add case to map
    caseMap.insert(caseName, CaseData{casePath, caseFiles, targetSystemId, openFoamPath});

    // Display case in navigator
    navigator->addCase(caseName, caseFiles);
    navigator->expandCase(caseName);
}

// Create new case folder
void MainWindow::newCaseFolder() {

    // Launch new case wizard
    NewCaseWizard wizard(this);
    wizard.exec();
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
        int targetSystemId = caseMap[caseName].targetSystemId;
        QString remotePath = caseMap[caseName].casePath + "/" + tabData.fullPath + "/" + fileName;

        // Transfer data to the server
        if (tabData.type == EditorType::TEXT) {
            TextEditor* editor = qobject_cast<TextEditor*>(tabWidget->currentWidget());
            if (editor) {
                bool save = targetSystems[targetSystemId]->writeData(editor->toPlainText().toUtf8(),
                                                                     remotePath);
                if (save) editor->document()->setModified(false);
            }
        }
    }
}

// Launch new mesh wizard
void MainWindow::createMesh() {
    MeshWizard wizard(this);
    wizard.exec();
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
                    m_solverFamilies.append(family);
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

                QMap<QString, QList<FlowCompute::TurbulenceModel>> subCatMap;

                // Iterate through subcategories
                for (auto subCatIt = subCatObj.constBegin(); subCatIt != subCatObj.constEnd(); ++subCatIt) {
                    QString subCategoryName = subCatIt.key();
                    QJsonObject modelsObj = subCatIt.value().toObject();

                    QList<FlowCompute::TurbulenceModel> modelList;

                    // Iterate through models
                    for (auto modelIt = modelsObj.constBegin(); modelIt != modelsObj.constEnd(); ++modelIt) {
                        QString modelName = modelIt.key();
                        QJsonObject modelDataObj = modelIt.value().toObject();

                        FlowCompute::TurbulenceModel model;
                        model.name = modelName;
                        model.description = modelDataObj["description"].toString();
                        model.fields = modelDataObj["fields"].toVariant().toStringList();
                        modelList.append(model);
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

        FlowCompute::FieldData data;
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
        FlowCompute::BoundaryCondition bc;

        bc.name = obj.value("name").toString();

        // Convert the nested JSON arrays into QStringLists
        bc.categories = jsonArrayToStringList(obj.value("categories").toArray());
        bc.types      = jsonArrayToStringList(obj.value("types").toArray());
        bc.patchTypes = jsonArrayToStringList(obj.value("patchTypes").toArray());
        bc.parameters = jsonArrayToStringList(obj.value("parameters").toArray());
        m_boundaryConditions.append(bc);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {

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
        CaseData data = caseMap.value(caseName);

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

    for (int i = 0; i < tabWidget->count(); ++i) {
        settings.setArrayIndex(i);
        QString fileName = tabWidget->tabText(i);
        TabData data = tabMap.value(fileName);

        // Save values to settings
        settings.setValue("tabName", fileName);
        settings.setValue("fullPath", data.fullPath);
        settings.setValue("type", static_cast<int>(data.type));
    }
    settings.endArray();
}
