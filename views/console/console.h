#ifndef CONSOLE_H
#define CONSOLE_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QWidget>

class Console : public QPlainTextEdit {
    Q_OBJECT

public:
    Console(QMainWindow* parent = 0);
};

#endif // CONSOLE_H
