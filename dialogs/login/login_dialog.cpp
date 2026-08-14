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

#include "login_dialog.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>

LoginDialog::LoginDialog(const QString& defaultUser, const QString& defaultHost,
                         QWidget *parent): QDialog(parent) {
    // Set appearance and layout
    setWindowTitle(tr("Please Enter Login Credentials"));
    setMinimumWidth(300);
    QFormLayout* layout = new QFormLayout(this);
    layout->setSpacing(15);

    // User name
    m_userNameEdit = new QLineEdit(this);
    layout->addRow(tr("User name: "), m_userNameEdit);
    m_userNameEdit->setText(defaultUser);

    // Hostname
    m_hostNameEdit = new QLineEdit(this);
    layout->addRow(tr("Hostname/IP address: "), m_hostNameEdit);
    m_hostNameEdit->setText(defaultHost);

    // Password
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addRow(tr("Password (optional): "), m_passwordEdit);

    // Port number
    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(22);
    layout->addRow(tr("Port number: "), m_portSpin);

    // Standard OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                        QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);

    // Connections
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SshCredentials LoginDialog::getCredentials() {
    SshCredentials cred;
    cred.userName = m_userNameEdit->text();
    cred.hostName = m_hostNameEdit->text();
    cred.password = m_passwordEdit->text();
    cred.port = m_portSpin->value();
    return cred;
}