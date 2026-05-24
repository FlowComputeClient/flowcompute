#include "page_20_tutorial.h"
#include "page_10_intro.h"

#include "wizard_new_case.h"

TutorialPage::TutorialPage(QWidget *parent): QWizardPage(parent) {
    setTitle(tr("Select Base Case"));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);

    tutorialTree = new QTreeWidget(this);
    layout->addWidget(tutorialTree);

    // Create hidden text field
    hiddenPathField = new QLineEdit(this);
    hiddenPathField->hide();
    layout->addWidget(hiddenPathField);

    // Register the field
    registerField("tutorialPath*", hiddenPathField);

    connect(tutorialTree, &QTreeWidget::itemSelectionChanged,
            this, &TutorialPage::onTreeSelectionChanged);

    setLayout(layout);
}

void TutorialPage::initializePage() {

    QStringList pathList;
    NewCaseWizard* newWizard = qobject_cast<NewCaseWizard*>(wizard());
    if (newWizard) {
        QStringList pathList = newWizard->getTutorials();
        populateTutorialTree(tutorialTree, pathList);
    }
}

int TutorialPage::nextId() const {
    return static_cast<int>(WizardPage::Page_Project);
}

void TutorialPage::onTreeSelectionChanged() {
    QList<QTreeWidgetItem*> selectedItems = tutorialTree->selectedItems();

    if (!selectedItems.isEmpty()) {
        QTreeWidgetItem* item = selectedItems.first();
        QVariant pathData = item->data(0, Qt::UserRole);

        // Validate: Is it a valid case (leaf node)?
        if (pathData.isValid() && !pathData.toString().isEmpty()) {
            hiddenPathField->setText(pathData.toString());
            return;
        }
    }

    // If nothing is selected, clear the field
    hiddenPathField->clear();
}

void TutorialPage::populateTutorialTree(QTreeWidget* treeWidget, const QStringList& paths) {
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