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

#include "wizards/post_processing/page_10_tasks.h"

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
    const QStringList& fieldNames,
    std::vector<std::unique_ptr<CaseIO::FunctionObject>>& functionObjects,
    QWidget *parent): QWizardPage(parent), m_patchNames(patchNames),
    m_fieldNames(fieldNames), m_functionObjects(functionObjects) {

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

    // Populate list of function objects
    for ( const auto& funcObject: m_functionObjects ) {
        // Convert function object type to string
        QString typeStr =
            QMetaEnum::fromType<CaseIO::FunctionObject::FuncObjType>().
            valueToKey(static_cast<int>(funcObject->type));

        // Update table
        updateTable(funcObject->name, typeStr, funcObject.get());
    }
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
    CaseIO::FunctionObject* obj = launchDialog(index, -1);
    if (obj) {
        // Get string for type
        QString typeStr =
            QMetaEnum::fromType<CaseIO::FunctionObject::FuncObjType>().
                valueToKey(index);

        // Update table
        updateTable(obj->name, typeStr, obj);
    }
}

void TasksPage::updateTable(const QString& taskName, const QString& taskType,
                            CaseIO::FunctionObject* funcObjPtr) {
    if (taskName.isEmpty() || !funcObjPtr)
        return;
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
        connect(editButton, &QPushButton::clicked, this,
                [this, editButton, taskType, funcObjPtr]() {
            int currentRow = -1;
            for (int i = 0; i < m_taskTable->rowCount(); ++i) {
                if (m_taskTable->cellWidget(i, 2) == editButton) {
                    currentRow = i;
                    break;
                }
            }
            if (currentRow == -1) return;

            QMetaEnum metaEnum =
                QMetaEnum::fromType<CaseIO::FunctionObject::FuncObjType>();
            int typeIndex = metaEnum.keyToValue(taskType.toUtf8().constData());

            CaseIO::FunctionObject* obj = launchDialog(typeIndex, currentRow);
            if (obj)
                m_taskTable->item(currentRow, 0)->setText(obj->name);
        });

        // Add the delete button
        QPushButton* deleteButton = new QPushButton(tr("Delete"), this);
        m_taskTable->setCellWidget(row, 3, deleteButton);

        // Respond when the Delete button is pressed
        connect(deleteButton, &QPushButton::clicked, this,
                [this, deleteButton, funcObjPtr]() {
            int currentRow = -1;
            for (int i = 0; i < m_taskTable->rowCount(); ++i) {
                if (m_taskTable->cellWidget(i, 3) == deleteButton) {
                    currentRow = i;
                    break;
                }
            }
            if (currentRow == -1)
                return;

            auto it =
                std::find_if(m_functionObjects.begin(), m_functionObjects.end(),
               [funcObjPtr](
                const std::unique_ptr<CaseIO::FunctionObject>& ptr) {
                   return ptr.get() == funcObjPtr;
               });

            if (it != m_functionObjects.end()) {
                m_functionObjects.erase(it);
            }

            // Remove row from table
            m_taskTable->removeRow(currentRow);
        });
    }
}

// Launch dialog for the given patch type
CaseIO::FunctionObject* TasksPage::launchDialog(int rawTypeIndex, int vectorIndex) {
    auto type = static_cast<CaseIO::FunctionObject::FuncObjType>(rawTypeIndex);
    CaseIO::FunctionObject* updatedObject = nullptr;

    switch (type) {
    case CaseIO::FunctionObject::FuncObjType::forces: {
        CaseIO::ForcesConfig config;
        if (vectorIndex != -1) {
            auto* existing = static_cast<CaseIO::ForcesConfig*>(
                m_functionObjects[vectorIndex].get());
            config = *existing;
        }

        ForcesDialog dlg(m_patchNames, m_fieldNames, config, this);
        if (dlg.exec() != QDialog::Accepted)
            return nullptr;

        auto newConfig =
            std::make_unique<CaseIO::ForcesConfig>(dlg.getFunctionObject());
        updatedObject = newConfig.get();

        if (vectorIndex == -1)
            m_functionObjects.push_back(std::move(newConfig));
        else m_functionObjects[vectorIndex] = std::move(newConfig);
        break;
    }
    case CaseIO::FunctionObject::FuncObjType::forceCoeffs: {
        CaseIO::ForceCoeffsConfig config;
        if (vectorIndex != -1) {
            auto* existing =
                static_cast<CaseIO::ForceCoeffsConfig*>(
                    m_functionObjects[vectorIndex].get());
            config = *existing;
        }

        ForceCoeffsDialog dlg(m_patchNames, m_fieldNames, config, this);
        if (dlg.exec() != QDialog::Accepted)
            return nullptr;

        auto newConfig =
            std::make_unique<CaseIO::ForceCoeffsConfig>(
                dlg.getFunctionObject());
        updatedObject = newConfig.get();

        if (vectorIndex == -1)
            m_functionObjects.push_back(std::move(newConfig));
        else m_functionObjects[vectorIndex] = std::move(newConfig);
        break;
    }
    case CaseIO::FunctionObject::FuncObjType::fieldMinMax: {
        CaseIO::FieldMinMaxConfig config;
        if (vectorIndex != -1) {
            auto* existing =
                static_cast<CaseIO::FieldMinMaxConfig*>(
                    m_functionObjects[vectorIndex].get());
            config = *existing;
        }

        FieldMinMaxDialog dlg(m_fieldNames, config, this);
        if (dlg.exec() != QDialog::Accepted)
            return nullptr;

        auto newConfig =
            std::make_unique<CaseIO::FieldMinMaxConfig>(
                dlg.getFunctionObject());
        updatedObject = newConfig.get();

        if (vectorIndex == -1)
            m_functionObjects.push_back(std::move(newConfig));
        else m_functionObjects[vectorIndex] = std::move(newConfig);
        break;
    }
    case CaseIO::FunctionObject::FuncObjType::probes: {
        CaseIO::ProbesConfig config;
        if (vectorIndex != -1) {
            auto* existing =
                static_cast<CaseIO::ProbesConfig*>(
                    m_functionObjects[vectorIndex].get());
            config = *existing;
        }

        ProbesDialog dlg(m_fieldNames, config, this);
        if (dlg.exec() != QDialog::Accepted)
            return nullptr;

        auto newConfig =
            std::make_unique<CaseIO::ProbesConfig>(dlg.getFunctionObject());
        updatedObject = newConfig.get();

        if (vectorIndex == -1)
            m_functionObjects.push_back(std::move(newConfig));
        else m_functionObjects[vectorIndex] = std::move(newConfig);
        break;
    }
    case CaseIO::FunctionObject::FuncObjType::surfaces: {
        CaseIO::SurfacesConfig config;
        if (vectorIndex != -1) {
            auto* existing =
                static_cast<CaseIO::SurfacesConfig*>(
                    m_functionObjects[vectorIndex].get());
            config = *existing;
        }

        SurfacesDialog dlg(m_fieldNames, config, this);
        if (dlg.exec() != QDialog::Accepted)
            return nullptr;

        auto newConfig =
            std::make_unique<CaseIO::SurfacesConfig>(dlg.getFunctionObject());
        updatedObject = newConfig.get();

        if (vectorIndex == -1)
            m_functionObjects.push_back(std::move(newConfig));
        else m_functionObjects[vectorIndex] = std::move(newConfig);
        break;
    }
    case CaseIO::FunctionObject::FuncObjType::yPlus: {
        CaseIO::YPlusConfig config;
        if (vectorIndex != -1) {
            auto* existing =
                static_cast<CaseIO::YPlusConfig*>(
                    m_functionObjects[vectorIndex].get());
            config = *existing;
        }

        YPlusDialog dlg(m_patchNames, config, this);
        if (dlg.exec() != QDialog::Accepted)
            return nullptr;

        auto newConfig =
            std::make_unique<CaseIO::YPlusConfig>(dlg.getFunctionObject());
        updatedObject = newConfig.get();

        if (vectorIndex == -1)
            m_functionObjects.push_back(std::move(newConfig));
        else m_functionObjects[vectorIndex] = std::move(newConfig);
        break;
    }
    }

    return updatedObject;
}