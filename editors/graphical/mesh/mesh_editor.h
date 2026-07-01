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

#ifndef MESH_EDITOR_H
#define MESH_EDITOR_H

#include <QHBoxLayout>
#include <QVulkanInstance>
#include <QWidget>

#include "../vulkan/vulkan_window.h"
#include "../../../geometry/graphic_data.h"
#include "mesh_left_pane.h"

class MainWindow;

class MeshEditor : public QWidget {
    Q_OBJECT

public:
    explicit MeshEditor(std::shared_ptr<RenderData> RenderData,
                        const QString& casePath, int targetId,
                        const std::vector<FlowCompute::SolverFamily>& families,
                        const FlowCompute::TurbulenceDatabase& turbModels,
                        const QHash<QString, FlowCompute::FieldDef>& fieldData,
                        const std::vector<FlowCompute::BoundaryConditionDef>& boundaryConditions,
                        QVulkanInstance* instance, QWidget* parent = nullptr);
    void updateMesh(std::shared_ptr<RenderData> newData);
    void applyTheme(const QString& theme);

signals:
    void dirtyStateChanged(bool isDirty);
    void meshCheckRequested(const QString& fullPath, int targetId);
    void meshPatchRequested(double featureAngle, const QString& fullPath, int targetId);
    void meshRenumberRequested(const QString& fullPath, int targetId);

private:
    MainWindow* m_mainWin;
    MeshLeftPane* m_leftPane;
    bool m_isSurfaceChanged = false;
    int m_targetId;
    QString m_casePath;
    std::vector<std::string> m_patchNames;

    std::vector<FlowCompute::SolverFamily> m_families;
    FlowCompute::TurbulenceDatabase m_turbModels;
    std::vector<FlowCompute::MeshPatch> m_boundaries;
    QHash<QString, FlowCompute::FieldDef> m_fieldData;

    VulkanWindow* m_vulkanWindow;
    QVulkanInstance* m_vulkanInstance;
    std::shared_ptr<RenderData> m_renderData;

    QStringList getSolverFields(const QString& solver);
    QStringList getTurbulenceFields(const QString& simulationType, const QString& modelType);
    void updatePatches();

private slots:
    void onMeshCheckRequest();
    void onMeshPatchRequest(double featureAngle);
    void onMeshRenumberRequest();
    void onPatchApply(std::vector<FlowCompute::MeshPatch>& patches);
};

#endif // MESH_EDITOR_H