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

#ifndef EDITORS_GRAPHICAL_VULKAN_VULKAN_WINDOW_H_
#define EDITORS_GRAPHICAL_VULKAN_VULKAN_WINDOW_H_

#include <QMatrix4x4>
#include <QVulkanWindow>
#include <QVulkanWindowRenderer>
#include <QVector3D>
#include <QWheelEvent>

#include <memory>
#include <mutex>

#include "vulkan_renderer.h"

#include "../../../geometry/graphic_data.h"

struct TransformMatrices {
    QMatrix4x4 model;
    QMatrix4x4 view;
    QMatrix4x4 proj;
};

class VulkanWindow : public QVulkanWindow {
    Q_OBJECT

 public:
    explicit VulkanWindow(std::shared_ptr<RenderData> data,
                           QWindow *parent = nullptr);
    QVulkanWindowRenderer *createRenderer() override;
    void renderGeometry();

    // Theme
    void applyTheme(const QString& theme);
    std::array<float, 3> getClearColor();
    bool takeThemeDirtyFlag() {
        return m_isThemeDirty.exchange(false, std::memory_order_acquire);
    }

    // Data
    std::shared_ptr<RenderData> getRenderData();
    void setRenderData(std::shared_ptr<RenderData> meshData);
    bool takeDataDirtyFlag() {
        return m_isDataDirty.exchange(false, std::memory_order_acquire);
    }

    // Ubo and matrices
    TransformMatrices getMatrices();
    void setModelMatrix(const QMatrix4x4& modelMatrix);
    void setViewMatrix(const QMatrix4x4& viewMatrix);
    void setProjMatrix(const QMatrix4x4& projMatrix);
    bool takeUboDirtyFlag() {
        return m_isUboDirty.exchange(false, std::memory_order_acquire);
    }

 protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

 private:
    // New camera state variables
    QPointF m_lastMousePos;
    std::array<float, 3> m_clearColor;

    // Start with an isometric-style viewing angle
    float m_yaw = -45.0f;
    float m_pitch = 45.0f;

    void updateViewMatrix();

    // Persistent camera state
    float m_cameraDistance = 1.0f;
    QVector3D m_centroid;
    std::shared_ptr<RenderData> m_renderData;
    std::mutex m_mutex;
    TransformMatrices m_matrices;
    std::atomic<bool> m_isDataDirty{false};
    std::atomic<bool> m_isThemeDirty{false};
    std::atomic<bool> m_isUboDirty{true};
};

#endif  // EDITORS_GRAPHICAL_VULKAN_VULKAN_WINDOW_H_
