#include "tree_sitter_highlighter.h"

TreeSitterHighlighter::TreeSitterHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {

    // Initialize this highlighter's exclusive parser
    m_parser.reset(ts_parser_new());
    ts_parser_set_language(m_parser.get(), tree_sitter_openfoam());

    // Setup some basic Qt text formats
    m_keywordFormat.setForeground(Qt::darkBlue);
    m_keywordFormat.setFontWeight(QFont::Bold);

    m_numberFormat.setForeground(Qt::darkGreen);
    m_stringFormat.setForeground(Qt::darkRed);
    m_enumFormat.setForeground(Qt::darkCyan);
}

void TreeSitterHighlighter::setTree(TSTree* newTree, const QByteArray& documentUtf8) {
    m_tree.reset(newTree);
    m_documentUtf8 = documentUtf8;

    // Force Qt to redraw the visible text using the new tree
    rehighlight();
}

// Async parse hand-off: For massive files
void TreeSitterHighlighter::setTreeAsync(TSTree* newTree, const QByteArray& utf8Text) {
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

void TreeSitterHighlighter::applyFormatting(TSNode node, int blockStart, int blockEnd) {
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
                // 1. Get the parent node (usually an 'entry' or 'list')
                TSNode parent = ts_node_parent(node);

                // 2. Ask the parent for its specific "key" child
                TSNode keyNode = ts_node_child_by_field_name(parent, "key", 3);

                // 3. If this identifier IS the key node, style it as a key
                if (!ts_node_is_null(keyNode) && ts_node_eq(node, keyNode)) {
                    setFormat(relativeStart, length, m_keywordFormat);
                } else {
                    // Otherwise, it is an identifier acting as a value (e.g. 'timeStep')
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