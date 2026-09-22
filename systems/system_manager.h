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

// Store case settings
enum CaseFlag {
    Initial            = 0,
    HasMeshFiles       = 1 << 0,
    HasFieldFiles      = 1 << 1,
    HasTimeDirs        = 1 << 2,
    NotChecked         = 1 << 3
};
Q_DECLARE_FLAGS(CaseFlags, CaseFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(CaseFlags)

// Store case properties
enum CaseProperty {
    Transient             = 1 << 0,
    Compressible          = 1 << 1,
    Multiphase            = 1 << 2,
    TurbulenceRAS         = 1 << 3,
    TurbulenceLES         = 1 << 4,
    FluidHeat             = 1 << 5,
    ConjugateHeat         = 1 << 6,
    MeshMRF               = 1 << 7,
    MeshAMI               = 1 << 8,
    MeshOverset           = 1 << 9,
    MeshDeforming         = 1 << 10,
    Radiation             = 1 << 11,
    Combustion            = 1 << 12,
    Buoyancy              = 1 << 13,
    Lagrangian            = 1 << 14,
    IsOpenCFD             = 1 << 15
};
Q_DECLARE_FLAGS(CaseType, CaseProperty)
Q_DECLARE_OPERATORS_FOR_FLAGS(CaseType)

// Store information about case
struct CaseData {
    QString casePath;
    QStringList openFolders;
    int targetId;
    QString openFoamPath;
    CaseFlags caseFlags;
    CaseType caseType;
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

    // Add case
    bool addCase(const QString& caseName, const CaseData& data);

    // Remove case
    void renameCase(const QString& oldName, const QString& newName);

    // Remove case
    void removeCase(const QString& caseName);

    // Check if case is present
    bool contains(const QString& caseName) const;

    // Get data for a given case
    CaseData getData(const QString& caseName) const;

    // Set flag
    void setFlag(const QString& caseName, CaseFlag flag, bool enabled = true);

    // Check flags
    CaseFlags updateFlags(const QString& caseNane,
                         const QString& casePath = QString());

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

    // Working with expanded folders
    void addOpenFolder(const QString& caseName, const QString& path);
    void removeOpenFolder(const QString& caseName, const QString& path);
    void updateOpenFolders(const QString& caseName, const QStringList& folders);

    // Case type functions
    CaseType updateType(int targetId, const QString& casePath, bool isOpenCFD);
    void setCaseType(const QString& caseName, CaseType type);
    bool testCaseFlag(const QString& caseName, CaseType flags) const;
    bool isLaminar(const QString& caseName) const;

    bool isOpenCFD(const QString& caseName) const {
        return testCaseFlag(caseName, IsOpenCFD);
    }

    bool isSteadyState(const QString& caseName) const {
        return !testCaseFlag(caseName, Transient);
    }

    bool isTransient(const QString& caseName) const {
        return testCaseFlag(caseName, Transient);
    }

    bool isCompressible(const QString& caseName) const {
        return testCaseFlag(caseName, Compressible);
    }

    bool isMultiphase(const QString& caseName) const {
        return testCaseFlag(caseName, Multiphase);
    }

    bool isRAS(const QString& caseName) const {
        return testCaseFlag(caseName, TurbulenceRAS);
    }

    bool isLES(const QString& caseName) const {
        return testCaseFlag(caseName, TurbulenceLES);
    }

    bool hasFluidHeatTransfer(const QString& caseName) const {
        return testCaseFlag(caseName, FluidHeat);
    }    

    bool hasRadiation(const QString& caseName) const {
        return testCaseFlag(caseName, Radiation);
    }

    bool hasCombustion(const QString& caseName) const {
        return testCaseFlag(caseName, Combustion);
    }

    bool hasBuoyancy(const QString& caseName) const {
        return testCaseFlag(caseName, Buoyancy);
    }

    bool isDynamicMesh(const QString& caseName) const {
        return testCaseFlag(caseName,
            MeshAMI | MeshDeforming | MeshOverset | MeshMRF);
    }

    bool isLagrangian(const QString& caseName) const {
        return testCaseFlag(caseName, Lagrangian);
    }

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
