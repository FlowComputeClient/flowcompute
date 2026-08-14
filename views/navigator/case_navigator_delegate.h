#ifndef VIEWS_NAVIGATOR_CASE_NAVIGATOR_DELEGATE_H_
#define VIEWS_NAVIGATOR_CASE_NAVIGATOR_DELEGATE_H_

#include <QStyledItemDelegate>
#include <QObject>

class CaseNavigator;

class CaseNavigatorDelegate : public QStyledItemDelegate {
    Q_OBJECT

 public:
    explicit CaseNavigatorDelegate(CaseNavigator* navigator,
                                   QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

 private:
    CaseNavigator* m_navigator;
};

#endif  // VIEWS_NAVIGATOR_CASE_NAVIGATOR_DELEGATE_H_