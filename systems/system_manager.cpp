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
#include "dialogs/selection/selection_dialog.h"

#include "systems/wsl_system.h"
#include "systems/remote_system.h"

// Get WSL distribution
QString SystemManager::getWslDistribution() {
    // Check for existing member
    if (!m_wslDistribution.isEmpty())
        return m_wslDistribution;

    // Check if wsl.exe exists
    QString wslPath = QStandardPaths::findExecutable("wsl.exe");
    if (wslPath.isEmpty()) {
        m_wslDistribution = "";
        return "";
    }

    // Query installed WSL distributions FIRST
    QProcess process;
    QStringList distributions;
    process.start("wsl", QStringList() << "--list" << "--quiet");
    if (process.waitForFinished(3000)) {
        QByteArray output = process.readAllStandardOutput();

        // wsl.exe outputs in UTF-16LE
        QString strOutput = QString::fromUtf16(
            reinterpret_cast<const char16_t*>(output.constData()),
            output.size() / 2);

        // Split output by newlines
        QStringList lines =
            strOutput.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);

        for (const QString& line : std::as_const(lines)) {
            QString dist = line.trimmed();
            if (!dist.isEmpty()) {
                distributions.append(dist);
            }
        }
    } else {
        m_wslDistribution = "";
        return "";
    }

    if (distributions.empty()) {
        m_wslDistribution = "";
        return "";
    }

    // Check settings and validate against installed distributions
    QSettings settings;
    settings.beginGroup("Preferences");
    QString savedDistro = settings.value("wsl_distribution").toString();
    settings.endGroup();

    if (!savedDistro.isEmpty() && distributions.contains(savedDistro)) {
        m_wslDistribution = savedDistro;
        return m_wslDistribution;
    }

    // Select distribution
    if (distributions.size() == 1) {
        m_wslDistribution = distributions[0];
    } else {
        QString title = QCoreApplication::translate("SystemManager",
                            "Multiple WSL Distributions Detected");
        QString msg = QCoreApplication::translate("SystemManager",
                            "Select a WSL distribution:");
        SelectionDialog selectionDialog(title, msg, distributions);
        if (selectionDialog.exec() == QDialog::Accepted) {
            m_wslDistribution = selectionDialog.getSelectedItem();
        }
    }

    // Check if the user cancelled the dialog
    if (m_wslDistribution.isEmpty()) {
        m_wslDistribution = "";
        return "";
    }

    // Save the selected distribution to settings
    settings.beginGroup("Preferences");
    settings.setValue("wsl_distribution", m_wslDistribution);
    settings.endGroup();
    return m_wslDistribution;
}

// Check WSL server, update if needed
bool SystemManager::checkWslServer() {
    if (m_wslServerPresent)
        return true;

    // Check distribution
    if (m_wslDistribution.isEmpty()) {
        m_wslDistribution = getWslDistribution();
        if (m_wslDistribution.isEmpty()) {
            return false;
        }
    }

    const QString targetDir = "$HOME/.config/flowcompute";
    const QString targetBin = targetDir + "/wsl_server";
    std::shared_ptr<WslSystem> wslSystem =
        std::static_pointer_cast<WslSystem>(
            m_systems[static_cast<int>(TargetType::LOCAL_WINDOWS)]);

    // Check if the executable exists in WSL
    QString checkScript = QString("test -f %1").arg(targetBin);
    QProcess checkProcess;
    checkProcess.start("wsl.exe", QStringList() << "-d" << m_wslDistribution
                            << "--" << "bash" << "-c" << checkScript);

    // Check if the process is running
    if (checkProcess.waitForFinished(3000) && checkProcess.exitCode() == 0) {
        QString pgrepScript = "pgrep -f wsl_server";
        QProcess pgrepProcess;
        pgrepProcess.start("wsl.exe", QStringList() << "-d" << m_wslDistribution
                                << "--" << "bash" << "-c" << pgrepScript);

        bool isRunning = (pgrepProcess.waitForFinished(3000) &&
                          pgrepProcess.exitCode() == 0);

        // Launch the server if the binary exists but isn't running
        if (!isRunning) {
            QString launchScript = QString("%1").arg(targetBin);
            QStringList launchArgs;
            launchArgs << "-d" << m_wslDistribution << "--"
                       << "bash" << "-c" << launchScript;
            QProcess::startDetached("wsl.exe", launchArgs);
            QThread::msleep(500);
        }

        // Check the server version
        QString installVersion = wslSystem->getVersion();
        if (!installVersion.isEmpty()) {
            QVersionNumber v1 = QVersionNumber::fromString(installVersion);
            QVersionNumber v2 = QVersionNumber::fromString(m_serverVersion);

            if (v1 < v2) {
                wslSystem->shutdown();
                QString waitScript =
                    "while pgrep -f wsl_server > /dev/null; do sleep 0.1; done";
                QProcess waitProcess;
                waitProcess.start("wsl.exe", QStringList()
                    << "-d" << m_wslDistribution
                    << "--" << "bash" << "-c" << waitScript);
                waitProcess.waitForFinished(2000);
            } else {
                m_wslServerPresent = true;
                return true;
            }
        }
    }

    // Install latest server
    QString appDir = QCoreApplication::applicationDirPath();
    QString wslPath = appDir + "/wsl_server";
    if (!QFile::exists(wslPath)) {
        return false;
    }

    // Construct strings
    QString safeWslPath = wslPath;
    safeWslPath.replace("'", "'\\''");

    // Construct the bash script
    QString installScript =
        QString("mkdir -p %1 && rm -f %2 && cp \"$(wslpath '%3')\" %2"
                " && chmod +x %2").arg(targetDir, targetBin, safeWslPath);

    QProcess installProcess;
    installProcess.start("wsl.exe", QStringList() << "-d" << m_wslDistribution
                            << "--" << "bash" << "-c" << installScript);

    if (installProcess.waitForFinished(5000) &&
        installProcess.exitCode() == 0) {

        // Launch the server detached.
        QString launchScript = QString("%1").arg(targetBin);
        QStringList launchArgs;
        launchArgs << "-d" << m_wslDistribution << "--" <<
            "bash" << "-c" << launchScript;

        bool launchSuccess = QProcess::startDetached("wsl.exe", launchArgs);
        if (launchSuccess) {
            m_wslServerPresent = true;
            return true;
        }
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

// Add a new case to the case map
bool SystemManager::addCase(const QString& caseName, const CaseData& data) {
    auto it = m_caseMap.find(caseName);
    if (it == m_caseMap.end()) {
        m_caseMap.insert(caseName, data);
        return true;
    }
    return false;
}

// Rename a case
void SystemManager::renameCase(const QString& oldName, const QString& newName) {
    // Validate inputs
    auto it = m_caseMap.find(oldName);
    if (it == m_caseMap.end() || m_caseMap.contains(newName)) {
        return;
    }

    // Rename key in map
    CaseData data = it.value();
    m_caseMap.erase(it);
    m_caseMap.insert(newName, data);

    // Update QSettings
    QSettings settings;
    settings.beginGroup("Cases");

    // Replacing the name at the same index
    QStringList caseOrder = settings.value("caseOrder").toStringList();
    int orderIndex = caseOrder.indexOf(oldName);
    if (orderIndex != -1) {
        caseOrder.replace(orderIndex, newName);
        settings.setValue("caseOrder", caseOrder);
    }

    // Write the data to the new group
    settings.beginGroup(newName);
    settings.setValue("casePath", data.casePath);
    settings.setValue("targetSystemId", data.targetId);
    settings.setValue("openFoamPath", data.openFoamPath);
    settings.setValue("caseFlags", static_cast<quint32>(data.caseFlags));
    settings.setValue("caseType", static_cast<quint32>(data.caseType));
    settings.setValue("openFolders", data.openFolders);

    if (data.targetId == static_cast<int>(TargetType::REMOTE_LINUX)) {
        settings.setValue("userName", data.userName);
        settings.setValue("hostName", data.hostName);
        settings.setValue("port", data.port);
    }
    settings.endGroup();

    // Remove the group for the old case
    settings.remove(oldName);
    settings.endGroup();
}

// Remove a case from the case map
void SystemManager::removeCase(const QString& caseName) {
    auto it = m_caseMap.find(caseName);
    if (it != m_caseMap.end()) {
        // Remove case
        m_caseMap.erase(it);

        // Update settings
        QSettings settings;
        settings.beginGroup("Cases");

        // Remove the case from the master order list
        QStringList caseOrder = settings.value("caseOrder").toStringList();
        if (caseOrder.removeOne(caseName)) {
            if (caseOrder.isEmpty()) {
                settings.remove("caseOrder");
            } else {
                settings.setValue("caseOrder", caseOrder);
            }
        }

        // Remove the subgroup from settings
        settings.remove(caseName);
        settings.endGroup();
    }
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

void SystemManager::setFlag(const QString& caseName, CaseFlag flag,
                            bool enabled) {
    // Search for the case's name
    auto it = m_caseMap.find(caseName);
    if (it == m_caseMap.end())
        return;

    // Set the flag
    it.value().caseFlags.setFlag(flag, enabled);
}

CaseFlags SystemManager::updateFlags(const QString& caseName,
                               const QString& casePath) {
    // Search for the case's name
    auto it = m_caseMap.find(caseName);
    if (it == m_caseMap.end())
        return CaseFlag::NotChecked;

    // Initialize case flag
    CaseFlags caseFlags = CaseFlag::Initial;

    // Determine fullPath
    QString fullPath = (casePath.isEmpty())
        ? getData(caseName).casePath + "/" + caseName
        : casePath + "/" + caseName;

    // Check for mesh files
    auto system = getSystem(caseName);
    QStringList files = {fullPath + "/system/blockMeshDict",
                         fullPath + "/system/snappyHexMeshDict",
                         fullPath + "/system/meshDict",
                         fullPath + "/constant/polyMesh/points",
                         fullPath + "/constant/polyMesh/faces",
                         fullPath + "/constant/polyMesh/owner",
                         fullPath + "/constant/polyMesh/boundary" };
    QStringList results =
        system->processPaths(files.join("\n"), PathOperationType::CHECK);

    // Check if any mesh configuration files are present
    bool hasMeshConfig = std::any_of(results.cbegin(), results.cbegin() + 3,
        [](const QString& str) { return str == "0"; });
    if (hasMeshConfig)
        caseFlags |= CaseFlag::HasMeshConfigFiles;

    // Check if all of the mesh files are present
    bool missingMeshFiles = results[3] == "-1" || results[4] == "-1" ||
        results[5] == "-1" || results[6] == "-1";
    if (!missingMeshFiles)
        caseFlags |= CaseFlag::HasMeshFiles;

    // Check if field files are in 0.orig folder
    results = system->processPaths(
        fullPath + "/0.orig", PathOperationType::LIST);
    bool hasFieldFiles = !results.isEmpty();

    // Check if field files are in 0 folder
    if (!hasFieldFiles) {
        results = system->processPaths(
            fullPath + "/0", PathOperationType::LIST);
        hasFieldFiles = !results.isEmpty();
    }

    if (hasFieldFiles)
        caseFlags |= CaseFlag::HasFieldFiles;

    // Check if time directories are present
    bool hasTimeDirs = false;
    results = system->processPaths(fullPath, PathOperationType::LIST);
    for (const QString& result : std::as_const(results)) {
        bool ok = false;
        double value = result.toDouble(&ok);
        if (ok && value > 0) {
            hasTimeDirs = true;
            break;
        }
    }
    if (hasTimeDirs)
        caseFlags |= CaseFlag::HasTimeDirs;

    // Return flags
    m_caseMap[caseName].caseFlags = caseFlags;
    return caseFlags;
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

    // Run SSH authentication
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

CaseType SystemManager::updateType(int targetId, const QString& casePath,
                                   bool isOpenCFD) {
    // Initialize the CaseType
    CaseType caseType;
    caseType.setFlag(IsOpenCFD, isOpenCFD);

    // Get system
    auto system = getSystem(targetId);

    // Find files in the constant folder
    QString constantPath = casePath + "/constant";
    QStringList constFiles = system->processPaths(constantPath,
                                                  PathOperationType::LIST);

    // Base properties
    caseType.setFlag(Compressible,
                     constFiles.contains("thermophysicalProperties"));
    caseType.setFlag(Multiphase,
                     constFiles.contains("phaseProperties"));
    caseType.setFlag(Combustion,
                     constFiles.contains("combustionProperties"));
    caseType.setFlag(Lagrangian,
                     constFiles.contains("kinematicCloudProperties")
                    || constFiles.contains("reactingCloudProperties"));
    caseType.setFlag(Radiation, constFiles.contains("radiationProperties"));
    caseType.setFlag(Buoyancy, constFiles.contains("g"));

    if (constFiles.contains("thermophysicalProperties")) {
        caseType.setFlag(FluidHeat, true);
    }

    QRegularExpression commentRegex(R"(//.*|/\*[\s\S]*?\*/)");
    QString text;
    std::optional<QByteArray> fileData;

    // Check for MRF
    QString systemPath = casePath + "/system";
    QStringList systemFiles =
        system->processPaths(systemPath, PathOperationType::LIST);

    if (constFiles.contains("MRFProperties")) {
        caseType.setFlag(MeshMRF, true);
    } else {
        // Check for fvOptions
        QString optionsPath;
        if (constFiles.contains("fvOptions")) {
            optionsPath = constantPath + "/fvOptions";
        } else if (systemFiles.contains("fvOptions")) {
            optionsPath = systemPath + "/fvOptions";
        }

        if (!optionsPath.isEmpty()) {
            fileData = system->getFileContent(optionsPath);
            if (fileData && !fileData->isEmpty()) {
                text = QString::fromUtf8(fileData.value());
                text.remove(commentRegex);
                if (text.contains("MRFSource")) {
                    caseType.setFlag(MeshMRF, true);
                }
            }
        }
    }

    // Parse dynamicMeshDict for AMI, Overset, or Deforming
    if (constFiles.contains("dynamicMeshDict")) {
        fileData = system->getFileContent(constantPath + "/dynamicMeshDict");
        if (fileData && !fileData->isEmpty()) {
            text = QString::fromUtf8(fileData.value());
            text.remove(commentRegex);

            QRegularExpression meshRegex(R"(dynamicFvMesh\s+([^\s;]+)\s*;)");
            QRegularExpressionMatch match = meshRegex.match(text);
            if (match.hasMatch()) {
                QString meshType = match.captured(1);
                if (meshType.contains("overset", Qt::CaseInsensitive)) {
                    caseType.setFlag(MeshOverset, true);
                } else if (meshType.contains("solidBody")) {
                    caseType.setFlag(MeshAMI, true);
                } else if (meshType.contains("MotionSolver") ||
                           meshType.contains("topoChanger")) {
                    caseType.setFlag(MeshDeforming, true);
                }
            }
        }
    }

    // Read turbulenceProperties
    if (constFiles.contains("turbulenceProperties")) {
        fileData = system->getFileContent(constantPath +
                                          "/turbulenceProperties");
        if (fileData && !fileData->isEmpty()) {
            text = QString::fromUtf8(fileData.value());
            text.remove(commentRegex);
            QRegularExpression
                simTypeRegex(R"(simulationType\s+(RAS|LES)\s*;)");
            QRegularExpressionMatch match = simTypeRegex.match(text);
            if (match.hasMatch()) {
                QString type = match.captured(1);
                if (type == "RAS") {
                    caseType.setFlag(TurbulenceRAS, true);
                } else if (type == "LES") {
                    caseType.setFlag(TurbulenceLES, true);
                }
            }
        }
    }

    // Read fvSchemes
    if (systemFiles.contains("fvSchemes")) {
        fileData = system->getFileContent(systemPath + "/fvSchemes");
        if (fileData && !fileData->isEmpty()) {
            text = QString::fromUtf8(fileData.value());
            text.remove(commentRegex);
            QRegularExpression
                regex(R"(ddtSchemes\s*\{[^}]*?default\s+([^;]+);)");
            QRegularExpressionMatch match = regex.match(text);
            if (match.hasMatch()) {
                caseType.setFlag(Transient,
                                 match.captured(1).trimmed() != "steadyState");
            }
        }
    }

    // Read controlDict
    if (systemFiles.contains("controlDict")) {
        fileData = system->getFileContent(systemPath + "/controlDict");

        if (fileData && !fileData->isEmpty()) {
            text = QString::fromUtf8(fileData.value());
            text.remove(commentRegex);

            QString solverName;
            QRegularExpressionMatch match;

            if (!isOpenCFD) {
                QRegularExpression solverRegex(R"(^\s*solver\s+([^\s;]+)\s*;)",
                        QRegularExpression::MultilineOption);
                match = solverRegex.match(text);
            }

            if (!match.hasMatch()) {
                QRegularExpression
                    appRegex(R"(^\s*application\s+([^\s;]+)\s*;)",
                        QRegularExpression::MultilineOption);
                match = appRegex.match(text);
            }

            if (match.hasMatch()) {
                solverName = match.captured(1);

                // Heat Transfer & Compressibility
                if (solverName.contains("cht") ||
                    solverName.contains("MultiRegion")) {
                    caseType.setFlag(ConjugateHeat, true);
                    caseType.setFlag(Compressible, true);
                    caseType.setFlag(FluidHeat, false); // Override fluid heat
                } else if (solverName.startsWith("rho") ||
                           solverName.startsWith("sonic") ||
                           solverName.contains("compressible")) {
                    caseType.setFlag(Compressible, true);
                    caseType.setFlag(FluidHeat, true);
                }

                // Buoyancy
                if (solverName.contains("buoyant")) {
                    caseType.setFlag(Buoyancy, true);
                    caseType.setFlag(FluidHeat, true);
                } else if (solverName.contains("Boussinesq")) {
                    caseType.setFlag(FluidHeat, true);
                }

                // Phase Composition
                if (solverName.contains("inter") ||
                    solverName.contains("multiphase") ||
                    solverName.contains("cavitating")) {
                    caseType.setFlag(Multiphase, true);
                }

                // Combustion
                if (solverName.contains("reacting") ||
                    solverName.contains("fire") ||
                    solverName.contains("chem")) {
                    caseType.setFlag(Combustion, true);
                    caseType.setFlag(FluidHeat, true);
                }

                // Lagrangian / Discrete Particles
                if (solverName.contains("Parcel") ||
                    solverName.contains("spray") ||
                    solverName.contains("DPM") ||
                    solverName.contains("coal")) {
                    caseType.setFlag(Lagrangian, true);
                }

                // Time Handling
                if (solverName.contains("simple", Qt::CaseInsensitive)) {
                    caseType.setFlag(Transient, false);
                } else {
                    caseType.setFlag(Transient, true);
                }
            }
        }
    }
    return caseType;
}

void SystemManager::setCaseType(const QString& caseName, CaseType type) {
    auto it = m_caseMap.find(caseName);
    if (it != m_caseMap.end()) {
        it->caseType = type;
    }
}

bool SystemManager::testCaseFlag(const QString& caseName,
                                 CaseType flags) const {
    auto it = m_caseMap.find(caseName);
    if (it != m_caseMap.end()) {
        return (it->caseType & flags) != 0;
    }
    return false;
}

bool SystemManager::isLaminar(const QString& caseName) const {
    auto it = m_caseMap.find(caseName);
    if (it != m_caseMap.end()) {
        return !it->caseType.testFlag(TurbulenceRAS) &&
               !it->caseType.testFlag(TurbulenceLES);
    }
    return false;
}

void SystemManager::addOpenFolder(const QString& caseName, const QString& path) {
    auto it = m_caseMap.find(caseName);
    if (it != m_caseMap.end()) {
        // Update map
        if (!it->openFolders.contains(path)) {
            it->openFolders.append(path);
        }

        // Update QSettings
        QSettings settings;
        settings.beginGroup("Cases");
        settings.beginGroup(caseName);
        settings.setValue("openFolders", it->openFolders);
        settings.endGroup();
        settings.endGroup();
    }
}

void SystemManager::removeOpenFolder(const QString& caseName,
                                     const QString& path) {
    auto it = m_caseMap.find(caseName);
    if (it != m_caseMap.end()) {

        // Update map
        QString prefix = path + "/";
        it->openFolders.erase(
            std::remove_if(it->openFolders.begin(), it->openFolders.end(),
               [&path, &prefix](const QString& storedPath) {
                   return storedPath == path || storedPath.startsWith(prefix);
               }),
            it->openFolders.end());

        // Update QSettings
        QSettings settings;
        settings.beginGroup("Cases");
        settings.beginGroup(caseName);
        if (it->openFolders.isEmpty()) {
            settings.remove("openFolders");
        } else {
            settings.setValue("openFolders", it->openFolders);
        }
        settings.endGroup();
        settings.endGroup();
    }
}

void SystemManager::updateOpenFolders(const QString& caseName,
                                      const QStringList& folders) {
    auto it = m_caseMap.find(caseName);
    if (it != m_caseMap.end()) {
        it->openFolders = folders;
    }
}
