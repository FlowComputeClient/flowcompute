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

#include "wizard_open_case.h"

#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>

#include "wizards/new_case/page_15_remote.h"
#include "wizards/open_case/page_20_casefolder.h"

// Wizard to open an existing case
OpenCaseWizard::OpenCaseWizard(TargetType targetType, SystemManager& systemMgr,
    QString openFoamPath, QWidget *parent): QWizard(parent),
    m_targetType(targetType), m_systemMgr(systemMgr),
    m_openFoamPath(openFoamPath) {
    // Configure wizard appearance
    setWizardStyle(QWizard::ClassicStyle);
    setWindowTitle(tr("Open Case Wizard"));

    // Add pages
    if (targetType == TargetType::REMOTE_LINUX) {
        addPage(new RemotePage(systemMgr, this));
    }
    addPage(new CaseFolderPage(targetType, systemMgr, this));
    setOption(QWizard::NoBackButtonOnStartPage);
}

void OpenCaseWizard::accept() {
    // Get case path and case name
    QString path = field("casePath").toString();
    QFileInfo info(path);
    QString casePath = info.path();
    QString originalCaseName = info.fileName();

    // Access system
    auto system = m_systemMgr.getSystem(static_cast<int>(m_targetType));
    if (!system) {
        qWarning() << "Failed to access server";
        return;
    }

    // Check if a case with the given name is in the map
    int count = 1;
    QString caseName = originalCaseName;
    while (m_systemMgr.contains(caseName)) {
        caseName = originalCaseName + "_" + QString::number(count++);
    }

    // Prompt user if duplicate
    if (caseName != originalCaseName) {
        QMessageBox::StandardButton reply;
        QString msg =
            tr("The case '%1' already exists.\n Rename the case to '%2'?").
                      arg(originalCaseName, caseName);
        reply = QMessageBox::question(this, tr("Existing Case Detected"), msg,
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            return;
        } else {
            // Rename existing case
            QStringList fileNames = { path, casePath + "/" + caseName };
            QStringList results = system->processPaths(fileNames.join("\n"),
                                PathOperationType::RENAME);
        }
    }

    // Get credentials for remote cases
    int port = 0;
    QString userName = "", hostName = "";
    int targetId = static_cast<int>(m_targetType);
    if (targetId == static_cast<int>(TargetType::REMOTE_LINUX)) {
        userName = field("userName").toString();
        hostName = field("hostName").toString();
        port = field("port").toInt();
    }

    // Get files in existing case
    QStringList caseFiles = system->processPaths(path, PathOperationType::LIST);

    // Request case creation
    emit requestCaseCreation(caseName, casePath, caseFiles, targetId,
                             m_openFoamPath, userName, hostName, port);

    QWizard::accept();
}