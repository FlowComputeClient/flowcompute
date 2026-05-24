#ifndef OPEN_FOAM_DICTIONARY_H
#define OPEN_FOAM_DICTIONARY_H

#include "../third_party/tree_sitter/include/tree_sitter/api.h"

#include <QByteArray>
#include <QString>

struct TSTreeDeleter {
    void operator()(TSTree* tree) const {
        if (tree) {
            ts_tree_delete(tree);
        }
    }
};

struct TSParserDeleter {
    void operator()(TSParser* parser) const {
        if (parser) {
            ts_parser_delete(parser);
        }
    }
};

class OpenFoamDictionary {
public:
    OpenFoamDictionary(const QByteArray& sourceText);
    ~OpenFoamDictionary();

    // Clean getters for the UI
    QString getString(const QString& path) const;
    double getNumber(const QString& path) const;
    void setValue(const QString& path, const QString& newValue);
    QStringList getList(const QString& path) const;
    QStringList getSubDictKeys(const QString& path) const;
    bool hasSyntaxErrors() const;

    // Get the final mutated text to send back over ASIO
    QByteArray getRawText() const;

private:
    QByteArray m_sourceText;
    std::unique_ptr<TSTree, TSTreeDeleter> m_tree;
    TSParser* m_parser;

    // Hidden Tree-sitter traversal logic
    TSNode findNode(const QString& path) const;
};

#endif // OPEN_FOAM_DICTIONARY_H
