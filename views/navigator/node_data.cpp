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

NodeData::NodeData(const QString& name, const QString& fullPath, NodeType type) :
    QStandardItem(name), name(name), fullPath(fullPath), nodeType(type) {

    setForeground(QBrush(Qt::black));
    setIcon(getIconForType(type));
    setEditable(false);

    // Force the expand arrow to appear
    if (type == NodeType::Folder) {
        QStandardItem* dummy = new QStandardItem("Loading...");
        dummy->setData(true, Qt::UserRole + 1);
        appendRow(dummy);
    }
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
        default:                    return QIcon(":/images/folder.png");
    }
}