#include "open_foam_dictionary.h"

#include <QDebug>

// Forward declare the C-generated language function
extern "C" const TSLanguage *tree_sitter_openfoam();

// --- Helper: Extract text from a TSNode ---
static QString getNodeText(TSNode node, const QByteArray& sourceText) {
    if (ts_node_is_null(node)) return QString();
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    return QString::fromUtf8(sourceText.mid(start, end - start));
}

// --- Constructor & Destructor ---
OpenFoamDictionary::OpenFoamDictionary(const QByteArray& sourceText)
    : m_sourceText(sourceText) {

    // Initialize the parser specifically for this dictionary
    m_parser = ts_parser_new();
    ts_parser_set_language(m_parser, tree_sitter_openfoam());

    // Perform the initial parse
    TSTree* tree = ts_parser_parse_string(
        m_parser, nullptr, m_sourceText.constData(),
        m_sourceText.size());
    m_tree.reset(tree);
}

OpenFoamDictionary::~OpenFoamDictionary() {
    if (m_parser) {
        ts_parser_delete(m_parser);
    }
}

// Access string in file
QString OpenFoamDictionary::getString(const QString& path) const {
    TSNode valueNode = findNode(path);
    if (ts_node_is_null(valueNode)) {
        return QString();
    }

    QString text = getNodeText(valueNode, m_sourceText);
    QString nodeType = QString(ts_node_type(valueNode));

    // Strip standard quotes
    if (text.startsWith('"') && text.endsWith('"')) {
        return text.mid(1, text.length() - 2);
    }
    // Strip verbatim block markers and trim whitespace
    else if (nodeType == "verbatim_block" || (text.startsWith("#{") && text.endsWith("#}"))) {
        return text.mid(2, text.length() - 4).trimmed();
    }

    return text;
}

double OpenFoamDictionary::getNumber(const QString& path) const {
    TSNode valueNode = findNode(path);
    if (ts_node_is_null(valueNode)) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    QString nodeType = QString(ts_node_type(valueNode));
    if (nodeType == "variable") {
        qWarning() << "Variable resolution not yet supported for path:" << path
                   << "Value:" << getNodeText(valueNode, m_sourceText);
        return std::numeric_limits<double>::quiet_NaN();
    }

    QString text = getNodeText(valueNode, m_sourceText);
    bool ok;
    double val = text.toDouble(&ok);
    return ok ? val : std::numeric_limits<double>::quiet_NaN();
}

void OpenFoamDictionary::setValue(const QString& path, const QString& newValue) {
    TSNode valueNode = findNode(path);

    if (ts_node_is_null(valueNode)) {
        qWarning() << "Cannot set value. Path not found:" << path;
        return;
    }

    uint32_t startByte = ts_node_start_byte(valueNode);
    uint32_t endByte = ts_node_end_byte(valueNode);
    uint32_t length = endByte - startByte;

    m_sourceText.replace(startByte, length, newValue.toUtf8());

    // Re-parse to keep the AST in sync
    TSTree* newTree = ts_parser_parse_string(
        m_parser,
        nullptr,
        m_sourceText.constData(),
        m_sourceText.size()
        );

    m_tree.reset(newTree);
}

QByteArray OpenFoamDictionary::getRawText() const {
    return m_sourceText;
}

static QString unquote(const QString& text) {
    if (text.length() >= 2 && text.startsWith('"') && text.endsWith('"')) {
        return text.mid(1, text.length() - 2);
    }
    return text;
}

// Traverse the AST
TSNode OpenFoamDictionary::findNode(const QString& path) const {
    if (!m_tree || path.isEmpty()) return {};

    QStringList parts = path.split('/', Qt::SkipEmptyParts);
    TSNode currentNode = ts_tree_root_node(m_tree.get());

    for (int i = 0; i < parts.size(); ++i) {
        const QString& currentKey = parts[i];
        bool isLastPart = (i == parts.size() - 1);
        bool foundNextLevel = false;

        uint32_t childCount = ts_node_child_count(currentNode);

        for (uint32_t j = 0; j < childCount; ++j) {
            TSNode child = ts_node_child(currentNode, j);

            if (QString(ts_node_type(child)) == "entry") {
                TSNode keyNode = ts_node_child_by_field_name(child, "key", strlen("key"));

                // Extract AND unquote the key before checking
                QString extractedKey = unquote(getNodeText(keyNode, m_sourceText));

                if (extractedKey == currentKey) {
                    if (isLastPart) {
                        TSNode valNode = ts_node_child_by_field_name(child, "value", strlen("value"));
                        if (!ts_node_is_null(valNode)) {
                            return valNode;
                        }

                        for (uint32_t k = 0; k < ts_node_child_count(child); ++k) {
                            TSNode dictCandidate = ts_node_child(child, k);
                            if (QString(ts_node_type(dictCandidate)) == "dict") {
                                return dictCandidate;
                            }
                        }
                        return {};
                    } else {
                        for (uint32_t k = 0; k < ts_node_child_count(child); ++k) {
                            TSNode dictCandidate = ts_node_child(child, k);
                            if (QString(ts_node_type(dictCandidate)) == "dict") {
                                currentNode = dictCandidate;
                                foundNextLevel = true;
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }

        if (!isLastPart && !foundNextLevel) {
            return {};
        }
    }

    return {};
}

QStringList OpenFoamDictionary::getList(const QString& path) const {
    QStringList result;

    // 1. Find the target node using your existing path logic
    TSNode listNode = findNode(path);

    // 2. Validate that it actually exists and is a list
    if (ts_node_is_null(listNode) || QString(ts_node_type(listNode)) != "list") {
        return result; // Return empty list
    }

    // 3. Iterate through the flat AST children
    uint32_t childCount = ts_node_child_count(listNode);
    for (uint32_t i = 0; i < childCount; ++i) {
        TSNode child = ts_node_child(listNode, i);
        QString nodeType = QString(ts_node_type(child));

        // Skip Tree-sitter punctuation tokens like "(" and ")"
        if (nodeType == "(" || nodeType == ")") {
            continue;
        }

        // Extract the raw text
        QString text = getNodeText(child, m_sourceText);

        // Clean up string literals by stripping quotes
        if (nodeType == "string" && text.startsWith('"') && text.endsWith('"')) {
            text = text.mid(1, text.length() - 2);
        }

        // Add the cleaned value to our container
        if (!text.isEmpty()) {
            result.append(text);
        }
    }

    return result;
}

QStringList OpenFoamDictionary::getSubDictKeys(const QString& path) const {
    QStringList keys;

    // 1. Find the target node
    TSNode dictNode = findNode(path);

    // 2. Validate that it actually exists and is a dictionary
    if (ts_node_is_null(dictNode) || QString(ts_node_type(dictNode)) != "dict") {
        return keys;
    }

    // 3. Iterate through the children of the dictionary
    uint32_t childCount = ts_node_child_count(dictNode);
    for (uint32_t i = 0; i < childCount; ++i) {
        TSNode child = ts_node_child(dictNode, i);

        // According to grammar.js, dictionaries are made of 'entry' nodes
        if (QString(ts_node_type(child)) == "entry") {

            // Extract just the 'key' field, ignoring the value block completely
            TSNode keyNode = ts_node_child_by_field_name(child, "key", 3);

            if (!ts_node_is_null(keyNode)) {
                QString keyText = getNodeText(keyNode, m_sourceText);

                // Strip quotes if the user used string literals for keys
                if (keyText.startsWith('"') && keyText.endsWith('"')) {
                    keyText = keyText.mid(1, keyText.length() - 2);
                }

                keys.append(keyText);
            }
        }
    }

    return keys;
}

bool OpenFoamDictionary::hasSyntaxErrors() const {
    // If the tree failed to generate entirely, treat it as an error state
    if (!m_tree) {
        return true;
    }

    // Get the root node of the parsed abstract syntax tree
    TSNode rootNode = ts_tree_root_node(m_tree.get());

    // Tree-sitter's built-in check for any error anywhere in the tree
    return ts_node_has_error(rootNode);
}