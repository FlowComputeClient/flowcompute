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

#include "page_30_physics.h"

#include "wizard_solver.h"

// Configures turbulence and transport properties
PhysicsPage::PhysicsPage(const std::vector<FlowCompute::SolverFamily>& families,
    const FlowCompute::TurbulenceDatabase& turbModels,
    const std::map<QString, FlowCompute::TransportPropertyDef>&
        transportProperties,
    QWidget *parent): QWizardPage(parent), m_families(families),
    m_turbModels(turbModels), m_transportProperties(transportProperties) {

    // Set title and style
    setTitle(tr("Turbulence and Transport Properties"));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);
    setLayout(layout);

    QGroupBox* turbulenceGroup =
        new QGroupBox(tr("Turbulence Selection"), this);
    layout->addWidget(turbulenceGroup);
    QFormLayout* turbulenceLayout = new QFormLayout(turbulenceGroup);

    // Create the label
    turbulenceLayout->addRow(new QLabel(tr("<b>Select the turbulence model "
                                           "for the simulation:</b>")));

    // Create the tree
    turbulenceTree = new QTreeWidget(this);
    turbulenceTree->setHeaderHidden(true);
    turbulenceTree->setSelectionMode(QAbstractItemView::SingleSelection);
    turbulenceLayout->addRow(turbulenceTree);
    connect(turbulenceTree, &QTreeWidget::itemSelectionChanged, this,
            &PhysicsPage::modelChanged);

    // Add the Laminar option to the tree
    QTreeWidgetItem* laminarCategoryNode = new QTreeWidgetItem(turbulenceTree);
    laminarCategoryNode->setText(0, "Laminar");
    laminarCategoryNode->setFlags(laminarCategoryNode->flags() &
                                  ~Qt::ItemIsSelectable);
    QTreeWidgetItem* laminarNode = new QTreeWidgetItem(laminarCategoryNode);
    laminarNode->setText(0, "laminar");

    // Populate the tree
    for (auto catIt = m_turbModels.constBegin();
            catIt != m_turbModels.constEnd(); ++catIt) {
        QString categoryName = catIt.key();
        QTreeWidgetItem* catNode = new QTreeWidgetItem(turbulenceTree);
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
    QMetaEnum metaEnum = QMetaEnum::fromType<Solver::DeltaModel>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_deltaModelCombo->addItem(metaEnum.key(i));
    }
    turbulenceLayout->addRow(tr("LES filter width (Delta):"),
                             m_deltaModelCombo);
    m_deltaModelCombo->setEnabled(false);

    // Create group box for transport properties
    QGroupBox* transportGroup = new QGroupBox(tr("Transport Properties"), this);
    layout->addWidget(transportGroup);
    QFormLayout* transportLayout = new QFormLayout(transportGroup);
    transportLayout->setSpacing(10);

    // Transport model selector
    m_transportModelCombo = new QComboBox(transportGroup);
    metaEnum = QMetaEnum::fromType<Solver::TransportModel>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        m_transportModelCombo->addItem(metaEnum.key(i));
    }
    transportLayout->addRow(tr("Transport Model:"), m_transportModelCombo);

    // Add space
    transportLayout->addItem(new QSpacerItem(0, 10, QSizePolicy::Minimum,
        QSizePolicy::Fixed));

    // Warning Label
    QLabel* warningLabel =
        new QLabel(tr("<i>Enter the properties required by your solver. "
        "Leave irrelevant fields blank.</i>"), transportGroup);
    warningLabel->setWordWrap(true);
    transportLayout->addRow(warningLabel);

    // Create the table
    m_propertiesTable = new QTableWidget(transportGroup);
    m_propertiesTable->setColumnCount(4);
    m_propertiesTable->setHorizontalHeaderLabels(
        { "Property", "Variable", "Dimension", "Value" });
    m_propertiesTable->verticalHeader()->setVisible(false);
    m_propertiesTable->horizontalHeader()->setSectionResizeMode(0,
                                            QHeaderView::ResizeToContents);
    m_propertiesTable->horizontalHeader()->setSectionResizeMode(1,
                                            QHeaderView::Stretch);
    m_propertiesTable->horizontalHeader()->setSectionResizeMode(2,
                                            QHeaderView::Stretch);
    m_propertiesTable->horizontalHeader()->setSectionResizeMode(3,
                                            QHeaderView::Stretch);
    m_propertiesTable->setSelectionMode(QAbstractItemView::NoSelection);
    transportLayout->addRow(m_propertiesTable);

    transportLayout->addItem(
        new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void PhysicsPage::initializePage() {

    // Access the parsed structure
    solverWizard = qobject_cast<SolverWizard*>(this->wizard());    
    m_cfg = &(solverWizard->getPhysicsConfig());

    // Update tree
    QList<QTreeWidgetItem*> items = turbulenceTree->findItems(
        m_cfg->model, Qt::MatchExactly | Qt::MatchRecursive, 0);
    if (items.isEmpty()) {
        items = turbulenceTree->findItems("kOmegaSST",
            Qt::MatchExactly | Qt::MatchRecursive, 0);
    }
    QTreeWidgetItem* targetItem = items.first();
    turbulenceTree->setCurrentItem(targetItem);
    targetItem->setSelected(true);

    // Expand parents
    QTreeWidgetItem* parent = targetItem->parent();
    while (parent) {
        parent->setExpanded(true);
        parent = parent->parent();
    }

    // Update combo boxes
    m_transportModelCombo->setCurrentIndex(
        static_cast<int>(m_cfg->transportModel));
    m_deltaModelCombo->setCurrentIndex(
        static_cast<int>(m_cfg->deltaModel));

    // Get transport properties for solver
    ControlConfig* controlConfig = &(solverWizard->getControlConfig());
    QString solverCategory = controlConfig->solverCategory;
    QString solverName = controlConfig->solver;
    QStringList transportProperties = [&]() -> QStringList {
        for (const auto& family : m_families) {
            if (family.name == solverCategory) {
                for (const auto& solver : family.solvers) {
                    if (solver.name == solverName) {
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

bool PhysicsPage::validatePage() {
    if (!m_cfg) return false;

    // Set Simulation Type and Model
    QList<QTreeWidgetItem*> selectedItems = turbulenceTree->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, tr("Selection Required"),
                             tr("Please select a turbulence model."));
        return false;
    }

    QTreeWidgetItem* selectedModelItem = selectedItems.first();
    m_cfg->model = selectedModelItem->text(0);

    // Find the simulation type (RAS, LES, or Laminar)
    QTreeWidgetItem* topLevel = selectedModelItem;
    while (topLevel->parent()) {
        topLevel = topLevel->parent();
    }
    m_cfg->simulationType = topLevel->text(0);

    // Get data from combo boxes
    m_cfg->transportModel = static_cast<Solver::TransportModel>(
        m_transportModelCombo->currentIndex());
    m_cfg->deltaModel = static_cast<Solver::DeltaModel>(
        m_deltaModelCombo->currentIndex());

    // Extract Fluid Properties from the Table
    m_cfg->fluidProperties.clear();
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
                m_cfg->fluidProperties.insert(varName, value);
            }
        }
    }
    return true;
}

void PhysicsPage::modelChanged() {

    QList<QTreeWidgetItem*> selected = turbulenceTree->selectedItems();

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