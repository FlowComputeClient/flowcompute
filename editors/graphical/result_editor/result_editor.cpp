#include "result_editor.h"

ResultEditor::ResultEditor(const QStringList& timeFolders,
                           std::shared_ptr<RenderData> renderData,
                           const QString& casePath, int targetId,
                           QVulkanInstance* instance, QWidget* parent):
    QWidget(parent), m_renderData(renderData), m_targetId(targetId), m_vulkanInstance(instance) {

    // Compute bounds of render data
    std::array<float, 3> bounds;
    bounds[0] = renderData->boundingBoxMax[0] - renderData->boundingBoxMin[0];
    bounds[1] = renderData->boundingBoxMax[1] - renderData->boundingBoxMin[1];
    bounds[2] = renderData->boundingBoxMax[2] - renderData->boundingBoxMin[2];

    // Create widgets for the left pane
    m_leftPane = new ResultLeftPane(timeFolders, this);
    m_leftPane->setFixedWidth(150);

    // Create Vulkan window
    QWidget* rightPane;
    if(!renderData->data.empty()) {
        m_vulkanWindow = new VulkanWindow(m_renderData);
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
}

// Update surface data
void ResultEditor::updateResult(std::shared_ptr<RenderData> newData) {

    // Update data
    m_renderData = newData;

    // Render new mesh data
    m_vulkanWindow->setRenderData(m_renderData);

    // Set dirty state
    m_isSurfaceChanged = true;
    emit dirtyStateChanged(true);
}
