#include "page_50_parallel.h"

#include "wizard_solver.h"

// Introduction page asks for the case name and platform
ParallelPage::ParallelPage(QWidget *parent): QWizardPage(parent) {

    // Set title and style
    setTitle(tr("Parallel Mesh Decomposition"));
    QVBoxLayout* layout = new QVBoxLayout(this);

    // Parallel group
    QGroupBox* parallelGroup = new QGroupBox(tr("Decomposition Settings"), this);
    layout->addWidget(parallelGroup);
    QFormLayout* parallelLayout = new QFormLayout(parallelGroup);

    // Determine if decomposition should be used
    m_parallelCheck = new QCheckBox(parallelGroup);
    parallelLayout->addRow(tr("Employ parallel decomposition:"), m_parallelCheck);
    connect(m_parallelCheck, &QCheckBox::toggled,
            this, &ParallelPage::parallelChanged);

    // Set the number of subdomains
    m_subdomainsSpin = new QSpinBox(this);
    parallelLayout->addRow(tr("Set the number of subdomains:"), m_subdomainsSpin);

    // Select the method
    m_methodCombo = new QComboBox(this);
    QMetaEnum methodEnum = QMetaEnum::fromType<FlowCompute::DecompositionMethod>();
    for (int i = 0; i < methodEnum.keyCount(); ++i) {
        m_methodCombo->addItem(methodEnum.key(i), methodEnum.value(i));
    }
    connect(m_methodCombo, &QComboBox::currentIndexChanged, this, &ParallelPage::methodChanged);
    parallelLayout->addRow(tr("Decomposition method: "), m_methodCombo);

    // Create the stacked widget
    m_methodStack = new QStackedWidget(parallelGroup);
    parallelLayout->addRow(m_methodStack);

    // ---------------------------------------------------------
    // PAGE 0: Scotch (Index 0)
    // ---------------------------------------------------------
    QWidget* automaticPage = new QWidget();
    QVBoxLayout* automaticLayout = new QVBoxLayout(automaticPage);
    automaticLayout->addWidget(new QLabel(tr("This method will automatically load-balance the mesh.")));
    m_methodStack->addWidget(automaticPage);

    // ---------------------------------------------------------
    // PAGE 1: Simple (Index 1)
    // ---------------------------------------------------------
    QWidget* simplePage = new QWidget();
    QHBoxLayout* simpleLayout = new QHBoxLayout(simplePage);

    m_simpleXSpin = new QSpinBox(); m_simpleXSpin->setMinimum(1);
    m_simpleYSpin = new QSpinBox(); m_simpleYSpin->setMinimum(1);
    m_simpleZSpin = new QSpinBox(); m_simpleZSpin->setMinimum(1);

    simpleLayout->addWidget(new QLabel("x:")); simpleLayout->addWidget(m_simpleXSpin);
    simpleLayout->addWidget(new QLabel("y:")); simpleLayout->addWidget(m_simpleYSpin);
    simpleLayout->addWidget(new QLabel("z:")); simpleLayout->addWidget(m_simpleZSpin);
    m_methodStack->addWidget(simplePage);

    // ---------------------------------------------------------
    // PAGE 2: Hierarchical (Index 2)
    // ---------------------------------------------------------
    QWidget* hierarchicalPage = new QWidget();
    QHBoxLayout* hierarchicalLayout = new QHBoxLayout(hierarchicalPage);

    m_hierXSpin = new QSpinBox(); m_hierXSpin->setMinimum(1);
    m_hierYSpin = new QSpinBox(); m_hierYSpin->setMinimum(1);
    m_hierZSpin = new QSpinBox(); m_hierZSpin->setMinimum(1);

    m_hierOrderCombo = new QComboBox();
    m_hierOrderCombo->addItems({"xyz", "xzy", "yxz", "yzx", "zxy", "zyx"});

    hierarchicalLayout->addWidget(new QLabel("x:")); hierarchicalLayout->addWidget(m_hierXSpin);
    hierarchicalLayout->addWidget(new QLabel("y:")); hierarchicalLayout->addWidget(m_hierYSpin);
    hierarchicalLayout->addWidget(new QLabel("z:")); hierarchicalLayout->addWidget(m_hierZSpin);
    hierarchicalLayout->addWidget(new QLabel(tr("Order:"))); hierarchicalLayout->addWidget(m_hierOrderCombo);
    m_methodStack->addWidget(hierarchicalPage);

    setLayout(layout);
}

void ParallelPage::initializePage() {
    // Access the wizard's structure
    solverWizard = qobject_cast<SolverWizard*>(this->wizard());
    if (!solverWizard) return;

    m_cfg = &(solverWizard->getParallelConfig());

    // Populate the UI with existing config data (useful if the user clicks "Back" then "Next")
    m_parallelCheck->setChecked(m_cfg->useParallel);
    m_subdomainsSpin->setValue(m_cfg->numSubdomains > 0 ? m_cfg->numSubdomains : 1);

    // Find the combo box index that matches the saved enum
    int methodIndex = m_methodCombo->findData(QVariant::fromValue(m_cfg->method));
    if (methodIndex != -1) {
        m_methodCombo->setCurrentIndex(methodIndex);
    }

    m_simpleXSpin->setValue(m_cfg->nx);
    m_simpleYSpin->setValue(m_cfg->ny);
    m_simpleZSpin->setValue(m_cfg->nz);

    m_hierXSpin->setValue(m_cfg->nx);
    m_hierYSpin->setValue(m_cfg->ny);
    m_hierZSpin->setValue(m_cfg->nz);
    m_hierOrderCombo->setCurrentText(m_cfg->order);

    // Ensure the UI state matches the checkbox
    parallelChanged(m_cfg->useParallel);
}

bool ParallelPage::validatePage() {
    if (!m_cfg) return false;

    // 1. Save top-level parallel settings
    m_cfg->useParallel = m_parallelCheck->isChecked();

    // If parallel is disabled, we don't need to validate or save the rest
    if (!m_cfg->useParallel) {
        return true;
    }

    m_cfg->numSubdomains = m_subdomainsSpin->value();

    // Retrieve the strongly-typed enum from item data
    QVariant data = m_methodCombo->currentData();
    m_cfg->method = static_cast<FlowCompute::DecompositionMethod>(data.toInt());

    // 2. Validate geometry and save method-specific settings
    if (m_cfg->method == FlowCompute::DecompositionMethod::Simple) {
        int nx = m_simpleXSpin->value();
        int ny = m_simpleYSpin->value();
        int nz = m_simpleZSpin->value();

        if ((nx * ny * nz) != m_cfg->numSubdomains) {
            QMessageBox::warning(this, tr("Invalid Geometry"),
                                 tr("For the 'Simple' method, x * y * z must equal the total number of subdomains."));
            return false;
        }

        m_cfg->nx = nx;
        m_cfg->ny = ny;
        m_cfg->nz = nz;
    }
    else if (m_cfg->method == FlowCompute::DecompositionMethod::Hierarchical) {
        int nx = m_hierXSpin->value();
        int ny = m_hierYSpin->value();
        int nz = m_hierZSpin->value();

        if ((nx * ny * nz) != m_cfg->numSubdomains) {
            QMessageBox::warning(this, tr("Invalid Geometry"),
                                 tr("For the 'Hierarchical' method, x * y * z must equal the total number of subdomains."));
            return false;
        }

        m_cfg->nx = nx;
        m_cfg->ny = ny;
        m_cfg->nz = nz;
        m_cfg->order = m_hierOrderCombo->currentText();
    }
    return true;
}

// Populate list of solvers based on solver family
void ParallelPage::parallelChanged(bool checked) {
    m_subdomainsSpin->setEnabled(checked);
    m_methodCombo->setEnabled(checked);
    m_methodStack->setEnabled(checked);
}

// Update values when a new solver is chosen
void ParallelPage::methodChanged(int index) {

    // Get the selected solver
    QVariant data = m_methodCombo->itemData(index);
    auto currentMethod = static_cast<FlowCompute::DecompositionMethod>(data.toInt());

    switch (currentMethod) {
    case FlowCompute::DecompositionMethod::Scotch:
    case FlowCompute::DecompositionMethod::Metis:
        m_methodStack->setCurrentIndex(0);
        break;
    case FlowCompute::DecompositionMethod::Simple:
        m_methodStack->setCurrentIndex(1);
        break;
    case FlowCompute::DecompositionMethod::Hierarchical:
        m_methodStack->setCurrentIndex(2);
        break;
    }
}