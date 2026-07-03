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

#ifndef VIEWS_NAVIGATOR_NAVIGATOR_MODEL_H_
#define VIEWS_NAVIGATOR_NAVIGATOR_MODEL_H_

#include <QStandardItemModel>

#include "./node_data.h"

class NavigatorModel : public QStandardItemModel {
    Q_OBJECT
 public:
    explicit NavigatorModel(QObject *parent = nullptr);

    // Get node at the given index
    NodeData* nodeFromIndex(const QModelIndex &index) const;
};

#endif  // VIEWS_NAVIGATOR_NAVIGATOR_MODEL_H_
