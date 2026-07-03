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

#ifndef EDITORS_GRAPHICAL_SURFACE_SURFACE_EDITOR_H_
#define EDITORS_GRAPHICAL_SURFACE_SURFACE_EDITOR_H_

#include <QHBoxLayout>
#include <QVulkanInstance>
#include <QWidget>

#include <memory>
#include <string>
#include <vector>

#include "surface_left_pane.h"
#include "../vulkan/vulkan_window.h"
#include "../../../geometry/graphic_data.h"

class SurfaceEditor : public QWidget {
    Q_OBJECT

 public:
    explicit SurfaceEditor(std::shared_ptr<RenderData> renderData,
        const QString& fullPath, int targetId, QVulkanInstance* instance,
        bool isBinary, QWidget* parent = nullptr);
    std::vector<std::pair<std::string, std::string>> getPatchChanges();
    void updateModel(std::shared_ptr<RenderData> newMesh);
    bool isSurfacePatched() { return m_isSurfaceChanged; }
    void setSurfaceChanged(bool val) { m_isSurfaceChanged = val; }
    void changeBounds(double scaleFactor);
    void applyTheme(const QString& theme);

 signals:
    void surfaceCheckRequested(const QString& fullPath, int targetId,
                               bool isBinary);
    void surfacePatchRequested(double featureAngle, const QString& fullPath,
                               int targetId, bool isBinary);
    void surfaceScaleRequested(double scaleFactor, const QString& fullPath,
                               int targetId);
    void dirtyStateChanged(bool isDirty);

 private:
    bool m_isBinary, m_isSurfaceChanged = false;
    int m_targetId;
    QString m_fullPath;
    std::vector<std::string> m_patchNames;

    SurfaceLeftPane* m_leftPane;
    VulkanWindow* m_vulkanWindow;
    QVulkanInstance* m_vulkanInstance;
    std::shared_ptr<RenderData> m_renderData;

 private slots:
    void onSurfaceCheckRequest();
    void onSurfacePatchRequest(double featureAngle);
    void onSurfaceScaleRequest(double scaleFactor);
};

#endif  // EDITORS_GRAPHICAL_SURFACE_SURFACE_EDITOR_H_
