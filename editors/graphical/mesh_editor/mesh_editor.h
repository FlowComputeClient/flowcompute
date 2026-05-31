#ifndef MESH_EDITOR_H
#define MESH_EDITOR_H

#include <QHBoxLayout>
#include <QVulkanInstance>
#include <QWidget>

#include "../vulkan/vulkan_window.h"
#include "../../../geometry/mesh_data.h"
#include "mesh_left_pane.h"

class MeshEditor : public QWidget {
    Q_OBJECT

public:
    explicit MeshEditor(std::shared_ptr<MeshData> meshData, const QString& fullPath, int targetId,
                         QVulkanInstance* instance, bool isBinary, QWidget* parent = nullptr);
    void updateModel(std::shared_ptr<MeshData> newMesh);

signals:
    void dirtyStateChanged(bool isDirty);

private:
    MeshLeftPane* m_leftPane;
    bool m_isBinary, m_isSurfaceChanged = false;
    int m_targetId;
    QString m_fullPath;
    std::vector<std::string> m_patchNames;

    VulkanWindow* m_vulkanWindow;
    QVulkanInstance* m_vulkanInstance;
    std::shared_ptr<MeshData> m_meshData;
};

#endif // MESH_EDITOR_H