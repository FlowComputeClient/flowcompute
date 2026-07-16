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
    layout->addWidget(new QLabel(tr("Geometry files:")), 0, 0);
    m_geometryFileEdit = new QLineEdit(this);
    m_geometryFileEdit->setReadOnly(true);
    // m_geometryFileEdit->setText("C:/demo/y_junction.stl");
    registerField("geometryFile", m_geometryFileEdit);
    layout->addWidget(m_geometryFileEdit, 0, 1);

    // Create Browse button to search for geometry file
    QPushButton *browseButton = new QPushButton("Browse...", this);
    connect(browseButton, &QPushButton::clicked, this, [this]() {

        // Access the user's home directory
        QString homeDir =
            QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

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
    connect(m_directoryTree, &QTreeWidget::itemExpanded,
            this, &ProjectPage::onItemExpanded);

    // Set the page layout
    setLayout(layout);
}

void ProjectPage::initializePage() {

    // Get folders in the home directory
    NewCaseWizard* newWizard = qobject_cast<NewCaseWizard*>(this->wizard());
    QStringList pathList = newWizard->processPaths("");
    if (pathList.size() > 0) {
        m_homeFolder = pathList[0];
    }

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
        // Retrieve the absolute path
        QString selectedPath =
            selected.first()->data(0, Qt::UserRole).toString();

        // Update the LineEdit box
        m_casePathEdit->setText(selectedPath);
    }
}

void ProjectPage::populateDirectoryTree(QTreeWidget* treeWidget,
                                        const QStringList& paths) {
    // Safety check to ensure we have at least the home directory path
    if (paths.isEmpty())
        return;

    treeWidget->clear();
    treeWidget->setHeaderLabel(tr("Available Directories"));

    // Index 0 is the full path of the home directory
    QString rootPath = paths.at(0).trimmed();

    // Extract the display name
    QString rootDisplayName = rootPath.section('/', -1);
    if (rootDisplayName.isEmpty()) {
        rootDisplayName = "home";
    }

    // Create the single top-level root node
    QIcon folderIcon(":/images/folder.png");
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(treeWidget);
    rootItem->setText(0, rootDisplayName);
    rootItem->setData(0, Qt::UserRole, rootPath);
    rootItem->setIcon(0, folderIcon);

    // Populate the pre-known files/folders inside the home directory
    for (int i = 1; i < paths.size(); ++i) {
        QString name = paths.at(i).trimmed();

        // Skip regular files ending with '|'
        if (name.endsWith('|') || name.isEmpty()) {
            continue;
        }

        // Construct the full path for this immediate subfolder
        QString childPath = rootPath + "/" + name;

        // Create the child node under the root item
        QTreeWidgetItem* childItem = new QTreeWidgetItem(rootItem);
        childItem->setText(0, name);
        childItem->setData(0, Qt::UserRole, childPath);
        childItem->setIcon(0, folderIcon);

        // Add a dummy child to these subfolders so they show expansion arrows
        new QTreeWidgetItem(childItem);
    }

    // Configure the root item
    rootItem->sortChildren(0, Qt::AscendingOrder);
    rootItem->setExpanded(true);
    rootItem->setSelected(true);
}

void ProjectPage::onItemExpanded(QTreeWidgetItem* item) {
    if (!item) {
        return;
    }

    // Get the absolute path of the folder being expanded
    QString currentPath = item->data(0, Qt::UserRole).toString();

    // Clear the dummy placeholder node to make room for real contents
    qDeleteAll(item->takeChildren());

    // Get the list of file/folder names directly under this path
    NewCaseWizard* newWizard = qobject_cast<NewCaseWizard*>(this->wizard());
    QStringList contents = newWizard->processPaths(currentPath);

    // Populate the tree with the discovered subfolders
    for (const QString& name : std::as_const(contents)) {
        // Skip regular files ending with '|'
        if (name.endsWith('|')) {
            continue;
        }

        QString trimmedName = name.trimmed();
        if (trimmedName.isEmpty()) {
            continue;
        }

        // Construct the full path for this subfolder
        QString childPath = currentPath + "/" + trimmedName;

        // Create the child node
        QIcon folderIcon(":/images/folder.png");
        QTreeWidgetItem* childItem = new QTreeWidgetItem(item);
        childItem->setText(0, trimmedName);
        childItem->setData(0, Qt::UserRole, childPath);
        childItem->setIcon(0, folderIcon);

        // Add a dummy child to this new subfolder
        new QTreeWidgetItem(childItem);
    }

    // Sort the newly added sub-items alphabetically
    item->sortChildren(0, Qt::AscendingOrder);
}