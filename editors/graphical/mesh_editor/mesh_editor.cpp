#include "mesh_editor.h"

MeshEditor::MeshEditor(std::shared_ptr<MeshData> meshData, const QString& fullPath, int targetId,
                         QVulkanInstance* instance, bool isBinary, QWidget* parent):
    QWidget(parent), m_meshData(meshData), m_fullPath(fullPath), m_targetId(targetId),
    m_isBinary(isBinary), m_vulkanInstance(instance) {

    // Create left pane
    m_leftPane = new MeshLeftPane(this);
    m_leftPane->setFixedWidth(160);

    // Create right pane
    QWidget* rightPane;
    if(!meshData->data.empty()) {
        m_vulkanWindow = new VulkanWindow(m_meshData);
        m_vulkanWindow->setVulkanInstance(m_vulkanInstance);

        // Create widget for the window (right pane)
        rightPane = QWidget::createWindowContainer(m_vulkanWindow);
        rightPane->setAttribute(Qt::WA_OpaquePaintEvent, true);
        rightPane->setAttribute(Qt::WA_NoSystemBackground, true);
        rightPane->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        rightPane->setFocusPolicy(Qt::StrongFocus);
        rightPane->setAttribute(Qt::WA_DeleteOnClose);
    } else {
        rightPane = new QWidget(this);
    }

    // Layout for the main widget
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_leftPane);
    layout->addWidget(rightPane, 1);

    setLayout(layout);

    // Handle events from the left pane
    /*
    connect(m_leftPane, &LeftPane::surfacePatchRequested,
            this, &MeshEditor::onSurfacePatchRequest);
    connect(m_leftPane, &LeftPane::surfaceCheckRequested,
            this, &MeshEditor::onSurfaceCheckRequest);
    connect(m_leftPane, &LeftPane::dirtyStateChanged,
            this, &MeshEditor::dirtyStateChanged);
    */
}

// Update surface data
void MeshEditor::updateModel(std::shared_ptr<MeshData> newMesh) {

    // Update data
    m_meshData = newMesh;

    // Render new mesh data
    m_vulkanWindow->setMeshData(m_meshData);

    // Set dirty state
    m_isSurfaceChanged = true;
    emit dirtyStateChanged(true);
}