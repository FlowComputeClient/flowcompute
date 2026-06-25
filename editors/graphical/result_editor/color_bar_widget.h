#ifndef COLOR_BAR_WIDGET_H
#define COLOR_BAR_WIDGET_H

#include <QPainter>
#include <QLinearGradient>
#include <QPaintEvent>
#include <QFrame>

class ColorBarWidget : public QWidget {
public:
    explicit ColorBarWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // COLOR_BAR_WIDGET_H
