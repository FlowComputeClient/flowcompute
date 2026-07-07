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

#include "editors/text/text_editor.h"

#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>

LineNumberArea::LineNumberArea(TextEditor *editor)
    : QWidget(editor), textEditor(editor) {}

QSize LineNumberArea::sizeHint() const {
    return QSize(textEditor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event) {
    textEditor->lineNumberAreaPaintEvent(event);
}

TextEditor::TextEditor(QWidget *parent) : QPlainTextEdit(parent) {
    lineNumberArea = new LineNumberArea(this);
    connect(document(), &QTextDocument::contentsChange,
            this, &TextEditor::onDocumentEdited);
    connect(this, &TextEditor::blockCountChanged,
            this, &TextEditor::updateLineNumberAreaWidth);
    connect(this, &TextEditor::updateRequest,
            this, &TextEditor::updateLineNumberArea);
    connect(this, &TextEditor::cursorPositionChanged,
            this, &TextEditor::highlightCurrentLine);
    connect(document(), &QTextDocument::modificationChanged,
            this, &TextEditor::dirtyStateChanged);

    m_highlighter = new TreeSitterHighlighter(document());
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void TextEditor::setTextData(const QByteArray &textData) {
    // Update the document
    setPlainText(QString::fromUtf8(textData));

    // Launch the parsing process
    if (textData.size() < 500 * 1024) {
        m_highlighter->parseStringSync(textData);
    } else {
        triggerBackgroundParse(textData);
    }
}

void TextEditor::triggerBackgroundParse(const QByteArray& fileData) {
    QFuture<TSTree*> future = QtConcurrent::run([fileData]() {
        TSParser* bgParser = ts_parser_new();
        ts_parser_set_language(bgParser, tree_sitter_openfoam());

        TSTree* tree = ts_parser_parse_string(
            bgParser, nullptr, fileData.constData(), fileData.size());

        ts_parser_delete(bgParser);
        return tree;
    });

    // Use a QFutureWatcher to catch the AST when it's done
    auto watcher = new QFutureWatcher<TSTree*>(this);
    connect(watcher, &QFutureWatcher<TSTree*>::finished,
            [this, watcher, fileData]() {
        TSTree* tree = watcher->result();
        m_highlighter->setTreeAsync(tree, fileData);
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void TextEditor::onDocumentEdited(int position, int charsRemoved,
                                  int charsAdded) {
    QByteArray currentText = this->document()->toPlainText().toUtf8();
    m_highlighter->parseStringSync(currentText);
}

int TextEditor::lineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void TextEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth() + 5, 0, 0, 0);
}

void TextEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(),
            lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void TextEditor::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(
        QRect(cr.left(), cr.top(),
              lineNumberAreaWidth(), cr.height()));
}

void TextEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), m_gutterBackground);

    // Draw the border at the extreme right edge
    painter.setPen(m_lineNumberBorder);
    painter.drawLine(lineNumberArea->width() - 1, event->rect().top(),
                     lineNumberArea->width() - 1, event->rect().bottom());

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top =
        static_cast<int>(blockBoundingGeometry(block).translated(
            contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            if (blockNumber == textCursor().blockNumber()) {
                painter.setPen(m_lineNumberActive);
            } else {
                painter.setPen(m_lineNumberNormal);
            }
            painter.drawText(0, top, lineNumberArea->width() - 6,
                             fontMetrics().height(), Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// Set the style used in the text editor
void TextEditor::applyTheme(const TextEditorConfig& cfg) {
    // Set background color
    this->setStyleSheet(QString("background-color: %1;").
        arg(cfg.background.name()));

    // Set internal color variables
    m_gutterBackground = cfg.gutterBackground;
    m_currentLineHighlight = cfg.currentLineHighlight;
    m_lineNumberBorder = cfg.lineNumberBorder;
    m_lineNumberNormal = cfg.lineNumberNormal;
    m_lineNumberActive = cfg.lineNumberActive;

    // Update the highlighter
    m_highlighter->setSyntaxConfig(cfg.syntaxConfig);

    // Force redraw
    this->viewport()->update();
    if (lineNumberArea) {
        lineNumberArea->update();
    }
    highlightCurrentLine();
}

void TextEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(m_currentLineHighlight);
        selection.format.setProperty(
            QTextFormat::FullWidthSelection, true);

        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}
