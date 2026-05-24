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

    // Compute geometry
    QVector3D min = m_meshData->boundingBoxMin;
    QVector3D max = m_meshData->boundingBoxMax;
    QVector3D centroid = (min + max) / 2.0f;

    // Update model matrix
    m_matrices.model.setToIdentity();
    // m_matrices.model.translate(-centroid);

    // Set the view matrix
    QVector3D extents = max - min;
    float maxDim = std::max({extents.x(), extents.y(), extents.z()});
    if (maxDim < 0.001f) maxDim = 1.0f;
    float cameraDistance = maxDim * 3.0f;
    QVector3D eye = centroid + QVector3D(0.0f, cameraDistance * 0.7071f, cameraDistance * 0.7071f);
    QVector3D center = centroid;
    QVector3D up(0.0f, 1.0f, 0.0f);
    m_matrices.view.setToIdentity();
    m_matrices.view.lookAt(eye, center, up);

    // Set dirty flags
    // m_isDataDirty = true;
    m_isUboDirty = true;
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