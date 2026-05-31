#ifndef VULKAN_WINDOW_H
#define VULKAN_WINDOW_H

#include <QMatrix4x4>
#include <QVulkanWindow>
#include <QVulkanWindowRenderer>
#include <QVector3D>
#include <QWheelEvent>

#include <memory>
#include <mutex>

#include "vulkan_renderer.h"

#include "../../../geometry/mesh_data.h"

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
    void setMeshData(std::shared_ptr<MeshData> meshData);
    bool isDirty() { return m_isDataDirty; };
    void clearDirty();
    bool isUboDirty()  { return m_isUboDirty; };
    void clearUboDirty();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    // New camera state variables
    QPoint m_lastMousePos;

    // Start with an isometric-style viewing angle
    float m_yaw = -45.0f;
    float m_pitch = 45.0f;

    void updateViewMatrix();

    // Persistent camera state
    float m_cameraDistance = 1.0f;
    QVector3D m_centroid;
    std::shared_ptr<MeshData> m_meshData;
    std::mutex m_mutex;
    TransformMatrices m_matrices;
    bool m_isDataDirty = false;
    bool m_isUboDirty = true;
};

#endif // VULKAN_WINDOW