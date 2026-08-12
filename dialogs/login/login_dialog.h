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

#ifndef DIALOG_LOGIN_DIALOG_H_
#define DIALOG_LOGIN_DIALOG_H_

#include <QComboBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

struct SshCredentials {
    QString userName;
    QString hostName;
    QString password;
    int port = 22;
};

class QLineEdit;
class QSpinBox;

class LoginDialog : public QDialog {
    Q_OBJECT

 public:
    LoginDialog(const QString& defaultUser, const QString& defaultHost,
                 QWidget *parent = nullptr);
    SshCredentials getCredentials();

 private:
    QLineEdit *m_userNameEdit, *m_hostNameEdit, *m_passwordEdit;
    QSpinBox *m_portSpin;
};

#endif  // DIALOG_LOGIN_DIALOG_H_
