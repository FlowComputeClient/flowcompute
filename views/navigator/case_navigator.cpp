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

#include "views/navigator/case_navigator.h"

#include <QMenu>

#include <algorithm>

CaseNavigator::CaseNavigator(QAction* deleteAction,
    QAction* configureMeshAction, QAction* runMeshAction,
    QAction* viewMeshAction, QAction* configureSolverAction,
    QAction* runSolverAction, QAction* viewResultAction,
    const SystemManager& systemMgr, QWidget *parent):
    m_deleteAction(deleteAction), m_configureMeshAction(configureMeshAction),
    m_runMeshAction(runMeshAction), m_viewMeshAction(viewMeshAction),
    m_configureSolverAction(configureSolverAction),
    m_runSolverAction(runSolverAction), m_viewResultAction(viewResultAction),
    m_systemMgr(systemMgr), QTreeView(parent) {
    // Configure behavior
    setHeaderHidden(true);
    setExpandsOnDoubleClick(true);
    setSelectionMode(QAbstractItemView::SingleSelection);

    // Create model
    m_model = new NavigatorModel(this);
    setModel(m_model);
    m_root = m_model->invisibleRootItem();

    // Configure the context menu
    setContextMenuPolicy(Qt::CustomContextMenu);

    // Connect signals
    connect(this, &QWidget::customContextMenuRequested,
            this, &CaseNavigator::showContextMenu);
    connect(this, &QTreeView::expanded, this, &CaseNavigator::onNodeExpanded);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CaseNavigator::onSelectionChanged);
}

void CaseNavigator::checkCaseFiles(QString caseName) {
    // Get case path
    QString casePath = m_systemMgr.getData(caseName).casePath + "/" + caseName;

    // Check if mesh/field files are present
    bool meshFilesPresent = true, fieldFilesPresent = true;
    QStringList meshFiles = {casePath + "/constant/polyMesh/points",
                             casePath + "/constant/polyMesh/faces",
                             casePath + "/constant/polyMesh/owner",
                             casePath + "/constant/polyMesh/boundary" };
    QString meshFileString = meshFiles.join("\n");
    QStringList results = m_systemMgr.getSystem(caseName)->
        processPaths(meshFileString, PathOperationType::CHECK);

    if (results.contains("-1"))
        meshFilesPresent = false;

    // Check if field files are in 0.orig
    QString fieldFileString = casePath + "/0.orig";
    results = m_systemMgr.getSystem(caseName)->processPaths(
        fieldFileString, PathOperationType::LIST);
    fieldFilesPresent = !results.empty();

    // Check if field files are in 0
    if (!fieldFilesPresent) {
        fieldFileString = casePath + "/0";
        results = m_systemMgr.getSystem(caseName)->processPaths(
            fieldFileString, PathOperationType::LIST);
        fieldFilesPresent = !results.empty();
    }

    // Update actions
    m_viewMeshAction->setEnabled(meshFilesPresent);
    m_configureSolverAction->setEnabled(meshFilesPresent);
    m_runSolverAction->setEnabled(meshFilesPresent && fieldFilesPresent);
    m_viewResultAction->setEnabled(meshFilesPresent && fieldFilesPresent);
}

void CaseNavigator::onSelectionChanged(const QItemSelection &selected,
                                       const QItemSelection &deselected) {
    Q_UNUSED(deselected);
    if (selected.indexes().isEmpty())
        return;

    // Access the selected node
    QString caseName;
    NodeData* node = m_model->nodeFromIndex(selected.indexes().first());
    if (node->nodeType == NodeType::CaseFolder) {
        caseName = node->text();
    } else {
        int pos = node->fullPath.indexOf('/');
        caseName = (pos == -1) ? node->fullPath : node->fullPath.left(pos);
    }
    checkCaseFiles(caseName);
}

// Add a new case to the navigator
void CaseNavigator::addCase(QString caseName, QStringList caseFiles) {
    // Create a node for the project
    NodeData* caseFolder = new NodeData(caseName, "", NodeType::CaseFolder);
    m_root->appendRow(caseFolder);

    // Create a node for top-level files/folders
    NodeData* node;
    NodeType type;
    for (QString item : caseFiles) {
        if (!item.endsWith('|')) {
            node = new NodeData(item, caseName, NodeType::Folder);
        } else {
            item.chop(1);
            type = checkType(item, caseName);
            node = new NodeData(item, caseName, type);
        }
        caseFolder->appendRow(node);
    }
}

NodeType CaseNavigator::checkType(QString name, QString fullPath) {
    // Check for dictionary files
    if (name.endsWith("Dict") || name.endsWith("Properties") ||
        name.endsWith(".eMesh") || name == "fvSchemes" ||
        name.startsWith("fvSolution") || name.endsWith(".log")) {
        return NodeType::DictionaryFile;
    }

    // Check for script files
    if (name.startsWith("Allrun") || name.startsWith("Allclean") ||
        name.endsWith(".sh") || name.endsWith(".py")) {
        return NodeType::ScriptFile;
    }

    // Check for geometry files
    if (name.endsWith(".stl") || name.endsWith(".obj")) {
        return NodeType::GeometryFile;
    }

    // Check for mesh files
    if (fullPath.contains("constant/polyMesh")) {
        return NodeType::MeshFile;
    }

    // Check for Field files
    QStringList pathParts = fullPath.split('/');
    if (pathParts.size() > 1) {
        QString topLevelFolder = pathParts[1];

        // Check for field files
        if (topLevelFolder == "0.orig") {
            return NodeType::FieldFile;
        }
        bool isNumeric;
        topLevelFolder.toDouble(&isNumeric);
        if (isNumeric) {
            if (name != "uniform" && name != "polyMesh") {
                return NodeType::FieldFile;
            }
        }
    }
    // Fallback
    return NodeType::DictionaryFile;
}

void CaseNavigator::expandCase(QString caseName) {
    if (!m_root) return;

    // Iterate through top-level items
    for (int i = 0; i < m_root->rowCount(); ++i) {
        // Cast the generic QStandardItem back to your NodeData
        NodeData* node = static_cast<NodeData*>(m_root->child(i));
        if (node) {
            // Check if the node matches the criteria
            if (node->text() == caseName &&
                node->nodeType == NodeType::CaseFolder) {
                this->expand(node->index());
                break;
            }
        }
    }
}

void CaseNavigator::mouseDoubleClickEvent(QMouseEvent *event) {
    // Get model index
    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        // Access the node
        NodeData* node = m_model->nodeFromIndex(index);

        if (node) {
            // Open dictionary file
            if ((node->nodeType == NodeType::DictionaryFile) ||
                (node->nodeType == NodeType::ScriptFile) ||
                (node->nodeType == NodeType::FieldFile) ||
                (node->nodeType == NodeType::MeshFile)) {
                // Create editor for file
                emit createEditor(EditorType::TEXT, node->name,
                                  node->fullPath, true);
            }

            // Open geometry file
            if (node->nodeType == NodeType::GeometryFile) {
                // Create editor for file
                emit createEditor(EditorType::SURFACE, node->name,
                                    node->fullPath, true);
            }
        }
    }
    QTreeView::mouseDoubleClickEvent(event);
}

void CaseNavigator::onNodeExpanded(const QModelIndex &index) {
    // Get node
    NodeData* node = m_model->nodeFromIndex(index);
    if (!node) return;

    // Check if the node has a dummy child
    if (node->rowCount() == 1) {
        QStandardItem* firstChild = node->child(0);
        if (firstChild->data(Qt::UserRole + 1).toBool() == true) {
            fetchChildren(node);
        }
    }
}

void CaseNavigator::fetchChildren(NodeData* node) {
    // Remove the dummy child
    node->removeRow(0);

    // Construct the full path
    int pos = node->fullPath.indexOf('/');
    QString caseName = (pos == -1) ? node->fullPath : node->fullPath.left(pos);
    QString casePath = m_systemMgr.getData(caseName).casePath;
    QString nodePath = node->fullPath + "/" + node->name;
    QString fullPath = casePath + "/" + nodePath;

    // Access children at the given path
    QStringList items = m_systemMgr.getSystem(caseName)->processPaths(fullPath,
        PathOperationType::LIST);

    // Create a node for each child
    QList<NodeData*> childFolders, childFiles;
    for (QString item : std::as_const(items)) {
        NodeData* childNode;
        if (!item.endsWith('|')) {
            childNode = new NodeData(item, nodePath, NodeType::Folder);
            childFolders.push_back(childNode);
        } else {
            item.chop(1);
            NodeType type = checkType(item, nodePath);
            childNode = new NodeData(item, nodePath, type);
            childFiles.push_back(childNode);
        }
    }

    // Lambda to compare nodes
    auto sortAlphabetically = [](const NodeData* a, const NodeData* b) {
        return a->name.compare(b->name, Qt::CaseInsensitive) < 0;
    };

    // Sort folders and files
    std::sort(childFolders.begin(), childFolders.end(), sortAlphabetically);
    std::sort(childFiles.begin(), childFiles.end(), sortAlphabetically);

    // Display folders, then files
    for (auto const& child : childFolders) {
        node->appendRow(child);
    }
    for (auto const& child : childFiles) {
        node->appendRow(child);
    }
}

void CaseNavigator::updatePath(QString path, QStringList children) {
    if (!m_root || path.isEmpty()) return;

    // Traverse the tree to find the target node
    QStringList pathParts = path.split('/');
    NodeData* currentNode = nullptr;

    // Find the top-level case node
    for (int i = 0; i < m_root->rowCount(); ++i) {
        NodeData* node = static_cast<NodeData*>(m_root->child(i));
        if (node && node->text() == pathParts[0]) {
            currentNode = node;
            break;
        }
    }

    if (!currentNode) return;

    // Traverse down the rest of the path components, creating them if missing
    for (int i = 1; i < pathParts.size(); ++i) {
        QString part = pathParts[i];
        bool found = false;

        // Remove the dummy "Loading..." node
        if (currentNode->rowCount() == 1) {
            QStandardItem* firstChild = currentNode->child(0);
            if (firstChild->data(Qt::UserRole + 1).toBool() == true) {
                currentNode->removeRow(0);
            }
        }

        // Search for the existing folder
        for (int j = 0; j < currentNode->rowCount(); ++j) {
            // This static_cast is now 100% safe because the dummy is gone
            NodeData* child = static_cast<NodeData*>(currentNode->child(j));
            if (child && child->name == part) {
                currentNode = child;
                found = true;
                break;
            }
        }

        // Create intermediate folders
        if (!found) {
            QString parentPath;
            if (currentNode->nodeType == NodeType::CaseFolder) {
                parentPath = currentNode->name;
            } else {
                parentPath = currentNode->fullPath + "/" + currentNode->name;
            }

            NodeData* newFolder = new NodeData(part, parentPath,
                                               NodeType::Folder);
            currentNode->appendRow(newFolder);
            currentNode = newFolder;
        }
    }

    // Clear existing children of the target node to prepare for the update
    if (currentNode->rowCount() > 0) {
        currentNode->removeRows(0, currentNode->rowCount());
    }

    // Determine the base path for the new children
    QString nodePath;
    if (currentNode->nodeType == NodeType::CaseFolder) {
        nodePath = currentNode->name;
    } else {
        nodePath = currentNode->fullPath + "/" + currentNode->name;
    }

    // Parse children, create nodes, and sort them
    QList<NodeData*> childFolders, childFiles;
    for (QString item : std::as_const(children)) {
        NodeData* childNode;
        if (!item.endsWith('|')) {
            childNode = new NodeData(item, nodePath, NodeType::Folder);
            childFolders.push_back(childNode);
        } else {
            item.chop(1);
            NodeType type = checkType(item, nodePath);
            childNode = new NodeData(item, nodePath, type);
            childFiles.push_back(childNode);
        }
    }

    // Lambda to compare nodes alphabetically
    auto sortAlphabetically = [](const NodeData* a, const NodeData* b) {
        return a->name.compare(b->name, Qt::CaseInsensitive) < 0;
    };

    std::sort(childFolders.begin(), childFolders.end(), sortAlphabetically);
    std::sort(childFiles.begin(), childFiles.end(), sortAlphabetically);

    // Append folders first, then files
    for (auto const& child : childFolders) {
        currentNode->appendRow(child);
    }
    for (auto const& child : childFiles) {
        currentNode->appendRow(child);
    }

    // Expand the tree to ensure the updated node is visible
    this->expand(currentNode->index());

    // Walk up the tree and expand all ancestors
    QModelIndex parentIndex = currentNode->index().parent();
    while (parentIndex.isValid()) {
        this->expand(parentIndex);
        parentIndex = parentIndex.parent();
    }
}

QString CaseNavigator::getSelectedCase() {
    // If there's only one case, return the case
    if (m_root->rowCount() == 1) {
        NodeData* node = static_cast<NodeData*>(m_root->child(0));
        return node->text();
    }

    // Check selection
    QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        return QString();
    }

    // Access the selected node
    NodeData* node = m_model->nodeFromIndex(selectedIndexes.first());
    if (!node)
        return QString();
    if (node->nodeType == NodeType::CaseFolder) {
        return node->text();
    } else {
        int pos = node->fullPath.indexOf('/');
        QString caseName = (pos == -1) ? node->fullPath :
                               node->fullPath.left(pos);
        return caseName;
    }
}

QStringList CaseNavigator::getCases() const {
    QStringList caseNames;
    if (!m_root)
        return caseNames;

    // Get names of top-level items in tree
    for (int i = 0; i < m_root->rowCount(); ++i) {
        NodeData* node = static_cast<NodeData*>(m_root->child(i));
        if (node && node->nodeType == NodeType::CaseFolder) {
            caseNames.append(node->text());
        }
    }
    return caseNames;
}

void CaseNavigator::showContextMenu(const QPoint &pos) {

    QModelIndex index = indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    NodeData* node = m_model->nodeFromIndex(index);
    if (!node)
        return;

    // Create a fresh, local context menu instance
    QMenu contextMenu(this);

    // Setup the ubiquitous Delete action
    m_deleteAction->setData(QVariant::fromValue(node));
    contextMenu.addAction(m_deleteAction);

    // Conditionally add folder-specific actions
    switch (node->nodeType) {
    case NodeType::CaseFolder: {

        // Check files in case'
        checkCaseFiles(node->name);

        contextMenu.addSeparator();

        // Mesh Pipeline
        m_configureMeshAction->setData(QVariant::fromValue(node));
        contextMenu.addAction(m_configureMeshAction);

        m_runMeshAction->setData(QVariant::fromValue(node));
        contextMenu.addAction(m_runMeshAction);

        m_viewMeshAction->setData(QVariant::fromValue(node));
        contextMenu.addAction(m_viewMeshAction);

        contextMenu.addSeparator();

        // Solver Pipeline
        m_configureSolverAction->setData(QVariant::fromValue(node));
        contextMenu.addAction(m_configureSolverAction);

        m_runSolverAction->setData(QVariant::fromValue(node));
        contextMenu.addAction(m_runSolverAction);

        m_viewResultAction->setData(QVariant::fromValue(node));
        contextMenu.addAction(m_viewResultAction);
        break;
    }
    default:
        break;
    }

    // Execute blocking exec call safely on the stack-allocated menu
    contextMenu.exec(viewport()->mapToGlobal(pos));
}

void CaseNavigator::removeNode(NodeData* node) {
    if (!node)
        return;

    // Find the parent standard item
    QStandardItem* parentItem = node->parent();
    if (!parentItem) {
        parentItem = m_root;
    }

    // Identify and remove the row
    int row = node->row();
    if (row >= 0) {
        parentItem->removeRow(row);
    }
}
