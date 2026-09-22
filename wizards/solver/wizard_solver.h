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
#include "parser/function_object.h"
#include "parser/fv_solution.h"
#include "parser/thermo_physical_properties.h"
#include "core_types.h"
#include "systems/system_manager.h"

class SolverWizard : public QWizard {
    Q_OBJECT

 public:
    enum {
        Page_Control = 0,
        Page_Physics,
        Page_Thermo,
        Page_Boundary,
        Page_Algorithm,
        Page_Simple,
        Page_Pimple,
        Page_Piso,
        Page_Parallel,
        Page_Tasks
    };

    SolverWizard(const QString& caseName, SystemManager& systemMgr,
    const std::vector<FlowCompute::SolverFamily>& families,
    const FlowCompute::TurbulenceDatabase& turbModels,
    const std::map<QString, FlowCompute::TransportPropertyDef>&
        transportProperties,
    const QHash<QString, FlowCompute::FieldDef>& fieldData,
    const std::vector<FlowCompute::BoundaryConditionDef>&
        boundaryConditions, const QStringList& patchNames, QWidget *parent);

    bool parseFiles();
    CaseIO::ControlConfig& getControlConfig() { return m_controlConfig; };
    CaseIO::TurbulenceConfig& getTurbulenceConfig() {
        return m_turbulenceConfig; };
    CaseIO::TransportConfig& getTransportConfig() {
        return m_transportConfig; };
    CaseIO::ThermoConfig& getThermoConfig() { return m_thermoConfig; };
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

    bool isThermoRequired();
    bool isSteadyState() const { return m_isSteadyState; }
    bool isCompressible() const { return m_isCompressible; }
    void setCaseName(const QString& text) { m_caseName = text; };
    void setFieldNames(const QStringList& fields) { m_fieldNames = fields; }
    QStringList getFieldNames() const { return m_fieldNames; }
    FlowCompute::Algorithm getSolverAlgorithm();

 signals:
    void createTextEditor(QString& fileName, const QString& path,
                      bool logMessage);
    void updatePath(QString caseName, QString subDir);

 protected:
    void accept() override;

 private:
    SystemManager& m_systemMgr;
    std::vector<CaseIO::MeshPatch> m_boundaries;

    // Function objects
    std::vector<std::unique_ptr<CaseIO::FunctionObject>> m_functionObjects;

    // Data from config files
    std::vector<FlowCompute::SolverFamily> m_families;
    FlowCompute::TurbulenceDatabase m_turbModels;
    QHash<QString, FlowCompute::FieldDef> m_fieldData;
    std::vector<FlowCompute::BoundaryConditionDef> m_boundaryConditions;

    // Lookup maps
    QHash<QString, FlowCompute::Algorithm> m_solverAlgorithmMap;

    // Solver dictionary structures
    bool m_isOpenCFD = false, m_isSteadyState = false,
        m_isThermoRequired = false, m_isCompressible = false;;
    bool showParsingErrorMessage(QString fileName);
    QMap<QString, std::shared_ptr<OpenFoamDictionary>> m_dictMap;
    CaseIO::ControlConfig m_controlConfig;
    CaseIO::TurbulenceConfig m_turbulenceConfig;
    CaseIO::TransportConfig m_transportConfig;
    CaseIO::ThermoConfig m_thermoConfig;
    QHash<QString, CaseIO::FieldData> m_boundaryConfig;
    CaseIO::MathConfig m_mathConfig;
    CaseIO::ParallelConfig m_parallelConfig;

    QStringList m_fieldNames, m_patchNames;
    QString m_caseName;
    QString createSelectionDialog(const QStringList& paths);
};

#endif  // WIZARDS_SOLVER_WIZARD_SOLVER_H_
