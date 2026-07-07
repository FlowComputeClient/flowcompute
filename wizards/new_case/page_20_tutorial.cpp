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

#include "page_20_tutorial.h"
#include "page_10_intro.h"

#include "wizard_new_case.h"

TutorialPage::TutorialPage(QWidget *parent): QWizardPage(parent) {
    setTitle(tr("Select Base Case"));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);

    m_tutorialTree = new QTreeWidget(this);
    layout->addWidget(m_tutorialTree);

    // Create hidden text field
    m_hiddenPathEdit = new QLineEdit(this);
    m_hiddenPathEdit->hide();
    layout->addWidget(m_hiddenPathEdit);

    // Register the field
    registerField("tutorialPath*", m_hiddenPathEdit);

    connect(m_tutorialTree, &QTreeWidget::itemSelectionChanged,
            this, &TutorialPage::onTreeSelectionChanged);

    setLayout(layout);
}

void TutorialPage::initializePage() {

    QStringList pathList;
    NewCaseWizard* newWizard = qobject_cast<NewCaseWizard*>(wizard());
    if (newWizard) {
        QStringList pathList = newWizard->getTutorials();
        populateTutorialTree(m_tutorialTree, pathList);
    }
}

int TutorialPage::nextId() const {
    return static_cast<int>(WizardPage::Page_Project);
}

void TutorialPage::onTreeSelectionChanged() {
    QList<QTreeWidgetItem*> selectedItems = m_tutorialTree->selectedItems();

    if (!selectedItems.isEmpty()) {
        QTreeWidgetItem* item = selectedItems.first();
        QVariant pathData = item->data(0, Qt::UserRole);

        // Validate: Is it a valid case (leaf node)?
        if (pathData.isValid() && !pathData.toString().isEmpty()) {
            m_hiddenPathEdit->setText(pathData.toString());
            return;
        }
    }

    // If nothing is selected, clear the field
    m_hiddenPathEdit->clear();
}

void TutorialPage::populateTutorialTree(QTreeWidget* treeWidget,
                                        const QStringList& paths) {
    treeWidget->clear();
    treeWidget->setHeaderLabel(QObject::tr("OpenFOAM Tutorials"));

    for (const QString& rawPath : paths) {
        QString path = rawPath.trimmed();

        if (path.startsWith("./")) {
            path.remove(0, 2);
        }

        if (path.endsWith("/system")) {
            path.chop(7);
        } else if (path.endsWith("system")) {
            path.chop(6);
        }

        if (path.isEmpty()) {
            continue;
        }

        QStringList parts = path.split('/', Qt::SkipEmptyParts);

        int tutIndex = parts.indexOf("tutorials");
        if (tutIndex != -1) {
            parts = parts.mid(tutIndex + 1);
        }

        if (parts.isEmpty()) {
            continue;
        }

        QTreeWidgetItem* currentParent = treeWidget->invisibleRootItem();
        for (const QString& part : std::as_const(parts)) {
            QTreeWidgetItem* foundChild = nullptr;

            for (int i = 0; i < currentParent->childCount(); ++i) {
                if (currentParent->child(i)->text(0) == part) {
                    foundChild = currentParent->child(i);
                    break;
                }
            }

            if (!foundChild) {
                foundChild = new QTreeWidgetItem(currentParent);
                foundChild->setText(0, part);
            }
            currentParent = foundChild;
        }
        currentParent->setData(0, Qt::UserRole, path);
    }
    treeWidget->sortItems(0, Qt::AscendingOrder);
}
