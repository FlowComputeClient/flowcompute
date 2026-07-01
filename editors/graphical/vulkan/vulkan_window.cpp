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

#include "vulkan_window.h"

#include <cmath>

VulkanWindow::VulkanWindow(std::shared_ptr<RenderData> meshData,
                           QWindow *parent)
    : QVulkanWindow(parent), m_renderData(std::move(meshData)) {

    // Make sure resources aren't released when the window loses visibility
    setFlags(QVulkanWindow::PersistentResources);

    // Check data
    if (!m_renderData) {
        qWarning("VulkanWindow initialized with null MeshData.");
        return;
    }

    // Compute geometry and store in member variables
    QVector3D min(m_renderData->boundingBoxMin[0],
                  m_renderData->boundingBoxMin[1],
                  m_renderData->boundingBoxMin[2]);
    QVector3D max(m_renderData->boundingBoxMax[0],
                  m_renderData->boundingBoxMax[1],
                  m_renderData->boundingBoxMax[2]);
    m_centroid = (min + max) / 2.0f;

    // Set initial model matrix to the identity matrix
    m_matrices.model.setToIdentity();

    // Determine initial camera distance
    QVector3D extents = max - min;
    float maxDim = std::max({extents.x(), extents.y(), extents.z()});
    if (maxDim < 0.001f) maxDim = 1.0f;

    m_cameraDistance = maxDim * 3.0f;

    // The initial m_yaw and m_pitch values in your header will
    // dictate the starting angle automatically.
    updateViewMatrix();

    // Set dirty flags
    // m_isDataDirty = true;
    m_isUboDirty = true;
}

void VulkanWindow::updateViewMatrix() {
    QVector3D up(0.0f, 0.0f, 1.0f);

    float yawRad = qDegreesToRadians(m_yaw);
    float pitchRad = qDegreesToRadians(m_pitch);

    // Set the eye vector
    float x = m_cameraDistance * std::cos(pitchRad) * std::cos(yawRad);
    float y = m_cameraDistance * std::cos(pitchRad) * std::sin(yawRad);
    float z = m_cameraDistance * std::sin(pitchRad);
    QVector3D eye = m_centroid + QVector3D(x, y, z);

    // Lock the mutex before updating the view matrix
    std::lock_guard<std::mutex> lock(m_mutex);

    // Update the view matrix
    m_matrices.view.setToIdentity();
    m_matrices.view.lookAt(eye, m_centroid, up);
    m_isUboDirty.store(true, std::memory_order_release);
}

void VulkanWindow::wheelEvent(QWheelEvent *event) {

    // angleDelta().y() returns the scroll amount. Standard click is 120.
    float scrollDelta = event->angleDelta().y();
    const float zoomBase = 1.1f;
    const float minDistance = 0.01f;

    // Calculate the number of "wheel clicks"
    float numSteps = scrollDelta / 120.0f;

    // Apply proportional zoom.
    m_cameraDistance *= std::pow(zoomBase, -numSteps);

    // Enforce minimum distance
    if (m_cameraDistance < minDistance) {
        m_cameraDistance = minDistance;
    }

    updateViewMatrix();
    requestUpdate();
    event->accept();
}

void VulkanWindow::mousePressEvent(QMouseEvent *event) {

    if (event->button() == Qt::LeftButton ||
        event->button() == Qt::MiddleButton) {
        m_lastMousePos = event->pos();
    }
}

void VulkanWindow::mouseMoveEvent(QMouseEvent *event) {

    // Use Qt 6 position() for sub-pixel precision
    QPointF currentPos = event->position();
    QPointF delta = currentPos - m_lastMousePos;

    if (event->buttons() & Qt::LeftButton) {
        m_lastMousePos = currentPos;

        // Set camera angles
        m_yaw -= delta.x() * 0.5f;
        m_pitch += delta.y() * 0.5f;

        // Clamp the pitch
        if (m_pitch > 89.0f) m_pitch = 89.0f;
        if (m_pitch < -89.0f) m_pitch = -89.0f;

        updateViewMatrix();
        requestUpdate();
        event->accept();

    } else if (event->buttons() & Qt::MiddleButton) {
        m_lastMousePos = currentPos;

        float yawRad = qDegreesToRadians(m_yaw);
        float pitchRad = qDegreesToRadians(m_pitch);

        QVector3D forward(
            -std::cos(pitchRad) * std::cos(yawRad),
            -std::cos(pitchRad) * std::sin(yawRad),
            -std::sin(pitchRad));

        QVector3D worldUp(0.0f, 0.0f, 1.0f);
        QVector3D right =
            QVector3D::crossProduct(forward, worldUp).normalized();
        QVector3D up = QVector3D::crossProduct(right, forward).normalized();

        float panSpeed = std::max(m_cameraDistance * 0.002f, 0.01f);

        // Move the centroid using the floating-point delta
        m_centroid -= right * (delta.x() * panSpeed);
        m_centroid += up * (delta.y() * panSpeed);

        updateViewMatrix();
        requestUpdate();
        event->accept();
    }
}

QVulkanWindowRenderer* VulkanWindow::createRenderer() {
    return new VulkanRenderer(this, m_renderData);
}

void VulkanWindow::setRenderData(std::shared_ptr<RenderData> renderData) {
    std::shared_ptr<RenderData> newRenderData = renderData;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_renderData.swap(newRenderData);
        m_isDataDirty.store(true, std::memory_order_release);
    }
    requestUpdate();
}

std::shared_ptr<RenderData> VulkanWindow::getRenderData() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_renderData;
}

void VulkanWindow::applyTheme(const QString& theme) {
    QColor color(theme);
    std::array<float, 3> newClearColor;

    if (!color.isValid()) {
        newClearColor = { 1.0f, 1.0f, 1.0f };
    } else {
        newClearColor = {
            static_cast<float>(color.redF()),
            static_cast<float>(color.greenF()),
            static_cast<float>(color.blueF())
        };
    }

    /*
    qDebug() << newClearColor[0];
    qDebug() << newClearColor[1];
    qDebug() << newClearColor[2];
    */

    // Lock to update the variable and alert the renderer
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_clearColor = newClearColor;
        m_isThemeDirty.store(true, std::memory_order_release);
    }
    requestUpdate();
}

std::array<float, 3> VulkanWindow::getClearColor() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_clearColor;
}

TransformMatrices VulkanWindow::getMatrices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_matrices;
}

void VulkanWindow::setModelMatrix(const QMatrix4x4& matrix) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_matrices.model = matrix;
    m_isUboDirty.store(true, std::memory_order_release);
    requestUpdate();
}

void VulkanWindow::setViewMatrix(const QMatrix4x4& matrix) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_matrices.view = matrix;
    m_isUboDirty.store(true, std::memory_order_release);
    requestUpdate();
}

void VulkanWindow::setProjMatrix(const QMatrix4x4& matrix) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_matrices.proj = matrix;
    m_isUboDirty.store(true, std::memory_order_release);
    requestUpdate();
}