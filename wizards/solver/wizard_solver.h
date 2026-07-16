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

#ifndef WIZARDS_SOLVER_WIZARD_SOLVER_H_
#define WIZARDS_SOLVER_WIZARD_SOLVER_H_

#include <QFile>
#include <QList>
#include <QRegularExpression>
#include <QWizard>

#include "parser/open_foam_dictionary.h"
#include "core_types.h"
#include "solver_structs.h"
#include "systems/system_manager.h"

#include "page_10_control.h"
#include "page_20_transient.h"
#include "page_30_physics.h"
#include "page_40_boundary.h"
#include "page_50_algorithm.h"
#include "page_60_simple.h"
#include "page_70_pimple.h"
#include "page_80_piso.h"
#include "page_90_parallel.h"
#include "page_93_visualization.h"

class SolverWizard : public QWizard {
    Q_OBJECT

 public:
    SolverWizard(const QString& caseName, const SystemManager& systemMgr,
    const std::vector<FlowCompute::SolverFamily>& families,
    const FlowCompute::TurbulenceDatabase& turbModels,
    const std::map<QString, FlowCompute::TransportPropertyDef>&
        transportProperties,
    const QHash<QString, FlowCompute::FieldDef>& fieldData,
    const std::vector<FlowCompute::BoundaryConditionDef>&
        boundaryConditions, QWidget *parent);

    bool parseFiles();
    ControlConfig& getControlConfig() { return m_controlConfig; };
    PhysicsConfig& getPhysicsConfig() { return m_physicsConfig; };
    QHash<QString, FlowCompute::FieldData>& getBoundaryConfig() {
        return m_boundaryConfig;
    };
    MathConfig& getMathConfig() { return m_mathConfig; };
    ParallelConfig& getParallelConfig() { return m_parallelConfig; };
    VisualizationConfig& getVisualizationConfig() {
        return m_visualizationConfig;
    };

    QStringList getSolverFields();
    QStringList getTurbulenceFields();
    std::vector<FlowCompute::MeshPatch>& getBoundaries() {
        return m_boundaries;
    };
    std::vector<FlowCompute::MeshPatch>
        parseBoundaryPatches(const QByteArray& fileData);

    void setCaseName(const QString& text) { m_caseName = text; };
    void setConfiguredFields(const QStringList& fields) {
        m_configuredFields = fields;
    }
    QStringList getConfiguredFields() const { return m_configuredFields; }

    FlowCompute::Algorithm getSolverAlgorithm();

 signals:
    void createEditor(EditorType type, QString& fileName, const QString& path,
                      bool logMessage);
    void updatePath(QString caseName, QString subDir);

 protected:
    void accept() override;

 private:
    const SystemManager& m_systemMgr;
    std::vector<FlowCompute::MeshPatch> m_boundaries;

    // Data from config files
    std::vector<FlowCompute::SolverFamily> m_families;
    FlowCompute::TurbulenceDatabase m_turbModels;
    QHash<QString, FlowCompute::FieldDef> m_fieldData;
    std::vector<FlowCompute::BoundaryConditionDef> m_boundaryConditions;

    // Lookup maps
    QHash<QString, FlowCompute::Algorithm> m_solverAlgorithmMap;

    // Solver dictionary structures
    bool showParsingErrorMessage(QString fileName);
    QMap<QString, std::shared_ptr<OpenFoamDictionary>> m_dictMap;
    ControlConfig m_controlConfig;
    PhysicsConfig m_physicsConfig;
    QHash<QString, FlowCompute::FieldData> m_boundaryConfig;
    MathConfig m_mathConfig;
    ParallelConfig m_parallelConfig;
    VisualizationConfig m_visualizationConfig;

    QStringList m_configuredFields;
    QString m_caseName;
    QString createSelectionDialog(const QStringList& paths);
};

#endif  // WIZARDS_SOLVER_WIZARD_SOLVER_H_
