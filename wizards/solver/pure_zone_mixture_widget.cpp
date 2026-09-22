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

#include "wizards/solver/pure_zone_mixture_widget.h"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "wizards/solver/pure_mixture_widget.h"

PureZoneMixtureWidget::PureZoneMixtureWidget(const QString& thermo,
    const QString& transport, const QString& eos, QWidget* parent):
    MixtureWidget(parent), m_thermo(thermo), m_transport(transport),
    m_eos(eos) {

    // Create the main layout
    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    // Create the layout for the left side
    QVBoxLayout* leftLayout = new QVBoxLayout();

    // Create the list widget
    m_zoneList = new QListWidget(this);
    leftLayout->addWidget(m_zoneList);

    // Create buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* addButton = new QPushButton(tr("Add Zone"), this);
    QPushButton* removeButton = new QPushButton(tr("Remove Zone"), this);
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    leftLayout->addLayout(buttonLayout);
    mainLayout->addLayout(leftLayout, 1);

    // Create the stacked widget
    m_stackedWidget = new QStackedWidget(this);

    // Placeholder label
    QLabel* emptyLabel = new QLabel(tr("Add or select a zone to "
                                       "configure its properties."), this);
    emptyLabel->setAlignment(Qt::AlignCenter);
    m_stackedWidget->addWidget(emptyLabel);

    mainLayout->addWidget(m_stackedWidget, 2);

    // Signal Connections
    connect(addButton, &QPushButton::clicked, this,
            &PureZoneMixtureWidget::addZone);
    connect(removeButton, &QPushButton::clicked, this,
            &PureZoneMixtureWidget::removeZone);

    // Sync the list selection with the stacked widget view
    connect(m_zoneList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0) {
            m_stackedWidget->setCurrentIndex(row + 1);
        } else {
            m_stackedWidget->setCurrentIndex(0);
        }
    });
}

void PureZoneMixtureWidget::addZone() {
    bool ok;
    // Create input dialog for zone name
    QString zoneName = QInputDialog::getText(this, tr("Add Zone"),
        tr("Enter zone name:"), QLineEdit::Normal, "", &ok);

    if (ok && !zoneName.trimmed().isEmpty()) {
        zoneName = zoneName.trimmed();

        // Prevent duplicate zone names
        if (!m_zoneList->findItems(zoneName, Qt::MatchExactly).isEmpty()) {
            QMessageBox::warning(this, tr("Duplicate"),
                                 tr("Zone already exists."));
            return;
        }

        // Add to the list
        m_zoneList->addItem(zoneName);

        // Create a PureMixtureWidget
        PureMixtureWidget* zoneWidget =
            new PureMixtureWidget(m_thermo, m_transport, m_eos, this);
        m_stackedWidget->addWidget(zoneWidget);

        m_zoneList->setCurrentRow(m_zoneList->count() - 1);
    }
}

void PureZoneMixtureWidget::removeZone() {
    int currentRow = m_zoneList->currentRow();

    if (currentRow >= 0) {
        // Remove the list item
        delete m_zoneList->takeItem(currentRow);

        // Remove and delete the widget (offset by 1 for the placeholder)
        QWidget* widgetToRemove = m_stackedWidget->widget(currentRow + 1);
        m_stackedWidget->removeWidget(widgetToRemove);
        widgetToRemove->deleteLater();
    }
}

CaseIO::MixtureVariant PureZoneMixtureWidget::getData() const {
    CaseIO::PureZoneMixtureConfig config;

    // Iterate through zones
    for (int i = 0; i < m_zoneList->count(); ++i) {
        // Get the zone name
        QString zoneName = m_zoneList->item(i)->text();

        // Retrieve the corresponding widget from the stack
        QWidget* stackedWidget = m_stackedWidget->widget(i + 1);

        // Cast the QWidget to PureMixtureWidget
        PureMixtureWidget* zoneWidget =
            qobject_cast<PureMixtureWidget*>(stackedWidget);

        if (zoneWidget) {
            // Extract the data from the child widget
            CaseIO::MixtureVariant childVariant = zoneWidget->getData();
            if (std::holds_alternative<
                    CaseIO::PureMixtureConfig>(childVariant)) {
                config.zoneMixtures.insert(zoneName,
                    std::get<CaseIO::PureMixtureConfig>(childVariant));
            }
        }
    }
    return config;
}