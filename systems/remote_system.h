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

#ifndef SYSTEMS_REMOTE_SYSTEM_H_
#define SYSTEMS_REMOTE_SYSTEM_H_

#include "target_system.h"

#include <libssh/libssh.h>

class RemoteSystem : public TargetSystem {
    Q_OBJECT

 public:
    RemoteSystem() {}
    ~RemoteSystem();

    // Create connection
    bool establishSession(const QString& host, const QString& user,
          int port, const QString& passwd, QString& errorMessage,
          std::function<bool(const QString&, const QString&)> callback);

    // Execute command through SSH
    QString execCommand(const QString& cmd);

    QStringList findOpenFoam() override;
    QStringList getTutorials(const QString& path) override;

    void processAllrunScript(const QString& scriptPath,
        const QString& projectPath, const QString& originalTutorialPath);
    QStringList copyTutorialFolders(const QString& tutPath,
                                    const QString& projPath) override;
    QByteArray getFileContent(const QString& path) override;
    bool writeData(const QByteArray& payload,
                   const QString& remoteFilePath) override;
    bool writeData(const QString& localPath,
                   const QString& remoteFilePath) override;
    int launchShortUtility(const QString& cmd, QString& output) override;
    void launchLongUtility(const QString& cmd, const QString& caseName,
                           UtilityType utilityType) override;
    std::pair<QStringList, QStringList>
        getTimesAndFields(const QString& projPath) override;
    QStringList processPaths(const QString& path,
                             PathOperationType type) override;
    RenderData getMeshData(const QString& path) override;
    RenderData getResultData(const QString& path) override;

 private:
    ssh_session m_session = nullptr;
};

#endif  // SYSTEMS_REMOTE_SYSTEM_H_
