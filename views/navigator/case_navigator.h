#ifndef CASE_NAVIGATOR_H
#define CASE_NAVIGATOR_H

#include <QDebug>
#include <QMouseEvent>
#include <QStyleFactory>
#include <QTreeView>

#include "navigator_model.h"
#include "node_data.h"

class MainWindow;

class CaseNavigator : public QTreeView {
    Q_OBJECT

public:
    explicit CaseNavigator(QWidget *parent = nullptr);
    void addCase(QString caseName, QStringList caseFiles);
    void expandCase(QString caseName);
    QStringList getCases() const;
    QString getSelectedCase();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    // Menu and actions
    QMenu *m_contextMenu;
    QAction *m_deleteAction;
    void createActions();

private slots:
    // Slot to catch when a user clicks the expand arrow
    void onNodeExpanded(const QModelIndex &index);

    // Displays the context menu
    void showContextMenu(const QPoint &pos);

    // Responds to actions
    void deleteNode();

private:
    NavigatorModel* model;
    QStandardItem* root;
    MainWindow* mainWin;

    // Skeleton function to request children from the WSL server
    void fetchChildren(NodeData* parentNode);
    NodeType checkType(QString name, QString fullPath);
};

#endif // CASE_NAVIGATOR_H