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

#ifndef WIZARDS_POST_PROCESSING_PAGE_10_TASKS_H_
#define WIZARDS_POST_PROCESSING_PAGE_10_TASKS_H_

#include <QWizardPage>

#include <memory>

#include "parser/function_object.h"

class QTableWidget;

class TasksPage : public QWizardPage {
    Q_OBJECT

 public:
    explicit TasksPage(const QStringList& patchNames,
        const QStringList& fieldNames,
        std::vector<std::unique_ptr<CaseIO::FunctionObject>>& functionObjects,
        QWidget *parent);

 protected:
    void initializePage() override;
    // bool validatePage() override;

 private:
    const QStringList& m_patchNames;
    QStringList m_fieldNames;
    QTableWidget* m_taskTable;

    // Structures for function objects
    std::vector<std::unique_ptr<CaseIO::FunctionObject>>& m_functionObjects;

    // launch dialog for given type
    CaseIO::FunctionObject* launchDialog(int rawTypeIndex, int vectorIndex);

    // Update dialog with a function object
    void updateTable(const QString& taskName, const QString& taskType,
                        CaseIO::FunctionObject* funcObjPtr);

 private slots:
    void addTask();
};

#endif  // WIZARDS_POST_PROCESSING_PAGE_10_TASKS_H_
