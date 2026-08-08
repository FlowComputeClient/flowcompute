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

#ifndef WIZARDS_MESH_WIZARD_MESH_H_
#define WIZARDS_MESH_WIZARD_MESH_H_

#include <memory>

#include <QWizard>

#include "parser/block_mesh_dict.h"
#include "parser/snappy_hex_mesh_dict.h"
#include "parser/surface_feature.h"
#include "parser/open_foam_dictionary.h"
#include "systems/system_manager.h"

class MeshWizard : public QWizard {
    Q_OBJECT

 public:
    enum {
        Page_Geometry = 0,
        Page_BlockMesh1,
        Page_BlockMesh2,
        Page_SurfaceExtraction,
        Page_Castellation,
        Page_SnapControl,
        Page_LayerControl
    };
    MeshWizard(const QString& caseName, const SystemManager& systemMgr,
        QWidget *parent);

    // Load and parse mesh files
    bool loadParseFiles();

    // Get mesh data
    QMap<QString, GeometryMetrics>& getGeometryMap() { return m_geometryMap; };
    CaseIO::BlockMeshConfig& getBlockMeshConfig() { return m_blockMeshConfig; };
    std::map<QString, CaseIO::SurfaceFeatureEntry>& getFeatureMap() {
        return m_surfaceFeatureMap; };
    CaseIO::CastellatedMeshConfig& getCastellatedMeshConfig() {
        return m_castellatedMeshConfig; };
    CaseIO::SnapControlConfig& getSnapControlConfig() {
        return m_snapControlConfig; };
    CaseIO::LayerControlConfig& getLayerControlConfig() {
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
    CaseIO::BlockMeshConfig m_blockMeshConfig;
    std::map<QString, CaseIO::SurfaceFeatureEntry> m_surfaceFeatureMap;
    CaseIO::CastellatedMeshConfig m_castellatedMeshConfig;
    CaseIO::SnapControlConfig m_snapControlConfig;
    CaseIO::LayerControlConfig m_layerControlConfig;
};

#endif  // WIZARDS_MESH_WIZARD_MESH_H_
