#include "page_20_physics.h"

#include "wizard_solver.h"

// Introduction page asks for the case name and platform
PhysicsPage::PhysicsPage(const FlowCompute::TurbulenceDatabase& turbModels,
                         QWidget *parent): QWizardPage(parent), m_turbModels(turbModels) {

    // Set title and style
    setTitle(tr("Physics Configuration"));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);
    setLayout(layout);

    QGroupBox* turbulenceBox = new QGroupBox(tr("Turbulence Selection"), this);
    layout->addWidget(turbulenceBox);
    QVBoxLayout* turbulenceLayout = new QVBoxLayout(turbulenceBox);

    // Create the label
    turbulenceLayout->addWidget(new QLabel(tr("<b>Select the turbulence model for the simulation:</b>")));

    // Create the tree
    turbulenceTree = new QTreeWidget(this);
    turbulenceTree->setHeaderHidden(true);
    turbulenceTree->setSelectionMode(QAbstractItemView::SingleSelection);
    turbulenceLayout->addWidget(turbulenceTree);
    connect(turbulenceTree, &QTreeWidget::itemSelectionChanged, this, &PhysicsPage::modelChanged);

    // Add the Laminar option to the tree
    QTreeWidgetItem* laminarCategoryNode = new QTreeWidgetItem(turbulenceTree);
    laminarCategoryNode->setText(0, "Laminar");
    laminarCategoryNode->setFlags(laminarCategoryNode->flags() & ~Qt::ItemIsSelectable);
    QTreeWidgetItem* laminarNode = new QTreeWidgetItem(laminarCategoryNode);
    laminarNode->setText(0, "laminar");

    // Populate the tree
    for (auto catIt = m_turbModels.constBegin(); catIt != m_turbModels.constEnd(); ++catIt) {
        QString categoryName = catIt.key();
        QTreeWidgetItem* catNode = new QTreeWidgetItem(turbulenceTree);
        catNode->setText(0, categoryName);
        catNode->setFlags(catNode->flags() & ~Qt::ItemIsSelectable);

        const auto& subCatMap = catIt.value();
        for (auto subCatIt = subCatMap.constBegin(); subCatIt != subCatMap.constEnd(); ++subCatIt) {
            QString subCategoryName = subCatIt.key();
            QTreeWidgetItem* subCatNode = new QTreeWidgetItem(catNode);
            subCatNode->setText(0, subCategoryName);
            // Disable selection for the subcategory folder
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

    // Create group box for transport properties
    QGroupBox* transportBox = new QGroupBox(tr("Transport Properties"), this);
    layout->addWidget(transportBox);
    QFormLayout* transportLayout = new QFormLayout(transportBox);

    // Transport model selector
    transportModelBox = new QComboBox(transportBox);
    transportModelBox->addItems({"Newtonian", "CrossPowerLaw", "BirdCarreau", "HerschelBulkley"});
    transportLayout->addRow(tr("Transport Model:"), transportModelBox);

    // Warning Label
    QLabel* warningLabel = new QLabel(tr("<i>Enter the properties required by your solver. "
           "Leave irrelevant fields blank.</i>"), transportBox);
    warningLabel->setWordWrap(true);
    transportLayout->addRow(warningLabel);

    standardProperties = {"nu", "rho", "Pr", "Prt", "TRef", "Cp", "Cv", "k", "n", "alpha"};
    QStringList propertyDescriptors = {tr("Kinematic viscosity"), tr("Density"), tr("Prandtl Number"),
                                       tr("Turbulent Prandtl Number"), tr("Reference temperature"),
                                       tr("Specific Heat at Constant Pressure"),
                                       tr("Specific Heat at Constant Volume"),
                                       tr("Consistency index"), tr("Flow behavior index"),
                                       tr("Volume fraction of a phase")};

    // Create the table
    propertiesTable = new QTableWidget(standardProperties.size(), 2, transportBox);
    propertiesTable->setHorizontalHeaderLabels({"Property", "Value"});
    propertiesTable->verticalHeader()->setVisible(false);
    propertiesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    propertiesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    propertiesTable->setSelectionMode(QAbstractItemView::NoSelection);

    // Add rows
    for (int i = 0; i < standardProperties.size(); ++i) {
        const QString& prop = standardProperties[i];
        const QString& label = QString("%1 (%2)").arg(propertyDescriptors[i], prop);

        // The label column
        QTableWidgetItem* labelItem = new QTableWidgetItem(label);
        labelItem->setFlags(labelItem->flags() & ~Qt::ItemIsEditable);
        labelItem->setData(Qt::UserRole, prop);
        propertiesTable->setItem(i, 0, labelItem);

        // The input field
        QTableWidgetItem* valueItem = new QTableWidgetItem();
        valueItem->setBackground(QBrush(Qt::white));
        propertiesTable->setItem(i, 1, valueItem);
    }
    transportLayout->addRow(propertiesTable);

    // Register fields
    registerField("turbulenceModel*", this,
                  "turbulenceModel", SIGNAL(selectionChanged()));
    registerField("turbulenceCategory", this,
                  "turbulenceCategory", SIGNAL(selectionChanged()));
    registerField("turbulenceSubCategory", this,
                  "turbulenceSubCategory", SIGNAL(selectionChanged()));
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
}

void PhysicsPage::modelChanged() {

    QList<QTreeWidgetItem*> selected = turbulenceTree->selectedItems();
    if (selected.isEmpty()) {
        m_selectedModel.clear();
        m_selectedCategory.clear();
        m_selectedSubCategory.clear();
    } else {
        QTreeWidgetItem* item = selected.first();
        m_selectedModel = item->text(0);
        m_selectedCategory.clear();
        m_selectedSubCategory.clear();

        // Trace up the tree to determine if this is a parsed model or the laminar override
        QTreeWidgetItem* parent = item->parent();
        if (parent) {
            QTreeWidgetItem* grandParent = parent->parent();
            if (grandParent) {
                // Standard parsed model: Depth 3 (Model -> Subcategory -> Category)
                m_selectedSubCategory = parent->text(0);
                m_selectedCategory = grandParent->text(0);
            }
            // If grandParent is null, it's the manually added "laminar".
            // Variables remain empty strings.
        }
    }

    // Notify the QWizard that the properties have been updated
    emit selectionChanged();
}