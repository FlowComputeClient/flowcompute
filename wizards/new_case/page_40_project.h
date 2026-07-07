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
    QLineEdit *m_geometryFileEdit, *m_casePathEdit;
    QTreeWidget *m_directoryTree;
    QString m_homeFolder;

    void onTreeSelectionChanged();
    void populateDirectoryTree(QTreeWidget* treeWidget, const QStringList& paths);

private slots:
    void onItemExpanded(QTreeWidgetItem* item);
};

#endif  // PAGE_40_PROJECT_H
