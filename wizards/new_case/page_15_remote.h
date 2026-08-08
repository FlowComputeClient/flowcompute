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

#ifndef PAGE_15_REMOTE_H_
#define PAGE_15_REMOTE_H_

#include <QFutureWatcher>
#include <QWizardPage>

class NewCaseWizard;
class QLineEdit;
class QSpinBox;
class QString;

#include "systems/system_manager.h"

class RemotePage : public QWizardPage {
    Q_OBJECT

 public:
    RemotePage(SystemManager& systemMgr, QWidget *parent);
    int nextId() const override;
    bool isComplete() const override;

 private:
    bool m_isConnected;
    SystemManager& m_systemMgr;
    QFutureWatcher<std::pair<bool, QString>>* m_authWatcher;
    QLineEdit *m_userNameEdit, *m_hostNameEdit, *m_passwordEdit;
    QSpinBox *m_portSpin;
    QPushButton *m_connectButton;

 private slots:
    void onConnectClicked();
    void onAuthFinished();
};

#endif  // PAGE_15_REMOTE_H_
