#include "vulkan_window.h"

VulkanWindow::VulkanWindow(std::shared_ptr<MeshData> meshData, QWindow *parent)
    : QVulkanWindow(parent), m_meshData(std::move(meshData)) {

    // Make sure resources aren't released when the window loses visibility
    setFlags(QVulkanWindow::PersistentResources);

    // Check data
    if (!m_meshData) {
        qWarning("VulkanWindow initialized with null MeshData.");
        return;
    }

    // Compute geometry and store in member variables
    QVector3D min(m_meshData->boundingBoxMin[0], m_meshData->boundingBoxMin[1], m_meshData->boundingBoxMin[2]);
    QVector3D max(m_meshData->boundingBoxMax[0], m_meshData->boundingBoxMax[1], m_meshData->boundingBoxMax[2]);
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
    m_isUboDirty = true;
}

void VulkanWindow::wheelEvent(QWheelEvent *event) {
    float scrollDelta = event->angleDelta().y();
    const float zoomFactor = 1.1f;
    const float minDistance = 0.01f;

    if (scrollDelta > 0) {
        m_cameraDistance /= zoomFactor;
    } else if (scrollDelta < 0) {
        m_cameraDistance *= zoomFactor;
    }

    // Enforce minimum distance
    if (m_cameraDistance < minDistance) {
        m_cameraDistance = minDistance;
    }

    updateViewMatrix();
    requestUpdate();
}

void VulkanWindow::mousePressEvent(QMouseEvent *event) {

    if (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) {
        m_lastMousePos = event->pos();
    }
}

void VulkanWindow::mouseMoveEvent(QMouseEvent *event) {

    QPoint delta = event->pos() - m_lastMousePos;

    // Check which button has been clicked
    if (event->buttons() & Qt::LeftButton) {

        // Calculate how far the mouse moved
        m_lastMousePos = event->pos();

        // Set camera angles
        m_yaw -= delta.x() * 0.5f;
        m_pitch += delta.y() * 0.5f;

        // Clamp the pitch
        if (m_pitch > 89.0f) m_pitch = 89.0f;
        if (m_pitch < -89.0f) m_pitch = -89.0f;

        // Rebuild the matrix and render
        updateViewMatrix();
        requestUpdate();

    } else if (event->buttons() & Qt::MiddleButton) {

        m_lastMousePos = event->pos();

        float yawRad = qDegreesToRadians(m_yaw);
        float pitchRad = qDegreesToRadians(m_pitch);

        // Compute the vector from the eye to the centroid
        QVector3D forward(
            -std::cos(pitchRad) * std::cos(yawRad),
            -std::cos(pitchRad) * std::sin(yawRad),
            -std::sin(pitchRad));

        // Set the up vector for the entire model
        QVector3D worldUp(0.0f, 0.0f, 1.0f);

        // Compute local right and up vectors
        QVector3D right = QVector3D::crossProduct(forward, worldUp).normalized();
        QVector3D up = QVector3D::crossProduct(right, forward).normalized();

        // Compute pan speed by multiplying by camera distance
        float panSpeed = std::max(m_cameraDistance * 0.002f, 0.01f);

        // Move the centroid
        m_centroid -= right * (delta.x() * panSpeed);
        m_centroid += up * (delta.y() * panSpeed);

        // Rebuild matrix and redraw
        updateViewMatrix();
        requestUpdate();
    }
}

QVulkanWindowRenderer* VulkanWindow::createRenderer() {
    return new VulkanRenderer(this, m_meshData);
}

void VulkanWindow::setMeshData(std::shared_ptr<MeshData> meshData) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_meshData = meshData;
    m_isDataDirty = true;
    requestUpdate();
}

std::shared_ptr<MeshData> VulkanWindow::getMeshData() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_meshData;
}

TransformMatrices VulkanWindow::getMatrices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_matrices;
}

void VulkanWindow::setModelMatrix(const QMatrix4x4& matrix) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_matrices.model = matrix;
    m_isUboDirty = true;
    requestUpdate();
}

void VulkanWindow::setViewMatrix(const QMatrix4x4& matrix) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_matrices.view = matrix;
    m_isUboDirty = true;
    requestUpdate();
}

void VulkanWindow::setProjMatrix(const QMatrix4x4& matrix) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_matrices.proj = matrix;
    m_isUboDirty = true;
    requestUpdate();
}

void VulkanWindow::clearUboDirty() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isUboDirty = false;
}

void VulkanWindow::clearDirty() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isDataDirty = false;
}