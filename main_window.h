#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "core_types.h"

#include "dialogs/new_case/wizard_new_case.h"
#include "editors/tab_widget.h"
#include "editors/text/text_editor.h"
#include "geometry/graphic_data.h"
#include "systems/wsl_system.h"
#include "systems/local_system.h"
#include "views/navigator/case_navigator.h"
#include "views/console/console.h"

#include <QAction>
#include <QCoreApplication>
#include <QDockWidget>
#include <QFont>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QStandardPaths>
#include <QToolBar>
#include <QVersionNumber>
#include <QVulkanInstance>
#include <QWindow>

// Store information about project in navigator
struct CaseData {
    QString casePath;
    QStringList caseFiles;
    int targetSystemId;
    QString openFoamPath;
};

// Store information about tab in tabbar
struct TabData {
    QString fullPath;
    EditorType type;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // Display text in the tabbar
    void createEditor(EditorType type, QString& fileName, const QString& fullPath, bool logMessage = true);

    // Create a new project
    void createCase(QString, QString, QStringList, int, QString);

    // Run mesh utilities
    void runMesh(const QString& caseName, bool blockMesh, bool surfaceFeatureExtract,
                 bool snappyHexMesh, const QString& snappyCmd, int numCores);

    // Run solver utilities
    void runSolver(const QString& caseName, const QString& command);

    static bool isWindows() { return s_isWindows; }
    static bool isWslAvailable() { return s_isWslAvailable; }
    QMap<QString, CaseData> m_caseMap;
    QMap<QString, TabData> tabMap;
    TargetSystem* targetSystems[static_cast<int>(TargetType::COUNT)];
    void updatePath(const QString& caseName, const QString& subDir, int targetId);

public slots:
    void log(const QString& text);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    CaseNavigator* m_navigator;
    Console* m_console;
    TabWidget* m_tabWidget;
    QFont m_font;
    QDir m_configDir;
    QString m_themeFile;
    TextEditorConfig m_editorConfig;

    std::shared_ptr<RenderData> getMeshData(
        QString caseName, QString casePath,
        QString openFoamPath, int targetId);

    // Check utilities
    QMap<QString, QMap<QString, bool>> m_utilMap;
    QStringList m_utilities = { "surfaceCheck", "surfacePatch", "surfaceAutoPatch",
        "blockMesh", "surfaceFeatureExtract", "snappyHexMesh", "autoPatch", "renumberMesh",
        "checkMesh", "simpleFoam", "pimpleFoam", "decomposePar", "reconstructPar", "topoSet" };
    QMap<QString, bool> checkUtilities(const QString& fullPath, int targetId, const QStringList& utilities);

    QString getSelectedCase();

    // Access solvers and families
    std::vector<FlowCompute::SolverFamily> m_solverFamilies;
    void loadSolverFamilies();

    // Access material properties
    std::map<QString, FlowCompute::TransportPropertyDef> m_transportProperties;
    void loadMaterialProperties();

    // Access turbulence models
    FlowCompute::TurbulenceDatabase m_turbulenceModels;
    void loadTurbulenceModels();

    // Access fields
    QHash<QString, FlowCompute::FieldDef> m_fieldData;
    void loadFieldData();

    // Access boundary conditions
    std::vector<FlowCompute::BoundaryConditionDef> m_boundaryConditions;
    void loadBoundaryConditions();

    QDockWidget *navigatorWidget, *consoleWidget;

    // Create actions
    void createActions();
    QAction *newCaseAction;
    QAction *saveFileAction;

    /*
    QAction *newFileAction;
    QAction *openFileAction;
    QAction *saveAsAction;
    QAction *printAction;
    QAction *exitAction;
    */

    QAction *undoAction, *redoAction;
    QAction *zoomInAction, *zoomOutAction;
    QAction *meshAction, *runMeshAction;
    QAction *postProcessAction;
    QAction *solverAction, *runSolverAction, *stopSolverAction;
    QAction *aboutAction;

    // Menus
    void createMenus();
    QMenu *fileMenu, *editMenu, *viewMenu, *meshMenu, *solveMenu, *postProcessMenu, *helpMenu;

    // Toolbars
    void createToolBar();
    QToolBar *toolBar;

    // Check the OS and WSL
    static bool s_isWindows, s_isWslAvailable;

    WslSystem wslSystem;
    LocalSystem localSystem;
    QVulkanInstance m_vulkanInstance;

private slots:

    void applyTheme(QString themeFile);

    // File actions
    void saveFile();

    // Redo actions
    void undo();
    void redo();

    // Dirty state changed
    void onDirtyStateChanged(bool isDirty, QWidget* widget);

    // Check if utilities are available
    void runMeshConfiguration();
    void runMeshExecution();
    void runNewCaseWizard();
    void runSolverConfiguration();
    void runSolverExecution();
    void runSurfaceCheck(const QString& fullPath, int targetId, bool isBinary);
    void runSurfacePatch(double featureAngle, const QString& fullPath, int targetId, bool isBinary);
    void runSurfaceScale(double scaleFactor, const QString& fullPath, int targetId);
    void runMeshCheck(const QString& fullPath, int targetId);
    void runMeshPatch(double featureAngle, const QString& fullPath, int targetId);
    void runMeshRenumber(const QString& casePath, int targetId);
    void stopSolverExecution();

    // Utility finished
    void longUtilityFinished(const QString& status, const QString& caseName, UtilityType utilityType);
};
#endif // MAIN_WINDOW_H
