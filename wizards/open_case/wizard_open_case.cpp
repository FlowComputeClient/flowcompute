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

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>

#include "dialogs/selection/selection_dialog.h"
#include "wizards/new_case/page_15_remote.h"
#include "wizards/open_case/page_20_casefolder.h"

// Wizard to open an existing case
OpenCaseWizard::OpenCaseWizard(TargetType targetType, SystemManager& systemMgr,
    const QString& openFoamPath, QWidget *parent): QWizard(parent),
    m_targetType(targetType), m_systemMgr(systemMgr),
    m_openFoamPath(openFoamPath) {
    // Configure wizard appearance
    setWizardStyle(QWizard::ClassicStyle);
    setWindowTitle(tr("Open Case Wizard"));

    // Add pages
    if (targetType == TargetType::REMOTE_LINUX) {
        setPage(static_cast<int>(OpenCasePage::Page_Remote),
            new RemotePage(systemMgr, this));
    }
    setPage(static_cast<int>(OpenCasePage::Page_CaseFolder),
        new CaseFolderPage(targetType, systemMgr, this));
    setOption(QWizard::NoBackButtonOnStartPage);
}

bool OpenCaseWizard::validateCurrentPage() {
    // Validate remote page
    if (currentId() == static_cast<int>(OpenCasePage::Page_Remote)) {
        return checkOpenFoam();
    }
    return QWizard::validateCurrentPage();
}

bool OpenCaseWizard::checkOpenFoam() {
    // Determine OpenFOAM installation
    QStringList ofList =
        m_systemMgr.getSystem(
            static_cast<int>(TargetType::REMOTE_LINUX))->findOpenFoam();
    if(ofList.empty()) {
        QMessageBox::critical(this, tr("Missing OpenFOAM"),
                              tr("No OpenFOAM installations detected..."));
        m_openFoamPath = "";
        return false;
    } else if (ofList.size() > 1) {
        // Create selection dialog
        SelectionDialog selectionDialog(
            tr("Multiple OpenFOAM Installations Detected"),
            tr("Select one of the following:"), ofList, this);
        if (selectionDialog.exec() != QDialog::Accepted) {
            return false;
        }
        m_openFoamPath = selectionDialog.getSelectedItem();
    } else {
        m_openFoamPath = ofList[0];
    }
    return true;
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

    // Determine OpenFOAM version
    bool isOpenCFD = true;
    QRegularExpression re("openfoam-?v?(\\d+)",
                          QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(m_openFoamPath.split("/").last());
    if (match.hasMatch()) {
        QString digits = match.captured(1);
        isOpenCFD = digits.toInt() > 100;
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

    // Get case type fields
    CaseType type =
        m_systemMgr.updateType(targetId, casePath + "/" + caseName, isOpenCFD);

    // Get files in existing case
    QStringList caseFiles = system->processPaths(path, PathOperationType::LIST);

    // Request case creation
    emit requestCaseCreation(caseName, casePath, caseFiles, targetId,
        m_openFoamPath, CaseFlag::NotChecked, type, userName, hostName, port);

    QWizard::accept();
}