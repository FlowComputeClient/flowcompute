#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include <QFuture>
#include <QtConcurrent>
#include <QPlainTextEdit>
#include <QWidget>

#include "tree_sitter_highlighter.h"

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
};

class TextEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    TextEditor(QWidget *parent = nullptr);
    int lineNumberAreaWidth();
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void setTextData(const QByteArray &textData);

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
};

#endif