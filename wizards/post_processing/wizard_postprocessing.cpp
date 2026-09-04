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
#include <QRegularExpression>

#include "parser/common.h"

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
    setPage(Page_Tasks, new TasksPage(patchNames, fieldNames,
                                      m_functionObjects, this));
    setPage(Page_Time_Region, new TimeRegionPage(this));
    setOption(QWizard::NoBackButtonOnStartPage);
}

bool PostprocessingWizard::parseFile() {
    // Access OpenFOAM path on server
    QString casePath = m_systemMgr.getData(m_caseName).casePath;
    auto system = m_systemMgr.getSystem(m_caseName);

    // Read data from file
    QString fileName = "system/postProcessDict";
    QString fullRemotePath = casePath + "/" + m_caseName + "/" + fileName;
    std::optional<QByteArray> fileData = system->getFileContent(fullRemotePath);
    if (!fileData || fileData.value().isEmpty()) {
        return true;
    }

    // Parse file data
    m_postProcessDict = std::make_shared<OpenFoamDictionary>(fileData.value());
    if (!m_postProcessDict->hasSyntaxErrors()) {
        m_functionObjects = CaseIO::parsePostProcessDict(m_postProcessDict);
        return true;
    }

    // Syntax errors found
    auto action = CaseIO::showParsingErrorMessage(fileName, this);
    switch(action) {
    case CaseIO::ParseErrorAction::EditFile:
        emit createTextEditor(fileName.split('/').last(),
                              m_caseName + "/" + fileName, false);
        return false;
    case CaseIO::ParseErrorAction::Overwrite:
        return true;
    case CaseIO::ParseErrorAction::Cancel:
        return false;
    }
    return false;
}

// Update controlDict with function objects
void PostprocessingWizard::accept() {
    QWizard::accept();

    // Access the tasks page
    TasksPage* tasksPage = qobject_cast<TasksPage*>(page(Page_Tasks));
    if (!tasksPage) {
        qWarning() << "Error: Could not resolve TasksPage";
        return;
    }

    // Create the dictionary text
    QString funcText = "FoamFile\n{\n    version 2.0;\n    format ascii;\n"
                "    class dictionary;\n    object postProcessDict;\n}\n\n" +
                CaseIO::createFunctionsBlock(m_functionObjects);

    // Access server if necessary
    auto system = m_systemMgr.getSystem(m_caseName);
    if (system == nullptr) {
        QMessageBox::critical(this, tr("Server access failure"),
                              tr("Couldn't reach server."));
        return;
    }

    // Write data to dictionary file
    CaseData caseData = m_systemMgr.getData(m_caseName);
    QString casePath = caseData.casePath + "/" + m_caseName;
    QString dictPath = QDir::cleanPath(casePath + "/system/postProcessDict");
    system->writeData(funcText.toUtf8(), dictPath);

    // End processing if there are no function objects
    if (m_functionObjects.empty())
        return;

    // Determine which OpenFOAM installation is being used
    QRegularExpression re("openfoam-?v?(\\d+)",
                          QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(caseData.openFoamPath);
    bool isESI = true;
    if (match.hasMatch()) {
        QString digits = match.captured(1);
        isESI = digits.toInt() > 100;
    } else {
        qDebug() << "Couldn't read OpenFOAM path: " << caseData.openFoamPath;
    }

    // Form the initial command
    QString baseCmd;
    std::optional<QByteArray> fileData =
        system->getFileContent(casePath + "/system/controlDict");
    if (fileData && !fileData.value().isEmpty()) {
        QString content = QString::fromUtf8(fileData.value());
        if (isESI) {
            QRegularExpression
                appRegex(QStringLiteral("application\\s+([\\w\\-]+)\\s*;"));
            match = appRegex.match(content);
            if (match.hasMatch()) {
                QString appName = match.captured(1);
                baseCmd = QString("%1 -postProcess").arg(appName);
            } else {
                baseCmd = QString("postProcess");
            }
        } else {
            QRegularExpression
                solverRegex(QStringLiteral("solver\\s+([\\w\\-]+)\\s*;"));
            match = solverRegex.match(content);
            if (match.hasMatch()) {
                QString solverName = match.captured(1);
                baseCmd = QString("foamPostProcess -solver %1").arg(solverName);
            } else {
                baseCmd = QString("foamPostProcess");
            }
        }
    }

    // Create command using QStringList
    QStringList cmdArgs;
    cmdArgs << baseCmd << "-dict" << dictPath;

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
}