#include "model_editor.h"

ModelEditor::ModelEditor(std::shared_ptr<MeshData> meshData, const QString& fullPath, int targetId,
                         QVulkanInstance* instance, bool isBinary, QWidget* parent):
    QWidget(parent), m_meshData(meshData), m_fullPath(fullPath), m_targetId(targetId),
    m_isBinary(isBinary), m_vulkanInstance(instance) {

    // Get list of patch names
    for(const auto& patch: m_meshData->patches) {
        m_patchNames.push_back(std::string(patch.name));
    }

    // Create widgets for the left pane
    m_leftPane = new ModelLeftPane(this);
    m_leftPane->setPatchNames(m_patchNames);
    m_leftPane->setFixedWidth(160);

    // Create Vulkan window
    QWidget* rightPane;
    if(!meshData->data.empty()) {
        m_vulkanWindow = new VulkanWindow(m_meshData);
        m_vulkanWindow->setVulkanInstance(m_vulkanInstance);

        // Create widget for the Vulkan window
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
    connect(m_leftPane, &ModelLeftPane::surfacePatchRequested,
            this, &ModelEditor::onSurfacePatchRequest);
    connect(m_leftPane, &ModelLeftPane::surfaceCheckRequested,
            this, &ModelEditor::onSurfaceCheckRequest);
    connect(m_leftPane, &ModelLeftPane::dirtyStateChanged,
            this, &ModelEditor::dirtyStateChanged);
}

// Update surface data
void ModelEditor::updateModel(std::shared_ptr<MeshData> newMesh) {

    // Update data
    m_meshData = newMesh;

    // Render new mesh data
    m_vulkanWindow->setMeshData(m_meshData);

    // Update patch names in left pane
    m_patchNames.clear();
    for(const auto& patch: m_meshData->patches) {
        m_patchNames.push_back(patch.name);
    }
    m_leftPane->setPatchNames(m_patchNames);

    // Set dirty state
    m_isSurfaceChanged = true;
    emit dirtyStateChanged(true);
}

// Access changes to the patch names
std::vector<std::pair<std::string, std::string>> ModelEditor::getPatchChanges() {

    std::vector<std::pair<std::string, std::string>> differences;
    std::vector<std::string> newPatchNames = m_leftPane->getPatchNames();

    const int maxSize = std::max(newPatchNames.size(), m_patchNames.size());

    for (int i = 0; i < maxSize; ++i) {
        const std::string left = (i < m_patchNames.size()) ? m_patchNames[i] : "";
        const std::string right  = (i < newPatchNames.size()) ? newPatchNames[i] : "";
        if (left != right) {
            differences.emplace_back(left, right);
        }
    }
    return differences;
}

void ModelEditor::onSurfacePatchRequest(double featureAngle) {
    emit surfacePatchRequested(featureAngle, m_fullPath, m_targetId, m_isBinary);
}

void ModelEditor::onSurfaceCheckRequest() {
    emit surfaceCheckRequested(m_fullPath, m_targetId, m_isBinary);
}
