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

#include "page_20_casefolder.h"

#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QTreeWidget>

// Asks for initial geometry file and project location
CaseFolderPage::CaseFolderPage(TargetType targetType, SystemManager& systemMgr,
    QWidget *parent): QWizardPage(parent), m_targetType(targetType),
    m_systemMgr(systemMgr), m_goodPath(false) {

    setTitle(tr("Select Case Folder"));

    QGridLayout* layout = new QGridLayout(this);
    layout->setSpacing(20);

    // Display selected path
    layout->addWidget(new QLabel(tr("Existing Case Location:")), 1, 0);
    m_casePathEdit = new QLineEdit(this);
    m_casePathEdit->setReadOnly(true);

    // Register selected path
    registerField("casePath", m_casePathEdit);
    layout->addWidget(m_casePathEdit, 1, 1);

    // Add the error label directly under the path box
    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet("color: red; font-weight: bold;");
    m_errorLabel->hide();
    layout->addWidget(m_errorLabel, 2, 1);

    m_directoryTree = new QTreeWidget(this);
    layout->addWidget(m_directoryTree, 3, 0, 1, 2);

    connect(m_directoryTree, &QTreeWidget::itemSelectionChanged,
            this, &CaseFolderPage::onTreeSelectionChanged);
    connect(m_directoryTree, &QTreeWidget::itemExpanded,
            this, &CaseFolderPage::onItemExpanded);

    setLayout(layout);
}

void CaseFolderPage::initializePage() {
    auto system = m_systemMgr.getSystem(static_cast<int>(m_targetType));
    if (!system) {
        qWarning() << "Failed to access server";
        return;
    }

    // Get list of files in the home directory
    QStringList pathList = system->processPaths("", PathOperationType::LIST);

    // Get the path of the home directory
    m_homeFolder = "";
    if (!pathList.isEmpty() && pathList.first().contains("home")) {
        QStringList parts = pathList.first().split('/', Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            m_homeFolder = "/" + parts[0] + "/" + parts[1];
        }
    }

    m_casePathEdit->setText(m_homeFolder);
    populateDirectoryTree(m_directoryTree, pathList);
}

bool CaseFolderPage::isComplete() const {
    return m_goodPath;
}

void CaseFolderPage::onTreeSelectionChanged() {
    QList<QTreeWidgetItem*> selected = m_directoryTree->selectedItems();
    if (selected.isEmpty()) return;

    QString selectedPath = selected.first()->data(0, Qt::UserRole).toString();
    m_casePathEdit->setText(selectedPath);

    // Reset state on every new click
    m_goodPath = false;
    m_errorLabel->hide();
    if (selectedPath.isEmpty() || selectedPath == m_homeFolder) {
        emit completeChanged();
        return;
    }

    QFileInfo info(selectedPath);
    QString casePath = info.path();
    QString caseName = info.fileName();

    // Check for duplicate
    if (m_systemMgr.contains(caseName)) {
        CaseData caseData = m_systemMgr.getData(caseName);
        if (caseData.casePath == casePath) {
            m_errorLabel->setText(tr("Error: This case is already "
                                     "open in the navigator."));
            m_errorLabel->show();
            emit completeChanged();
            return;
        }
    }

    // Validate it is an actual OpenFOAM case directory
    auto system = m_systemMgr.getSystem(static_cast<int>(m_targetType));
    if (system) {
        // Fetch contents to verify OpenFOAM structure
        QStringList folderContents = system->processPaths(selectedPath,
                                            PathOperationType::LIST);
        bool isValidCase = false;

        for (const QString& item : std::as_const(folderContents)) {
            if (item.endsWith("/system") || item == "system") {
                isValidCase = true;
                break;
            }
        }

        if (!isValidCase) {
            m_errorLabel->setText(tr("Error: Selected folder is not a "
                                     "valid OpenFOAM case."));
            m_errorLabel->show();
            emit completeChanged();
            return;
        }
    }

    // Enable page completion
    m_goodPath = true;
    emit completeChanged();
}

void CaseFolderPage::populateDirectoryTree(QTreeWidget* treeWidget,
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

    // Populate files/folders inside the home directory
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

        // Add a dummy child
        new QTreeWidgetItem(childItem);
    }

    // Configure the root item
    rootItem->sortChildren(0, Qt::AscendingOrder);
    rootItem->setExpanded(true);
    rootItem->setSelected(true);
}

void CaseFolderPage::onItemExpanded(QTreeWidgetItem* item) {
    if (!item) {
        return;
    }

    // Get the absolute path of the folder being expanded
    QString currentPath = item->data(0, Qt::UserRole).toString();

    // Clear the dummy placeholder node to make room for real contents
    qDeleteAll(item->takeChildren());

    // Get the list of file/folder names directly under this path
    auto system = m_systemMgr.getSystem(static_cast<int>(m_targetType));
    QStringList contents = system->processPaths(currentPath,
        PathOperationType::LIST);

    // Populate the tree with the discovered subfolders
    for (const QString& name : std::as_const(contents)) {
        // Skip regular files
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