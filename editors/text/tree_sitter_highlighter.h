#ifndef TREE_SITTER_HIGHLIGHTER_H
#define TREE_SITTER_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextCharFormat>
#include <memory>
#include <tree_sitter/api.h>

// Forward declare your generated language function
extern "C" const TSLanguage *tree_sitter_openfoam();

struct SyntaxItem {
    QColor color;
    bool bold = false;
    bool italic = false;
};

struct SyntaxConfig {
    SyntaxItem keyword = { QColor("#569CD6"), true, false };
    SyntaxItem number = { QColor("#B5CEA8"), false, false };
    SyntaxItem string = { QColor("#CE9178"), false, false };
    SyntaxItem enumItem = { QColor("#4EC9B0"), false, false };
    SyntaxItem comment = { QColor("#6A9955"), false, true };
    SyntaxItem punctuation = { QColor("#808080"), false, false };
    SyntaxItem macro = { QColor("#C586C0"), true, false };
};

class TreeSitterHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit TreeSitterHighlighter(QTextDocument *parent = nullptr);

    // Call this when the file is first loaded (or after a background parse)
    void setTree(TSTree* newTree, const QByteArray& documentUtf8);
    void parseStringSync(const QByteArray& utf8Text);
    void setTreeAsync(TSTree* newTree, const QByteArray& utf8Text);
    void setSyntaxConfig(const SyntaxConfig& cfg);

protected:
    // Qt's standard override: called for every visible paragraph/line
    void highlightBlock(const QString &text) override;

private:
    void applyFormatting(TSNode node, int blockStart, int blockEnd);

    // Custom deleters for Tree-sitter C structs
    struct TSParserDeleter { void operator()(TSParser* p) const { ts_parser_delete(p); } };
    struct TSTreeDeleter   { void operator()(TSTree* t)   const { ts_tree_delete(t); } };

    std::unique_ptr<TSParser, TSParserDeleter> m_parser;
    std::unique_ptr<TSTree, TSTreeDeleter> m_tree;

    // Cache the UTF-8 representation of the document
    QByteArray m_documentUtf8;

    // Formatting rules
    QTextCharFormat m_keywordFormat, m_numberFormat, m_stringFormat, m_enumFormat, m_macroFormat, m_commentFormat;
};

#endif // TREE_SITTER_HIGHLIGHTER_H
