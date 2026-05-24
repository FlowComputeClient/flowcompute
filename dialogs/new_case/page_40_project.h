#ifndef PAGE_40_PROJECT_H
#define PAGE_40_PROJECT_H

#include <QDir>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QTreeView>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWizardPage>

class NewCaseWizard;

class ProjectPage : public QWizardPage {
    Q_OBJECT

public:
    explicit ProjectPage(QWidget*);
    QString openFoamPath;

protected:
    void initializePage() override;

private:
    QLineEdit *geometryFileEdit, *casePathEdit;
    QTreeWidget *directoryTree;

    void onTreeSelectionChanged();
    void populateDirectoryTree(QTreeWidget* treeWidget, const QStringList& paths);
};

#endif  // PAGE_40_PROJECT_H
