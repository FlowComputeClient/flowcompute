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

#include "surface_editor.h"

#include <algorithm>
#include <string>
#include <vector>

SurfaceEditor::SurfaceEditor(std::shared_ptr<RenderData> renderData,
    const QString& fullPath, int targetId, QVulkanInstance* instance,
    bool isBinary, QWidget* parent):

    QWidget(parent), m_renderData(renderData), m_fullPath(fullPath),
    m_targetId(targetId), m_isBinary(isBinary), m_vulkanInstance(instance) {
    // Get list of patch names
    for (const auto& patch : m_renderData->patches) {
        m_patchNames.push_back(std::string(patch.name));
    }

    // Compute bounds of surface
    std::array<float, 3> bounds;
    bounds[0] = renderData->boundingBoxMax[0] - renderData->boundingBoxMin[0];
    bounds[1] = renderData->boundingBoxMax[1] - renderData->boundingBoxMin[1];
    bounds[2] = renderData->boundingBoxMax[2] - renderData->boundingBoxMin[2];

    // Create widgets for the left pane
    m_leftPane = new SurfaceLeftPane(this);
    m_leftPane->setFixedWidth(180);
    m_leftPane->setPatchNames(m_patchNames);
    m_leftPane->setBounds(bounds);

    // Create Vulkan window
    QWidget* rightPane;
    if (!renderData->data.empty()) {
        m_vulkanWindow = new VulkanWindow(m_renderData);
        m_vulkanWindow->setVulkanInstance(m_vulkanInstance);

        // Create widget for the Vulkan window
        rightPane = QWidget::createWindowContainer(m_vulkanWindow);
        rightPane->setAttribute(Qt::WA_OpaquePaintEvent, true);
        rightPane->setAttribute(Qt::WA_NoSystemBackground, true);
        rightPane->setSizePolicy(QSizePolicy::Expanding,
                                 QSizePolicy::Expanding);
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
    connect(m_leftPane, &SurfaceLeftPane::surfacePatchRequested,
            this, &SurfaceEditor::onSurfacePatchRequest);
    connect(m_leftPane, &SurfaceLeftPane::surfaceCheckRequested,
            this, &SurfaceEditor::onSurfaceCheckRequest);
    connect(m_leftPane, &SurfaceLeftPane::surfaceScaleRequested,
            this, &SurfaceEditor::onSurfaceScaleRequest);
    connect(m_leftPane, &SurfaceLeftPane::dirtyStateChanged,
            this, &SurfaceEditor::dirtyStateChanged);
}

void SurfaceEditor::applyTheme(const QString& theme) {
    m_vulkanWindow->applyTheme(theme);
}

// Update surface data
void SurfaceEditor::updateModel(std::shared_ptr<RenderData> newData) {
    // Update data
    m_renderData = newData;

    // Render new mesh data
    m_vulkanWindow->setRenderData(m_renderData);

    // Update patch names in left pane
    m_patchNames.clear();
    for (const auto& patch : m_renderData->patches) {
        m_patchNames.push_back(patch.name);
    }
    m_leftPane->setPatchNames(m_patchNames);

    // Update bounds in left pane
    std::array<float, 3> bounds;
    bounds[0] = m_renderData->boundingBoxMax[0] -
                m_renderData->boundingBoxMin[0];
    bounds[1] = m_renderData->boundingBoxMax[1] -
                m_renderData->boundingBoxMin[1];
    bounds[2] = m_renderData->boundingBoxMax[2] -
                m_renderData->boundingBoxMin[2];
    m_leftPane->setBounds(bounds);

    // Set dirty state
    m_isSurfaceChanged = true;
    emit dirtyStateChanged(true);
}

void SurfaceEditor::changeBounds(double scaleFactor) {
    m_leftPane->changeBounds(scaleFactor);
}

// Access changes to the patch names
std::vector<std::pair<std::string, std::string>>
    SurfaceEditor::getPatchChanges() {
    std::vector<std::pair<std::string, std::string>> differences;
    std::vector<std::string> newPatchNames = m_leftPane->getPatchNames();

    const int maxSize = std::max(newPatchNames.size(), m_patchNames.size());

    for (int i = 0; i < maxSize; ++i) {
        const std::string left =
            (i < m_patchNames.size()) ? m_patchNames[i] : "";
        const std::string right =
            (i < newPatchNames.size()) ? newPatchNames[i] : "";
        if (left != right) {
            differences.emplace_back(left, right);
        }
    }
    return differences;
}

void SurfaceEditor::onSurfaceCheckRequest() {
    emit surfaceCheckRequested(m_fullPath, m_targetId, m_isBinary);
}

void SurfaceEditor::onSurfacePatchRequest(double featureAngle) {
    emit surfacePatchRequested(featureAngle, m_fullPath, m_targetId,
                               m_isBinary);
}

void SurfaceEditor::onSurfaceScaleRequest(double scaleFactor) {
    emit surfaceScaleRequested(scaleFactor, m_fullPath, m_targetId);
}
