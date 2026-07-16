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

#include "./target_system.h"

#include <QMap>

// Store information about project in navigator
struct CaseData {
    QString casePath;
    QStringList caseFiles;
    int targetId;
    QString openFoamPath;
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

    // Assign systems for communication
    void setSystems(
        const std::array<std::shared_ptr<TargetSystem>, 3>& systems);

    // Add case to the manager
    bool addCase(const QString& caseName, const CaseData& data);

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

private:
    QMap<QString, CaseData> m_caseMap;
    std::array<std::shared_ptr<TargetSystem>,
        static_cast<int>(TargetType::COUNT)> m_systems;
};

#endif  // SYSTEMS_SYSTEM_MANAGER_H_
