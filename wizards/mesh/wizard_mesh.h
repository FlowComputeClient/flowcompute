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

#ifndef WIZARD_MESH_H_
#define WIZARD_MESH_H_

#include <memory>

#include <QWizard>

#include "page_10_geometry.h"
#include "page_20_blockmesh.h"
#include "page_30_blockmesh.h"
#include "page_40_surface_feature.h"
#include "page_50_castellation.h"
#include "page_60_snapcontrol.h"
#include "page_70_layercontrol.h"

#include "parser/open_foam_dictionary.h"
#include "mesh_structs.h"
#include "systems/system_manager.h"

// IDs for wizard pages
enum {
    Page_Geometry = 0,
    Page_BlockMesh1,
    Page_BlockMesh2,
    Page_SurfaceExtraction,
    Page_Castellation,
    Page_SnapControl,
    Page_LayerControl
};

class MeshWizard : public QWizard {
    Q_OBJECT

 public:
    MeshWizard(const QString& caseName, const SystemManager& systemMgr,
        QWidget *parent);

    // Load and parse mesh files
    bool loadParseFiles();

    // Get mesh data
    QMap<QString, GeometryMetrics>& getGeometryMap() { return m_geometryMap; };
    BlockMeshConfig& getBlockMeshConfig() { return m_blockMeshConfig; };
    std::map<QString, SurfaceFeatureEntry>& getFeatureMap() {
        return m_surfaceFeatureMap; };
    CastellatedMeshConfig& getCastellatedMeshConfig() {
        return m_castellatedMeshConfig; };
    SnapControlConfig& getSnapControlConfig() {
        return m_snapControlConfig; };
    LayerControlConfig& getLayerControlConfig() {
        return m_layerControlConfig; };

    // Identify which stages should be executed
    bool m_runBlockMesh, m_runExtract, m_runCastellated;
    bool m_runSnap, m_runLayers;

 signals:
    void createEditor(EditorType type, QString& fileName, const QString& path,
        bool logMessage);
    void updatePath(QString caseName, QString subDir);

 protected:
    void accept() override;

 private:
    const SystemManager& m_systemMgr;
    QString m_caseName, m_casePath;

    QMap<QString, GeometryMetrics> m_geometryMap;
    QMap<QString, std::shared_ptr<OpenFoamDictionary>> m_dictMap;

    // Mesh file structures
    BlockMeshConfig m_blockMeshConfig;
    std::map<QString, SurfaceFeatureEntry> m_surfaceFeatureMap;
    CastellatedMeshConfig m_castellatedMeshConfig;
    SnapControlConfig m_snapControlConfig;
    LayerControlConfig m_layerControlConfig;
};

#endif // WIZARD_MESH_H_
