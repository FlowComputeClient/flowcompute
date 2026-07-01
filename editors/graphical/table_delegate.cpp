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

#include "table_delegate.h"

#include <QEvent>
#include <QKeyEvent>
#include <QTableWidget>

bool TableDelegate::eventFilter(QObject *editor, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Tab) {

            // Access the widget
            QWidget *widgetEditor = qobject_cast<QWidget*>(editor);
            if (!widgetEditor) {
                return false;
            }

            // Commit data and close editor
            emit commitData(widgetEditor);
            emit closeEditor(widgetEditor, QAbstractItemDelegate::NoHint);

            // Update table
            QTableWidget *table = qobject_cast<QTableWidget*>(parent());
            if (table) {
                int nextRow = table->currentRow() + 1;
                if (nextRow < table->rowCount()) {
                    table->setCurrentCell(nextRow, table->currentColumn());
                    table->editItem(table->currentItem());
                }
            }
            return true;
        }
    }
    return QStyledItemDelegate::eventFilter(editor, event);
}
