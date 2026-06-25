#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include <QFuture>
#include <QtConcurrent>
#include <QPlainTextEdit>
#include <QWidget>

#include "tree_sitter_highlighter.h"

/*
  "text_editor": {
    "background": "#1E1E1E",
    "line_number_border": "#007ACC",
    "line_number_normal": "#858585",
    "line_number_active": "#C6C6C6",
    "current_line_highlight": "#2A2D2E",
    "syntax": {
      "keyword": { "color": "#569CD6", "bold": true, "italic": false },
      "number": { "color": "#B5CEA8", "bold": false, "italic": false },
      "string": { "color": "#CE9178", "bold": false, "italic": false },
      "enum": { "color": "#4EC9B0", "bold": false, "italic": false },
      "comment": { "color": "#6A9955", "bold": false, "italic": true },
      "punctuation": { "color": "#808080", "bold": false, "italic": false },
      "macro": { "color": "#C586C0", "bold": true, "italic": false }
    }
  }
*/

struct TextEditorConfig {
    QColor background = QColor("#1E1E1E");
    QColor gutterBackground = QColor("#1E1E1E");
    QColor lineNumberBorder = QColor("#007ACC");
    QColor lineNumberNormal = QColor("#858585");
    QColor lineNumberActive = QColor("#C6C6C6");
    QColor currentLineHighlight = QColor("#2A2D2E");
    SyntaxConfig syntaxConfig;
};

class TextEditor;

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(TextEditor *editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    TextEditor *textEditor;
    TextEditorConfig m_textEditorConfig;
};

class TextEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    TextEditor(QWidget *parent = nullptr);
    int lineNumberAreaWidth();
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void setTextData(const QByteArray &textData);
    void applyThemeConfig(const TextEditorConfig& config);

signals:
    void dirtyStateChanged(bool isDirty);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &, int);
    void highlightCurrentLine();
    void onDocumentEdited(int position, int charsRemoved, int charsAdded);
    void triggerBackgroundParse(const QByteArray& fileData);

private:
    QWidget *lineNumberArea;
    TreeSitterHighlighter *m_highlighter;
    QColor m_gutterBackground = QColor("#1E1E1E");
    QColor m_currentLineHighlight = QColor("#2A2D2E");
    QColor m_lineNumberBorder = QColor("#007ACC");
    QColor m_lineNumberNormal = QColor("#858585");
    QColor m_lineNumberActive = QColor("#C6C6C6");
};

#endif