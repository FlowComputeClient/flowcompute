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

#include "page_10_tasks.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QMetaEnum>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "dialogs/selection/selection_dialog.h"
#include "wizards/post_processing/forces_dialog.h"
#include "wizards/post_processing/force_coeffs_dialog.h"
#include "wizards/post_processing/field_minmax_dialog.h"
#include "wizards/post_processing/probes_dialog.h"
#include "wizards/post_processing/surfaces_dialog.h"
#include "wizards/post_processing/yplus_dialog.h"
#include "wizards/solver/wizard_solver.h"

// Configure post-processing tasks
TasksPage::TasksPage(const QStringList& patchNames,
        const QStringList& fieldNames, QWidget *parent):
        m_patchNames(patchNames), m_fieldNames(fieldNames),
        QWizardPage(parent) {
    // Set title
    setTitle(tr("Post-Processing Tasks"));

    // Set layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    // Describe page
    QLabel* description = new QLabel(tr("Post-processing tasks extract results "
        "after a simulation has completed.\nThey make it possible to compute "
        "derived quantities and export data."),
        this);
    mainLayout->addWidget(description);
    mainLayout->addSpacing(10);

    // Create button
    QPushButton* taskButton = new QPushButton(tr("Add New Task"), this);
    connect(taskButton, &QPushButton::clicked, this, &TasksPage::addTask);
    taskButton->setFixedWidth(150);
    mainLayout->addWidget(taskButton);

    // Create table to identify post-processing tasks
    m_taskTable = new QTableWidget(this);
    m_taskTable->setColumnCount(4);

    // Set headers
    m_taskTable->setHorizontalHeaderLabels(
        {tr("Name"), tr("Type"), tr(""), tr("")});
    m_taskTable->setColumnWidth(0, 170);
    m_taskTable->setColumnWidth(1, 120);
    m_taskTable->setColumnWidth(2, 80);
    m_taskTable->setColumnWidth(3, 80);
    m_taskTable->verticalHeader()->setVisible(false);
    m_taskTable->setSelectionMode(QAbstractItemView::NoSelection);
    mainLayout->addWidget(m_taskTable);
    mainLayout->addStretch();

    // Set the page layout
    setLayout(mainLayout);
}

void TasksPage::addTask() {
    // Create task list
    QStringList taskList = {
        tr("Forces and Moments - "
           "Writes pressure and viscous force data to forces.dat"),
        tr("Force Coefficients - "
           "Writes lift, drag, and moment coefficients to forceCoeffs.dat"),
        tr("Field Extrema - "
           "Writes min/max field values and coordinates to fieldMinMax.dat"),
        tr("Probe Locations - "
           "Writes field values at specific times and locations to probes.dat"),
        tr("Surface Extraction - "
           "Produces geometry files containing field values on surfaces"),
        tr("Wall Distance Validation - "
           "Adds the yPlus field to time directories for turbulence validation")
    };

    // Create dialog
    SelectionDialog selectionDialog(tr("Post-processing Task Selection"),
        tr("Select one of the following tasks to be performed:"),
        taskList, this);

    // Proceed if the user clicked OK
    if (selectionDialog.exec() != QDialog::Accepted) {
        return;
    }

    // Get selection and launch dialog
    int index = selectionDialog.getSelectedIndex();
    QStringList res = launchDialog(index, -1);
    if (res.size() < 2) {
        return;
    }
    QString taskName = res[0];
    QString taskType = res[1];

    // Add a row to the table
    if (!taskName.isEmpty()) {
        int row = m_taskTable->rowCount();
        m_taskTable->insertRow(row);

        // Set the task name
        QTableWidgetItem* nameItem = new QTableWidgetItem(taskName);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_taskTable->setItem(row, 0, nameItem);

        // Set the task type
        QTableWidgetItem* typeItem = new QTableWidgetItem(taskType);
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
        m_taskTable->setItem(row, 1, typeItem);

        // Add the edit button
        QPushButton* editButton = new QPushButton(tr("Edit"), this);
        m_taskTable->setCellWidget(row, 2, editButton);

        // Respond when Edit is pressed
        connect(editButton,
                &QPushButton::clicked, this, [this, editButton, taskType]() {
            // Dynamically find current row
            int currentRow = -1;
            for (int i = 0; i < m_taskTable->rowCount(); ++i) {
                if (m_taskTable->cellWidget(i, 2) == editButton) {
                    currentRow = i;
                    break;
                }
            }
            if (currentRow == -1)
                return;

            // Launch dialog for given type
            QMetaEnum metaEnum =
                QMetaEnum::fromType<CaseIO::FunctionObject::FuncObjType>();
            QByteArray typeBytes = taskType.toUtf8();
            int typeIndex = metaEnum.keyToValue(typeBytes.constData());

            QStringList editRes = launchDialog(typeIndex, currentRow);
            if (editRes.size() == 2 && !editRes[0].isEmpty()) {
                m_taskTable->item(currentRow, 0)->setText(editRes[0]);
            }
        });

        // Add the delete button
        QPushButton* deleteButton = new QPushButton(tr("Delete"), this);
        m_taskTable->setCellWidget(row, 3, deleteButton);

        // Respond when the Delete button is pressed
        connect(deleteButton,
            &QPushButton::clicked, this, [this, deleteButton]() {
            // Dynamically find current row
            int currentRow = -1;
            for (int i = 0; i < m_taskTable->rowCount(); ++i) {
                if (m_taskTable->cellWidget(i, 3) == deleteButton) {
                    currentRow = i;
                    break;
                }
            }
            if (currentRow == -1)
                return;

            // Remove function object from vector
            m_functionObjects.erase(m_functionObjects.begin() + currentRow);

            // Delete row from table
            m_taskTable->removeRow(currentRow);
        });
    }
}

// Launch dialog for the given patch type
QStringList TasksPage::launchDialog(int typeIndex, int vectorIndex) {
    QString taskName;
    QString taskType;
    switch (typeIndex) {
    case 0: {
        ForcesDialog dlg = ForcesDialog(m_patchNames, m_forcesConfig, this);
        dlg.exec();
        m_forcesConfig = dlg.getFunctionObject();
        taskName = m_forcesConfig.name;
        taskType = "forces";
        if (vectorIndex == -1) {
            m_functionObjects.push_back(
                std::make_unique<CaseIO::ForcesConfig>(m_forcesConfig));
        } else {
            m_functionObjects[vectorIndex] =
                std::make_unique<CaseIO::ForcesConfig>(m_forcesConfig);
        }
        break;
    }
    case 1: {
        ForceCoeffsDialog dlg =
            ForceCoeffsDialog(m_patchNames, m_forceCoeffsConfig, this);
        dlg.exec();
        m_forceCoeffsConfig = dlg.getFunctionObject();
        taskName = m_forceCoeffsConfig.name;
        taskType = "forceCoeffs";
        if (vectorIndex == -1) {
            m_functionObjects.push_back(
                std::make_unique<CaseIO::ForceCoeffsConfig>(
                    m_forceCoeffsConfig));
        } else {
            m_functionObjects[vectorIndex] =
                std::make_unique<CaseIO::ForceCoeffsConfig>(
                    m_forceCoeffsConfig);
        }
        break;
    }
    case 2: {
        FieldMinMaxDialog dlg =
            FieldMinMaxDialog(m_fieldNames, m_fieldMinMaxConfig, this);
        dlg.exec();
        m_fieldMinMaxConfig = dlg.getFunctionObject();
        taskName = m_fieldMinMaxConfig.name;
        taskType = "fieldMinMax";
        if (vectorIndex == -1) {
            m_functionObjects.push_back(
                std::make_unique<CaseIO::FieldMinMaxConfig>(
                    m_fieldMinMaxConfig));
        } else {
            m_functionObjects[vectorIndex] =
                std::make_unique<CaseIO::FieldMinMaxConfig>(
                    m_fieldMinMaxConfig);
        }
        break;
    }
    case 3: {
        ProbesDialog dlg =
            ProbesDialog(m_fieldNames, m_probesConfig, this);
        dlg.exec();
        m_probesConfig = dlg.getFunctionObject();
        taskName = m_probesConfig.name;
        taskType = "probes";
        if (vectorIndex == -1) {
            m_functionObjects.push_back(
                std::make_unique<CaseIO::ProbesConfig>(m_probesConfig));
        } else {
            m_functionObjects[vectorIndex] =
                std::make_unique<CaseIO::ProbesConfig>(m_probesConfig);
        }
        break;
    }
    case 4: {
        SurfacesDialog dlg =
            SurfacesDialog(m_fieldNames, m_surfacesConfig, this);
        dlg.exec();
        m_surfacesConfig = dlg.getFunctionObject();
        taskName = m_surfacesConfig.name;
        taskType = "surfaces";
        if (vectorIndex == -1) {
            m_functionObjects.push_back(
                std::make_unique<CaseIO::SurfacesConfig>(m_surfacesConfig));
        } else {
            m_functionObjects[vectorIndex] =
                std::make_unique<CaseIO::SurfacesConfig>(m_surfacesConfig);
        }
        break;
    }
    case 5: {
        YPlusDialog dlg =
            YPlusDialog(m_patchNames, m_yPlusConfig, this);
        dlg.exec();
        m_yPlusConfig = dlg.getFunctionObject();
        taskName = m_yPlusConfig.name;
        taskType = "yPlus";
        if (vectorIndex == -1) {
            m_functionObjects.push_back(
                std::make_unique<CaseIO::YPlusConfig>(m_yPlusConfig));
        } else {
            m_functionObjects[vectorIndex] =
                std::make_unique<CaseIO::YPlusConfig>(m_yPlusConfig);
        }
        break;
    }
    };
    return {taskName, taskType};
}

void TasksPage::initializePage() {
    // Populate list of fields if empty
    if (m_fieldNames.isEmpty()) {

        // Access wizard
        QWizard* parentWizard = wizard();
        if (!parentWizard)
            return;

        // Make sure the parent is the SolverWizard
        SolverWizard* solverWizard = qobject_cast<SolverWizard*>(parentWizard);
        if (solverWizard) {
            m_fieldNames = solverWizard->getFieldNames();
        }
    }
}

std::vector<std::unique_ptr<CaseIO::FunctionObject>>
    TasksPage::getFunctionObjects() {
    return std::move(m_functionObjects);
}

bool TasksPage::validatePage() {
    return true;
}