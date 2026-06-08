#include "case_navigator.h"

#include "../../main_window.h"

CaseNavigator::CaseNavigator(QWidget *parent): QTreeView(parent) {

    setHeaderHidden(true);
    setExpandsOnDoubleClick(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setStyleSheet(R"(
        QTreeView {
            background-color: white;
            color: black;
            show-decoration-selected: 1;
        }
        QTreeView::branch:has-children:!has-siblings:closed,
        QTreeView::branch:closed:has-children:has-siblings {
            border-image: none;
            image: url(:/images/arrow_right.png);
        }
        QTreeView::branch:open:has-children:!has-siblings,
        QTreeView::branch:open:has-children:has-siblings  {
            border-image: none;
            image: url(:/images/arrow_down.png);
        }
        QMenu {
            background-color: white; border: 1px solid black;
        }
        QMenu::item {
            color: black; background-color: transparent;
        }
        QMenu::item:selected {
            background-color: #0078D7; color: white;
        }
    )");

    // Access main window
    mainWin = qobject_cast<MainWindow*>(this->parentWidget());

    // Set palette
    QPalette p = this->palette();
    p.setColor(QPalette::Text, Qt::black);
    p.setColor(QPalette::WindowText, Qt::black);
    p.setColor(QPalette::ButtonText, Qt::black);
    this->setPalette(p);

    // Create model
    model = new NavigatorModel(this);
    setModel(model);
    root = model->invisibleRootItem();

    // Configure the context menu and actions
    m_contextMenu = new QMenu(this);
    setContextMenuPolicy(Qt::CustomContextMenu);
    createActions();

    // Connect signal
    connect(this, &QWidget::customContextMenuRequested,
            this, &CaseNavigator::showContextMenu);

    // Connect the expanded signal to our custom slot
    connect(this, &QTreeView::expanded, this, &CaseNavigator::onNodeExpanded);
}

// Create actions for the context menu
void CaseNavigator::createActions() {

    // View mesh
    m_viewMeshAction = new QAction(QIcon(":/images/view_mesh.png"), tr("&View Mesh"), this);
    m_viewMeshAction->setStatusTip(tr("View Mesh"));
    connect(m_viewMeshAction, &QAction::triggered, this, &CaseNavigator::viewMesh);

    // Delete action
    m_deleteAction = new QAction(QIcon(":/images/delete.png"), tr("&Delete"), this);
    m_deleteAction->setShortcuts(QKeySequence::Delete);
    m_deleteAction->setStatusTip(tr("Delete"));
    connect(m_deleteAction, &QAction::triggered, this, &CaseNavigator::deleteNode);
}

// Add a new case to the navigator
void CaseNavigator::addCase(QString caseName, QStringList caseFiles) {

    // Create a node for the project
    NodeData* caseFolder = new NodeData(caseName, "", NodeType::CaseFolder);
    root->appendRow(caseFolder);

    // Create a node for top-level files/folders
    NodeData* node;
    NodeType type;
    for(QString item: caseFiles) {
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
    if (name.endsWith("Dict") || name.endsWith("Properties") || name.endsWith(".eMesh") ||
        name == "fvSchemes" || name.startsWith("fvSolution") || name.endsWith(".log")) {
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
        // The top-level folder is immediately after the Case Name
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
    if (!root) return;

    // Iterate through top-level items
    for (int i = 0; i < root->rowCount(); ++i) {

        // Cast the generic QStandardItem back to your NodeData
        NodeData* node = static_cast<NodeData*>(root->child(i));
        if (node) {
            // Check if the node matches the criteria
            if (node->text() == caseName && node->nodeType == NodeType::CaseFolder) {
                this->expand(node->index());
                break;
            }
        }
    }
}

void CaseNavigator::mouseDoubleClickEvent(QMouseEvent *event) {

    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {

        // Access the node
        NodeData* node = model->nodeFromIndex(index);

        if (node) {

            // Open dictionary file
            if ((node->nodeType == NodeType::DictionaryFile) ||
                (node->nodeType == NodeType::ScriptFile) ||
                (node->nodeType == NodeType::FieldFile) ||
                (node->nodeType == NodeType::MeshFile)) {

                // Create editor for file
                mainWin->createEditor(EditorType::TEXT, node->name, node->fullPath);
            }

            // Open geometry file
            if (node->nodeType == NodeType::GeometryFile) {

                // Create editor for file
                mainWin->createEditor(EditorType::SURFACE, node->name, node->fullPath);
            }
        }
    }
    QTreeView::mouseDoubleClickEvent(event);
}

void CaseNavigator::onNodeExpanded(const QModelIndex &index) {
    NodeData* node = model->nodeFromIndex(index);
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
    QString casePath = mainWin->m_caseMap[caseName].casePath;
    int targetSystemId = mainWin->m_caseMap[caseName].targetSystemId;
    QString nodePath = node->fullPath + "/" + node->name;
    QString fullPath = casePath + "/" + nodePath;

    // Access children at the given path
    QStringList items = mainWin->targetSystems[targetSystemId]->getFiles(fullPath);

    // Create a node for each child
    QList<NodeData*> childFolders, childFiles;
    for(QString item : std::as_const(items)) {
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
    for(auto const& child: childFolders) {
        node->appendRow(child);
    }
    for(auto const& child: childFiles) {
        node->appendRow(child);
    }
}

void CaseNavigator::updatePath(QString path, QStringList children) {
    if (!root || path.isEmpty()) return;

    // Traverse the tree to find the target node
    QStringList pathParts = path.split('/');
    NodeData* currentNode = nullptr;

    // Find the top-level case node
    for (int i = 0; i < root->rowCount(); ++i) {
        NodeData* node = static_cast<NodeData*>(root->child(i));
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

        for (int j = 0; j < currentNode->rowCount(); ++j) {
            NodeData* child = static_cast<NodeData*>(currentNode->child(j));
            if (child && child->name == part) {
                currentNode = child;
                found = true;
                break;
            }
        }

        // If the intermediate folder node does not exist, create it
        if (!found) {
            QString parentPath;
            if (currentNode->nodeType == NodeType::CaseFolder) {
                parentPath = currentNode->name;
            } else {
                parentPath = currentNode->fullPath + "/" + currentNode->name;
            }

            NodeData* newFolder = new NodeData(part, parentPath, NodeType::Folder);
            currentNode->appendRow(newFolder);
            currentNode = newFolder; // Step into the newly created folder
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
    for(QString item : std::as_const(children)) {
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
    for(auto const& child: childFolders) {
        currentNode->appendRow(child);
    }
    for(auto const& child: childFiles) {
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
    if (root->rowCount() == 1) {
        NodeData* node = static_cast<NodeData*>(root->child(0));
        return node->text();
    }

    // Check selection
    QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        return QString();
    }

    // Access the selected node
    NodeData* node = model->nodeFromIndex(selectedIndexes.first());
    if (!node) return QString();
    if (node->nodeType == NodeType::CaseFolder) {
        return node->text();
    } else {
        int pos = node->fullPath.indexOf('/');
        QString caseName = (pos == -1) ? node->fullPath : node->fullPath.left(pos);
        return caseName;
    }
}

QStringList CaseNavigator::getCases() const {
    QStringList caseNames;
    if (!root) {
        return caseNames;
    }

    // Get names of top-level items in tree
    for (int i = 0; i < root->rowCount(); ++i) {
        NodeData* node = static_cast<NodeData*>(root->child(i));
        if (node && node->nodeType == NodeType::CaseFolder) {
            caseNames.append(node->text());
        }
    }
    return caseNames;
}

void CaseNavigator::showContextMenu(const QPoint &pos) {

    // Get the node under the mouse cursor
    QModelIndex index = indexAt(pos);
    if (!index.isValid()) return;
    NodeData* node = model->nodeFromIndex(index);
    if (!node) return;

    // Add actions based on type
    switch(node->nodeType) {
    case NodeType::CaseFolder:
        m_viewMeshAction->setData(QVariant::fromValue(node));
        m_contextMenu->addAction(m_viewMeshAction);
        break;
    default:
        break;
    }

    // Add delete action
    m_deleteAction->setData(QVariant::fromValue(node));
    m_contextMenu->addAction(m_deleteAction);

    // Execute the menu at the given location
    m_contextMenu->exec(viewport()->mapToGlobal(pos));
}

void CaseNavigator::deleteNode() {
    qDebug() << "Deleted!";
}

void CaseNavigator::viewMesh() {

    // Access node
    QVariant data = m_deleteAction->data();
    NodeData* node = data.value<NodeData*>();
    if (!node) { return; }

    // Create editor for mesh
    mainWin->createEditor(EditorType::MESH, node->name, node->fullPath);

    // Clear data
    m_deleteAction->setData(QVariant());
}