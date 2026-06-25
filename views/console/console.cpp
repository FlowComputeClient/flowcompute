#include "console.h"

Console::Console(QMainWindow *qw) {

    // Configure text display
    QFont logFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    logFont.setPointSize(11);
    setFont(logFont);
    setReadOnly(true);
    setWordWrapMode(QTextOption::NoWrap);
}
