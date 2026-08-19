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

#ifndef WIZARDS_OPEN_CASE_WIZARD_OPEN_CASE_H_
#define WIZARDS_OPEN_CASE_WIZARD_OPEN_CASE_H_

#include <QWizard>

#include "systems/system_manager.h"

class OpenCaseWizard : public QWizard {
    Q_OBJECT

 public:
    OpenCaseWizard(TargetType targetType, SystemManager& systemMgr,
                   QString openFoamPath, QWidget *parent);

 signals:
    void requestCaseCreation(QString caseName, QString casePath,
        QStringList caseFiles, int systemId, QString openFoamPath,
        QString userName, QString hostName, int port);
    void logMessage(const QString& msg);

 protected:
    void accept() override;

 private:
    SystemManager& m_systemMgr;
    TargetType m_targetType;
    QString m_openFoamPath;
};

#endif  // WIZARDS_OPEN_CASE_WIZARD_OPEN_CASE_H_
