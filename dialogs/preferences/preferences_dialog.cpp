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

#include "preferences_dialog.h"

#include <QFormLayout>

PreferencesDialog::PreferencesDialog(QWidget* parent): QDialog(parent) {

    setWindowTitle(tr("Preferences"));
    setMinimumWidth(300);

    QFormLayout* layout = new QFormLayout(this);

    // Select the theme
    m_themeCombo = new QComboBox(this);
    m_themeCombo->addItems( { "dark", "light" } );
    layout->addRow(tr("Select a theme: "), m_themeCombo);

    // Standard OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                        QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);

    // Connections
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString PreferencesDialog::getTheme() const {
    if (m_themeCombo) {
        return m_themeCombo->currentText();
    }
    return QString();
}
