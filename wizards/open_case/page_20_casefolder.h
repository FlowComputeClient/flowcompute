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

#ifndef WIZARDS_OPEN_CASE_PAGE_40_CASE_FOLDER_H_
#define WIZARDS_OPEN_CASE_PAGE_40_CASE_FOLDER_H_

class NewCaseWizard;
class QLabel;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

#include <QWizardPage>

#include "systems/system_manager.h"

class CaseFolderPage : public QWizardPage {
    Q_OBJECT

public:
    explicit CaseFolderPage(TargetType targetType, SystemManager& systemMgr,
                            QWidget* parent);
    QString openFoamPath;

protected:
    void initializePage() override;
    bool isComplete() const override;

private:
    SystemManager& m_systemMgr;
    TargetType m_targetType;
    QLabel *m_errorLabel;
    QLineEdit *m_geometryFileEdit, *m_casePathEdit;
    QTreeWidget *m_directoryTree;
    QString m_homeFolder;
    bool m_goodPath = false;

    void onTreeSelectionChanged();
    void populateDirectoryTree(QTreeWidget* treeWidget,
                               const QStringList& paths);

private slots:
    void onItemExpanded(QTreeWidgetItem* item);
};

#endif  // WIZARDS_OPEN_CASE_PAGE_40_CASE_FOLDER_H_
