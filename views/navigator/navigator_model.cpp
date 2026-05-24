#include "navigator_model.h"

NavigatorModel::NavigatorModel(QObject *parent)
    : QStandardItemModel(parent) {}

NodeData* NavigatorModel::nodeFromIndex(const QModelIndex &index) const {
    if (!index.isValid()) return nullptr;

    // QStandardItemModel naturally returns the item for an index
    QStandardItem* item = itemFromIndex(index);

    // Safely cast it to your custom NodeData type
    return dynamic_cast<NodeData*>(item);
}