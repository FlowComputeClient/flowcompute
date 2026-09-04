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

#include "node_data.h"

NodeData::NodeData(const QString& name, NodeType type, bool isDisabled) :
    QStandardItem(name), name(name), nodeType(type) {

    setForeground(QBrush(Qt::black));
    setIcon(getIconForType(type));
    setSizeHint(QSize(0, 24));

    // Force the expand arrow to appear
    if (type == NodeType::Folder) {
        if (!isDisabled) {
            QStandardItem* dummy = new QStandardItem(tr("Loading..."));
            dummy->setData(true, Qt::UserRole + 1);
            appendRow(dummy);
        }
    }
}

// Returns the case containing the node
QString NodeData::getCase() const {
    const QStandardItem* current = this;

    // Traverse up until the item has no parent (the top-level case item)
    while (current->parent() != nullptr) {
        current = current->parent();
    }
    return current->text();
}

// Returns the full path starting from the case name
QString NodeData::getPath() const {
    QStringList pathElements;
    const QStandardItem* current = this;

    // Traverse up to the top-level item, prepending each name to the list
    while (current != nullptr) {
        pathElements.prepend(current->text());
        current = current->parent();
    }

    // Join the elements with forward slashes
    return pathElements.join("/");
}

QIcon NodeData::getIconForType(NodeType type) const {
    switch(type) {
        case NodeType::CaseFolder:  return QIcon(":/images/case_folder.png");
        case NodeType::Folder:      return QIcon(":/images/folder.png");
        case NodeType::DictionaryFile: return QIcon(":/images/dict.png");
        case NodeType::FieldFile:   return QIcon(":/images/field.png");
        case NodeType::MeshFile:    return QIcon(":/images/mesh.png");
        case NodeType::ScriptFile:  return QIcon(":/images/script.png");
        case NodeType::GeometryFile: return QIcon(":/images/geometry.png");
        case NodeType::TextFile: return QIcon(":/images/text.png");
        default:                    return QIcon(":/images/text.png");
    }
}