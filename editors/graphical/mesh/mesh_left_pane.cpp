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

#include "editors/graphical/mesh/mesh_left_pane.h"

#include <QComboBox>
#include <QVBoxLayout>

#include <vector>

#include "geometry/graphic_data.h"
#include "editors/graphical/table_delegate.h"

MeshLeftPane::MeshLeftPane(QStringList fields,
       const QHash<QString, FlowCompute::FieldDef>& fieldData,
       const std::vector<FlowCompute::BoundaryConditionDef>& boundaryConditions,
       QWidget* parent):
    QWidget(parent), m_fields(fields), m_fieldData(fieldData),
    m_boundaryConditions(boundaryConditions) {
    // Vertical layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    setLayout(layout);
    setProperty("widgetType", "pane");

    // Label
    layout->addSpacing(5);
    QLabel* paneTitle = new QLabel(tr("<b>Mesh Utilities</b>"));
    layout->addWidget(paneTitle, 0, Qt::AlignHCenter);
    layout->addSpacing(5);

    // Set dimensions
    int buttonWidth = 150;
    int spinBoxWidth = 80;

    // Check mesh
    m_checkButton = new QPushButton(tr("Check Mesh"));
    m_checkButton->setFixedWidth(buttonWidth);
    layout->addWidget(m_checkButton, 0, Qt::AlignHCenter);
    connect(m_checkButton, &QPushButton::clicked,
            this, &MeshLeftPane::onCheckButtonClicked);

    // Horizontal line
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setProperty("lineStyle", "hline");
    layout->addWidget(line);

    // Renumber mesh
    m_renumberButton = new QPushButton(tr("Renumber Mesh"));
    m_renumberButton->setFixedWidth(buttonWidth);
    layout->addWidget(m_renumberButton, 0, Qt::AlignHCenter);
    connect(m_renumberButton, &QPushButton::clicked,
            this, &MeshLeftPane::onRenumberButtonClicked);

    // Horizontal line
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setProperty("lineStyle", "hline");
    layout->addWidget(line);

    // Create horizontal layout
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->setSpacing(5);
    hLayout->setContentsMargins(10.0, 0.0, 10.0, 0.0);

    // Label for feature angle
    QLabel* angleLabel = new QLabel(tr("Feature Angle:"));
    hLayout->addWidget(angleLabel);

    // Spin box for feature angle
    m_angleSpin = new QDoubleSpinBox();
    m_angleSpin->setValue(45);
    m_angleSpin->setSingleStep(5.0);
    m_angleSpin->setRange(0.0, 180.0);
    m_angleSpin->setFixedWidth(spinBoxWidth);
    hLayout->addWidget(m_angleSpin);
    layout->addLayout(hLayout);

    // Button to launch autoPatch
    m_patchButton = new QPushButton(tr("Generate Patches"));
    m_patchButton->setFixedWidth(buttonWidth);
    layout->addWidget(m_patchButton, 0, Qt::AlignHCenter);
    connect(m_patchButton, &QPushButton::clicked,
            this, &MeshLeftPane::onPatchButtonClicked);

    // Horizontal line
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setProperty("lineStyle", "hline");
    layout->addWidget(line);

    // Set table title
    QLabel* tableTitle = new QLabel(tr("<b>Mesh Patches</b>"));
    layout->addWidget(tableTitle, 0, Qt::AlignHCenter);

    // Patch table
    m_patchTable = new QTableWidget(this);
    m_patchTable->setColumnCount(3);
    m_patchTable->horizontalHeader()->setVisible(false);
    m_patchTable->setColumnWidth(0, 30);
    m_patchTable->horizontalHeader()
        ->setSectionResizeMode(1, QHeaderView::Stretch);
    m_patchTable->verticalHeader()->setVisible(false);
    m_patchTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_patchTable->setTabKeyNavigation(true);
    m_patchTable->setItemDelegateForColumn(1, new TableDelegate(m_patchTable));
    layout->addWidget(m_patchTable, 0, Qt::AlignHCenter);

    // Button to apply changes
    m_applyButton = new QPushButton(tr("Apply Changes"));
    m_applyButton->setFixedWidth(buttonWidth);
    layout->addWidget(m_applyButton, 0, Qt::AlignHCenter);
    connect(m_applyButton, &QPushButton::clicked,
            this, &MeshLeftPane::onApplyButtonClicked);
    m_applyButton->setEnabled(false);

    // Set event handling for the patch name
    connect(m_patchTable, &QTableWidget::itemChanged, this,
            [this](QTableWidgetItem* item) {
        int row = item->row();

        // Handle name change
        if (item->column() == 1) {
            QString text = item->text();
            if (m_boundaryPatches[row].newName != text) {
                m_boundaryPatches[row].newName = text;
                m_boundaryPatches[row].nameChanged = true;
                m_applyButton->setEnabled(true);
            }
        }
    });

    layout->addStretch();
}

void MeshLeftPane::setPatches(const std::vector<FlowCompute::MeshPatch>&
                                  patches) {
    m_boundaryPatches = patches;

    // Configure the table
    m_patchTable->setRowCount(0);
    m_patchTable->setRowCount(static_cast<int>(patches.size()));
    m_patchTable->blockSignals(true);

    // Add rows to the table
    for (int i = 0; i < static_cast<int>(patches.size()); i++) {
        // Get the patch color
        QColor color(
            static_cast<int>(patchColors[i][0] * 255.0f),
            static_cast<int>(patchColors[i][1] * 255.0f),
            static_cast<int>(patchColors[i][2] * 255.0f),
            static_cast<int>(patchColors[i][3] * 255.0f));

        // Column 1: Patch color
        QTableWidgetItem *colorItem = new QTableWidgetItem();
        colorItem->setFlags(Qt::ItemIsEnabled);
        colorItem->setBackground(color);
        m_patchTable->setItem(i, 0, colorItem);

        // Column 2: Patch name
        QTableWidgetItem *nameItem = new QTableWidgetItem(patches[i].name);
        nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
                           Qt::ItemIsEditable);
        m_patchTable->setItem(i, 1, nameItem);

        // Column 3: Patch type
        QComboBox* patchBox = new QComboBox(m_patchTable);
        patchBox->addItems(FlowCompute::patchTypes);
        if (FlowCompute::patchTypes.contains(patches[i].type)) {
            patchBox->setCurrentText(patches[i].type);
        }
        m_patchTable->setCellWidget(i, 2, patchBox);

        // Set event-handling for the patch type
        connect(patchBox, &QComboBox::currentTextChanged, this,
                [this, i](const QString& text) {
            if (m_boundaryPatches[i].type != text) {
                m_boundaryPatches[i].type = text;
                m_boundaryPatches[i].typeChanged = true;
                m_applyButton->setEnabled(true);
            }
        });
    }

    m_patchTable->blockSignals(false);

    // Set the table height
    int height = m_patchTable->frameWidth() * 2;
    for (int row = 0; row < m_patchTable->rowCount(); ++row) {
        height += m_patchTable->rowHeight(row);
    }
    m_patchTable->setFixedHeight(height);

    update();
}

// Respond when the apply button is pressed
void MeshLeftPane::onApplyButtonClicked() {
    emit patchApplyRequested(m_boundaryPatches);
}

// Respond when the check button is pressed
void MeshLeftPane::onCheckButtonClicked() {
    emit meshCheckRequested();
}

// Respond when the renumber button is pressed
void MeshLeftPane::onRenumberButtonClicked() {
    emit meshRenumberRequested();
}

// Respond when the patch button is pressed
void MeshLeftPane::onPatchButtonClicked() {
    double angle = m_angleSpin->value();
    emit meshPatchRequested(angle);
}
