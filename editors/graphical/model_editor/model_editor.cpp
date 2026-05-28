#include "model_editor.h"

ModelEditor::ModelEditor(std::shared_ptr<MeshData> meshData, const QString& fullPath, int targetId,
                         QVulkanInstance* instance, bool isBinary, QWidget* parent):
    QWidget(parent), m_meshData(meshData), m_fullPath(fullPath), m_targetId(targetId),
    m_isBinary(isBinary), m_vulkanInstance(instance) {

    // Create the splitter
    m_splitter = new QSplitter(Qt::Horizontal, this);

    // Create widgets for each pane
    m_leftPane = new LeftPane(this);
    m_leftPane->setFixedWidth(120);

    // Create Vulkan window
    VulkanWindow* window = new VulkanWindow(meshData);
    window->setVulkanInstance(m_vulkanInstance);

    // Create widget for the window
    QWidget* rightPane = QWidget::createWindowContainer(window);
    rightPane->setAttribute(Qt::WA_OpaquePaintEvent, true);
    rightPane->setAttribute(Qt::WA_NoSystemBackground, true);
    rightPane->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    rightPane->setFocusPolicy(Qt::StrongFocus);
    rightPane->setAttribute(Qt::WA_DeleteOnClose);

    // Fix size of left widget, let right widget grow as needed
    m_splitter->addWidget(m_leftPane);
    m_splitter->addWidget(rightPane);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);

    // Layout for the widget
    auto* layout = new QHBoxLayout(this);
    layout->addWidget(m_splitter);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    // Process events
    connect(m_leftPane, &LeftPane::autoPatchRequested,
            this, &ModelEditor::onSurfacePatchRequest);
    connect(m_leftPane, &LeftPane::surfaceCheckRequested,
            this, &ModelEditor::onSurfaceCheckRequest);
}

void ModelEditor::onSurfacePatchRequest(double featureAngle) {
    emit surfacePatchRequested(featureAngle, m_fullPath, m_targetId, m_isBinary);
}

void ModelEditor::onSurfaceCheckRequest() {
    emit surfaceCheckRequested(m_fullPath, m_targetId, m_isBinary);
}
