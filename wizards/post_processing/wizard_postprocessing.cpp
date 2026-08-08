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

#include "wizard_postprocessing.h"

#include <QDir>
#include <QMessageBox>

#include "page_10_tasks.h"
#include "page_20_time_region.h"

PostprocessingWizard::PostprocessingWizard(const QString& caseName,
    const QStringList& patchNames, const QStringList& fieldNames,
    const SystemManager& systemMgr, QWidget *parent): m_caseName(caseName),
    m_systemMgr(systemMgr), QWizard(parent) {
    // Configure the appearance
    setWizardStyle(QWizard::ClassicStyle);
    setWindowTitle(tr("Post-Processing Wizard"));

    // Add pages
    QStringList cases = m_systemMgr.getCases();
    setPage(Page_Tasks, new TasksPage(patchNames, fieldNames, this));
    setPage(Page_Time_Region, new TimeRegionPage(this));
    setOption(QWizard::NoBackButtonOnStartPage);
}

// Update controlDict with function objects
void PostprocessingWizard::accept() {
    // Access the tasks page
    TasksPage* tasksPage = qobject_cast<TasksPage*>(page(Page_Tasks));
    if (!tasksPage) {
        qWarning() << "Error: Could not resolve TasksPage";
        return;
    }

    // Create the dictionary text
    QString funcText = "FoamFile\n{\n    version 2.0;\n    format ascii;\n"
                "    class dictionary;\n    object postProcessDict;\n}\n\n" +
                CaseIO::createFunctionsBlock(tasksPage->getFunctionObjects());

    // Access server if necessary
    auto system = m_systemMgr.getSystem(m_caseName);
    if (system == nullptr) {
        QMessageBox::critical(this, tr("Server access failure"),
                              tr("Couldn't reach server."));
        return;
    }

    // Write data to dictionary file
    CaseData caseData = m_systemMgr.getData(m_caseName);
    QString casePath = caseData.casePath + QDir::separator() + m_caseName;
    QString dictPath = QDir::cleanPath(casePath + "/system/postProcessDict");
    system->writeData(funcText.toUtf8(), dictPath);

    // Create command using QStringList
    QStringList cmdArgs;
    cmdArgs << "postProcess" << "-dict" << dictPath;

    // Access flags for postProcess time
    bool allTimes = field("time_allTimes").toBool();
    if (!allTimes) {
        if (field("time_latestTime").toBool()) {
            cmdArgs << "-latestTime";
        } else {
            QString timeRanges = field("time_ranges").toString().trimmed();
            if (!timeRanges.isEmpty()) {
                cmdArgs << "-time" << timeRanges;
            }
        }
    }

    // Update command to ignore 0 directory
    if (field("time_noZero").toBool()) {
        cmdArgs << "-noZero";
    }

    // Update command to access 'constant' folder
    if (field("time_constant").toBool()) {
        cmdArgs << "-constant";
    }

    // Launch the postProcess utility
    QString openFoamPath = caseData.openFoamPath;
    QString command =
        QString("cd %1; source %2/etc/bashrc; " + cmdArgs.join(" ")).
            arg(casePath, openFoamPath);
    system->launchLongUtility(
        command, m_caseName, UtilityType::POSTPROCESS);
    QWizard::accept();
}