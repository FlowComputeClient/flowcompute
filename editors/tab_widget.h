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

#ifndef EDITORS_TAB_WIDGET_H_
#define EDITORS_TAB_WIDGET_H_

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
    bool promptToSave(int index);

 public slots:
    void closeAllTabs();

 signals:
    void saveTab();

 private:
    QMainWindow* window;
    QTabBar* tabbar;
    QStringList opened;
    QString workspacePath;

    int createEditor(QString name, QString path);

 private slots:
    // void editFile(NodeData*);
    void destroyTab(int index);
    void changeDirtyState(QWidget*, bool);
};

#endif  // EDITORS_TAB_WIDGET_H_
