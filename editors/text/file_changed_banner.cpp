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

#include "file_changed_banner.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>

FileChangedBanner::FileChangedBanner(QWidget *parent) : QFrame(parent) {
    // Set style
    setProperty("widgetType", "pane");

    // Create layout
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 4, 10, 4);

    // Warning Icon
    m_iconLabel = new QLabel(this);
    QIcon warningIcon =
        QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning);
    m_iconLabel->setPixmap(warningIcon.pixmap(16, 16));
    layout->addWidget(m_iconLabel);

    // Notification Text
    m_messageLabel = new QLabel("This file has been modified externally.", this);
    layout->addWidget(m_messageLabel);

    // Add stretch to push buttons right
    layout->addSpacing(50);

    // Reload button
    m_reloadButton = new QPushButton("Reload", this);
    layout->addWidget(m_reloadButton);

    // Ignore button
    m_ignoreButton = new QPushButton("Ignore", this);
    layout->addWidget(m_ignoreButton);
    layout->addStretch();

    // Signal Routing
    connect(m_reloadButton, &QPushButton::clicked, this,
            &FileChangedBanner::reloadClicked);

    connect(m_ignoreButton, &QPushButton::clicked, this, [this]() {
        this->hide();
        emit ignoreClicked();
    });
}
