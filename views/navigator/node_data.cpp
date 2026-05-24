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