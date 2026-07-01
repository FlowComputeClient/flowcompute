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

#include "utility_output_dialog.h"

#include <QFontDatabase>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QStyle>
#include <QApplication>
#include <QFont>

UtilityOutputDialog::UtilityOutputDialog(const QString& title,
                                         const QString& description,
                                         const std::vector<QString>& summaryItems,
                                         const QString& rawLog,
                                         QWidget *parent)
    : QDialog(parent), m_rawLogTextEdit(nullptr), m_toggleButton(nullptr) {

    // Create layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    // mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    mainLayout->setSpacing(0);

    // Set appearance
    setWindowTitle(title);

    // Set description
    if (!description.isEmpty()) {
        mainLayout->addSpacing(10);
        QLabel* descLabel = new QLabel("<b>" + description + "</b>", this);
        descLabel->setWordWrap(true);
        QFont f = descLabel->font();
        f.setPointSize(f.pointSize() + 2);
        descLabel->setFont(f);
        mainLayout->addWidget(descLabel);
    }

    mainLayout->addSpacing(20);

    // Display Summary Items
    for (const auto& item : summaryItems) {
        QHBoxLayout* itemLayout = new QHBoxLayout();
        QLabel* textLabel = new QLabel(item, this);
        textLabel->setIndent(15);
        itemLayout->addWidget(textLabel);
        itemLayout->addStretch();
        mainLayout->addLayout(itemLayout);
    }

    mainLayout->addSpacing(15);

    // Log button
    m_toggleButton = new QPushButton(tr("Show Log"), this);
    m_toggleButton->setCheckable(true);
    connect(m_toggleButton, &QPushButton::toggled, this,
            &UtilityOutputDialog::toggleLogViewer);
    mainLayout->addWidget(m_toggleButton);

    // Show/hide log text
    m_rawLogTextEdit = new QTextEdit(this);
    m_rawLogTextEdit->setReadOnly(true);
    m_rawLogTextEdit->setPlainText(rawLog);
    m_rawLogTextEdit->setMinimumSize(400, 300);

    // Use a monospace font for terminal output
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_rawLogTextEdit->setFont(monoFont);
    monoFont.setStyleHint(QFont::Monospace);
    m_rawLogTextEdit->setFont(monoFont);
    m_rawLogTextEdit->setVisible(false);
    mainLayout->addWidget(m_rawLogTextEdit);

    mainLayout->addSpacing(20);

    // Close Button
    QPushButton* closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    this->adjustSize();
    setMinimumWidth(320);
}

void UtilityOutputDialog::toggleLogViewer(bool checked) {
    m_rawLogTextEdit->setVisible(checked);

    if (checked) {
        m_toggleButton->setText(tr("Hide Log"));
    } else {
        m_toggleButton->setText(tr("Show Log"));
    }

    // Resize the dialog
    this->adjustSize();
}