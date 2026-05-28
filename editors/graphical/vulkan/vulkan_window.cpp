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
    QVector3D min = m_meshData->boundingBoxMin;
    QVector3D max = m_meshData->boundingBoxMax;
    m_centroid = (min + max) / 2.0f;

    m_matrices.model.setToIdentity();

    // Determine INITIAL camera distance
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

    // Convert degrees to radians for C++ math functions
    float yawRad = qDegreesToRadians(m_yaw);
    float pitchRad = qDegreesToRadians(m_pitch);

    // Convert Spherical Coordinates to Cartesian (X, Y, Z)
    // Because Z is UP in our engineering setup, the math maps out like this:
    float x = m_cameraDistance * std::cos(pitchRad) * std::cos(yawRad);
    float y = m_cameraDistance * std::cos(pitchRad) * std::sin(yawRad);
    float z = m_cameraDistance * std::sin(pitchRad);

    // Apply the offset to the centroid
    QVector3D eye = m_centroid + QVector3D(x, y, z);

    // Update the matrix
    m_matrices.view.setToIdentity();
    m_matrices.view.lookAt(eye, m_centroid, up);

    // Flag the UBO to upload to the GPU
    m_isUboDirty = true;
}

void VulkanWindow::wheelEvent(QWheelEvent *event) {
    float scrollDelta = event->angleDelta().y();
    const float zoomFactor = 1.1f;

    if (scrollDelta > 0) {
        m_cameraDistance /= zoomFactor;
    } else if (scrollDelta < 0) {
        m_cameraDistance *= zoomFactor;
    }

    // Update the matrix with the new distance
    updateViewMatrix();

    // Re-render model
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

        // Adjust camera angles
        m_yaw   -= delta.x() * 0.5f;
        m_pitch -= delta.y() * 0.5f;

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

        // 1. Calculate the Forward vector (pointing from the Eye to the Centroid)
        QVector3D forward(
            -std::cos(pitchRad) * std::cos(yawRad),
            -std::cos(pitchRad) * std::sin(yawRad),
            -std::sin(pitchRad)
            );

        // 2. Define the World Up vector (Z-Up coordinate system)
        QVector3D worldUp(0.0f, 0.0f, 1.0f);

        // 3. Calculate Local Right and Local Up vectors using the Right-Hand Rule
        QVector3D right = QVector3D::crossProduct(forward, worldUp).normalized();
        QVector3D up = QVector3D::crossProduct(right, forward).normalized();

        // 4. Calculate Pan Speed
        // Multiplying by m_cameraDistance ensures that panning feels consistent.
        // If you are zoomed way out, it pans faster. If zoomed in, it pans slower.
        float panSpeed = m_cameraDistance * 0.002f;

        // 5. Shift the centroid
        // (Note: If the panning feels 'reversed' to your preference, swap the + and - signs here)
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