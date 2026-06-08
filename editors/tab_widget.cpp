#include "tab_widget.h"
// #include "main_window.h"

#include <QMessageBox>
#include <QStringBuilder>

// Custom tab bar moves the close button
class TabBar : public QTabBar {
public:
    using QTabBar::QTabBar;

protected:
    void tabLayoutChange() override {
        QTabBar::tabLayoutChange();

        for (int i = 0; i < count(); ++i) {
            QWidget* btn = tabButton(i, QTabBar::RightSide);
            if (btn) {
                QRect r = btn->geometry();
                r.translate(-8, 0);
                btn->setGeometry(r);
            }
        }
    }
};

TabWidget::TabWidget(QMainWindow *parent) : QTabWidget(parent) {

    // Access parent
    window = parent;

    // Set the custom tab bar
    setTabBar(new TabBar());

    // Configure behavior
    setMovable(true);
    setTabsClosable(true);
    connect(this, &QTabWidget::tabCloseRequested, this, &TabWidget::destroyTab);

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

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Unsaved Changes");
    msgBox.setText(QString("The file '%1' has been modified.\nDo you want to save your changes?")
                       .arg(fileName.remove('*').trimmed()));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    msgBox.setStyleSheet("QLabel { color: black; } QPushButton { color: black; }");

    int resBtn = msgBox.exec();
    if (resBtn == QMessageBox::Save) {
        emit saveTab();
        return true;
    } else if (resBtn == QMessageBox::Cancel) {
        return false; // Tell the caller to abort!
    }

    // If they clicked Discard, it's safe to proceed.
    return true;
}

void TabWidget::destroyTab(int index) {
    // If the user clicked Cancel, promptToSave returns false, so we abort destruction.
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
    if(dirty && (name[0] != '*')) {
        setTabText(index, name.prepend("* "));
        return;
    }

    // Remove asterisk to mark editor as not dirty
    if(!dirty && (name[0] == '*')) {
        setTabText(index, name.remove(0, 2));
        return;
    }
}

/*
void TabWidget::editFile(NodeData* node) {

    QString path = workspacePath + "/" + node->file_path;
    if(opened.contains(path)) {
        setCurrentIndex(opened.indexOf(path));
    }
    else {
        opened.append(path);
        setCurrentIndex(createEditor(node->file_name, path));
    }
}

void TabWidget::setWorkspace(QString path) {

    // Read list of files to open
    workspacePath = path;
    QString line;
    QString filename = workspacePath + "/.ngenie-workspace";
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        while (!stream.atEnd() && !line.startsWith("open")) {
            line = stream.readLine();
        }
        file.close();
    }
    if(line.startsWith("open")) {
        opened = line.split("=").last().split(",");
    }

    for(int i=0; i<opened.count(); i++) {
        int pos = opened[i].lastIndexOf("/");
        QString name = opened[i].mid(pos+1);
        setCurrentIndex(createEditor(name, opened[i]));
    }
}

int TabWidget::createEditor(QString name, QString path) {

    CodeEditor *editor = new CodeEditor(this, path);
    connect(editor, SIGNAL(changeDirtyState(QWidget*, bool)), this, SLOT(changeDirtyState(QWidget*, bool)));
    int index = addTab(editor, name);
    editor->setFocus();
    return index;
}

void TabWidget::updateWorkspace() {

    QString newFile = "";
    QString filename = workspacePath + "/.ngenie-workspace";
    QFile file(filename);
    if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if(!line.startsWith("open")) {
                newFile = newFile % line;
            }
        }
        newFile = newFile.trimmed();
        if(!opened.empty()) {
            if(newFile != "")
                newFile = newFile % "\n";
            newFile = newFile % "open=" % opened.join(",");
        }
        file.resize(0);
        stream << newFile;
        file.close();
    }
    else {
        qDebug() << "Couldn't open workspace file";
    }
}

CodeEditor* TabWidget::getCurrentEditor() {
    return (CodeEditor*)currentWidget();
}
*/