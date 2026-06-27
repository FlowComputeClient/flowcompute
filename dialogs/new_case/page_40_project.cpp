#include "page_40_project.h"

#include "wizard_new_case.h"

// Asks for initial geometry file and project location
ProjectPage::ProjectPage(QWidget *parent): QWizardPage(parent) {

    // Set title
    setTitle(tr("Project Configuration"));

    // Create a grid layout with two columns
    QGridLayout* layout = new QGridLayout(this);
    layout->setSpacing(20);

    // Ask for case folder name
    layout->addWidget(new QLabel(tr("Geometry file:")), 0, 0);
    m_geometryFileEdit = new QLineEdit(this);
    m_geometryFileEdit->setReadOnly(true);
    m_geometryFileEdit->setText("/home/mattscar/y_junction.stl");
    registerField("geometryFile", m_geometryFileEdit);
    layout->addWidget(m_geometryFileEdit, 0, 1);

    // Create Browse button to search for geometry file
    QPushButton *browseButton = new QPushButton("Browse...", this);
    connect(browseButton, &QPushButton::clicked, this, [this]() {

        // Access the user's home directory
        QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

        // Create the file dialog
        QString geometryFilePath = QFileDialog::getOpenFileName(
            this, tr("Select a File"), homeDir, "Geometry files (*.stl *.obj)");

        // Update the text edit with the path
        if (!geometryFilePath.isEmpty()) {
            m_geometryFileEdit->setText(geometryFilePath);
        }
    });
    layout->addWidget(browseButton, 0, 2);

    // Create a text edit box for the project path
    layout->addWidget(new QLabel(tr("New Case Location:")), 1, 0);
    m_casePathEdit = new QLineEdit(this);
    m_casePathEdit->setReadOnly(true);
    registerField("casePath*", m_casePathEdit);
    layout->addWidget(m_casePathEdit, 1, 1);

    // Create tree to display directories
    m_directoryTree = new QTreeWidget(this);
    layout->addWidget(m_directoryTree, 2, 1);

    // Set event handling for the tree
    connect(m_directoryTree, &QTreeWidget::itemSelectionChanged,
            this, &ProjectPage::onTreeSelectionChanged);

    // Set the page layout
    setLayout(layout);
}

void ProjectPage::initializePage() {

    // Get folders in the home directory
    NewCaseWizard* newWizard = qobject_cast<NewCaseWizard*>(this->wizard());
    QStringList pathList = newWizard->getHomeFolders();

    // Etract the WSL home directory
    QString wslHomePath = "";
    if (!pathList.isEmpty()) {
        QString firstPath = pathList.first().trimmed();
        QStringList parts = firstPath.split('/', Qt::SkipEmptyParts);

        // Verify the path structure and grab the first two segments
        if (parts.size() >= 2 && parts[0] == "home") {
            wslHomePath = "/" + parts[0] + "/" + parts[1];
        }
    }

    // Set the text box to initially display the WSL home directory
    m_casePathEdit->setText(wslHomePath);

    // Populate the tree
    populateDirectoryTree(m_directoryTree, pathList);
}

void ProjectPage::onTreeSelectionChanged() {
    // Get the currently selected item(s) from the tree
    QList<QTreeWidgetItem*> selected = m_directoryTree->selectedItems();

    if (!selected.isEmpty()) {
        // Retrieve the full absolute path we stored in the UserRole data
        QString selectedPath = selected.first()->data(0, Qt::UserRole).toString();

        // Update the LineEdit box
        m_casePathEdit->setText(selectedPath);
    }
}

void ProjectPage::populateDirectoryTree(QTreeWidget* treeWidget, const QStringList& paths) {
    treeWidget->clear();
    treeWidget->setHeaderLabel(tr("Available Directories"));

    for (const QString& rawPath : paths) {
        QString path = rawPath.trimmed();
        if (path.isEmpty()) {
            continue;
        }

        QStringList parts = path.split('/', Qt::SkipEmptyParts);
        QTreeWidgetItem* currentParent = treeWidget->invisibleRootItem();

        // Start with an empty string, we will prepend the '/' in the loop
        QString currentPath = "";

        for (int i = 0; i < parts.size(); ++i) {
            const QString& part = parts[i];

            // Reconstruct the absolute path behind the scenes
            currentPath += "/" + part;

            // Skip the UI creation for the "home" directory
            if (i == 0 && part == "home") {
                continue;
            }

            QTreeWidgetItem* foundChild = nullptr;

            // Check if this folder already exists under the current parent
            for (int j = 0; j < currentParent->childCount(); ++j) {
                if (currentParent->child(j)->text(0) == part) {
                    foundChild = currentParent->child(j);
                    break;
                }
            }

            // If it doesn't exist, create it
            if (!foundChild) {
                foundChild = new QTreeWidgetItem(currentParent);
                foundChild->setText(0, part);

                // Store the intact absolute path invisibly on this node
                foundChild->setData(0, Qt::UserRole, currentPath);
            }

            // Move down a level
            currentParent = foundChild;
        }
    }

    treeWidget->sortItems(0, Qt::AscendingOrder);

    // Expand the top-level node automatically
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        treeWidget->topLevelItem(i)->setExpanded(true);
    }
}