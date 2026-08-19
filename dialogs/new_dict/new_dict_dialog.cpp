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

#include "dialogs/new_dict/new_dict_dialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QMetaEnum>
#include <QVBoxLayout>

#include "./core_types.h"
#include "parser/common.h"

NewDictDialog::NewDictDialog(const QString& caseName,
                             const QString& openFoamPath, QWidget* parent):
    QDialog(parent), m_openFoamPath(openFoamPath) {
    // Set title and style
    setWindowTitle(tr("Create New Dictionary"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Create the layout
    QFormLayout* mainLayout = new QFormLayout(this);
    mainLayout->setSpacing(15);
    setLayout(mainLayout);

    // Create the line edit for the dictionary name
    m_nameEdit = new QLineEdit(this);
    mainLayout->addRow(tr("File name:"), m_nameEdit);

    // Initialize the combo box
    m_classCombo = new QComboBox(this);
    m_classCombo->addItem("dictionary");
    mainLayout->addRow(tr("Class:"), m_classCombo);

    // Add strings from the enum to the combo box
    QMetaEnum metaEnum = QMetaEnum::fromType<FlowCompute::FieldClass>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        QString enumString = QString::fromLatin1(metaEnum.key(i));
        m_classCombo->addItem(enumString);
    }

    // Create OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &NewDictDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
}

void NewDictDialog::onOkClicked() {
    // Validate input
    m_fileName = m_nameEdit->text().trimmed();
        if (m_fileName.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Input"),
                                 tr("Please enter a file name."));
        m_nameEdit->setFocus();
        return;
    }

    // Set dictionary content
    m_dictContent = CaseIO::createFoamHeader(m_fileName,
        m_openFoamPath, m_classCombo->currentText());
    m_dictContent += "\n\n" + CaseIO::createFoamFooter();

    // Close dialog
    accept();
}