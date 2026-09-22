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

#ifndef WIZARDS_SOLVER_PAGE_20_PHYSICS_H_
#define WIZARDS_SOLVER_PAGE_20_PHYSICS_H_

#include <QWizardPage>

#include "core_types.h"
#include "parser/common.h"

class SolverWizard;
class QComboBox;
class QGroupBox;
class QTableWidget;
class QTreeWidget;

class PhysicsPage : public QWizardPage {
    Q_OBJECT

 public:
    explicit PhysicsPage(const std::vector<FlowCompute::SolverFamily>& families,
        const FlowCompute::TurbulenceDatabase& turbModels,
        const std::map<QString, FlowCompute::TransportPropertyDef>&
            transportProperties, QWidget *parent);
    int nextId() const override;

    // Accessors for the properties
    QString getTurbulenceModel() const { return m_selectedModel; }
    void setTurbulenceModel(const QString& arg) { m_selectedModel = arg; }
    QString getTurbulenceCategory() const { return m_selectedCategory; }
    void setTurbulenceCategory(const QString& arg) { m_selectedCategory = arg; }
    QString getTurbulenceSubCategory() const { return m_selectedSubCategory; }
    void setTurbulenceSubCategory(const QString& arg) {
        m_selectedSubCategory = arg; }

 protected:
    void initializePage() override;
    bool validatePage() override;

 private:
    SolverWizard* m_solverWizard;
    CaseIO::TurbulenceConfig* m_turbCfg;
    CaseIO::TransportConfig* m_transCfg;
    std::vector<FlowCompute::SolverFamily> m_families;
    FlowCompute::TurbulenceDatabase m_turbModels;
    std::map<QString, FlowCompute::TransportPropertyDef> m_transportProperties;

    QGroupBox* m_propertyGroup;
    QTreeWidget* m_turbulenceTree;
    QComboBox *m_transportModelCombo, *m_deltaModelCombo;
    QTableWidget* m_propertiesTable;
    QStringList standardProperties;

    // Property variables
    QString m_selectedModel, m_selectedCategory, m_selectedSubCategory;
    bool m_isThermoRequired;

 private slots:
    void modelChanged();
};

#endif  // WIZARDS_SOLVER_PAGE_20_PHYSICS_H_
