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

#include "editors/tab_widget.h"

#include <QMainWindow>
#include <QMessageBox>
#include <QStringBuilder>

#include "editors/tab_bar.h"

TabWidget::TabWidget(QMainWindow *parent) : QTabWidget(parent) {
    // Access parent
    window = parent;

    // Set the custom tab bar
    setTabBar(new TabBar());

    // Configure behavior
    setMovable(true);
    setTabsClosable(true);
    connect(this, &QTabWidget::tabCloseRequested, this, [this](int index) {
        destroyTab(index, false);
    });
}

TabWidget::~TabWidget() {}

void TabWidget::tabInserted(int index) {
    // Always call the base class implementation first
    QTabWidget::tabInserted(index);

    // Retrieve the name of the newly added tab
    QString name = tabText(index);
}

void TabWidget::closeAllTabs() {
    for (int i = this->count() - 1; i >= 0; --i) {
        destroyTab(i);
    }
}

bool TabWidget::promptToSave(int index) {
    // Make the tab at the given index current
    this->setCurrentIndex(index);

    // Access the filename
    QString fileName = tabText(index);

    // If it's not dirty, it's safe to proceed.
    if (!fileName.endsWith('*')) {
        return true;
    }

    // Display message
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Unsaved Changes");
    msgBox.setText(QString("The file '%1' has been modified.\n"
                           "Do you want to save your changes?")
                       .arg(fileName.remove('*').trimmed()));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard |
                              QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);

    int resBtn = msgBox.exec();
    if (resBtn == QMessageBox::Save) {
        emit saveTab();
        return true;
    } else if (resBtn == QMessageBox::Cancel) {
        return false;
    }

    // If they clicked Discard, it's safe to proceed.
    return true;
}

void TabWidget::destroyTab(int index, bool force) {
    // Skip the prompt if forced
    if (!force && !promptToSave(index)) {
        return;
    }

    // Get the unique ID
    QString uniqueId = this->tabBar()->tabData(index).toString();

    // Notify MainWindow
    emit tabClosedSuccessfully(uniqueId);

    // Access the tab's editor
    QWidget* editorWidget = this->widget(index);

    // Remove the tab
    removeTab(index);

    // Delete the widget
    if (editorWidget) {
        editorWidget->deleteLater();
    }
}

void TabWidget::changeDirtyState(QWidget* editor, bool dirty) {
    // Get name of tab
    int index = indexOf(editor);
    QString name = tabText(index);

    // Add asterisk to mark editor as dirty
    if (dirty && (name[0] != '*')) {
        setTabText(index, name.prepend("* "));
        return;
    }

    // Remove asterisk to mark editor as not dirty
    if (!dirty && (name[0] == '*')) {
        setTabText(index, name.remove(0, 2));
        return;
    }
}
