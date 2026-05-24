#ifndef PAGE_20_TUTORIAL_H
#define PAGE_20_TUTORIAL_H

#include <QButtonGroup>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QStringList>
#include <QTreeWidget>
#include <QVariant>
#include <QVBoxLayout>
#include <QWizardPage>

class NewCaseWizard;

class TutorialPage : public QWizardPage {
    Q_OBJECT

public:
    explicit TutorialPage(QWidget*);
    int nextId() const override;

protected:
    void initializePage() override;

private:
    QLineEdit *hiddenPathField;
    QTreeWidget* tutorialTree;

    void onTreeSelectionChanged();
    void populateTutorialTree(QTreeWidget* treeWidget, const QStringList& paths);
};

#endif  // PAGE_20_TUTORIAL_H
