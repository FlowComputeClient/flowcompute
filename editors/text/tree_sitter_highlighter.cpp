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

#include "tree_sitter_highlighter.h"

TreeSitterHighlighter::TreeSitterHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {

    // Initialize this highlighter's exclusive parser
    m_parser.reset(ts_parser_new());
    ts_parser_set_language(m_parser.get(), tree_sitter_openfoam());

    // Setup some basic Qt text formats
    m_keywordFormat.setForeground(QColor(0x56, 0x9C, 0xD6));
    m_keywordFormat.setFontWeight(QFont::Bold);
    m_keywordFormat.setFontItalic(false);

    m_numberFormat.setForeground(QColor(0xB5, 0xCE, 0xA8));
    m_numberFormat.setFontItalic(false);

    m_stringFormat.setForeground(QColor(0xCE, 0x91, 0x78));
    m_stringFormat.setFontItalic(false);

    m_enumFormat.setForeground(QColor(0x4E, 0xC9, 0xB0));
    m_enumFormat.setFontItalic(false);

    m_macroFormat.setForeground(QColor(0xC5, 0x86, 0xC0));
    m_macroFormat.setFontWeight(QFont::Bold);
    m_macroFormat.setFontItalic(false);

    m_commentFormat.setForeground(QColor(0xE5, 0xC0, 0x7B));
    m_commentFormat.setFontItalic(true);
}

void TreeSitterHighlighter::setSyntaxConfig(const SyntaxConfig& cfg) {

    // Set the keyword format
    m_keywordFormat.setForeground(cfg.keyword.color);
    if (cfg.keyword.bold) {
        m_keywordFormat.setFontWeight(QFont::Bold);
    } else {
        m_keywordFormat.setFontWeight(QFont::Normal);
    }
    m_keywordFormat.setFontItalic(cfg.keyword.italic);

    // Set the number format
    m_numberFormat.setForeground(cfg.number.color);
    if (cfg.number.bold) {
        m_numberFormat.setFontWeight(QFont::Bold);
    } else {
        m_numberFormat.setFontWeight(QFont::Normal);
    }
    m_numberFormat.setFontItalic(cfg.number.italic);

    // Set the string format
    m_stringFormat.setForeground(cfg.string.color);
    if (cfg.string.bold) {
        m_stringFormat.setFontWeight(QFont::Bold);
    } else {
        m_stringFormat.setFontWeight(QFont::Normal);
    }
    m_stringFormat.setFontItalic(cfg.string.italic);

    // Set the enumerated type format
    m_enumFormat.setForeground(cfg.enumItem.color);
    if (cfg.enumItem.bold) {
        m_enumFormat.setFontWeight(QFont::Bold);
    } else {
        m_enumFormat.setFontWeight(QFont::Normal);
    }
    m_enumFormat.setFontItalic(cfg.enumItem.italic);

    // Set the macro format
    m_macroFormat.setForeground(cfg.macro.color);
    if (cfg.macro.bold) {
        m_macroFormat.setFontWeight(QFont::Bold);
    } else {
        m_macroFormat.setFontWeight(QFont::Normal);
    }
    m_macroFormat.setFontItalic(cfg.macro.italic);

    // Set the comment format
    m_commentFormat.setForeground(cfg.comment.color);
    if (cfg.comment.bold) {
        m_commentFormat.setFontWeight(QFont::Bold);
    } else {
        m_commentFormat.setFontWeight(QFont::Normal);
    }
    m_commentFormat.setFontItalic(cfg.comment.italic);

    // Redraw text
    rehighlight();
}

void TreeSitterHighlighter::setTree(TSTree* newTree,
                                    const QByteArray& documentUtf8) {
    m_tree.reset(newTree);
    m_documentUtf8 = documentUtf8;

    // Redraw text
    rehighlight();
}

// Async parse hand-off: For massive files
void TreeSitterHighlighter::setTreeAsync(TSTree* newTree,
                                         const QByteArray& utf8Text) {
    m_documentUtf8 = utf8Text;
    m_tree.reset(newTree);
    rehighlight();
}

// Synchronous parse: For small files and incremental edits
void TreeSitterHighlighter::parseStringSync(const QByteArray& utf8Text) {

    m_documentUtf8 = utf8Text;
    TSTree* newTree = ts_parser_parse_string(
        m_parser.get(),
        nullptr,
        m_documentUtf8.constData(),
        m_documentUtf8.size()
        );

    m_tree.reset(newTree);
    rehighlight();
}

void TreeSitterHighlighter::applyFormatting(TSNode node,
                                            int blockStart, int blockEnd) {
    if (ts_node_is_null(node)) return;

    int nodeStart = ts_node_start_byte(node);
    int nodeEnd = ts_node_end_byte(node);

    if (nodeEnd <= blockStart || nodeStart >= blockEnd) return;

    uint32_t childCount = ts_node_child_count(node);

    if (childCount == 0) {
        int overlapStart = std::max(nodeStart, blockStart);
        int overlapEnd = std::min(nodeEnd, blockEnd);

        if (overlapStart < overlapEnd) {
            std::string nodeType(ts_node_type(node));
            int relativeStart = overlapStart - blockStart;
            int length = overlapEnd - overlapStart;

            if (nodeType == "identifier") {
                TSNode parent = ts_node_parent(node);
                TSNode keyNode = ts_node_child_by_field_name(parent, "key", 3);
                if (!ts_node_is_null(keyNode) && ts_node_eq(node, keyNode)) {
                    setFormat(relativeStart, length, m_keywordFormat);
                } else {
                    // Otherwise, it is an identifier acting as a value
                    setFormat(relativeStart, length, m_enumFormat);
                }
            }
            else if (nodeType == "number") {
                setFormat(relativeStart, length, m_numberFormat);
            }
            else if (nodeType == "string") {
                setFormat(relativeStart, length, m_stringFormat);
            }
            else if (nodeType == "boolean") {
                setFormat(relativeStart, length, m_keywordFormat);
            }
            else if (nodeType == "line_comment" ||
                       nodeType == "block_comment") {
                setFormat(relativeStart, length, m_commentFormat);
            }
            else if (nodeType == "include_directive" ||
                       nodeType == "calc_directive" ||
                       nodeType == "code_stream") {
                setFormat(relativeStart, length, m_macroFormat);
            }
        }
    } else {
        for (uint32_t i = 0; i < childCount; ++i) {
            applyFormatting(ts_node_child(node, i), blockStart, blockEnd);
        }
    }
}

// Update your highlightBlock to use it:
void TreeSitterHighlighter::highlightBlock(const QString &text) {

    if (!m_tree || text.isEmpty()) return;

    int blockStart = currentBlock().position();
    int blockEnd = blockStart + text.length();

    TSNode root = ts_tree_root_node(m_tree.get());

    // Start the recursive walk from the root
    applyFormatting(root, blockStart, blockEnd);
}