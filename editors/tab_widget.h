#ifndef TAB_WIDGET_H
#define TAB_WIDGET_H

#include <QDebug>
#include <QFile>
#include <QMainWindow>
#include <QMap>
#include <QModelIndex>
#include <QString>
#include <QTabBar>
#include <QTabWidget>
#include <QTextStream>

QT_FORWARD_DECLARE_CLASS(MainWindow);

class TabWidget : public QTabWidget {
    Q_OBJECT

public:
    TabWidget(QMainWindow* parent = 0);
    ~TabWidget();
    void setWorkspace(QString);
    void updateWorkspace();

private:
    QMainWindow* window;
    QTabBar* tabbar;
    QStringList opened;
    QString workspacePath;

    int createEditor(QString name, QString path);

private slots:
    // void editFile(NodeData*);
    void destroyTab(int);
    void changeDirtyState(QWidget*, bool);
};

#endif // TAB_WIDGET_H

