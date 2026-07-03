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

#include "tab_widget.h"

#include <QMessageBox>
#include <QStringBuilder>

#include "tab_bar.h"

TabWidget::TabWidget(QMainWindow *parent) : QTabWidget(parent) {
    // Access parent
    window = parent;

    // Set the custom tab bar
    setTabBar(new TabBar());

    // Configure behavior
    setMovable(true);
    setTabsClosable(true);
    connect(this, &QTabWidget::tabCloseRequested, this, &TabWidget::destroyTab);

    /*
    // Set style
    setStyleSheet(
        "QTabBar {"
        "    background: #E6E6E6;"
        "}"
        "QTabBar::tab {"
        "    background: #DCDCDC;"
        "    font: bold;"
        "    color: #404040;"
        "    padding: 6px 12px;"
        "    margin-right: 2px;"
        "    border: 1px solid #C0C0C0;"
        "    border-top-left-radius: 8px;"
        "    border-top-right-radius: 8px;"
        "    border-bottom: none;"
        "}"
        "QTabBar::tab:selected {"
        "    color: #000000;"
        "    background: #FFFFFF;"
        "}"
        "QTextEdit, QPlainTextEdit {"
        "    color: black;"
        "    background-color: white;"
        "}"
        "QTabWidget::pane {"
        "    border: 1px solid #C0C0C0;"
        "}"
        "QMessageBox { color: #000000; }"
        );
    */
}

TabWidget::~TabWidget() {}

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

void TabWidget::destroyTab(int index) {
    if (!promptToSave(index)) {
        return;
    }

    // Access the tab's editor
    QWidget* editorWidget = this->widget(index);

    // Remove the tab from the visual UI
    this->removeTab(index);

    // Safely delete the text editor from memory
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
