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

#ifndef PAGE_30_PHYSICS_H
#define PAGE_30_PHYSICS_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWizardPage>

#include "../../core_types.h"
#include "solver_structs.h"

class SolverWizard;

class PhysicsPage : public QWizardPage {
    Q_OBJECT

public:
    explicit PhysicsPage(const std::vector<FlowCompute::SolverFamily>& families,
                         const FlowCompute::TurbulenceDatabase& turbModels,
                         const std::map<QString, FlowCompute::TransportPropertyDef>& transportProperties,
                         QWidget *parent);

    // Accessors for the properties
    QString getTurbulenceModel() const { return m_selectedModel; }
    void setTurbulenceModel(QString arg) { m_selectedModel = arg; }
    QString getTurbulenceCategory() const { return m_selectedCategory; }
    void setTurbulenceCategory(QString arg) { m_selectedCategory = arg; }
    QString getTurbulenceSubCategory() const { return m_selectedSubCategory; }
    void setTurbulenceSubCategory(QString arg) { m_selectedSubCategory = arg; }

protected:
    void initializePage() override;
    bool validatePage() override;

private:
    SolverWizard* solverWizard;
    PhysicsConfig* m_cfg;
    std::vector<FlowCompute::SolverFamily> m_families;
    FlowCompute::TurbulenceDatabase m_turbModels;
    std::map<QString, FlowCompute::TransportPropertyDef> m_transportProperties;

    QTreeWidget* turbulenceTree;
    QComboBox *m_transportModelCombo, *m_deltaModelCombo;
    QTableWidget* m_propertiesTable;
    QStringList standardProperties;

    // Backing variables for the properties
    QString m_selectedModel;
    QString m_selectedCategory;
    QString m_selectedSubCategory;

private slots:
    void modelChanged();
};

#endif  // PAGE_30_PHYSICS_H
