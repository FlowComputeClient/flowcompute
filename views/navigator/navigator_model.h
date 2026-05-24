#ifndef NAVIGATOR_MODEL_H
#define NAVIGATOR_MODEL_H

#include <QStandardItemModel>
#include "node_data.h"

class NavigatorModel : public QStandardItemModel {
    Q_OBJECT
public:
    explicit NavigatorModel(QObject *parent = nullptr);

    // A clean helper to get your specific node type back out of the model
    NodeData* nodeFromIndex(const QModelIndex &index) const;
};

#endif // NAVIGATOR_MODEL_H