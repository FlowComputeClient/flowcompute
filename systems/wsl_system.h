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

#ifndef SYSTEMS_WSL_SYSTEM_H_
#define SYSTEMS_WSL_SYSTEM_H_

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSettings>
#include <QString>
#include <QTcpSocket>
#include <QVBoxLayout>

#include <string>
#include <vector>

#include "./target_system.h"

struct ResultHeader {
    uint32_t magicNumber;
    uint32_t dataByteSize;
    uint32_t indexByteSize;
    std::array<float, 3> boundingBoxMin;
    std::array<float, 3> boundingBoxMax;
};

class WslSystem : public TargetSystem {
    Q_OBJECT

 public:
    WslSystem() {}
    ~WslSystem() {}

    bool checkDistributions();
    QStringList findOpenFoam() override;
    QStringList getTutorials(const QString& path) override;
    QStringList copyTutorialFolders(const QString& tutPath,
                                    const QString& projPath) override;
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
    QStringList processPaths(const QString& path,
                             PathOperationType type) override;

 private:
    void terminateProcess();
    QString createSelectionDialog(const std::vector<std::string>& paths);
    QJsonObject contactServer(QString action, QString message, int opType = -1);
};

#endif  // SYSTEMS_WSL_SYSTEM_H_
