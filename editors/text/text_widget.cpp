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

#include "text_widget.h"

#include <QVBoxLayout>

TextWidget::TextWidget(QWidget *parent) : QWidget(parent) {
    // Create layout
    QVBoxLayout* m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Create banner
    m_banner = new FileChangedBanner(this);
    m_banner->hide();
    m_layout->addWidget(m_banner);

    // Create text editor
    m_textEditor = new TextEditor(this);
    m_layout->addWidget(m_textEditor);

    // Emit signal if the Reload button is pressed
    connect(m_banner, &FileChangedBanner::reloadClicked, this, [this]() {
        emit reloadRequested();
    });
}
