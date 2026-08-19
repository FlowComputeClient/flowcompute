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

#include <QWizard>

#include "parser/open_foam_dictionary.h"
#include "parser/control_dict.h"
#include "parser/decompose_par_dict.h"
#include "parser/field.h"
#include "parser/fv_solution.h"
#include "core_types.h"
#include "systems/system_manager.h"

class SolverWizard : public QWizard {
    Q_OBJECT

 public:
    enum {
        Page_Control = 0,
        Page_Transient = 1,
        Page_Physics = 2,
        Page_Boundary = 3,
        Page_Algorithm = 4,
        Page_Simple = 5,
        Page_Pimple = 6,
        Page_Piso = 7,
        Page_Parallel = 8,
        Page_Tasks = 9
    };

    SolverWizard(const QString& caseName, const SystemManager& systemMgr,
    const std::vector<FlowCompute::SolverFamily>& families,
    const FlowCompute::TurbulenceDatabase& turbModels,
    const std::map<QString, FlowCompute::TransportPropertyDef>&
        transportProperties,
    const QHash<QString, FlowCompute::FieldDef>& fieldData,
    const std::vector<FlowCompute::BoundaryConditionDef>&
        boundaryConditions, QStringList patchNames, QWidget *parent);

    bool parseFiles();
    CaseIO::ControlConfig& getControlConfig() { return m_controlConfig; };
    CaseIO::PhysicsConfig& getPhysicsConfig() { return m_physicsConfig; };
    QHash<QString, CaseIO::FieldData>& getBoundaryConfig() {
        return m_boundaryConfig;
    };
    CaseIO::MathConfig& getMathConfig() { return m_mathConfig; };
    CaseIO::ParallelConfig& getParallelConfig() { return m_parallelConfig; };

    QStringList getSolverFields();
    QStringList getTurbulenceFields();
    std::vector<CaseIO::MeshPatch>& getBoundaries() {
        return m_boundaries;
    };

    void setCaseName(const QString& text) { m_caseName = text; };
    void setFieldNames(const QStringList& fields) {
        m_fieldNames = fields;
    }
    QStringList getFieldNames() const { return m_fieldNames; }
    FlowCompute::Algorithm getSolverAlgorithm();

 signals:
    void createEditor(EditorType type, QString& fileName, const QString& path,
                      bool logMessage);
    void updatePath(QString caseName, QString subDir);

 protected:
    void accept() override;

 private:
    const SystemManager& m_systemMgr;
    std::vector<CaseIO::MeshPatch> m_boundaries;

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
    CaseIO::ControlConfig m_controlConfig;
    CaseIO::PhysicsConfig m_physicsConfig;
    QHash<QString, CaseIO::FieldData> m_boundaryConfig;
    CaseIO::MathConfig m_mathConfig;
    CaseIO::ParallelConfig m_parallelConfig;

    QStringList m_fieldNames, m_patchNames;
    QString m_caseName;
    QString createSelectionDialog(const QStringList& paths);
};

#endif  // WIZARDS_SOLVER_WIZARD_SOLVER_H_
