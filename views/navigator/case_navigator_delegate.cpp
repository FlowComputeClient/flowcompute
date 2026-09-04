#include "case_navigator_delegate.h"

#include <QLineEdit>
#include <QPainter>

#include "case_navigator.h"

CaseNavigatorDelegate::CaseNavigatorDelegate(CaseNavigator* navigator,
    QObject* parent): QStyledItemDelegate(parent), m_navigator(navigator) {}

// Repaint items in navigator
void CaseNavigatorDelegate::paint(QPainter *painter,
    const QStyleOptionViewItem &option, const QModelIndex &index) const {

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Save the initial state
    painter->save();

    // Delegate the logic check entirely to the navigator
    if (m_navigator->isItemCut(index)) {
        painter->setOpacity(0.5);
    }

    QStyledItemDelegate::paint(painter, opt, index);

    // Restore the painter back to its original state
    painter->restore();
}

void CaseNavigatorDelegate::setModelData(QWidget *editor,
                                         QAbstractItemModel *model,
                                         const QModelIndex &index) const {
    QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
    if (!lineEdit) {
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }

    QString newName = lineEdit->text();

    // Cast to your specific model to access the node
    NavigatorModel *navModel = qobject_cast<NavigatorModel *>(model);
    if (!navModel) return;

    NodeData* node = navModel->nodeFromIndex(index);
    if (!node) return;

    // Validate the new name before attempting a remote call
    if (newName.isEmpty() || ((newName == node->name) &&
        (newName != "NewFile") && (newName != "NewFolder"))) {
        return;
    }

    // Route the remote creation/rename through the controller
    bool success = m_navigator->renameNode(node, newName);

    // Only commit the data to the local Qt model if the server accepted it
    if (success) {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}