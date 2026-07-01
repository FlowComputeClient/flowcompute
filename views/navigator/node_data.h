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

#ifndef NODE_DATA_H_
#define NODE_DATA_H_

#include <QStandardItem>
#include <QString>
#include <QIcon>

// Keep this exactly as it is!
enum class NodeType {
    Root = QStandardItem::UserType + 1,
    CaseFolder,
    Folder,
    DictionaryFile,
    FieldFile,
    MeshFile,
    ScriptFile,
    GeometryFile
};

class NodeData : public QStandardItem {
 public:
    NodeData(const QString& name, const QString& fullPath, NodeType type);
    ~NodeData() override = default;

    QString name, fullPath;
    NodeType nodeType;

    enum { Type = QStandardItem::UserType + 1 };
    int type() const override { return Type; }

 private:
    QIcon getIconForType(NodeType type) const;
};

#endif // NODE_DATA_H_
