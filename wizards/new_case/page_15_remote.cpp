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
#include <QProgressDialog>
#include <QPushButton>
#include <QSpinBox>
#include <QtConcurrent>
#include <QVBoxLayout>

#include <libssh/libssh.h>

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
    registerField("userName", m_userNameEdit);
    m_userNameEdit->setText(m_systemMgr.getDefaultUser());

    // Hostname
    m_hostNameEdit = new QLineEdit(authGroup);
    authLayout->addRow(tr("Hostname/IP address: "), m_hostNameEdit);
    registerField("hostName", m_hostNameEdit);
    m_hostNameEdit->setText(m_systemMgr.getDefaultHost());

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
    registerField("port", m_portSpin);

    // Connection button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(20);
    m_connectButton = new QPushButton(tr("Establish Connection"), this);
    buttonLayout->addWidget(m_connectButton);
    m_connectLabel = new QLabel("");
    buttonLayout->addWidget(m_connectLabel);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Event handling
    connect(m_connectButton, &QPushButton::clicked, this,
            &RemotePage::onConnectClicked);

    // Set the page layout
    setLayout(mainLayout);
}

void RemotePage::onConnectClicked() {
    // Lock the UI
    m_connectButton->setEnabled(false);
    m_userNameEdit->setEnabled(false);
    m_hostNameEdit->setEnabled(false);
    m_portSpin->setEnabled(false);
    m_passwordEdit->setEnabled(false);

    // Gather inputs on the main GUI thread
    QString host = m_hostNameEdit->text();
    QString user = m_userNameEdit->text();
    QString password = m_passwordEdit->text();
    int port = m_portSpin->value();

    // Create progress dialog
    QProgressDialog* progress = new QProgressDialog(
        tr("Attempting to connect..."), QString(), 0, 0, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->show();

    // Launch connect
    auto watcher = m_systemMgr.sshConnect(user, host, password, port);
    connect(watcher, &QFutureWatcher<std::pair<bool, QString>>::finished,
            this, [this, watcher, progress]() {

        // Remove the progress dialog
        progress->accept();
        progress->deleteLater();

        std::pair<bool, QString> result = watcher->result();
        this->onAuthFinished(result.first, result.second);
        watcher->deleteLater();
    });
}

void RemotePage::onAuthFinished(bool isConnected, QString errorMsg) {
    // Retrieve results
    m_isConnected = isConnected;

    // Restore UI state
    m_connectButton->setEnabled(true);
    m_userNameEdit->setEnabled(true);
    m_hostNameEdit->setEnabled(true);
    m_portSpin->setEnabled(true);

    // Respond to success or failure
    if (m_isConnected) {
        m_connectLabel->setText(tr("Connection successful"));
        m_connectLabel->setStyleSheet("color: green;");
    } else {
        m_connectLabel->setText(tr("Connection failed: %1").arg(errorMsg));
        m_connectLabel->setStyleSheet("color: red;");
    }

    // Update wizard Next button state
    emit completeChanged();
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
