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

#ifndef TEXT_EDITOR_H_
#define TEXT_EDITOR_H_

#include <QFuture>
#include <QtConcurrent>
#include <QPlainTextEdit>
#include <QWidget>

#include "tree_sitter_highlighter.h"

struct TextEditorConfig {
    QColor background = QColor(0x1E, 0x1E, 0x1E);
    QColor gutterBackground = QColor(0x1E, 0x1E, 0x1E);
    QColor lineNumberBorder = QColor(0x00, 0x7A, 0xCC);
    QColor lineNumberNormal = QColor(0x85, 0x85, 0x85);
    QColor lineNumberActive = QColor(0xC6, 0xC6, 0xC6);
    QColor currentLineHighlight = QColor(0x2A, 0x2D, 0x2E);
    SyntaxConfig syntaxConfig;
};

class TextEditor;

class LineNumberArea : public QWidget {

 public:
    LineNumberArea(TextEditor *editor);

    QSize sizeHint() const override;

 protected:
    void paintEvent(QPaintEvent *event) override;

 private:
    TextEditor *textEditor;
    TextEditorConfig m_textEditorConfig;
};

class TextEditor : public QPlainTextEdit {
    Q_OBJECT

 public:
    TextEditor(QWidget *parent = nullptr);
    int lineNumberAreaWidth();
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void setTextData(const QByteArray &textData);
    void applyTheme(const TextEditorConfig& config);

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
    QColor m_gutterBackground = QColor(0x1E, 0x1E, 0x1E);
    QColor m_currentLineHighlight = QColor(0x2A, 0x2D, 0x2E);
    QColor m_lineNumberBorder = QColor(0x00, 0x7A, 0xCC);
    QColor m_lineNumberNormal = QColor(0x85, 0x85, 0x85);
    QColor m_lineNumberActive = QColor(0xC6, 0xC6, 0xC6);
};

#endif // TEXT_EDITOR_H_