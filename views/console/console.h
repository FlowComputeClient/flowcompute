#ifndef CONSOLE_H
#define CONSOLE_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(MainWindow);

class Console : public QPlainTextEdit {
    Q_OBJECT

public:
    Console(QMainWindow* parent = 0);

private:
    MainWindow* parent;
};

#endif // CONSOLE_H
