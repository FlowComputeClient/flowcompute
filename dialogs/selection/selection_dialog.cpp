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

#include "selection_dialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QDialogButtonBox>

SelectionDialog::SelectionDialog(const QString& title, const QString& prompt,
        const QStringList& items, QWidget* parent): QDialog(parent) {
    setWindowTitle(title);
    setMinimumWidth(300);
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

    // Create layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);

    // Set the prompt
    QLabel* label = new QLabel(prompt, this);
    QFont font = label->font();
    font.setBold(true);
    label->setFont(font);
    layout->addWidget(label);

    // Separate layout for radio buttons
    QVBoxLayout* radioLayout = new QVBoxLayout();
    radioLayout->setSpacing(20);
    radioLayout->setContentsMargins(15, 0, 0, 0);

    // Create a button group for the radio buttons
    m_buttonGroup = new QButtonGroup(this);

    // Add a radio button for each item
    for (int i = 0; i < items.size(); ++i) {
        QRadioButton* radioBtn = new QRadioButton(items.at(i), this);
        radioLayout->addWidget(radioBtn);
        m_buttonGroup->addButton(radioBtn, i);

        // Select first item by default
        if (i == 0) {
            radioBtn->setChecked(true);
        }
    }
    layout->addLayout(radioLayout);

    // Create the OK button
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    buttonBox->setCenterButtons(true);
    layout->addWidget(buttonBox);

    // Connections
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

// Access the currently checked radio button
QString SelectionDialog::getSelectedItem() const {
    if (QAbstractButton* checkedButton = m_buttonGroup->checkedButton()) {
        return checkedButton->text();
    }
    return QString();
}

// Access the currently checked radio button
int SelectionDialog::getSelectedIndex() const {
    return m_buttonGroup->checkedId();
}
