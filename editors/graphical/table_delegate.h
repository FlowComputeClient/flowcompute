#ifndef TABLE_DELEGATE_H
#define TABLE_DELEGATE_H

#include <QStyledItemDelegate>

class TableDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    bool eventFilter(QObject *editor, QEvent *event) override;
};

#endif // TABLE_DELEGATE_H
