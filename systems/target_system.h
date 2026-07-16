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

#include "geometry/graphic_data.h"

// Abstract class that represents a target running OpenFOAM
class TargetSystem: public QObject {
    Q_OBJECT

 public:
    virtual ~TargetSystem() = default;
    virtual QStringList findOpenFoam() = 0;
    virtual QStringList getTutorials(const QString& path) = 0;
    virtual QStringList copyTutorialFolders(const QString& tutPath,
                                            const QString& projPath) = 0;
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
    virtual QStringList processPaths(const QString& path,
                                     PathOperationType type) = 0;

 signals:
    // Updates the console
    void logMessage(const QString& outputChunk);

    // Emitted when the process finally completes or fails
    void longUtilityFinished(const QString& status, const QString& caseName,
                             UtilityType type);

    // Emitted if there is a network or parsing failure
    void longUtilityError(const QString& errorMessage);
};

#endif  // SYSTEMS_TARGET_SYSTEM_H_
