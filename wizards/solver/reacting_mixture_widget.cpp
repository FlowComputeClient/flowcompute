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

#include "reacting_mixture_widget.h"

#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

ReactingMixtureWidget::ReactingMixtureWidget(QWidget* parent):
    MixtureWidget(parent) {

    // Create layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    setMinimumWidth(250);

    // Create label
    QLabel* mixtureLabel = new QLabel(tr("Enter participating species "
                    " (separated by spaces or newlines):"), this);
    mainLayout->addWidget(mixtureLabel);

    // Create text edit
    m_speciesEdit = new QPlainTextEdit(this);
    mainLayout->addWidget(m_speciesEdit);
}

CaseIO::MixtureVariant ReactingMixtureWidget::getData() const {
    CaseIO::ReactingMixtureConfig config;

    // Extract the raw text from the text edit
    QString rawText = m_speciesEdit->toPlainText().trimmed();
    if (!rawText.isEmpty()) {
        config.participatingSpecies =
            rawText.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    }

    return config;
}