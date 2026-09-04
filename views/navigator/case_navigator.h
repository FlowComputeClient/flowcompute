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

#ifndef VIEWS_NAVIGATOR_CASE_NAVIGATOR_H_
#define VIEWS_NAVIGATOR_CASE_NAVIGATOR_H_

#include <QMouseEvent>
#include <QPersistentModelIndex>
#include <QTreeView>

#include "./navigator_model.h"
#include "./node_data.h"
#include "systems/system_manager.h"

// Identifies the nature of the item to be added
enum class NewItemType { File, Folder, Dictionary };

class CaseNavigator : public QTreeView {
    Q_OBJECT

 public:
    CaseNavigator(QAction* newCaseAction, QAction* openCaseAction,
        QAction* configureMeshAction, QAction* runMeshAction,
        QAction* viewMeshAction, QAction* configureSolverAction,
        QAction* runSolverAction, QAction* viewResultAction, QAction* cutAction,
        QAction* copyAction, QAction* pasteAction, QAction* uploadAction,
        QAction* downloadAction, QAction* postProcessingAction,
        SystemManager& systemMgr, QWidget *parent = nullptr);
    void addCase(const QString& caseName, const QStringList& caseFiles,
                 bool disable=false);
    void expandCase(const QString& caseName);
    QStringList getCases() const;
    QString getSelectedCase();
    void updatePath(const QString& path, const QStringList& children);
    void addNodes(NodeData* parent, const QList<NodeData*>& children);
    void removeNode(NodeData* node);
    bool renameNode(NodeData* node, const QString& newName);
    void createChildren(NodeData* node, const QString& nodePath,
                        const QStringList& children);
    void cutCopySelection(bool isCut);
    void pasteSelection();
    bool getClipboardCut() { return m_isClipboardCut; }
    bool isItemCut(const QModelIndex& index) const;
    QStringList getClipboardPaths() const { return m_clipboardPaths; }
    NodeData* findNodeByPath(const QString& path) const;
    void addNewItem(NewItemType type);
    NodeData* nodeFromIndex(const QModelIndex &index) {
        return m_model->nodeFromIndex(index); }

    QList<QAction*> getActions() { return {m_newFileAction, m_newFolderAction,
                                            m_newDictAction, m_deleteAction}; }

 protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

 signals:
    void createTextEditor(QString& fileName, const QString& path,
                           bool logMessage);
    void createSurfaceEditor(QString& fileName, const QString& path,
                             bool logMessage);
    void logMessage(const QString& msg);
    void requestUpdatePath(const QString& caseName, const QString& subDir);
    void renameFile(const QString& filePath, const QString& newName);
    void removeFile(const QString& filePath, bool isCase);
    void cutPasteFile(const QString& oldPath, const QString& newPath);

 private:
    void createActions();
    void updateActions(NodeData* node);
    void fetchChildren(NodeData* parentNode);
    NodeType checkType(const QString& name, const QString& fullPath);
    bool checkConnection(const QString& caseName);
    void refresh(NodeData* node);

    QStringList m_clipboardPaths;
    bool m_isClipboardCut = false;

    SystemManager& m_systemMgr;
    NavigatorModel* m_model;
    QStandardItem* m_root;

    // Menu and actions
    QAction *m_newCaseAction, *m_openCaseAction;
    QAction *m_newFileAction, *m_newFolderAction, *m_newDictAction;
    QAction *m_deleteAction, *m_renameAction, *m_uploadAction;
    QAction *m_downloadAction, *m_refreshAction, *m_postProcessingAction;
    QAction *m_cutAction, *m_copyAction, *m_pasteAction;
    QAction *m_configureMeshAction, *m_runMeshAction, *m_viewMeshAction;
    QAction *m_configureSolverAction, *m_runSolverAction, *m_viewResultAction;

 private slots:
    // Slot to catch when a user clicks the expand arrow
    void onNodeExpanded(const QModelIndex &index);

    // Sets selection
    void onSelectionChanged(const QItemSelection &selected,
                           const QItemSelection &deselected);

    // Displays the context menu
    void showContextMenu(QPoint pos);

    // Deletes selected files
    void deleteFile();
};

#endif  // VIEWS_NAVIGATOR_CASE_NAVIGATOR_H_
