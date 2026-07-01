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

SelectionDialog::SelectionDialog(const QString& title, const QString& prompt,
                                 const QStringList& items, QWidget* parent): QDialog(parent) {

    setWindowTitle(title);
    setMinimumWidth(300);

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Add the dynamic prompt
    QLabel* label = new QLabel(prompt, this);
    layout->addWidget(label);

    // Add the scalable list
    m_listWidget = new QListWidget(this);
    m_listWidget->addItems(items);
    if (!items.isEmpty()) {
        m_listWidget->setCurrentRow(0);
    }
    layout->addWidget(m_listWidget);

    // Standard OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);

    // Connections
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // The UX Magic: Double-clicking an item acts like clicking OK
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &QDialog::accept);
}

QString SelectionDialog::getSelectedItem() const {
    if (QListWidgetItem* item = m_listWidget->currentItem()) {
        return item->text();
    }
    return QString();
}