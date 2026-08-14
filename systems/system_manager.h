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

#ifndef SYSTEMS_SYSTEM_MANAGER_H_
#define SYSTEMS_SYSTEM_MANAGER_H_

#include <QFutureWatcher>
#include <QMap>

#include "./target_system.h"

// Store information about project in navigator
struct CaseData {
    QString casePath;
    QStringList caseFiles;
    int targetId;
    QString openFoamPath;
    QString userName;
    QString hostName;
    int port;
};

enum TargetType {
    LOCAL_WINDOWS = 0,
    LOCAL_LINUX = 1,
    REMOTE_LINUX = 2,
    COUNT
};

enum class EditorType : int {
    TEXT = 0,
    SURFACE,
    MESH,
    RESULT
};

class SystemManager {
public:
    SystemManager() {}

    // Access server
    bool checkWsl();
    bool checkWslServer();
    bool checkRemoteServer(const QString& host, int port = 22);

    // Assign systems for communication
    void setSystems(
        const std::array<std::shared_ptr<TargetSystem>, 3>& systems);

    // Add case to the manager
    bool addCase(const QString& caseName, const CaseData& data);
    void removeCase(const QString& caseName);

    // Check if case is present
    bool contains(const QString& caseName) const;

    // Get data for a given case
    CaseData getData(const QString& caseName) const;

    // Get names of cases
    QStringList getCases() const;

    // Remove all cases
    void clear();

    // Access target system for the given case
    std::shared_ptr<TargetSystem> getSystem(const QString& caseName) const;

    // Access target system for the given ID
    std::shared_ptr<TargetSystem> getSystem(int systemId) const;

    // Attempt to log into remote system
    QFutureWatcher<std::pair<bool, QString>>* setupConnection() const;

    QFutureWatcher<std::pair<bool, QString>>* sshConnect(
        const QString& user, const QString& host,
        const QString& password, int port) const;

    // Default credentials
    void setDefaultHost(const QString& host) { m_defaultHost = host; }
    void setDefaultUser(const QString& user) { m_defaultUser = user; }
    QString getDefaultHost() { return m_defaultHost; }
    QString getDefaultUser() { return m_defaultUser; }

private:
    QString m_serverVersion = "1.0.0", m_defaultHost, m_defaultUser;
    QMap<QString, CaseData> m_caseMap;
    std::array<std::shared_ptr<TargetSystem>,
        static_cast<int>(TargetType::COUNT)> m_systems;
    bool m_isWslAvailable = false;
    bool m_wslServerPresent = false;
    bool m_remoteServerPresent = false;
};

#endif  // SYSTEMS_SYSTEM_MANAGER_H_
