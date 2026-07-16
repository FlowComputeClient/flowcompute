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

#include "remote_system.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

namespace fs = std::filesystem;

QStringList RemoteSystem::processPaths(const QString& path,
                                       PathOperationType opType) {
    QStringList results;
    return results;
}

// Launch a utility in the server
int RemoteSystem::launchShortUtility(const QString& cmd, QString& output) {
    return 0;
}

void RemoteSystem::launchLongUtility(const QString& cmd, const QString& caseName,
                                    UtilityType utilityType) {
}

// Locate OpenFOAM installation(s)
QStringList RemoteSystem::findOpenFoam() {
    QStringList ofList;
    return ofList;
}

QStringList RemoteSystem::getTutorials(const QString& path) {
    return {};
}

QStringList RemoteSystem::copyTutorialFolders(const QString& tutPath,
                                              const QString& casePath) {
    return {};
}

bool RemoteSystem::writeData(const QByteArray& data, const QString& filePath) {
    return true;
}

bool RemoteSystem::writeData(const QString& srcPath, const QString& dstPath) {
    return true;
}

QString RemoteSystem::getResultFolders(QString casePath) {
    return "";
}

QByteArray RemoteSystem::getFileContent(const QString& path) {
    QByteArray fileData;
    return fileData;
}

RenderData RemoteSystem::getResultData(const QString& path) {

    // Populate the render data structure
    RenderData renderData;
    return renderData;
}
