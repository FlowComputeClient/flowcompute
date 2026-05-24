#ifndef VULKAN_WINDOW_H
#define VULKAN_WINDOW_H

#include <QVulkanWindow>

#include <memory>
#include <mutex>

#include "vulkan_renderer.h"

#include "../../geometry/mesh_data.h"
#include "../../geometry/stl/stl_reader.h"

struct TransformMatrices {
    QMatrix4x4 model;
    QMatrix4x4 view;
    QMatrix4x4 proj;
};

class VulkanWindow : public QVulkanWindow {
    Q_OBJECT

public:
    VulkanWindow(std::shared_ptr<MeshData> data, QWindow *parent = nullptr);
    QVulkanWindowRenderer *createRenderer() override;
    void renderGeometry();
    TransformMatrices getMatrices();
    void setModelMatrix(const QMatrix4x4& modelMatrix);
    void setViewMatrix(const QMatrix4x4& viewMatrix);
    void setProjMatrix(const QMatrix4x4& projMatrix);
    std::shared_ptr<MeshData> getMeshData();
    bool isDirty() { return m_isDataDirty; };
    void clearDirty();
    bool isUboDirty()  { return m_isUboDirty; };
    void clearUboDirty();

private:
    std::shared_ptr<MeshData> m_meshData;
    std::mutex m_mutex;
    TransformMatrices m_matrices;
    bool m_isDataDirty = true;
    bool m_isUboDirty = true;
};

#endif // VULKAN_WINDOW