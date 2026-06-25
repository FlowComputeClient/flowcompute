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
    QRect barRect(10, verticalPadding, barWidth, height() - (verticalPadding * 2));

    // Create a vertical gradient (Bottom to Top)
    QLinearGradient gradient(barRect.bottomLeft(), barRect.topLeft());

    // Viridis Color Stops
    gradient.setColorAt(0.00, QColor("#440154")); // Dark Purple
    gradient.setColorAt(0.13, QColor("#472c7a"));
    gradient.setColorAt(0.25, QColor("#3b528b")); // Blue
    gradient.setColorAt(0.38, QColor("#2c718e"));
    gradient.setColorAt(0.50, QColor("#21918c")); // Teal
    gradient.setColorAt(0.63, QColor("#27ad81"));
    gradient.setColorAt(0.75, QColor("#5ec962")); // Green
    gradient.setColorAt(0.88, QColor("#aadc32"));
    gradient.setColorAt(1.00, QColor("#fde725")); // Yellow

    // Draw the background color bar
    painter.fillRect(barRect, gradient);

    // Draw a border around the color bar
    painter.setPen(QPen(Qt::black, 1));
    painter.drawRect(barRect);

    // --- Draw the Labels ---

    // Use the default window text color so it works in both light and dark themes
    painter.setPen(palette().color(QPalette::WindowText));
    QFontMetrics fm = painter.fontMetrics();

    int textX = barRect.right() + 10; // 10 pixels to the right of the bar

    // Align "1.0" with the top of the bar.
    painter.drawText(textX, barRect.top() + (fm.ascent() / 3), "1.0");

    // Align "0.0" with the bottom of the bar
    painter.drawText(textX, barRect.bottom() + (fm.ascent() / 3), "0.0");
}
