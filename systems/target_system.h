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

#ifndef SYSTEMS_TARGET_SYSTEM_H_
#define SYSTEMS_TARGET_SYSTEM_H_

#include <QObject>
#include <QString>
#include <QStringList>

enum class UtilityType {
    MESH = 0,
    SOLVER
};

enum class PathOperationType {
    CREATE = 0,
    DELETE,
    CHECK,
    LIST
};

#include "../geometry/graphic_data.h"

// Abstract class that represents a target running OpenFOAM
class TargetSystem: public QObject {
    Q_OBJECT

 public:
    virtual ~TargetSystem() = default;

    virtual QStringList findOpenFoam() = 0;
    virtual QStringList getTutorials(QString path) = 0;
    virtual QStringList copyTutorialFolders(QString tutPath,
                                            QString projPath) = 0;
    virtual QByteArray getFileContent(const QString& path) = 0;
    virtual RenderData getResultData(const QString& path) = 0;
    virtual bool writeData(const QByteArray& payload,
                           const QString& remoteFilePath) = 0;
    virtual bool writeData(const QString& localPath,
                           const QString& remoteFilePath) = 0;
    virtual int launchShortUtility(const QString& cmd, QString& output) = 0;
    virtual void launchLongUtility(const QString& cmd, const QString& caseName,
                                   UtilityType type) = 0;
    virtual QString getResultFolders(QString path) = 0;
    virtual QStringList processPaths(QString path, PathOperationType type) = 0;
};

#endif  // SYSTEMS_TARGET_SYSTEM_H_
