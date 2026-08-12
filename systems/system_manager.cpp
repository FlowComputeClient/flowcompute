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

#include "system_manager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFuture>
#include <QMetaObject>
#include <QProcess>
#include <QStandardPaths>
#include <QtConcurrent>
#include <QVersionNumber>

#include "dialogs/login/login_dialog.h"

#include "./wsl_system.h"
#include "./remote_system.h"

// Check if WSL can be accessed
bool SystemManager::checkWsl() {
    // Check if wsl.exe exists in the system PATH
    QString wslPath = QStandardPaths::findExecutable("wsl.exe");
    if (wslPath.isEmpty()) {
        return false;
    }

    // List installed distributions
    QProcess process;
    process.start(wslPath, QStringList() << "-l" << "-q");

    // Wait for the process to finish
    if (!process.waitForFinished(3000)) {
        process.kill();
        return false;
    }

    // If WSL exited with error, no Linux distributions
    if (process.exitStatus() != QProcess::NormalExit ||
        process.exitCode() != 0) {
        return false;
    }

    // Check for one distribution
    QString output =
        QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    return !output.isEmpty();
}

// Check WSL server, update if needed
bool SystemManager::checkWslServer() {
    if (m_wslServerPresent)
        return true;

    // Access WSL target system
    std::shared_ptr<WslSystem> wslSystem =
        std::static_pointer_cast<WslSystem>(
            m_systems[static_cast<int>(TargetType::LOCAL_WINDOWS)]);

    // Check server version
    QString installVersion = wslSystem->getVersion();
    bool serverInstalled = !installVersion.isEmpty();
    if (serverInstalled) {
        QVersionNumber v1 = QVersionNumber::fromString(installVersion);
        QVersionNumber v2 = QVersionNumber::fromString(m_serverVersion);
        if (v1 < v2) {
            wslSystem->shutdown();
        } else {
            m_wslServerPresent = true;
            return true;
        }
    }

    // Install latest server
    if (!m_wslServerPresent) {
        QString appDir = QCoreApplication::applicationDirPath();
        QString wslPath = appDir + "/wsl_server";

        // Make sure the wsl_server binary is present
        if (!QFile::exists(wslPath)) {
            /*
            QString title = QCoreApplication::translate("SystemManager",
                                                        "Update Failed");
            QString msg = QCoreApplication::translate("SystemManager",
                "The wsl_server binary is missing.\n"
                "Please reinstall the application.");
            QMessageBox::critical(nullptr, title, msg);
            */
            return false;
        }

        // Install wsl_server
        QString cmd = QString("mkdir -p ~/.config/flowcompute && "
              "rm -f ~/.config/flowcompute/wsl_server && "
              "cp $(wslpath '%1') ~/.config/flowcompute/wsl_server && "
              "chmod +x ~/.config/flowcompute/wsl_server").arg(wslPath);
        QString output;
        int result = wslSystem->launchShortUtility(cmd, output);

        // Check output
        if (result == 0) {
            m_wslServerPresent = true;
            return true;
        }
        /*
        else {
            QString title = QCoreApplication::translate("SystemManager",
                                            "Server Installation Failed");
            QString msg = QCoreApplication::translate("SystemManager",
            "Failed to install the WSL server in ~/config/flowcompute.\n"
            "Please make sure this directory is accessible.");
            QMessageBox::critical(nullptr, title, msg);
        }
        */
    }
    return false;
}

// Check remote server
bool SystemManager::checkRemoteServer(const QString& host, int port) {
    // Attempt to connect to the host
    QTcpSocket socket;
    socket.connectToHost(host, port);

    // Wait three seconds
    if (socket.waitForConnected(3000)) {
        socket.disconnectFromHost();
        return true;
    }

    // Connection timed out or was refused
    return false;
}

void SystemManager::setSystems(
    const std::array<std::shared_ptr<TargetSystem>,
                     static_cast<size_t>(TargetType::COUNT)>& systems) {
    m_systems = systems;
}

bool SystemManager::addCase(const QString& caseName, const CaseData& data) {
    auto it = m_caseMap.find(caseName);
    if (it == m_caseMap.end()) {
        m_caseMap.insert(caseName, data);
        return true;
    }
    return false;
}

bool SystemManager::contains(const QString& caseName) const {
    return m_caseMap.contains(caseName);
}

CaseData SystemManager::getData(const QString& caseName) const {
    auto it = m_caseMap.constFind(caseName);
    if (it != m_caseMap.constEnd()) {
        return it.value();
    }
    return CaseData{};
}

QStringList SystemManager::getCases() const {
    return m_caseMap.keys();
}

void SystemManager::clear() {
    m_caseMap.clear();
}

std::shared_ptr<TargetSystem> SystemManager::getSystem(
    const QString& caseName) const {
    auto it = m_caseMap.constFind(caseName);
    if (it == m_caseMap.constEnd()) {
        return nullptr;
    }

    size_t index = static_cast<size_t>(it.value().targetId);
    if (index >= m_systems.size()) {
        return nullptr;
    }
    return m_systems[index];
}

std::shared_ptr<TargetSystem> SystemManager::getSystem(int systemId) const {
    if (systemId >= m_systems.size()) {
        return nullptr;
    }
    return m_systems[systemId];
}

QFutureWatcher<std::pair<bool, QString>>*
    SystemManager::setupConnection() const {
    // Access the active window
    QWidget* activeWindow = QApplication::activeWindow();

    // Create the login dialog
    LoginDialog dialog(m_defaultUser, m_defaultHost, activeWindow);
    if (dialog.exec() == QDialog::Accepted) {
        SshCredentials creds = dialog.getCredentials();

        // Attempt to establish connection
        return sshConnect(creds.userName, creds.hostName, creds.password,
                          creds.port);
    }
    return nullptr;
}

QFutureWatcher<std::pair<bool, QString>>* SystemManager::sshConnect(
    const QString& user, const QString& host, const QString& password,
    int port) const {

    // Access the remote system
    std::shared_ptr<RemoteSystem> remoteSystem =
        std::dynamic_pointer_cast<RemoteSystem>(
            getSystem(static_cast<int>(TargetType::REMOTE_LINUX)));

    // Allocate the future watcher
    auto* authWatcher = new QFutureWatcher<std::pair<bool, QString>>();

    // Define callback function
    auto callback =
        [](const QString& host, const QString& fingerprint) -> bool {
        bool userAccepted = false;

        // Run on the main GUI thread
        QMetaObject::invokeMethod(qApp, [host, fingerprint, &userAccepted]() {

            // Set message
            QString msg = QCoreApplication::translate("SystemManager",
                  "The authenticity of host '%1' cannot be established.\n"
                  "SHA1 key fingerprint is: %2\n\n"
                  "Are you sure you want to continue connecting?")
                              .arg(host, fingerprint);

            // Wait for reply
            QMessageBox::StandardButton reply = QMessageBox::warning(nullptr,
                QCoreApplication::translate("SystemManager", "Unknown Host"),
                msg, QMessageBox::Yes | QMessageBox::No);
            userAccepted = (reply == QMessageBox::Yes);
        }, Qt::BlockingQueuedConnection);
        return userAccepted;
    };

    // Run SSH authentication in a background thread
    QFuture<std::pair<bool, QString>> future = QtConcurrent::run(
        [host, user, port, password, remoteSystem, callback]() {
            QString errorMsg;
            bool success = remoteSystem->establishSession(
                host, user, port, password, errorMsg, callback);
            return std::make_pair(success, errorMsg);
        });

    authWatcher->setFuture(future);

    // Return the watcher
    return authWatcher;
}