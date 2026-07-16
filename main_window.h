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

#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

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

#include <map>
#include <memory>
#include <vector>

#include "editors/tab_widget.h"
#include "editors/text/text_editor.h"
#include "geometry/graphic_data.h"
#include "systems/system_manager.h"
#include "views/navigator/case_navigator.h"
#include "views/console/console.h"

#include "./core_types.h"

// Store information about each tab
struct TabData {
    QString fullPath;
    EditorType type;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

 public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

 protected:
    void closeEvent(QCloseEvent *event) override;

 private:
    // Create user interface
    void createActions();
    void createMenus();
    void createToolBar();
    void applyTheme(const QString &themeFile);

    // Load data
    std::shared_ptr<RenderData> getMeshData(QString caseName, QString casePath,
        QString openFoamPath);
    std::shared_ptr<RenderData> getResultData(QString caseName,
        QString resultPath);
    void loadSolverFamilies();
    void loadMaterialProperties();
    void loadTurbulenceModels();
    void loadFieldData();
    void loadBoundaryConditions();

    // Check if utilities are available
    QMap<QString, bool> checkUtilities(const QString& fullPath,
        const QStringList& utilities);

    // Determine the selected case
    QString getSelectedCase();

    // User interface members
    CaseNavigator* m_navigator;
    Console* m_console;
    TabWidget* m_tabWidget;
    QDockWidget *m_navigatorWidget, *m_consoleWidget;
    QMenu *fileMenu, *editMenu, *viewMenu, *meshMenu, *solveMenu,
        *postProcessMenu, *helpMenu;
    QToolBar *toolBar;

    // Actions
    QAction *m_newCaseAction, *m_saveFileAction, *m_preferencesAction;
    QAction *m_deleteAction, *m_undoAction, *m_redoAction;
    QAction *m_zoomInAction, *m_zoomOutAction;
    QAction *m_configureMeshAction, *m_runMeshAction, *m_viewMeshAction;
    QAction *m_configureSolverAction, *m_runSolverAction, *m_stopSolverAction;
    QAction *m_viewResultAction, *m_postProcessAction;
    QAction *m_aboutAction;

    // Configuration data containers
    QMap<QString, TabData> m_tabMap;
    QMap<QString, QMap<QString, bool>> m_utilMap;
    std::vector<FlowCompute::SolverFamily> m_solverFamilies;
    std::map<QString, FlowCompute::TransportPropertyDef> m_transportProperties;
    FlowCompute::TurbulenceDatabase m_turbulenceModels;
    QHash<QString, FlowCompute::FieldDef> m_fieldData;
    std::vector<FlowCompute::BoundaryConditionDef> m_boundaryConditions;

    // Other
    bool m_isWindows = false, m_isWslAvailable = false;
    QFont m_font;
    QDir m_configDir;
    QString m_themeFile;
    TextEditorConfig m_textTheme;
    QString m_graphicalTheme;
    SystemManager m_systemMgr;
    QStringList m_utilities = { "surfaceCheck", "surfacePatch",
        "surfaceAutoPatch", "blockMesh", "surfaceFeatureExtract",
        "surfaceFeatures", "snappyHexMesh", "autoPatch", "renumberMesh",
        "checkMesh", "simpleFoam", "pimpleFoam", "decomposePar",
        "reconstructPar", "topoSet" };
    QVulkanInstance m_vulkanInstance;

    /*
    QAction *newFileAction;
    QAction *openFileAction;
    QAction *saveAsAction;
    QAction *printAction;
    QAction *exitAction;
    */

 private slots:

    // Create new case
    void launchNewCaseWizard();
    void createCase(QString caseName, QString casePath, QStringList caseFiles,
                     int systemId, QString openFoamPath);

    // Editor/file actions
    void createEditor(EditorType type, QString& fileName,
                      const QString& fullPath, bool logMessage);
    void saveFile();
    void deleteFile();
    void undo();
    void redo();
    void onDirtyStateChanged(bool isDirty, QWidget* widget);

    // Surface editor slots
    void runSurfaceCheck(const QString& fullPath, bool isBinary);
    void runSurfacePatch(double featureAngle, const QString& fullPath,
                         bool isBinary);
    void runSurfaceScale(double scaleFactor, const QString& fullPath);

    // Mesh editor slots
    void runMeshCheck(const QString& fullPath);
    void runMeshPatch(double featureAngle, const QString& fullPath);
    void runMeshRenumber(const QString& casePath);

    // Mesh-related
    void launchMeshConfigurationWizard();
    void launchMeshExecutionDialog();
    void runMesh(const QString& caseName, bool blockMesh,
                 bool runSurfaceFeature, bool snappyHexMesh,
                 const QString& snappyCmd, int numCores);
    void viewMesh();

    // Solver-related
    void launchSolverConfigurationWizard();
    void launchSolverExecutionDialog();
    void runSolver(const QString& caseName, const QString& command);
    void stopSolver();
    void viewResult();
    void updateResult(const QString& casePath, const QString& timeFolder);

    // Other
    void log(const QString& text);
    void launchPreferencesDialog();
    void longUtilityFinished(const QString& status, const QString& caseName,
                             UtilityType utilityType);
    void updatePath(const QString& caseName, const QString& subDir);
};
#endif  // MAIN_WINDOW_H_
