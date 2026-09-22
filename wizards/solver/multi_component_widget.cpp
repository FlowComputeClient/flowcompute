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

#include "wizards/solver/multi_component_widget.h"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "wizards/solver/pure_mixture_widget.h"

MultiComponentWidget::MultiComponentWidget(const QString& thermo,
    const QString& transport, const QString& eos, QWidget* parent):
    MixtureWidget(parent), m_thermo(thermo), m_transport(transport),
    m_eos(eos) {

    // Create the main layout
    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    // Create the layout for the left side
    QVBoxLayout* leftLayout = new QVBoxLayout();

    // Create the list widget
    m_speciesList = new QListWidget(this);
    leftLayout->addWidget(m_speciesList);

    // Create buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* addButton = new QPushButton(tr("Add Species"), this);
    QPushButton* removeButton = new QPushButton(tr("Remove Species"), this);
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    leftLayout->addLayout(buttonLayout);
    mainLayout->addLayout(leftLayout, 1);

    // Placeholder label
    QLabel* emptyLabel = new QLabel(tr("Add or select a species to "
                                       "configure its properties."), this);
    emptyLabel->setAlignment(Qt::AlignCenter);
    m_stackedWidget->addWidget(emptyLabel);

    // Create the stacked widget
    m_stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(m_stackedWidget, 2);

    // --- Signal Connections ---
    connect(addButton, &QPushButton::clicked, this,
            &MultiComponentWidget::addSpecies);
    connect(removeButton, &QPushButton::clicked, this,
            &MultiComponentWidget::removeSpecies);

    // Sync the list selection with the stacked widget view
    connect(m_speciesList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0) {
            // Offset by +1 because Index 0 is the empty placeholder label
            m_stackedWidget->setCurrentIndex(row + 1);
        } else {
            m_stackedWidget->setCurrentIndex(0);
        }
    });
}

void MultiComponentWidget::addSpecies() {
    bool ok;
    QString speciesName = QInputDialog::getText(this, tr("Add Species"),
        tr("Enter species name (e.g., O2):"), QLineEdit::Normal, "", &ok);

    if (ok && !speciesName.trimmed().isEmpty()) {
        speciesName = speciesName.trimmed();

        // Prevent duplicate species names
        if (!m_speciesList->findItems(speciesName, Qt::MatchExactly).isEmpty())
        {
            QMessageBox::warning(this, tr("Duplicate"), tr("Species exists."));
            return;
        }

        // Add to the list
        m_speciesList->addItem(speciesName);

        // Create a PureMixtureWidget and add it to the stack
        PureMixtureWidget* speciesWidget =
            new PureMixtureWidget(m_thermo, m_transport, m_eos, this);
        m_stackedWidget->addWidget(speciesWidget);
        m_speciesList->setCurrentRow(m_speciesList->count() - 1);
    }
}

void MultiComponentWidget::removeSpecies() {
    int currentRow = m_speciesList->currentRow();

    if (currentRow >= 0) {
        // Remove/delete the list item
        delete m_speciesList->takeItem(currentRow);

        // Remove/delete the widget
        QWidget* widgetToRemove = m_stackedWidget->widget(currentRow + 1);
        m_stackedWidget->removeWidget(widgetToRemove);
        widgetToRemove->deleteLater();
    }
}

CaseIO::MixtureVariant MultiComponentWidget::getData() const {
    CaseIO::MultiComponentConfig config;

    // Iterate through all user-defined species in the list
    for (int i = 0; i < m_speciesList->count(); ++i) {
        QString speciesName = m_speciesList->item(i)->text();

        // Retrieve the corresponding widget from the stack
        QWidget* stackedWidget = m_stackedWidget->widget(i + 1);
        PureMixtureWidget* speciesWidget =
            qobject_cast<PureMixtureWidget*>(stackedWidget);

        if (speciesWidget) {
            CaseIO::MixtureVariant childVariant = speciesWidget->getData();

            // Add the PureMixtureConfig to the map
            if (std::holds_alternative<
                    CaseIO::PureMixtureConfig>(childVariant)) {
                config.speciesProperties.insert(speciesName,
                    std::get<CaseIO::PureMixtureConfig>(childVariant));
            }
        }
    }

    return config;
}