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
