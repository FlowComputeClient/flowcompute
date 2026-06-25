#ifndef CASE_NAVIGATOR_H
#define CASE_NAVIGATOR_H

#include <QDebug>
#include <QMouseEvent>
#include <QStyleFactory>
#include <QTreeView>

#include "navigator_model.h"
#include "node_data.h"

class MainWindow;

enum class EditorType : int {
    TEXT = 0,
    SURFACE,
    MESH,
    RESULT,
    COUNT
};

class CaseNavigator : public QTreeView {
    Q_OBJECT

public:
    explicit CaseNavigator(QWidget *parent = nullptr);
    void addCase(QString caseName, QStringList caseFiles);
    void expandCase(QString caseName);
    QStringList getCases() const;
    QString getSelectedCase();
    void updatePath(QString path, QStringList children);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

signals:
    void createEditor(EditorType type, QString& fileName, const QString& fullPath);

private:
    // Menu and actions
    QMenu *m_contextMenu;
    QAction *m_deleteAction, *m_viewMeshAction, *m_viewResultAction;
    void createActions();

private slots:
    // Slot to catch when a user clicks the expand arrow
    void onNodeExpanded(const QModelIndex &index);

    // Displays the context menu
    void showContextMenu(const QPoint &pos);

    // Responds to actions
    void deleteNode();
    void viewMesh();
    void viewResult();

private:
    NavigatorModel* model;
    QStandardItem* root;
    MainWindow* mainWin;

    // Skeleton function to request children from the WSL server
    void fetchChildren(NodeData* parentNode);
    NodeType checkType(QString name, QString fullPath);
};

#endif // CASE_NAVIGATOR_H