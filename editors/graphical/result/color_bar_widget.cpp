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

#include "color_bar_widget.h"

ColorBarWidget::ColorBarWidget(QWidget *parent) : QWidget(parent) {
    setMinimumSize(80, 200);
}

void ColorBarWidget::paintEvent(QPaintEvent *event)  {

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Define padding and geometry
    int verticalPadding = 15;
    int barWidth = 30;

    // Create the rectangle for the gradient
    QRect barRect(10, verticalPadding, barWidth, height() -
                    (verticalPadding * 2));

    // Create a vertical gradient (Bottom to Top)
    QLinearGradient gradient(barRect.bottomLeft(), barRect.topLeft());

    // Viridis Color Stops
    gradient.setColorAt(0.00, QColor(0x44, 0x01, 0x54));
    gradient.setColorAt(0.13, QColor(0x47, 0x2c, 0x7a));
    gradient.setColorAt(0.25, QColor(0x3b, 0x52, 0x8b));
    gradient.setColorAt(0.38, QColor(0x2c, 0x71, 0x8e));
    gradient.setColorAt(0.50, QColor(0x21, 0x91, 0x8c));
    gradient.setColorAt(0.63, QColor(0x27, 0xad, 0x81));
    gradient.setColorAt(0.75, QColor(0x5e, 0xc9, 0x62));
    gradient.setColorAt(0.88, QColor(0xaa, 0xdc, 0x32));
    gradient.setColorAt(1.00, QColor(0xfd, 0xe7, 0x25));

    // Draw the background color bar
    painter.fillRect(barRect, gradient);

    // Draw a border around the color bar
    painter.setPen(QPen(Qt::black, 1));
    painter.drawRect(barRect);

    // Use the default window text color
    painter.setPen(palette().color(QPalette::WindowText));
    QFontMetrics fm = painter.fontMetrics();

    int textX = barRect.right() + 10;

    // Align "1.0" with the top
    painter.drawText(textX, barRect.top() + (fm.ascent() / 3), "1.0");

    // Align "0.0" with the bottom
    painter.drawText(textX, barRect.bottom() + (fm.ascent() / 3), "0.0");
}
