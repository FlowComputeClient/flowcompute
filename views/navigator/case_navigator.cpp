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

    // Create new project
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
    if (name.endsWith("Dict") || name.endsWith("Properties") ||
        name == "fvSchemes" || name.startsWith("fvSolution") || name.endsWith(".log")) {
        return NodeType::DictionaryFile;
    }

    // Check for script files
    if (name.startsWith("Allrun") || name.startsWith("Allclean") ||
        name.endsWith(".sh") || name.endsWith(".py")) {
        return NodeType::ScriptFile;
    }

    // Check for geometry files
    if (name.endsWith(".stl") || name.endsWith(".obj") || name.endsWith(".eMesh")) {
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
                (node->nodeType == NodeType::FieldFile)) {

                // Create editor for file
                mainWin->createEditor(EditorType::TEXT, node->name, node->fullPath);
            }

            // Open geometry file
            if (node->nodeType == NodeType::GeometryFile) {

                // Create editor for file
                mainWin->createEditor(EditorType::MODEL, node->name, node->fullPath);
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
    for(QString item : std::as_const(items)) {
        NodeData* childNode;
        if (!item.endsWith('|')) {
            childNode = new NodeData(item, nodePath, NodeType::Folder);
        } else {
            item.chop(1);
            NodeType type = checkType(item, nodePath);
            childNode = new NodeData(item, nodePath, type);
        }
        node->appendRow(childNode);
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

    // Create the menu
    m_contextMenu->addAction(m_deleteAction);

    // Execute the menu at the given location
    m_contextMenu->exec(viewport()->mapToGlobal(pos));
}

void CaseNavigator::deleteNode() {
    qDebug() << "Deleted!";
}