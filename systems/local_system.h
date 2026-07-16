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

#ifndef SYSTEMS_LOCAL_SYSTEM_H_
#define SYSTEMS_LOCAL_SYSTEM_H_

#include "target_system.h"

class LocalSystem : public TargetSystem {
    Q_OBJECT

 public:
    LocalSystem() {}
    ~LocalSystem() {}

    QStringList findOpenFoam() override;
    QStringList getTutorials(QString path) override;
    QStringList copyTutorialFolders(QString tutPath,
                                    QString projPath) override;
    QByteArray getFileContent(const QString& path) override;
    RenderData getResultData(const QString& path) override;
    bool writeData(const QByteArray& payload,
                   const QString& remoteFilePath) override;
    bool writeData(const QString& localPath,
                   const QString& remoteFilePath) override;
    int launchShortUtility(const QString& cmd, QString& output) override;
    void launchLongUtility(const QString& cmd, const QString& caseName,
                           UtilityType utilityType) override;
    QString getResultFolders(QString path) override;
    QStringList processPaths(QString path, PathOperationType type) override;
};

#endif  // SYSTEMS_LOCAL_SYSTEM_H_
