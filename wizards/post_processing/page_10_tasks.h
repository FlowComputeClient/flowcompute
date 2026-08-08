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
        const QStringList& fieldNames, QWidget *parent);
    std::vector<std::unique_ptr<CaseIO::FunctionObject>> getFunctionObjects();

 protected:
    void initializePage() override;
    bool validatePage() override;

 private:
    const QStringList& m_patchNames;
    QStringList m_fieldNames;
    QTableWidget* m_taskTable;

    // Structures for function objects
    CaseIO::ForcesConfig m_forcesConfig;
    CaseIO::ForceCoeffsConfig m_forceCoeffsConfig;
    CaseIO::FieldMinMaxConfig m_fieldMinMaxConfig;
    CaseIO::ProbesConfig m_probesConfig;
    CaseIO::SurfacesConfig m_surfacesConfig;
    CaseIO::YPlusConfig m_yPlusConfig;
    std::vector<std::unique_ptr<CaseIO::FunctionObject>> m_functionObjects;

    // launch dialog for given type
    QStringList launchDialog(int typeIndex, int vectorIndex);

 private slots:
    void addTask();
};

#endif  // WIZARDS_POST_PROCESSING_PAGE_10_TASKS_H_
