#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "core_types.h"

#include "dialogs/new_case/wizard_new_case.h"
#include "dialogs/mesh/wizard_mesh.h"
#include "dialogs/solver/wizard_solver.h"
#include "editors/tab_widget.h"
#include "systems/wsl_system.h"
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

enum class EditorType : int {
    TEXT = 0,
    MODEL,
    MESH,
    RESULTS,
    COUNT
};

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
    void createEditor(EditorType type, const QString& fileName, const QString& fullPath);

    // Create a new project
    void createCase(QString, QString, QStringList, int, QString);

    // Run mesh utilities
    void runMesh(const QString&, const QString&, bool, bool, bool, const int&);

    static bool isWindows() { return s_isWindows; }
    static bool isWslAvailable() { return s_isWslAvailable; }
    QString getSelectedCase() { return navigator->getSelectedCase(); }
    QMap<QString, CaseData> m_caseMap;
    QMap<QString, TabData> tabMap;
    TargetSystem* targetSystems[static_cast<int>(TargetType::COUNT)];

public slots:
    void log(const QString& text);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    CaseNavigator* navigator;
    Console* console;
    TabWidget* tabWidget;
    QFont font;

    // Check utilities
    QMap<QString, QMap<QString, bool>> m_utilMap;
    QStringList m_utilities = { "surfaceCheck", "surfacePatch", "surfaceAutoPatch", "blockMesh",
                               "surfaceFeatureExtract", "snappyHexMesh", "simpleFoam", "pimpleFoam",
                               "checkMesh", "decomposePar", "reconstructPar", "topoSet" };
    QMap<QString, bool> checkUtilities(const QString& fullPath, int targetId, const QStringList& utilities);

    // Keep track of utility output
    bool m_isStarted = false;
    QString m_utilityText;
    std::vector<QString> m_utilityItems;

    // Access solvers and families
    QList<FlowCompute::SolverFamily> m_solverFamilies;
    void loadSolverFamilies();

    // Access turbulence models
    FlowCompute::TurbulenceDatabase m_turbulenceModels;
    void loadTurbulenceModels();

    // Access fields
    QHash<QString, FlowCompute::FieldData> m_fieldData;
    void loadFieldData();

    // Access boundary conditions
    QList<FlowCompute::BoundaryCondition> m_boundaryConditions;
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
    QAction *meshAction;
    QAction *postProcessAction;
    QAction *runAction, *stopAction;
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
    QVulkanInstance m_vulkanInstance;

private slots:

    // File actions
    void saveFile();

    // Redo actions
    void undo();
    void redo();

    // Dirty state changed
    void onDirtyStateChanged(bool isDirty, QWidget* widget);

    // Mesh actions

    // Receive output
    void processUtilityOutput(const QString& line, UtilityType type);

    // Check if utilities are available
    void runMeshWizard();
    void runNewCaseWizard();
    void runSolverWizard();
    void runSurfacePatch(double featureAngle, const QString& fullPath, int targetId, bool isBinary);
    void runSurfaceCheck(const QString& fullPath, int targetId, bool isBinary);
};
#endif // MAIN_WINDOW_H
