#include "case_navigator_delegate.h"

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