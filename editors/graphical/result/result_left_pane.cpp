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

#include "editors/graphical/result/result_left_pane.h"

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "color_bar_widget.h"

ResultLeftPane::ResultLeftPane(const QStringList& timeFolders,
    QString timeFolder, QWidget* parent): m_timeFolders(timeFolders),
    m_timeFolder(timeFolder), QWidget(parent) {
    setProperty("widgetType", "pane");

    // Vertical layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    setLayout(layout);

    // Label
    layout->addSpacing(5);
    QLabel* paneTitle = new QLabel(tr("<b>Simulation State</b>"));
    layout->addWidget(paneTitle, 0, Qt::AlignHCenter);
    layout->addSpacing(5);

    // Create the combo label
    QHBoxLayout* comboLayout = new QHBoxLayout();
    comboLayout->setSpacing(10);
    comboLayout->setAlignment(Qt::AlignCenter);

    // Create and configure the label
    QLabel* timeLabel = new QLabel(tr("Time:"), this);
    comboLayout->addWidget(timeLabel);

    // Create and configure the combo box
    QComboBox* timeComboBox = new QComboBox(this);
    comboLayout->addWidget(timeComboBox);
    layout->addLayout(comboLayout);

    timeComboBox->addItems(m_timeFolders);

    // Create the slider
    QSlider* timeSlider = new QSlider(Qt::Vertical, this);
    timeSlider->setMinimum(0);
    int maxIndex = m_timeFolders.isEmpty() ? 0 : m_timeFolders.size() - 1;
    timeSlider->setMaximum(maxIndex);

    // Configure slider ticks
    timeSlider->setTickPosition(QSlider::TicksRight);
    if (maxIndex > 0) {
        int tickInterval = (maxIndex > 10) ? (maxIndex / 10) : 1;
        timeSlider->setTickInterval(tickInterval);
    }

    // Event-handling
    connect(timeSlider, &QSlider::valueChanged,
            timeComboBox, &QComboBox::setCurrentIndex);
    connect(timeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            timeSlider, &QSlider::setValue);

    // Emit signal when the slider changes
    connect(timeSlider, &QSlider::valueChanged, this, [this](int index) {
        if (index >= 0 && index < m_timeFolders.size()) {
            emit timeChange(m_timeFolders.at(index));
        }
    });

    // Get index of timeFolder in the timeFolders QStringList
    int index = m_timeFolders.indexOf(m_timeFolder);
    if (index != -1) {
        timeSlider->setValue(index);
    } else if (maxIndex > 0) {
        timeSlider->setValue(maxIndex);
    }

    // Center the slider
    QGridLayout* innerGrid = new QGridLayout();
    innerGrid->setContentsMargins(0, 5, 0, 5);
    innerGrid->setHorizontalSpacing(4);

    // Create the Min and Max labels
    QString maxTimeStr =
        m_timeFolders.isEmpty() ? "End" : m_timeFolders.last();
    QString minTimeStr =
        m_timeFolders.isEmpty() ? "Start" : m_timeFolders.first();

    QLabel* topLabel = new QLabel(maxTimeStr, this);
    QLabel* bottomLabel = new QLabel(minTimeStr, this);

    // Use a smaller font for the labels
    QFont smallFont = topLabel->font();
    smallFont.setPointSize(smallFont.pointSize() - 2);
    topLabel->setFont(smallFont);
    bottomLabel->setFont(smallFont);

    // Add widgets to the grid
    innerGrid->addWidget(timeSlider, 0, 0, 3, 1, Qt::AlignRight);

    innerGrid->addWidget(topLabel, 0, 1, Qt::AlignBottom | Qt::AlignLeft);
    innerGrid->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum,
                                       QSizePolicy::Expanding), 1, 1);
    innerGrid->addWidget(bottomLabel, 2, 1, Qt::AlignTop | Qt::AlignLeft);

    // Create an outer horizontal layout
    QHBoxLayout* centerLayout = new QHBoxLayout();
    centerLayout->addStretch();
    centerLayout->addLayout(innerGrid);
    centerLayout->addStretch();
    layout->addLayout(centerLayout);

    layout->addSpacing(10);

    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setProperty("lineStyle", "hline");
    layout->addWidget(line);

    layout->addSpacing(10);

    // Color Map Display
    QLabel* colorMapTitle = new QLabel(tr("<b>Normalized Pressure</b>"));
    layout->addWidget(colorMapTitle, 0, Qt::AlignHCenter);

    // Create color bar
    ColorBarWidget* colorBar = new ColorBarWidget(this);
    colorBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    layout->addWidget(colorBar, 0, Qt::AlignHCenter);
}
