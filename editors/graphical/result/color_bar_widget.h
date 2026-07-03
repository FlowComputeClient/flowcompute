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

#ifndef EDITORS_GRAPHICAL_RESULT_COLOR_BAR_WIDGET_H_
#define EDITORS_GRAPHICAL_RESULT_COLOR_BAR_WIDGET_H_

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

#endif  // EDITORS_GRAPHICAL_RESULT_COLOR_BAR_WIDGET_H_
