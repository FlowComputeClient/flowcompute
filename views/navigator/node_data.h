#ifndef NODE_DATA_H
#define NODE_DATA_H

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

    // Qt calls this to figure out what kind of item it's looking at
    int type() const override { return Type; }

private:
    QIcon getIconForType(NodeType type) const;
};

#endif // NODE_DATA_H