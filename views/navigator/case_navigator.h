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

class CaseNavigator : public QTreeView {
    Q_OBJECT

 public:
    CaseNavigator(QAction* deleteAction, QAction* configureMeshAction,
        QAction* runMeshAction, QAction* viewMeshAction,
        QAction* configureSolverAction, QAction* runSolverAction,
        QAction* viewResultAction, QAction* cutAction, QAction* copyAction,
        QAction* pasteAction, const SystemManager& systemMgr,
        QWidget *parent = nullptr);
    void addCase(QString caseName, QStringList caseFiles, bool disable=false);
    void expandCase(QString caseName);
    QStringList getCases() const;
    QString getSelectedCase();
    void updatePath(QString path, QStringList children);
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

 protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

 signals:
    void createEditor(EditorType type, QString& fileName, const QString& path,
        bool logMessage);
    void logMessage(const QString& msg);
    void requestUpdatePath(const QString& caseName, const QString& subDir);

 private:
    void fetchChildren(NodeData* parentNode);
    NodeType checkType(QString name, QString fullPath);
    bool checkCaseFiles(QString caseName);

    QStringList m_clipboardPaths;
    bool m_isClipboardCut = false;

    const SystemManager& m_systemMgr;
    NavigatorModel* m_model;
    QStandardItem* m_root;

    // Menu and actions
    QAction *m_deleteAction, *m_renameAction;
    QAction *m_cutAction, *m_copyAction, *m_pasteAction;
    QAction *m_configureMeshAction, *m_runMeshAction, *m_viewMeshAction;
    QAction *m_configureSolverAction, *m_runSolverAction, *m_viewResultAction;

 private slots:
    // Slot to catch when a user clicks the expand arrow
    void onNodeExpanded(const QModelIndex &index);

    // Sets selection
    //void onSelectionChanged(const QItemSelection &selected,
    //                        const QItemSelection &deselected);

    // Displays the context menu
    void showContextMenu(const QPoint &pos);
};

#endif  // VIEWS_NAVIGATOR_CASE_NAVIGATOR_H_
