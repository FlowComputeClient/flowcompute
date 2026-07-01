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

#ifndef TREE_SITTER_HIGHLIGHTER_H_
#define TREE_SITTER_HIGHLIGHTER_H_

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
    SyntaxItem keyword = { QColor(0x56, 0x9C, 0xD6), true, false };
    SyntaxItem number = { QColor(0xB5, 0xCE, 0xA8), false, false };
    SyntaxItem string = { QColor(0xCE, 0x91, 0x78), false, false };
    SyntaxItem enumItem = { QColor(0x4E, 0xC9, 0xB0), false, false };
    SyntaxItem comment = { QColor(0x6A, 0x99, 0x55), false, true };
    SyntaxItem punctuation = { QColor(0x80, 0x80, 0x80), false, false };
    SyntaxItem macro = { QColor(0xC5, 0x86, 0xC0), true, false };
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
    void highlightBlock(const QString &text) override;

private:
    void applyFormatting(TSNode node, int blockStart, int blockEnd);

    // Custom deleters for Tree-sitter C structs
    struct TSParserDeleter {
        void operator()(TSParser* p) const { ts_parser_delete(p); }
    };
    struct TSTreeDeleter   {
        void operator()(TSTree* t)   const { ts_tree_delete(t); }
    };

    std::unique_ptr<TSParser, TSParserDeleter> m_parser;
    std::unique_ptr<TSTree, TSTreeDeleter> m_tree;

    // Cache the UTF-8 representation of the document
    QByteArray m_documentUtf8;

    // Formatting rules
    QTextCharFormat m_keywordFormat, m_numberFormat, m_stringFormat,
        m_enumFormat, m_macroFormat, m_commentFormat;
};

#endif // TREE_SITTER_HIGHLIGHTER_H_
