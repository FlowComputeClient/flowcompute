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

#include "navigator_model.h"

#include "case_navigator.h"

NavigatorModel::NavigatorModel(QObject *parent)
    : QStandardItemModel(parent) {}

bool NavigatorModel::setData(const QModelIndex &index,
                             const QVariant &value, int role) {
    // Respond to text edits
    if (role == Qt::EditRole) {
        NodeData* node = nodeFromIndex(index);
        if (!node)
            return false;

        // Validate the new name
        QString newName = value.toString();
        if (newName.isEmpty() || newName == node->name) {
            return false;
        }

        // Safely cast the parent to CaseNavigator
        CaseNavigator* navigator = qobject_cast<CaseNavigator*>(parent());
        if (navigator) {
            bool success = navigator->renameNode(node, newName);
            if (!success) {
                return false;
            }
        }

        // Update the node's name
        node->name = newName;
    }
    return QStandardItemModel::setData(index, value, role);
}

NodeData* NavigatorModel::nodeFromIndex(const QModelIndex &index) const {
    if (!index.isValid())
        return nullptr;

    // QStandardItemModel naturally returns the item for an index
    QStandardItem* item = itemFromIndex(index);

    // Safely cast it to your custom NodeData type
    return dynamic_cast<NodeData*>(item);
}
