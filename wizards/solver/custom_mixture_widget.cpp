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

#include "custom_mixture_widget.h"

#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

CustomMixtureWidget::CustomMixtureWidget(QWidget* parent):
    MixtureWidget(parent) {

    // Create layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    setMinimumWidth(250);

    // Create label
    QLabel* mixtureLabel =
        new QLabel(tr("Enter dictionary block for custom mixture:"), this);
    mainLayout->addWidget(mixtureLabel);

    // Create text edit
    m_dictionaryEdit = new QPlainTextEdit(this);
    mainLayout->addWidget(m_dictionaryEdit);
}

CaseIO::MixtureVariant CustomMixtureWidget::getData() const {
    CaseIO::CustomMixtureConfig config;
    config.dictionaryText = m_dictionaryEdit->toPlainText();
    return config;
}