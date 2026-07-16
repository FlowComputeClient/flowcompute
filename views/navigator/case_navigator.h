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
        QAction* viewResultAction, const SystemManager& systemMgr,
        QWidget *parent = nullptr);
    void addCase(QString caseName, QStringList caseFiles);
    void expandCase(QString caseName);
    QStringList getCases() const;
    QString getSelectedCase();
    void updatePath(QString path, QStringList children);
    void removeNode(NodeData* node);
    void createChildren(NodeData* node, const QString& nodePath,
                        const QStringList& children);

 protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

 signals:
    void createEditor(EditorType type, QString& fileName, const QString& path,
        bool logMessage);

 private:
    void fetchChildren(NodeData* parentNode);
    NodeType checkType(QString name, QString fullPath);
    void checkCaseFiles(QString caseName);

    const SystemManager& m_systemMgr;
    NavigatorModel* m_model;
    QStandardItem* m_root;

    // Menu and actions
    QAction *m_deleteAction;
    QAction *m_configureMeshAction, *m_runMeshAction, *m_viewMeshAction;
    QAction *m_configureSolverAction, *m_runSolverAction, *m_viewResultAction;

 private slots:
    // Slot to catch when a user clicks the expand arrow
    void onNodeExpanded(const QModelIndex &index);

    // Sets selection
    void onSelectionChanged(const QItemSelection &selected,
                            const QItemSelection &deselected);

    // Displays the context menu
    void showContextMenu(const QPoint &pos);
};

#endif  // VIEWS_NAVIGATOR_CASE_NAVIGATOR_H_
