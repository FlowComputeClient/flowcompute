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

#include "editors/graphical/surface/surface_left_pane.h"

#include <QStyleOption>
#include <QPainter>

#include <string>
#include <vector>

#include "geometry/graphic_data.h"
#include "editors/graphical/table_delegate.h"

SurfaceLeftPane::SurfaceLeftPane(QWidget* parent): QWidget(parent) {
    // Vertical layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    setLayout(layout);
    setProperty("widgetType", "pane");

    // Set dimensions
    int buttonWidth = 130;
    int spinBoxWidth = 80;

    layout->addSpacing(10);

    // Label
    layout->addSpacing(5);
    QLabel* paneTitle = new QLabel(tr("<b>Surface Utilities</b>"));
    layout->addWidget(paneTitle, 0, Qt::AlignHCenter);
    layout->addSpacing(5);

    // Button to check model
    m_checkButton = new QPushButton(tr("Check Surface"));
    m_checkButton->setFixedWidth(buttonWidth);
    layout->addWidget(m_checkButton, 0, Qt::AlignHCenter);
    connect(m_checkButton, &QPushButton::clicked,
            this, &SurfaceLeftPane::onCheckButtonClicked);

    layout->addSpacing(10);

    // Horizontal line
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setProperty("lineStyle", "hline");
    layout->addWidget(line);

    layout->addSpacing(10);

    // Label that displays surface size
    m_boundsLabel = new QLabel();
    layout->addWidget(m_boundsLabel, 0, Qt::AlignHCenter);

    // Create horizontal layout
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->setSpacing(5);

    // Label for scale factor
    QLabel* scaleLabel = new QLabel(tr("Scale Factor:"));
    hLayout->addWidget(scaleLabel);

    // Spin box for scale factor
    m_scaleSpin = new QDoubleSpinBox();
    m_scaleSpin->setValue(1.0);
    m_scaleSpin->setSingleStep(0.1);
    m_scaleSpin->setDecimals(5);
    m_scaleSpin->setRange(0.00001, 100000.0);
    m_scaleSpin->setFixedWidth(spinBoxWidth);
    hLayout->addWidget(m_scaleSpin);
    layout->addLayout(hLayout);

    // Button to launch surfaceTransformPoints
    m_scaleButton = new QPushButton(tr("Scale Surface"));
    m_scaleButton->setFixedWidth(buttonWidth);
    layout->addWidget(m_scaleButton, 0, Qt::AlignHCenter);
    connect(m_scaleButton, &QPushButton::clicked,
            this, &SurfaceLeftPane::onScaleButtonClicked);

    layout->addSpacing(10);

    // Horizontal line
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setProperty("lineStyle", "hline");
    layout->addWidget(line);

    layout->addSpacing(10);

    // Create horizontal layout
    hLayout = new QHBoxLayout();
    hLayout->setSpacing(5);

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

    // Button to launch surfaceAutoPatch
    m_patchButton = new QPushButton(tr("Generate Patches"));
    m_patchButton->setFixedWidth(buttonWidth);
    layout->addWidget(m_patchButton, 0, Qt::AlignHCenter);
    connect(m_patchButton, &QPushButton::clicked,
            this, &SurfaceLeftPane::onPatchButtonClicked);

    layout->addSpacing(10);

    // Horizontal line
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setProperty("lineStyle", "hline");
    layout->addWidget(line);

    layout->addSpacing(10);

    // Set table title
    QLabel* tableTitle = new QLabel(tr("<b>Surface Patches</b>"));
    layout->addWidget(tableTitle, 0, Qt::AlignHCenter);

    layout->addSpacing(10);

    // Patch table
    m_patchTable = new QTableWidget(this);
    m_patchTable->setColumnCount(2);
    m_patchTable->horizontalHeader()->setVisible(false);
    m_patchTable->setColumnWidth(0, 30);
    m_patchTable->horizontalHeader()->setSectionResizeMode(1,
        QHeaderView::Stretch);
    m_patchTable->verticalHeader()->setVisible(false);
    m_patchTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_patchTable->setTabKeyNavigation(true);
    m_patchTable->setItemDelegateForColumn(1, new TableDelegate(m_patchTable));
    m_patchTable->setStyleSheet("QTableView::item { padding-left: 10px; }");
    layout->addWidget(m_patchTable, 0, Qt::AlignHCenter);
    m_patchTable->setShowGrid(false);

    // Emit signal when a patch name changes
    connect(m_patchTable, &QTableWidget::itemChanged, this,
        [this](QTableWidgetItem* item) {
        if (item->column() == 1) {
            emit dirtyStateChanged(true);
        }
    });

    layout->addStretch();
}

void SurfaceLeftPane::setBounds(std::array<float, 3> bounds) {
    m_bounds = bounds;
    QString boundsStr = QString("<b>( %1, %2, %3 )</b>")
        .arg(m_bounds[0], 0, 'g', 5)
        .arg(m_bounds[1], 0, 'g', 5)
        .arg(m_bounds[2], 0, 'g', 5);
    m_boundsLabel->setText(boundsStr);
}

void SurfaceLeftPane::changeBounds(double scaleFactor) {
    m_bounds[0] *= scaleFactor;
    m_bounds[1] *= scaleFactor;
    m_bounds[2] *= scaleFactor;
    QString boundsStr = QString("<b>( %1, %2, %3 )</b>")
        .arg(m_bounds[0], 0, 'g', 5)
        .arg(m_bounds[1], 0, 'g', 5)
        .arg(m_bounds[2], 0, 'g', 5);
    m_boundsLabel->setText(boundsStr);
}

void SurfaceLeftPane::setPatchNames(
    const std::vector<std::string>& patchNames) {
    // Configure the table
    m_patchTable->setRowCount(0);
    m_patchTable->setRowCount(static_cast<int>(patchNames.size()));

    // Add rows to the table
    for (int i = 0; i < static_cast<int>(patchNames.size()); i++) {
        // Get the patch color
        QColor color(
            static_cast<int>(patchColors[i][0] * 255.0f),
            static_cast<int>(patchColors[i][1] * 255.0f),
            static_cast<int>(patchColors[i][2] * 255.0f),
            static_cast<int>(patchColors[i][3] * 255.0f));

        // Create item for patch color
        QTableWidgetItem *colorItem = new QTableWidgetItem();
        colorItem->setFlags(Qt::ItemIsEnabled);
        colorItem->setBackground(color);
        m_patchTable->setItem(i, 0, colorItem);

        // Create item for patch name
        QTableWidgetItem *nameItem =
            new QTableWidgetItem(QString::fromStdString(patchNames[i]));
        nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
                           Qt::ItemIsEditable);
        m_patchTable->setItem(i, 1, nameItem);
    }

    // Set the table height
    int height = m_patchTable->frameWidth() * 2;
    for (int row = 0; row < m_patchTable->rowCount(); ++row) {
        height += m_patchTable->rowHeight(row);
    }
    m_patchTable->setFixedHeight(height);

    update();
}

std::vector<std::string> SurfaceLeftPane::getPatchNames() const {
    // Reserve memory
    std::vector<std::string> patchNames;
    int rowCount = m_patchTable->rowCount();
    patchNames.reserve(rowCount);

    // Access patch names in table
    for (int row = 0; row < rowCount; ++row) {
        QTableWidgetItem* item = m_patchTable->item(row, 1);
        if (item) {
            patchNames.push_back(item->text().toStdString());
        } else {
            patchNames.push_back("");
        }
    }
    return patchNames;
}

// Respond when the check button is pressed
void SurfaceLeftPane::onCheckButtonClicked() {
    emit surfaceCheckRequested();
}

// Respond when the check button is pressed
void SurfaceLeftPane::onScaleButtonClicked() {
    double scale = m_scaleSpin->value();
    emit surfaceScaleRequested(scale);
}

// Respond when the check button is pressed
void SurfaceLeftPane::onPatchButtonClicked() {
    double angle = m_angleSpin->value();
    emit surfacePatchRequested(angle);
}

void SurfaceLeftPane::paintEvent(QPaintEvent *event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}
