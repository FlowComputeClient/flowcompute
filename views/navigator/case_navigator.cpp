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

#include <QFutureWatcher>
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>

#include <algorithm>

#include "dialogs/new_dict/new_dict_dialog.h"
#include "views/navigator/case_navigator_delegate.h"

CaseNavigator::CaseNavigator(QAction* newCaseAction, QAction* openCaseAction,
    QAction* configureMeshAction, QAction* runMeshAction,
    QAction* viewMeshAction, QAction* configureSolverAction,
    QAction* runSolverAction, QAction* viewResultAction, QAction* cutAction,
    QAction* copyAction, QAction* pasteAction, QAction* uploadAction,
    QAction* downloadAction, SystemManager& systemMgr, QWidget *parent):
    m_newCaseAction(newCaseAction), m_openCaseAction(openCaseAction),
    m_configureMeshAction(configureMeshAction), m_runMeshAction(runMeshAction),
    m_viewMeshAction(viewMeshAction),
    m_configureSolverAction(configureSolverAction),
    m_runSolverAction(runSolverAction), m_viewResultAction(viewResultAction),
    m_cutAction(cutAction), m_copyAction(copyAction),
    m_pasteAction(pasteAction), m_uploadAction(uploadAction),
    m_downloadAction(downloadAction), m_systemMgr(systemMgr),
    QTreeView(parent) {
    // Configure behavior
    setHeaderHidden(true);
    setExpandsOnDoubleClick(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Set delegate to change how items are displayed
    CaseNavigatorDelegate *delegate = new CaseNavigatorDelegate(this);
    setItemDelegate(delegate);

    // Create actions
    createActions();

    // Set proper size for line edit when renaming files
    setStyleSheet(
        "QTreeView QLineEdit { padding: 0px; margin: 0px; border: none;}");

    // Create model
    m_model = new NavigatorModel(this);
    setModel(m_model);
    m_root = m_model->invisibleRootItem();

    // Configure the context menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Connect signals
    connect(this, &QWidget::customContextMenuRequested,
            this, &CaseNavigator::showContextMenu);
    connect(this, &QTreeView::expanded, this, &CaseNavigator::onNodeExpanded);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CaseNavigator::onSelectionChanged);
}

void CaseNavigator::createActions() {
    // Create new file
    m_newFileAction = new QAction(QIcon(":/images/new_file.png"),
                                  tr("New &File"), this);
    m_newFileAction->setStatusTip(tr("Create a new file"));
    m_newFileAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_newFileAction, &QAction::triggered, this, [this]() {
        addNewItem(NewItemType::File);
    });

    // Create new folder
    m_newFolderAction = new QAction(QIcon(":/images/new_folder.png"),
                                    tr("New &Folder"), this);
    m_newFolderAction->setShortcut(QKeySequence("Ctrl+Shift+N"));
    m_newFolderAction->setStatusTip(tr("Create a new folder"));
    m_newFolderAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_newFolderAction, &QAction::triggered, this, [this]() {
        addNewItem(NewItemType::Folder);
    });

    // Create new dictionary
    m_newDictAction = new QAction(QIcon(":/images/new_dict.png"),
                                  tr("New &Dictionary"), this);
    m_newDictAction->setShortcut(QKeySequence("Ctrl+Alt+D"));
    m_newDictAction->setStatusTip(tr("Create new dictionary"));
    m_newDictAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_newDictAction, &QAction::triggered, this, [this]() {
        addNewItem(NewItemType::Dictionary);
    });

    // Create action for rename
    m_renameAction =
        new QAction(QIcon(":/images/rename.png"), tr("Rename"), this);
    m_renameAction->setShortcut(Qt::Key_F2);
    m_renameAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_renameAction, &QAction::triggered, this, [this]() {
        QModelIndex index = currentIndex();
        if (index.isValid()) {
            edit(index);
        }
    });

    // Refresh folder
    m_refreshAction =
        new QAction(QIcon(":/images/refresh.png"), tr("Refresh"), this);
    m_refreshAction->setStatusTip(tr("Refresh folder"));
    connect(m_refreshAction, &QAction::triggered, this, [this]() {
        QModelIndex index = currentIndex();
        if (index.isValid()) {
            refresh(nodeFromIndex(index));
        }
    });

    // Delete file
    m_deleteAction =
        new QAction(QIcon(":/images/delete.png"), tr("&Delete"), this);
    m_deleteAction->setShortcuts(QKeySequence::Delete);
    m_deleteAction->setStatusTip(tr("Delete"));
    m_deleteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(m_deleteAction);
    connect(m_deleteAction, &QAction::triggered, this,
            &CaseNavigator::deleteFile);
}

void CaseNavigator::onSelectionChanged(const QItemSelection &selected,
                                       const QItemSelection &deselected) {
    // Calculate base selection states
    bool hasSelection = selectionModel()->hasSelection();
    bool hasSingleSelection = hasSelection &&
                              (selectionModel()->selectedIndexes().size() == 1);

    // Calculate node-specific states
    bool isFolderOrCase = false;
    bool isCaseFolder = false;

    if (hasSingleSelection) {
        QModelIndex index = selectionModel()->currentIndex();
        NodeData* node = m_model->nodeFromIndex(index);
        if (node) {
            isCaseFolder = (node->nodeType == NodeType::CaseFolder);
            isFolderOrCase = isCaseFolder ||
                             (node->nodeType == NodeType::Folder);
        }
    }

    // Apply states directly to universal actions
    m_deleteAction->setEnabled(hasSelection);
    m_cutAction->setEnabled(hasSelection);
    m_copyAction->setEnabled(hasSelection);
    m_downloadAction->setEnabled(hasSelection);

    // Apply states directly to single-target actions
    m_renameAction->setEnabled(hasSingleSelection);

    // Apply states directly to creation actions
    m_newFileAction->setEnabled(isFolderOrCase);
    m_newFolderAction->setEnabled(isFolderOrCase);
    m_newDictAction->setEnabled(isFolderOrCase);
    m_uploadAction->setEnabled(isFolderOrCase);
    m_refreshAction->setEnabled(isFolderOrCase);

    // Apply states directly to case-specific execution actions
    m_configureMeshAction->setEnabled(isCaseFolder);
    m_runMeshAction->setEnabled(isCaseFolder);
    m_viewMeshAction->setEnabled(isCaseFolder);
    m_configureSolverAction->setEnabled(isCaseFolder);
    m_runSolverAction->setEnabled(isCaseFolder);
    m_viewResultAction->setEnabled(isCaseFolder);
}

bool CaseNavigator::checkCaseFiles(QString caseName) {
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

    // Attempt to establish connection
    if (results[0] == "-2") {
        // Launch connect
        QFutureWatcher<std::pair<bool, QString>>* watcher =
            m_systemMgr.setupConnection();

        if (watcher != nullptr) {
            // Create progress dialog
            QProgressDialog* progress = new QProgressDialog(
                tr("Attempting to connect..."), QString(), 0, 0, this);
            progress->setWindowModality(Qt::WindowModal);
            progress->show();

            connect(watcher,
                    &QFutureWatcher<std::pair<bool, QString>>::finished,
                this, [this, watcher, progress, caseName]() {

                    // Remove the progress dialog
                    progress->accept();
                    progress->deleteLater();

                    std::pair<bool, QString> result = watcher->result();
                    if (result.first) {
                        for (int i = 0; i < m_root->rowCount(); ++i) {
                            NodeData* node =
                                static_cast<NodeData*>(m_root->child(i));
                            if (node) {
                                if (node->text() == caseName &&
                                    node->nodeType == NodeType::CaseFolder) {
                                    fetchChildren(node);
                                    break;
                                }
                            }
                        }
                    }
                    watcher->deleteLater();
                });
        }
        return false;
    }

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
    return true;
}

// Add a new case to the navigator
void CaseNavigator::addCase(QString caseName, QStringList caseFiles,
                            bool isDisabled) {
    // Create a node for the project
    NodeData* caseFolder =
        new NodeData(caseName, "", NodeType::CaseFolder, isDisabled);

    // Disable the main folder if the server is unreachable
    if (isDisabled) {
        caseFolder->setEnabled(false);
    }
    m_root->appendRow(caseFolder);

    // Create a node for each child
    NodeData* node;
    QList<NodeData*> childFolders, childFiles;
    for (QString item : std::as_const(caseFiles)) {
        if (!item.endsWith('|')) {
            node = new NodeData(item, caseName, NodeType::Folder, isDisabled);
            childFolders.push_back(node);
        } else {
            item.chop(1);
            NodeType type = checkType(item, caseName);
            node = new NodeData(item, caseName, type, isDisabled);
            childFiles.push_back(node);
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
        caseFolder->appendRow(child);
    }
    for (auto const& child : childFiles) {
        caseFolder->appendRow(child);
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
    return NodeType::TextFile;
}

void CaseNavigator::expandCase(QString caseName) {
    if (!m_root)
        return;

    // Iterate through top-level items
    for (int i = 0; i < m_root->rowCount(); ++i) {
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

        if (node && node->isEnabled()) {
            // Open dictionary file
            if ((node->nodeType == NodeType::DictionaryFile) ||
                (node->nodeType == NodeType::ScriptFile) ||
                (node->nodeType == NodeType::FieldFile) ||
                (node->nodeType == NodeType::TextFile) ||
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
        if (!node->isEnabled()) {
            QMessageBox::critical(this, tr("Cannot access OpenFOAM"),
                tr("FlowCompute can't find the installation of OpenFOAM."));
        }
    }
    QTreeView::mouseDoubleClickEvent(event);
}

void CaseNavigator::onNodeExpanded(const QModelIndex &index) {
    // Get node
    NodeData* node = m_model->nodeFromIndex(index);
    if (!node || !node->isEnabled())
        return;

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
    if (node->rowCount() > 0) {
        node->removeRow(0);
    }

    // Construct the full path
    int pos = node->fullPath.indexOf('/');
    QString caseName, nodePath;
    if (node->fullPath.isEmpty()) {
        caseName = node->name;
        nodePath = caseName;
    } else {
        caseName = node->fullPath.left(pos);
        nodePath = node->fullPath + "/" + node->name;
    }
    QString casePath = m_systemMgr.getData(caseName).casePath;
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
    if (!m_root || path.isEmpty())
        return;

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

    if (!currentNode)
        return;

    // Traverse the rest of the path
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

    // Access the selected node
    NodeData* node = m_model->nodeFromIndex(currentIndex());
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
    // Create context menu
    QMenu contextMenu(this);

    // Check index
    QModelIndex index = indexAt(pos);
    if (!index.isValid()) {
        contextMenu.addActions( { m_newCaseAction, m_openCaseAction });
        contextMenu.exec(viewport()->mapToGlobal(pos));
        return;
    }

    // Access the selected node
    NodeData* node = m_model->nodeFromIndex(index);
    if (!node) {
        return;
    }
    if (!node->isEnabled()) {
        QMessageBox::critical(this, tr("Cannot access OpenFOAM"),
            tr("FlowCompute can't find the installation of OpenFOAM."));
        return;
    }

    // Get the case name
    QString caseName;
    if (node->nodeType == NodeType::CaseFolder) {
        caseName = node->text();
    } else {
        int pos = node->fullPath.indexOf('/');
        caseName = (pos == -1) ? node->fullPath : node->fullPath.left(pos);
    }

    // Add actions for case folders
    if (node->nodeType == NodeType::CaseFolder) {
        // Check files in case
        bool accessCase = checkCaseFiles(node->name);
        if (!accessCase) {
            return;
        }

        // Mesh actions
        m_configureMeshAction->setData(QVariant::fromValue(node));
        contextMenu.addAction(m_configureMeshAction);

        m_runMeshAction->setData(QVariant::fromValue(node));
        contextMenu.addAction(m_runMeshAction);

        m_viewMeshAction->setData(QVariant::fromValue(node));
        contextMenu.addAction(m_viewMeshAction);
        contextMenu.addSeparator();

        // Solver actions
        m_configureSolverAction->setData(QVariant::fromValue(node));
        contextMenu.addAction(m_configureSolverAction);

        m_runSolverAction->setData(QVariant::fromValue(node));
        contextMenu.addAction(m_runSolverAction);

        m_viewResultAction->setData(QVariant::fromValue(node));
        contextMenu.addAction(m_viewResultAction);
        contextMenu.addSeparator();
    }

    // New File/New Folder/New Dictionary...
    if ((node->nodeType == NodeType::CaseFolder) ||
        (node->nodeType == NodeType::Folder)) {
        // New file/folder/dictionary actions
        contextMenu.addActions( { m_newFileAction, m_newFolderAction,
                               m_newDictAction } );

        contextMenu.addSeparator();
    }

    // Cut/copy actions
    if (node->nodeType != NodeType::CaseFolder) {
        contextMenu.addActions( { m_cutAction, m_copyAction });
    }

    // Paste action
    if ((node->nodeType == NodeType::CaseFolder) ||
        (node->nodeType == NodeType::Folder)) {
        contextMenu.addAction(m_pasteAction);
        if (m_clipboardPaths.empty()) {
            m_pasteAction->setDisabled(true);
        } else {
            m_pasteAction->setDisabled(false);
        }
    }

    contextMenu.addSeparator();

    // Rename/delete actions
    contextMenu.addActions( { m_renameAction, m_deleteAction });

    // Upload/download actions - not for local Linux
    int targetId = m_systemMgr.getData(caseName).targetId;
    if (targetId != TargetType::LOCAL_LINUX) {
        // Upload only for folders
        if ((node->nodeType == NodeType::CaseFolder) ||
            (node->nodeType == NodeType::Folder)) {
            // Upload action
            contextMenu.addAction(m_uploadAction);
        }

        // Download action
        contextMenu.addAction(m_downloadAction);
    }

    // Refresh action
    if ((node->nodeType == NodeType::CaseFolder) ||
        (node->nodeType == NodeType::Folder)) {
        contextMenu.addSeparator();
        contextMenu.addAction(m_refreshAction);
    }
    contextMenu.exec(viewport()->mapToGlobal(pos));
}

// Add child nodes to a given node
void CaseNavigator::addNodes(NodeData* parent,
                             const QList<NodeData*>& children) {
    if (children.isEmpty())
        return;

    // Default to the root item if no parent is provided
    QStandardItem* parentItem = parent ? parent : m_root;
    if (parentItem->rowCount() == 1) {
        QStandardItem* firstChild = parentItem->child(0);
        if (firstChild && firstChild->data(Qt::UserRole + 1).toBool() == true) {
            parentItem->removeRow(0);
        }
    }

    // Sort the incoming list
    QList<NodeData*> sortedChildren = children;
    std::sort(sortedChildren.begin(),
        sortedChildren.end(), [](const NodeData* a, const NodeData* b) {
            bool aIsFolder = (a->nodeType == NodeType::Folder);
            bool bIsFolder = (b->nodeType == NodeType::Folder);

            if (aIsFolder && !bIsFolder)
                return true;
            if (!aIsFolder && bIsFolder)
                return false;

            return QString::compare(a->name, b->name,
                                  Qt::CaseInsensitive) < 0;
        });

    // Merge-insert into the parent
    int insertRow = 0;
    for (NodeData* child : std::as_const(sortedChildren)) {
        bool childIsFolder = (child->nodeType == NodeType::Folder);
        bool duplicateFound = false;

        // Resume searching from the last known insertion point
        for (; insertRow < parentItem->rowCount(); ++insertRow) {
            NodeData* existing =
                static_cast<NodeData*>(parentItem->child(insertRow));
            if (!existing)
                continue;

            bool existingIsFolder = (existing->nodeType == NodeType::Folder);

            if (childIsFolder && !existingIsFolder) {
                break;
            }
            if (childIsFolder == existingIsFolder) {
                // Check comparison
                int cmp = QString::compare(child->name, existing->name,
                                           Qt::CaseInsensitive);
                if (cmp == 0) {
                    duplicateFound = true;
                    break;
                } else if (cmp < 0) {
                    break;
                }
            }
        }

        // Handle duplicates
        if (duplicateFound) {
            delete child;
            continue;
        }

        // Insert the node and increment the row
        parentItem->insertRow(insertRow, child);
        insertRow++;
    }
}

void CaseNavigator::addNewItem(NewItemType itemType) {
    // Access selected node
    QModelIndex parentIndex = currentIndex();
    NodeData* parentNode = m_model->nodeFromIndex(parentIndex);
    if (!parentNode)
        return;

    // Focus on the parent if the user selected a file
    if (parentNode->nodeType != NodeType::Folder &&
        parentNode->nodeType != NodeType::CaseFolder) {
        parentIndex = parentIndex.parent();
        parentNode = m_model->nodeFromIndex(parentIndex);
        if (!parentNode)
            return;
    }

    // Refresh children and expand node
    refresh(parentNode);

    // Construct the paths
    QString caseName = parentNode->fullPath.isEmpty() ? parentNode->name :
                           parentNode->fullPath.split("/")[0];

    // Directory where the new item will be placed
    QString relativePath = parentNode->fullPath.isEmpty() ?
        parentNode->name : parentNode->fullPath + "/" + parentNode->name;

    QString dictName;
    QString dictContent;
    if (itemType == NewItemType::Dictionary) {
        // Create the New Dictionary dialog
        CaseData caseData = m_systemMgr.getData(caseName);
        NewDictDialog dlg(caseName, caseData.openFoamPath, this);
        if (dlg.exec() != QDialog::Accepted)
            return;

        // Get the dialog's results
        dictName = dlg.getFileName();
        dictContent = dlg.getDictContent();
    }

    // Create the node to be added
    NodeData* newNode = nullptr;
    switch (itemType) {
    case NewItemType::File:
        newNode = new NodeData(tr("NewFile"), relativePath, NodeType::TextFile);
        break;
    case NewItemType::Folder:
        newNode = new NodeData(tr("NewFolder"), relativePath, NodeType::Folder);
        break;
    case NewItemType::Dictionary:
        newNode =
            new NodeData(dictName, relativePath, NodeType::DictionaryFile);
        break;
    }

    // Insert node in parent
    if (!newNode)
        return;
    parentNode->insertRow(0, newNode);

    // Get the QModelIndex for the new item
    QModelIndex newIndex = m_model->index(0, 0, parentIndex);
    if (newIndex.isValid()) {
        setCurrentIndex(newIndex);

        // Create dictionary or file/folder
        if (itemType == NewItemType::Dictionary) {
            // Write dictionary to file
            auto system = m_systemMgr.getSystem(caseName);
            if (!system) {
                return;
            }

            // Write the data to the server
            CaseData caseData = m_systemMgr.getData(caseName);
            QString absolutePath = caseData.casePath + "/" +
                                        relativePath + "/" + dictName;
            if (system->writeData(dictContent.toUtf8(), absolutePath)) {
                emit logMessage(tr("Created %1").arg(absolutePath));
            } else {
                emit logMessage(tr("Failed to create %1").arg(absolutePath));
            }
        } else {
            edit(newIndex);
        }
    }
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

// Rename the selected item
bool CaseNavigator::renameNode(NodeData* node, const QString& newName) {
    // Get case name
    QString caseName;
    if (node->fullPath.isEmpty()) {
        caseName = node->name;
    } else {
        caseName = node->fullPath.split('/').first();
    }

    // Check target system
    auto system = m_systemMgr.getSystem(caseName);
    if (!system) {
        emit logMessage(tr("Failed to connect to %1.").arg(caseName));
        return false;
    }

    // Create file after New File or New Folder operation
    if (((node->nodeType == NodeType::TextFile) && (node->name == "NewFile")) ||
        ((node->nodeType == NodeType::Folder) && (node->name == "NewFolder"))) {
        // Determine path
        QString newPath = m_systemMgr.getData(caseName).casePath + "/" +
                  node->fullPath;

        // Create folder or file
        bool success = false;
        if (node->nodeType == NodeType::Folder) {
            QStringList res = m_systemMgr.getSystem(caseName)->processPaths(
                newPath + "/" + newName, PathOperationType::CREATE);
            success = (!res.isEmpty() && res[0] == "0");
        } else {
            QString output;
            QString cmd =
                QString("cd '%1' && touch '%2'").arg(newPath, newName);
            success = m_systemMgr.getSystem(caseName)->launchShortUtility(
                           cmd, output) == 0;
        }

        if (success) {
            emit logMessage(tr("Created %1/%2").arg(newPath, newName));
        } else {
            emit logMessage(tr("Failed to create %1/%2").arg(newPath, newName));
        }
        return success;
    } else {
        // Determine old path and new path
        QString oldPath, newPath;
        if (node->fullPath.isEmpty()) {
            caseName = node->name;
            oldPath = caseName;
            newPath = newName;
        } else {
            caseName = node->fullPath.split('/').first();
            oldPath = m_systemMgr.getData(caseName).casePath + "/" +
                        node->fullPath + "/" + node->name;
            newPath = m_systemMgr.getData(caseName).casePath + "/" +
                        node->fullPath + "/" + newName;
        }

        // Perform file rename operation
        QString str = QStringList({oldPath, newPath}).join("\n");
        QStringList res = m_systemMgr.getSystem(caseName)->
                          processPaths(str, PathOperationType::RENAME);
        if (!res.isEmpty() && res[0] == "0") {
            emit logMessage(tr("Renamed %1 to %2").arg(oldPath, newPath));
            return true;
        } else {
            emit logMessage(
                tr("Failed to rename %1 to %2").arg(oldPath, newPath));
            return false;
        }
    }
}

// Keep track of items selected for cut
void CaseNavigator::cutCopySelection(bool isCut) {
    m_clipboardPaths.clear();
    m_isClipboardCut = isCut;

    // Add paths of selected nodes to list
    QModelIndexList selected = selectionModel()->selectedIndexes();
    for (const QModelIndex& index : std::as_const(selected)) {
        QString path = m_model->nodeFromIndex(index)->getPath();
        if (!path.isEmpty()) {
            m_clipboardPaths.append(path);
        }
    }
    viewport()->update();
}

// Check if an index has been cut
bool CaseNavigator::isItemCut(const QModelIndex& index) const {
    if (!m_isClipboardCut) { return false; }

    // Compare index path to the transient index
    QString indexPath = m_model->nodeFromIndex(index)->getPath();
    for (const QString& path : m_clipboardPaths) {
        if (indexPath == path)
            return true;
    }
    return false;
}

void CaseNavigator::pasteSelection() {
    if (m_clipboardPaths.isEmpty())
        return;

    // Determine full path of destination folder
    QModelIndex selectedIndex = selectionModel()->currentIndex();
    if (!selectedIndex.isValid())
        return;

    NodeData* destNode = m_model->nodeFromIndex(selectedIndex);
    QString destCase = destNode->fullPath.isEmpty() ?
                           destNode->name : destNode->fullPath.split("/")[0];
    QString destNodePath = destNode->fullPath.isEmpty() ?
                           destCase : destNode->fullPath + "/" + destNode->name;
    QString destinationDir =
        m_systemMgr.getData(destCase).casePath + "/" + destNodePath;

    // Create QStringList containing full paths
    QString caseName, casePath;
    QStringList fullPaths;
    for (const QString& relativePath : std::as_const(m_clipboardPaths)) {
        caseName = relativePath.split("/")[0];
        casePath = m_systemMgr.getData(caseName).casePath;
        fullPaths.append(casePath + "/" + relativePath);
    }
    fullPaths.append(destinationDir);

    // Perform copy operation
    QString copyStr = fullPaths.join("\n");
    auto system = m_systemMgr.getSystem(caseName);
    QStringList copyResults =
        system->processPaths(copyStr, PathOperationType::COPY);

    // Delete files due to cut
    QStringList cutResults;
    if (m_isClipboardCut) {
        fullPaths.removeLast();
        copyStr = fullPaths.join("\n");
        cutResults = system->processPaths(copyStr, PathOperationType::REMOVE);
    }

    // Add/remove nodes
    QList<NodeData*> newNodes;
    for (int i = 0; i < m_clipboardPaths.size(); i++) {
        NodeData* originalNode = findNodeByPath(m_clipboardPaths[i]);
        if (!originalNode)
            continue;

        if (copyResults[i] == "0") {
            if (m_isClipboardCut && cutResults[i] == "0") {
                QStandardItem* parentItem =
                    originalNode->parent() ? originalNode->parent() : m_root;

                // Remove row without deleting nodes
                QList<QStandardItem*> taken =
                    parentItem->takeRow(originalNode->row());
                if (!taken.isEmpty()) {
                    NodeData* movedNode = static_cast<NodeData*>(taken.first());
                    movedNode->fullPath = destNodePath;
                    newNodes.append(movedNode);
                }
            } else if (!m_isClipboardCut) {
                NodeData* clonedNode =
                    new NodeData(originalNode->name, destNodePath,
                                originalNode->nodeType);
                newNodes.append(clonedNode);
            }
        }
    }

    // Check if the destination's children have already been loaded
    bool isDestLoaded = true;
    if (destNode->rowCount() == 1) {
        QStandardItem* firstChild = destNode->child(0);
        if (firstChild && firstChild->data(Qt::UserRole + 1).toBool() == true) {
            isDestLoaded = false;
        }
    }

    // Add nodes if the folder has already been loaded
    if (isDestLoaded) {
        addNodes(destNode, newNodes);
    } else {
        if (!m_isClipboardCut) {
            qDeleteAll(newNodes);
        }
    }

    // For a cut, clear the clipboard and redraw
    if (m_isClipboardCut) {
        m_clipboardPaths.clear();
        m_isClipboardCut = false;
        viewport()->update();
    }
}

NodeData* CaseNavigator::findNodeByPath(const QString& path) const {
    if (!m_root || path.isEmpty()) return nullptr;

    QStringList parts = path.split('/');
    NodeData* current = nullptr;

    // Find the top-level node
    for (int i = 0; i < m_root->rowCount(); ++i) {
        // Assuming the root only contains valid NodeData case folders
        NodeData* child = static_cast<NodeData*>(m_root->child(i));
        if (child && child->text() == parts[0]) {
            current = child;
            break;
        }
    }

    if (!current)
        return nullptr;

    // Traverse the rest of the path
    for (int i = 1; i < parts.size(); ++i) {
        bool found = false;

        for (int j = 0; j < current->rowCount(); ++j) {
            QStandardItem* item = current->child(j);

            // Safety check: Skip dummy "Loading..." nodes
            if (item->data(Qt::UserRole + 1).toBool() == true) {
                continue;
            }

            NodeData* child = static_cast<NodeData*>(item);
            if (child && child->name == parts[i]) {
                current = child;
                found = true;
                break;
            }
        }

        // If any part of the path is missing, the node doesn't exist
        if (!found)
            return nullptr;
    }
    return current;
}

// Load node children and expand node
void CaseNavigator::refresh(NodeData* node) {
    // Make sure node is a folder or case folder
    if (!node)
        return;
    if ((node->nodeType != NodeType::CaseFolder) &&
        (node->nodeType != NodeType::Folder)) {
        node = (NodeData*)(node->parent());
    }

    // Construct path
    QString caseName, fullPath, nodePath;
    if (node->fullPath.isEmpty()) {
        caseName = node->name;
        nodePath = node->name;
    } else {
        caseName = node->fullPath.split("/")[0];
        nodePath = node->fullPath + "/" + node->name;
    }
    fullPath = m_systemMgr.getData(caseName).casePath + "/" + nodePath;

    // Get list of files
    QStringList files = m_systemMgr.getSystem(caseName)->processPaths(
        fullPath, PathOperationType::LIST);

    // Add nodes and expand
    if (!files.isEmpty()) {
        updatePath(nodePath, files);
    }
}

void CaseNavigator::deleteFile() {
    // Get Selection
    QModelIndexList selected = selectionModel()->selectedIndexes();
    if (selected.isEmpty())
        return;

    // Warning message
    QString warningText;
    if (selected.size() == 1) {
        NodeData* node = m_model->nodeFromIndex(selected.first());
        if (node) {
            warningText = tr("Are you sure you want to permanently "
                             "delete '%1'?").arg(node->name);
        }
    } else {
        warningText = tr("Are you sure you want to permanently "
                         "delete these %1 items?").arg(selected.size());
    }

    // Display warning dialog
    QMessageBox::StandardButton reply = QMessageBox::warning(
        this, tr("Confirm Deletion"), warningText,
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (reply != QMessageBox::Yes) {
        return;
    }

    // Build maps
    QString relativePath, caseName, fullPath;
    QMap<QString, QStringList> pathMap;
    QMap<QString, NodeData*> nodeMap;

    // Iterate through selected indices
    for (const QModelIndex& index : std::as_const(selected)) {
        NodeData* node = m_model->nodeFromIndex(index);
        if (!node)
            continue;

        relativePath = node->getPath();
        caseName = relativePath.split("/")[0];
        fullPath = m_systemMgr.getData(caseName).casePath + "/" + relativePath;
        pathMap[caseName].append(fullPath);
        nodeMap[fullPath] = node;
    }

    // Perform deletions
    for (auto it = pathMap.keyValueBegin(); it != pathMap.keyValueEnd(); ++it) {
        caseName = it->first;
        QStringList filesToDelete = it->second;
        QString fileStr = filesToDelete.join("\n");

        // Check access
        auto system = m_systemMgr.getSystem(caseName);
        if (!system) {
            emit logMessage(
                QString(tr("Failed to connect to %1.")).arg(caseName));
            continue;
        }

        // Delete files
        QStringList result =
            system->processPaths(fileStr, PathOperationType::REMOVE);

        // Ensure results match expected size to prevent out-of-bounds crash
        if (result.size() != filesToDelete.size()) {
            emit logMessage(QString(tr("Error: Unexpected response size.")));
            continue;
        }

        // Check results
        for (int i = 0; i < filesToDelete.size(); i++) {
            QString filePath = filesToDelete[i];

            if (result[i] == "0") {
                NodeData* currentNode = nodeMap.value(filePath);
                if (currentNode) {
                    // Remove node from tree
                    removeNode(currentNode);

                    // Remove case if needed
                    if (currentNode->fullPath.isEmpty()) {
                        m_systemMgr.removeCase(caseName);
                        emit updateSettings();
                    }
                }
                emit logMessage(QString(tr("Deleted %1")).arg(filePath));
            } else {
                emit logMessage(
                    QString(tr("Failed to delete %1")).arg(filePath));
            }
        }
    }
}