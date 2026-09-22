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

#include "wizards/solver/page_20_physics.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTableWidget>
#include <QTreeWidget>

#include "wizards/solver/wizard_solver.h"

// Configures turbulence and transport properties
PhysicsPage::PhysicsPage(const std::vector<FlowCompute::SolverFamily>& families,
    const FlowCompute::TurbulenceDatabase& turbModels,
    const std::map<QString, FlowCompute::TransportPropertyDef>&
        transportProperties,
    QWidget *parent): QWizardPage(parent), m_families(families),
    m_turbModels(turbModels), m_transportProperties(transportProperties) {
    // Set title and layout
    setTitle(tr("Physical Properties"));
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);
    setLayout(layout);

    QGroupBox* turbulenceGroup =
        new QGroupBox(tr("Turbulence Selection"), this);
    turbulenceGroup->setSizePolicy(QSizePolicy::Preferred,
                                   QSizePolicy::Expanding);
    layout->addWidget(turbulenceGroup);
    QFormLayout* turbulenceLayout = new QFormLayout(turbulenceGroup);

    // Create the label
    turbulenceLayout->addRow(new QLabel(tr("<b>Select the turbulence model "
                                           "for the simulation:</b>")));

    // Create the tree
    m_turbulenceTree = new QTreeWidget(this);
    m_turbulenceTree->setSizePolicy(QSizePolicy::Expanding,
                                    QSizePolicy::Expanding);
    m_turbulenceTree->setHeaderHidden(true);
    m_turbulenceTree->setSelectionMode(QAbstractItemView::SingleSelection);
    turbulenceLayout->addRow(m_turbulenceTree);
    connect(m_turbulenceTree, &QTreeWidget::itemSelectionChanged, this,
            &PhysicsPage::modelChanged);

    // Add the Laminar option to the tree
    QTreeWidgetItem* laminarCategoryNode =
        new QTreeWidgetItem(m_turbulenceTree);
    laminarCategoryNode->setText(0, "Laminar");
    laminarCategoryNode->setFlags(laminarCategoryNode->flags() &
                                  ~Qt::ItemIsSelectable);
    QTreeWidgetItem* laminarNode = new QTreeWidgetItem(laminarCategoryNode);
    laminarNode->setText(0, "laminar");

    // Populate the tree
    for (auto catIt = m_turbModels.constBegin();
            catIt != m_turbModels.constEnd(); ++catIt) {
        QString categoryName = catIt.key();
        QTreeWidgetItem* catNode = new QTreeWidgetItem(m_turbulenceTree);
        catNode->setText(0, categoryName);
        catNode->setFlags(catNode->flags() & ~Qt::ItemIsSelectable);

        const auto& subCatMap = catIt.value();
        for (auto subCatIt = subCatMap.constBegin();
                subCatIt != subCatMap.constEnd(); ++subCatIt) {
            QString subCategoryName = subCatIt.key();
            QTreeWidgetItem* subCatNode = new QTreeWidgetItem(catNode);
            subCatNode->setText(0, subCategoryName);
            subCatNode->setFlags(subCatNode->flags() & ~Qt::ItemIsSelectable);

            const auto& modelList = subCatIt.value();
            for (const FlowCompute::TurbulenceModel& model : modelList) {
                QTreeWidgetItem* modelNode = new QTreeWidgetItem(subCatNode);
                modelNode->setText(0, model.name);

                // Store the description
                modelNode->setData(0, Qt::UserRole, model.description);
            }
        }
    }

    // Delta setting
    m_deltaModelCombo = new QComboBox(turbulenceGroup);
    QMetaEnum metaEnum = QMetaEnum::fromType<CaseIO::DeltaModel>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_deltaModelCombo->addItem(metaEnum.key(i));
    }
    turbulenceLayout->addRow(tr("LES filter width (Delta):"),
                             m_deltaModelCombo);
    m_deltaModelCombo->setEnabled(false);

    // Add spacer to the bottom of the layout
    turbulenceLayout->addItem(
        new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    // Create group box for transport properties
    m_propertyGroup = new QGroupBox(tr("Transport Properties"), this);
    layout->addWidget(m_propertyGroup);
    QFormLayout* propertyLayout = new QFormLayout(m_propertyGroup);
    propertyLayout->setSpacing(10);

    // Transport model selector
    m_transportModelCombo = new QComboBox(m_propertyGroup);
    metaEnum = QMetaEnum::fromType<CaseIO::TransportModel>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_transportModelCombo->addItem(metaEnum.key(i));
    }
    propertyLayout->addRow(tr("Transport Model:"), m_transportModelCombo);

    // Warning Label
    QLabel* warningLabel =
        new QLabel(tr("<i>Enter the properties required by your solver. "
        "Leave irrelevant fields blank.</i>"), m_propertyGroup);
    warningLabel->setWordWrap(true);
    propertyLayout->addRow(warningLabel);

    // Create the table
    m_propertiesTable = new QTableWidget(m_propertyGroup);
    m_propertiesTable->setColumnCount(4);
    m_propertiesTable->setHorizontalHeaderLabels(
        { "Property", "Variable", "Dimension", "Value" });
    m_propertiesTable->verticalHeader()->setVisible(false);
    m_propertiesTable->horizontalHeader()->
        setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_propertiesTable->horizontalHeader()->
        setSectionResizeMode(1, QHeaderView::Stretch);
    m_propertiesTable->horizontalHeader()->
        setSectionResizeMode(2, QHeaderView::Stretch);
    m_propertiesTable->horizontalHeader()->
        setSectionResizeMode(3, QHeaderView::Stretch);
    m_propertiesTable->setSelectionMode(QAbstractItemView::NoSelection);
    propertyLayout->addRow(m_propertiesTable);
}

void PhysicsPage::initializePage() {
    // Access the parsed structure
    m_solverWizard = qobject_cast<SolverWizard*>(this->wizard());
    m_turbCfg = &(m_solverWizard->getTurbulenceConfig());

    // Update tree
    QList<QTreeWidgetItem*> items = m_turbulenceTree->findItems(
        m_turbCfg->model, Qt::MatchExactly | Qt::MatchRecursive, 0);
    if (items.isEmpty()) {
        items = m_turbulenceTree->findItems("kOmegaSST",
            Qt::MatchExactly | Qt::MatchRecursive, 0);
    }
    QTreeWidgetItem* targetItem = items.first();
    m_turbulenceTree->setCurrentItem(targetItem);
    targetItem->setSelected(true);

    // Expand parents
    QTreeWidgetItem* parent = targetItem->parent();
    while (parent) {
        parent->setExpanded(true);
        parent = parent->parent();
    }

    // Update delta model combo box
    m_deltaModelCombo->setCurrentIndex(
        static_cast<int>(m_turbCfg->deltaModel));

    m_isThermoRequired = m_solverWizard->isThermoRequired();
    if (m_isThermoRequired) {
        m_propertyGroup->hide();
        return;
    } else {
        m_propertyGroup->show();
    }

    // Update transport model combo box
    m_transCfg = &(m_solverWizard->getTransportConfig());
    m_transportModelCombo->setCurrentIndex(
        static_cast<int>(m_transCfg->transportModel));

    // Get solver name
    QString solverName;
    CaseIO::ControlConfig* controlConfig =
        &(m_solverWizard->getControlConfig());
    if (controlConfig->application.startsWith("foam")) {
        solverName = controlConfig->solver;
    } else {
        solverName = controlConfig->application;
    }

    // Get transport properties for solver
    QString solverCategory = controlConfig->solverFamily;
    QStringList transportProperties = [&]() -> QStringList {
        for (const auto& family : m_families) {
            if (family.name == solverCategory) {
                for (const auto& solver : family.solvers) {
                    if ((solver.name == solverName) ||
                        (solver.foundationName == solverName)) {
                        return solver.transportProperties;
                    }
                }
            }
        }
        return QStringList();
    }();

    // Set the number of table rows
    int validRows = 0;
    for (const QString& prop : std::as_const(transportProperties)) {
        if (m_transportProperties.contains(prop)) {
            validRows++;
        }
    }
    m_propertiesTable->setRowCount(validRows);

    // Populate the table
    int row = 0;
    QString name, varName, dim, defaultVal;

    for (int i = 0; i < transportProperties.size(); ++i) {
        if (!m_transportProperties.contains(transportProperties[i])) {
            continue;
        }

        name = m_transportProperties[transportProperties[i]].name;
        varName = transportProperties[i];
        dim = m_transportProperties[transportProperties[i]].dimensions;
        defaultVal = m_transportProperties[transportProperties[i]].defaultVal;

        // Column 0: Description
        QTableWidgetItem* nameItem = new QTableWidgetItem(name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_propertiesTable->setItem(row, 0, nameItem);

        // Column 1: Variable name
        QTableWidgetItem* varItem = new QTableWidgetItem(varName);
        varItem->setFlags(varItem->flags() & ~Qt::ItemIsEditable);
        varItem->setTextAlignment(Qt::AlignCenter);
        m_propertiesTable->setItem(row, 1, varItem);

        // Column 2: Dimensions
        QTableWidgetItem* dimItem = new QTableWidgetItem(dim);
        dimItem->setFlags(dimItem->flags() & ~Qt::ItemIsEditable);
        dimItem->setTextAlignment(Qt::AlignCenter);
        m_propertiesTable->setItem(row, 2, dimItem);

        // Column 3: The Input Field
        QLineEdit* valueEdit = new QLineEdit(this);
        valueEdit->setText(defaultVal);
        valueEdit->setFrame(false);

        // Validate numeric input
        QDoubleValidator* validator = new QDoubleValidator(valueEdit);
        validator->setLocale(QLocale::C);
        valueEdit->setValidator(validator);
        m_propertiesTable->setCellWidget(row, 3, valueEdit);
        row++;
    }

    // Set the table height
    int height = m_propertiesTable->horizontalHeader()->height() +
                 m_propertiesTable->frameWidth() * 2;
    for (int row = 0; row < m_propertiesTable->rowCount(); ++row) {
        height += m_propertiesTable->rowHeight(row);
    }
    m_propertiesTable->setFixedHeight(height);
}

// Set next page of the wizard
int PhysicsPage::nextId() const {
    if (m_isThermoRequired) {
        return SolverWizard::Page_Thermo;
    }
    return SolverWizard::Page_Boundary;
}

bool PhysicsPage::validatePage() {
    if (!m_turbCfg)
        return false;

    // Set Simulation Type and Model
    QList<QTreeWidgetItem*> selectedItems = m_turbulenceTree->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, tr("Selection Required"),
                             tr("Please select a turbulence model."));
        return false;
    }

    QTreeWidgetItem* selectedModelItem = selectedItems.first();
    m_turbCfg->model = selectedModelItem->text(0);

    // Find the simulation type (RAS, LES, or Laminar)
    QTreeWidgetItem* topLevel = selectedModelItem;
    while (topLevel->parent()) {
        topLevel = topLevel->parent();
    }
    m_turbCfg->simulationType = topLevel->text(0);

    // Get data from combo boxes
    m_turbCfg->deltaModel = static_cast<CaseIO::DeltaModel>(
        m_deltaModelCombo->currentIndex());

    if (m_isThermoRequired) {
        return true;
    }

    m_transCfg->transportModel = static_cast<CaseIO::TransportModel>(
        m_transportModelCombo->currentIndex());

    // Extract Fluid Properties from the Table
    m_transCfg->fluidProperties.clear();
    for (int i = 0; i < m_propertiesTable->rowCount(); ++i) {

        // Read the variable name from Column 1
        QString varName = m_propertiesTable->item(i, 1)->text();

        // Extract the QLineEdit from Column 3
        QWidget* cellWidget = m_propertiesTable->cellWidget(i, 3);
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(cellWidget);

        if (lineEdit) {
            QString value = lineEdit->text().trimmed();

            // Update the map if the user entered text
            if (!value.isEmpty()) {
                m_transCfg->fluidProperties.insert(varName, value);
            }
        }
    }
    return true;
}

void PhysicsPage::modelChanged() {

    QList<QTreeWidgetItem*> selected = m_turbulenceTree->selectedItems();

    // Safety check for empty selection
    if (selected.isEmpty()) {
        m_selectedModel.clear();
        m_selectedCategory.clear();
        m_selectedSubCategory.clear();
        m_deltaModelCombo->setEnabled(false);
        return;
    }

    QTreeWidgetItem* item = selected.first();
    m_selectedModel = item->text(0);
    m_selectedCategory.clear();
    m_selectedSubCategory.clear();

    // Trace up the tree
    QTreeWidgetItem* parent = item->parent();
    if (parent) {
        QTreeWidgetItem* grandParent = parent->parent();
        if (grandParent) {
            m_selectedSubCategory = parent->text(0);
            m_selectedCategory = grandParent->text(0);
        } else {
            m_selectedCategory = parent->text(0);
        }
    }

    // Enable Delta combo box for LES
    bool isLES = (m_selectedCategory == "Large-Eddy Simulation (LES)");
    m_deltaModelCombo->setEnabled(isLES);
}