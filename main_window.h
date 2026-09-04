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

#include <QDir>
#include <QFont>
#include <QMainWindow>
#include <QVulkanInstance>

#include <map>
#include <memory>
#include <vector>

#include "editors/tab_widget.h"
#include "editors/text/text_widget.h"
#include "geometry/graphic_data.h"
#include "systems/system_manager.h"
#include "views/navigator/case_navigator.h"
#include "views/console/console.h"

#include "./core_types.h"

// Store information about each tab
struct TabData {
    EditorType type;
    std::optional<FileStats> stats;
};

class QAction;
class QDockWidget;
class QMenu;
class QToolBar;
class WslSystem;

class MainWindow : public QMainWindow {
    Q_OBJECT

 public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

 signals:
    void textThemeChanged(const TextEditorConfig& textTheme);
    void graphicalThemeChanged(const QString& graphicalTheme);

 protected:
    void closeEvent(QCloseEvent *event) override;

 private:
    // Create user interface
    void createActions();
    void createMenus();
    void createToolBar();
    void applyTheme(const QString &themeFile);

    // Load data
    std::shared_ptr<RenderData> getResultData(const QString& caseName,
        const QString& casePath, const QString& selectedField,
        const QString& timeFolder);
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
    QMenu *m_fileMenu, *m_editMenu, *m_viewMenu, *m_meshMenu, *m_simMenu,
        *helpMenu;
    QToolBar *toolBar;

    // Actions
    QAction *m_uploadAction, *m_downloadAction, *m_openCaseAction;
    QAction *m_newFileAction, *m_newFolderAction, *m_newDictAction;
    QAction *m_newCaseAction, *m_saveFileAction, *m_themeAction;
    QAction *m_deleteAction, *m_undoAction, *m_redoAction;
    QAction *m_languageAction, *m_cutAction, *m_copyAction, *m_pasteAction;
    QAction *m_zoomInAction, *m_zoomOutAction, *m_exitAction;
    QAction *m_configureMeshAction, *m_runMeshAction, *m_viewMeshAction;
    QAction *m_configureSolverAction, *m_runSolverAction, *m_stopSolverAction;
    QAction *m_viewResultAction, *m_postProcessAction;
    QAction *m_docAction, *m_issueAction, *m_licenseAction, *m_aboutQtAction;
    QAction *m_aboutFcAction;

    // Configuration data containers
    QMap<QString, TabData> m_tabMap;
    QMap<QString, QMap<QString, bool>> m_utilMap;
    std::vector<FlowCompute::SolverFamily> m_solverFamilies;
    std::map<QString, FlowCompute::TransportPropertyDef> m_transportProperties;
    FlowCompute::TurbulenceDatabase m_turbulenceModels;
    QHash<QString, FlowCompute::FieldDef> m_fieldData;
    std::vector<FlowCompute::BoundaryConditionDef> m_boundaryConditions;

    // Other
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
    TextWidget* m_currentEditor = nullptr;

 private slots:
    // Case-related functions
    void newCase();
    void openCase();
    void renameFile(const QString& filePath, const QString& newName);
    void removeFile(const QString& caseName, bool isCase);
    void cutPasteFile(const QString& oldPath, const QString& newPath);
    QString checkOpenFoam(int targetId);
    void createCase(const QString& caseName, const QString& casePath,
        const QStringList& caseFiles, int systemId, const QString& openFoamPath,
        CaseFlags flag, const QString& userName, const QString& hostName,
        int port);

    // Editor operations
    bool checkExistingEditor(const QString& fullPath);
    void createTextEditor(const QString& fileName, const QString& fullPath,
                            bool logMessage);
    void createSurfaceEditor(const QString& fileName, const QString& fullPath,
                             bool logMessage);
    void createMeshEditor(const QString& caseName, bool logMessage);
    void createResultEditor(const QString& caseName, bool logMessage);

    // Tab operations
    void tabChanged(int index);
    void tabClosed(const QString& tabId);
    void updateTab();
    // void updateTabSettings(const QString& tabName, const TabData& data);

    // File operations
    void saveFile();
    void undo();
    void redo();
    void upload();
    void download();
    void downloadFolder(std::shared_ptr<TargetSystem> system,
        const QString& basePath, const QString& nodeName,
        const QString& localPath);
    void onDirtyStateChanged(bool isDirty, QWidget* widget);

    // Surface editor slots
    void runSurfaceCheck(const QString& fullPath, bool isBinary);
    void runSurfacePatch(double featureAngle, const QString& fullPath,
                         bool isBinary, bool overwrite);
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

    // Solver-related
    void launchSolverConfigurationWizard();
    void launchSolverExecutionDialog();
    void runSolver(const QString& caseName, const QString& command);
    void stopSolver();
    void updateResult(const QString& casePath, const QString& timeFolder);
    void launchPostProcessingWizard();

    // Other
    void log(const QString& text);
    void longUtilityFinished(const QString& status, const QString& caseName,
                             UtilityType utilityType);
    void updatePath(const QString& caseName, const QString& subDir);
};
#endif  // MAIN_WINDOW_H_
