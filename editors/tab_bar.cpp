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

#include "editors/tab_bar.h"

void TabBar::tabLayoutChange() {
    QTabBar::tabLayoutChange();

    for (int i = 0; i < count(); ++i) {
        QWidget* btn = tabButton(i, QTabBar::RightSide);
        if (btn) {
            QRect r = btn->geometry();
            r.translate(-8, 0);
            btn->setGeometry(r);
        }
    }
}
