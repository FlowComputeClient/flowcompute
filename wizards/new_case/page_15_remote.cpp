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

#include "page_15_remote.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QOperatingSystemVersion>
#include <QPushButton>
#include <QSpinBox>
#include <QtConcurrent>
#include <QVBoxLayout>

#include <libssh/libssh.h>

#include "systems/remote_system.h"
#include "wizard_new_case.h"

// Configure remote access to server
RemotePage::RemotePage(SystemManager& systemMgr, QWidget *parent):
    QWizardPage(parent), m_systemMgr(systemMgr), m_isConnected(false) {
    // Set title
    setTitle(tr("Remote System Authentication"));

    // Create main vertical layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);

    // Add description
    QLabel* descLabel = new QLabel(tr("<b>Please provide the credentials to "
                            "connect to the remote system.</b>"), this);
    mainLayout->addWidget(descLabel);

    // Authentication group
    QGroupBox* authGroup = new QGroupBox(tr("Authentication"), this);
    QFormLayout* authLayout = new QFormLayout(authGroup);
    authLayout->setSpacing(25);
    authGroup->setLayout(authLayout);
    mainLayout->addWidget(authGroup);

    // User name
    m_userNameEdit = new QLineEdit(authGroup);
    authLayout->addRow(tr("User name: "), m_userNameEdit);
    registerField("remoteUser", m_userNameEdit);

    // Hostname
    m_hostNameEdit = new QLineEdit(authGroup);
    authLayout->addRow(tr("Hostname/IP address: "), m_hostNameEdit);
    registerField("remoteHost", m_hostNameEdit);

    m_userNameEdit->setText("mattscar");
    m_hostNameEdit->setText("192.168.100.33");

    // Password
    m_passwordEdit = new QLineEdit(authGroup);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    authLayout->addRow(tr("Password (optional): "), m_passwordEdit);
    registerField("remotePassword", m_passwordEdit);

    // Port number
    m_portSpin = new QSpinBox(authGroup);
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(22);
    authLayout->addRow(tr("Port number: "), m_portSpin);
    registerField("remotePort", m_portSpin);

    // Connection button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_connectButton = new QPushButton(tr("Establish Connection"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_connectButton);
    mainLayout->addLayout(buttonLayout);

    // Event handling
    m_authWatcher = new QFutureWatcher<std::pair<bool, QString>>(this);
    connect(m_connectButton, &QPushButton::clicked, this,
            &RemotePage::onConnectClicked);
    connect(m_authWatcher, &QFutureWatcher<std::pair<bool, QString>>::finished,
            this, &RemotePage::onAuthFinished);

    // Set the page layout
    setLayout(mainLayout);
}

void RemotePage::onAuthFinished() {
    // Retrieve results
    std::pair<bool, QString> result = m_authWatcher->result();
    m_isConnected = result.first;
    QString errorMsg = result.second;

    // Restore UI state
    m_connectButton->setEnabled(true);
    m_userNameEdit->setEnabled(true);
    m_hostNameEdit->setEnabled(true);
    m_portSpin->setEnabled(true);

    // Handle success or failure
    if (m_isConnected) {
        m_connectButton->setText(tr("Connected!"));
        m_connectButton->setStyleSheet("color: green;");
        m_connectButton->setEnabled(false);
    } else {
        m_connectButton->setText(tr("Connection Failed. Retry?"));
        m_connectButton->setStyleSheet("color: red;");

        // Display the specific libssh error to the user
        QMessageBox::critical(this, tr("Authentication Error"),
          tr("Failed to connect to the remote system.\n\nError: %1").arg(errorMsg));
    }

    // Update wizard Next button state
    emit completeChanged();
}
void RemotePage::onConnectClicked() {
    // Lock the UI
    m_connectButton->setEnabled(false);
    m_connectButton->setText(tr("Connecting..."));
    m_userNameEdit->setEnabled(false);
    m_hostNameEdit->setEnabled(false);
    m_portSpin->setEnabled(false);
    m_passwordEdit->setEnabled(false);

    // Gather inputs on the main GUI thread
    QString host = m_hostNameEdit->text();
    QString user = m_userNameEdit->text();
    QString passwd = m_passwordEdit->text();
    int port = m_portSpin->value();
    std::shared_ptr<RemoteSystem> remoteSystem =
        std::static_pointer_cast<RemoteSystem>(
            m_systemMgr.getSystem(static_cast<int>(TargetType::REMOTE_LINUX)));

    // Define callback function
    auto callback =
        [this](const QString& host, const QString& fingerprint) -> bool {
        bool userAccepted = false;

        // Run on the main GUI Thread
        QMetaObject::invokeMethod(this, [this, host, fingerprint,
                                         &userAccepted]() {
            QString msg =
                tr("The authenticity of host '%1' cannot be established.\n"
                    "SHA1 key fingerprint is: %2\n\n"
                    "Are you sure you want to continue connecting?")
                    .arg(host, fingerprint);

            // Wait for reply
            QMessageBox::StandardButton reply;
            reply = QMessageBox::warning(this, tr("Unknown Host"), msg,
                QMessageBox::Yes | QMessageBox::No);
            userAccepted = (reply == QMessageBox::Yes);
        }, Qt::BlockingQueuedConnection);
        return userAccepted;
    };

    // Run SSH authentication
    QString errorMessage;
    QFuture<std::pair<bool, QString>> future =
        QtConcurrent::run([host, user, port, passwd, remoteSystem, callback]() {
            QString errorMsg;
            bool success = remoteSystem->establishSession(host, user, port,
                                            passwd, errorMsg, callback);
            return std::make_pair(success, errorMsg);
        });
    m_authWatcher->setFuture(future);
}

int RemotePage::nextId() const {
    int caseCreationValue = field("caseCreationType").toInt();
    CaseCreationType caseCreation =
        static_cast<CaseCreationType>(caseCreationValue);
    switch (caseCreation) {
    case CaseCreationType::TUTORIAL:
        return static_cast<int>(WizardPage::Page_Tutorial);
    case CaseCreationType::INTERACTIVE:
        return static_cast<int>(WizardPage::Page_Interactive);
    }
    return -1;
}

bool RemotePage::isComplete() const {
    return m_isConnected && QWizardPage::isComplete();
}
