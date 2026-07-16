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

#include "QDebug"

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