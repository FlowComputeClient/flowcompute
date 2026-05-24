#include "text_editor.h"

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

    // Set highlighter
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
    // Launch a background thread
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
    connect(watcher, &QFutureWatcher<TSTree*>::finished, [this, watcher, fileData]() {
        TSTree* tree = watcher->result();
        m_highlighter->setTreeAsync(tree, fileData);
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void TextEditor::onDocumentEdited(int position, int charsRemoved, int charsAdded) {
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

    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

    return space;
}

void TextEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void TextEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(),
                               lineNumberArea->width(),
                               rect.height());

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
    painter.fillRect(event->rect(), QColor(240, 240, 240));
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();

    int top = (int) blockBoundingGeometry(block)
                  .translated(contentOffset()).top();

    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);

            painter.setPen(Qt::black);

            painter.drawText(0, top,
                             lineNumberArea->width() - 5,
                             fontMetrics().height(),
                             Qt::AlignRight,
                             number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void TextEditor::highlightCurrentLine() {

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {

        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(220, 220, 255);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(
            QTextFormat::FullWidthSelection, true);

        selection.cursor = textCursor();
        selection.cursor.clearSelection();

        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}