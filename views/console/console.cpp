#include "console.h"
#include "../../main_window.h"

Console::Console(QMainWindow *qw) {

    parent = reinterpret_cast<MainWindow*>(qw);

    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("border: 2px solid #CCCCCC; color: black ");

    // Configure text display
    QFont logFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    logFont.setPointSize(10);
    setFont(logFont);
    setReadOnly(true);
    setWordWrapMode(QTextOption::NoWrap);
}
