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

#ifndef OPEN_FOAM_DICTIONARY_H
#define OPEN_FOAM_DICTIONARY_H

#include "../third_party/tree_sitter/include/tree_sitter/api.h"

#include <QByteArray>
#include <QString>

struct SyntaxError {
    uint32_t line;
    uint32_t column;
    QString text;
    QString message;
};

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
    QStringList getDictKeys(const QString& path) const;
    bool hasSyntaxErrors() const;
    void renameKey(const QString& path, const QString& newName);
    QList<SyntaxError> getSyntaxErrors() const;
    void insertIntoDict(const QString& path, const QByteArray& content);
    void removeEntry(const QString& path);

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
