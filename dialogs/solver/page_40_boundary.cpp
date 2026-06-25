#include "page_40_boundary.h"

#include "wizard_solver.h"

#include "../../core_types.h"

// Introduction page asks for the case name and platform
BoundaryPage::BoundaryPage(const QHash<QString, FlowCompute::FieldDef>& fieldData,
                           const std::vector<FlowCompute::BoundaryConditionDef>& boundaryConditions,
                           QWidget *parent):
    QWizardPage(parent), m_fieldData(fieldData), m_boundaryConditions(boundaryConditions) {

    setTitle(tr("Boundary Configuration"));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(mainSplitter);

    // Fields
    QWidget* fieldContainer = new QWidget(mainSplitter);
    QVBoxLayout* fieldLayout = new QVBoxLayout(fieldContainer);
    fieldLayout->setContentsMargins(0, 0, 0, 0);
    fieldLayout->setSpacing(4);

    QLabel* fieldLabel = new QLabel(tr("Fields:"), fieldContainer);
    QFont labelFont = fieldLabel->font();
    labelFont.setBold(true);
    fieldLabel->setFont(labelFont);

    m_fieldListWidget = new QListWidget(fieldContainer);
    fieldLayout->addWidget(fieldLabel);
    fieldLayout->addWidget(m_fieldListWidget);

    // Patches
    QWidget* patchContainer = new QWidget(mainSplitter);
    QVBoxLayout* patchLayout = new QVBoxLayout(patchContainer);
    patchLayout->setContentsMargins(0, 0, 0, 0);
    patchLayout->setSpacing(4);

    QLabel* patchLabel = new QLabel(tr("Boundaries:"), patchContainer);
    patchLabel->setFont(labelFont);
    m_patchListWidget = new QListWidget(patchContainer);

    patchLayout->addWidget(patchLabel);
    patchLayout->addWidget(m_patchListWidget);

    // Parameters
    QGroupBox* detailGroup = new QGroupBox(tr("Configuration"), mainSplitter);
    QVBoxLayout* detailLayout = new QVBoxLayout(detailGroup);
    detailLayout->setContentsMargins(0, 0, 0, 0);

    // Stacked widget
    m_detailStack = new QStackedWidget(detailGroup);
    detailLayout->addWidget(m_detailStack);

    // Page 0: Internal Field Configuration
    QWidget* internalPage = new QWidget();
    QFormLayout* internalLayout = new QFormLayout(internalPage);
    internalLayout->setContentsMargins(0, 20, 0, 0);
    internalLayout->setFieldGrowthPolicy(
        QFormLayout::ExpandingFieldsGrow);
    m_internalFieldEdit = new QLineEdit(internalPage);
    m_internalFieldEdit->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Preferred);
    internalLayout->addRow(tr("Internal field:"), m_internalFieldEdit);
    m_detailStack->addWidget(internalPage);

    // Page 1: Boundary Condition Configuration
    QWidget* bcPage = new QWidget();
    m_bcLayout = new QFormLayout(bcPage);
    m_bcLayout->setContentsMargins(0, 20, 0, 0);
    bcPage->setLayout(m_bcLayout);
    m_detailStack->addWidget(bcPage);

    // Create combo box for patch type
    m_patchTypeCombo = new QComboBox(bcPage);
    m_patchTypeCombo->addItems(FlowCompute::patchTypes);
    m_bcLayout->addRow(tr("Patch Type:"), m_patchTypeCombo);

    // Create combo box for boundary condition type
    m_bcTypeCombo = new QComboBox(bcPage);
    m_bcLayout->addRow(tr("Condition Type:"), m_bcTypeCombo);
    m_bcLayout->setSpacing(15);

    // Sizing hints
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setStretchFactor(2, 4);

    // Event-handling
    connect(m_internalFieldEdit, &QLineEdit::textEdited, this, [this](const QString& text) {
        if (m_currentField.isEmpty()) return;
        (*m_cfg)[m_currentField].internalField = text;
    });

    connect(m_fieldListWidget, &QListWidget::currentTextChanged,
            this, &BoundaryPage::onFieldSelected);
    connect(m_patchListWidget, &QListWidget::currentTextChanged,
            this, &BoundaryPage::onPatchSelected);
    connect(m_bcTypeCombo, &QComboBox::currentTextChanged,
            this, &BoundaryPage::onBcTypeChanged);
    connect(m_patchTypeCombo, &QComboBox::currentTextChanged,
            this, &BoundaryPage::onPatchTypeChanged);
}

void BoundaryPage::initializePage() {
    solverWizard = qobject_cast<SolverWizard*>(this->wizard());
    if (!solverWizard) {
        qWarning() << "BoundaryPage must be embedded within a SolverWizard.";
        return;
    }

    // Access field data and boundaries
    m_cfg = &(solverWizard->getBoundaryConfig());
    m_boundaryPatches = solverWizard->getBoundaries();

    // Access solver and turbulence selections
    m_solverFields = solverWizard->getSolverFields();
    m_turbFields = solverWizard->getTurbulenceFields();

    // Update patch list
    m_patchListWidget->blockSignals(true);
    m_patchListWidget->clear();
    m_patchListWidget->addItem("< Internal >");
    for (const auto& patch : std::as_const(m_boundaryPatches)) {
        m_patchListWidget->addItem(patch.name);
    }
    m_patchListWidget->blockSignals(false);

    // Combine and deduplicate fields
    m_fieldList = m_solverFields + m_turbFields;
    m_fieldList.removeDuplicates();
    m_fieldList.sort();

    // Set boundary conditions for patches if not present in field files
    for (auto it = m_cfg->begin(); it != m_cfg->end(); ++it) {
        auto& fieldData = it.value();

        for (const auto& patch : std::as_const(m_boundaryPatches)) {

            // Search for the patch
            auto bcsIt = std::find_if(fieldData.bcs.begin(), fieldData.bcs.end(),
                [&patch](const std::pair<QString, FlowCompute::BoundaryCondition>& bcPair) {
                    return bcPair.first == patch.name;
                });

            // Add patch if not found
            if (bcsIt == fieldData.bcs.end()) {
                fieldData.bcs.push_back({patch.name, FlowCompute::BoundaryCondition()});
            }
        }
    }

    // Update map if there are any new fields
    for (const QString& f : std::as_const(m_fieldList)) {
        if (!m_cfg->contains(f)) {

            FlowCompute::FieldData fieldData;
            if (m_fieldData.contains(f)) {
                fieldData.dimension = m_fieldData[f].dimensions;
                fieldData.internalField = m_fieldData[f].defaultValue;
                fieldData.fieldClass = m_fieldData[f].fieldClass;
            }
            for (const auto& patch : std::as_const(m_boundaryPatches)) {
                fieldData.bcs.push_back({patch.name, FlowCompute::BoundaryCondition()});
            }
            m_cfg->insert(f, fieldData);
        }
    }

    // Populate the field list
    m_fieldListWidget->blockSignals(true);
    m_fieldListWidget->clear();
    m_fieldListWidget->addItems(m_fieldList);
    m_fieldListWidget->blockSignals(false);

    if (m_fieldListWidget->count() > 0) {
        m_fieldListWidget->setCurrentRow(0);
    }
}

bool BoundaryPage::validatePage() {
    if (!m_cfg) { return false; }

    // Update wizard fields
    solverWizard->setConfiguredFields(m_fieldList);

    // Iterate through fields
    for (auto fieldIt = m_cfg->begin(); fieldIt != m_cfg->end(); ++fieldIt) {
        const QString& fieldName = fieldIt.key();
        const FlowCompute::FieldData& fieldData = fieldIt.value();

        // Validate Internal Field
        if (fieldData.internalField.trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Validation Error"),
                                 tr("The internal field for '%1' cannot be empty.").arg(fieldName));
            return false;
        }

        // Validate Boundary Conditions
        for (const auto& bcPair : fieldData.bcs) {
            const QString& patchName = bcPair.first;
            const FlowCompute::BoundaryCondition& bc = bcPair.second;

            // Ensure a boundary condition type was selected
            if (bc.type.trimmed().isEmpty()) {
                QMessageBox::warning(this, tr("Validation Error"),
                     tr("No boundary condition type selected for patch '%1' in field '%2'.")
                         .arg(patchName, fieldName));
                return false;
            }

            // Check all dynamically generated parameters for this BC
            for (const auto& [paramName, paramValue] : bc.parameters) {
                if (paramValue.trimmed().isEmpty()) {
                    QMessageBox::warning(this, tr("Validation Error"),
                         tr("Parameter '%1' for field '%2' on patch '%3' is empty.\n\n"
                            "Please provide a valid configuration.")
                             .arg(paramName, fieldName, patchName));
                    return false;
                }
            }
        }
    }
    return true;
}

void BoundaryPage::onFieldSelected(const QString& field) {
    m_currentField = field;
    /*
    if(m_patchListWidget->selectedItems().empty()) {
        m_patchListWidget->setCurrentRow(0);
    } else {
        onPatchSelected(m_currentPatch);
    }
    */
    m_patchListWidget->setCurrentRow(0);
    onPatchSelected(m_patchListWidget->item(0)->text());
}

void BoundaryPage::onPatchSelected(const QString& patch) {

    m_currentPatch = patch;
    FlowCompute::FieldData fieldData = m_cfg->value(m_currentField);
    QString fieldClassName = FlowCompute::getBaseMathType(fieldData.fieldClass);

    if (patch == "< Internal >") {
        m_detailStack->setCurrentIndex(0);
        m_internalFieldEdit->blockSignals(true);
        m_internalFieldEdit->setText(fieldData.internalField);
        m_internalFieldEdit->blockSignals(false);
    } else {
        m_detailStack->setCurrentIndex(1);

        // Clear the combo box
        m_bcTypeCombo->blockSignals(true);
        m_bcTypeCombo->clear();

        // Get patch type
        QString patchType;
        for (const auto& patch: m_boundaryPatches) {
            if (patch.name == m_currentPatch) {
                patchType = patch.type;
                break;
            }
        }

        // Update the patch type combo box
        m_patchTypeCombo->blockSignals(true);
        m_patchTypeCombo->setCurrentText(patchType);
        m_patchTypeCombo->blockSignals(false);

        // Get valid boundary condition types
        QStringList validBcNames;
        for (const auto& bc : std::as_const(m_boundaryConditions)) {
            if (bc.types.contains(fieldClassName) && bc.patchTypes.contains(patchType)) {
                validBcNames.append(bc.name);
            }
        }

        // Update combo box
        m_bcTypeCombo->addItems(validBcNames);
        QString currentType;
        if (!m_currentField.isEmpty() && !m_currentPatch.isEmpty()) {
            const auto& bcs = (*m_cfg)[m_currentField].bcs;
            auto it = std::find_if(bcs.begin(), bcs.end(),
               [this](const std::pair<QString, FlowCompute::BoundaryCondition>& p) {
                   return p.first == m_currentPatch;
               });
            if (it != bcs.end()) {
                currentType = it->second.type;
            }
        }

        // Set the combo box to the saved type if valid
        if (!currentType.isEmpty() && validBcNames.contains(currentType)) {
            m_bcTypeCombo->setCurrentText(currentType);
        } else {
            if (m_bcTypeCombo->count() > 0) {
                m_bcTypeCombo->setCurrentIndex(0);
            }
        }
        m_bcTypeCombo->blockSignals(false);

        // Trigger the update
        if (!validBcNames.empty()) {
            onBcTypeChanged(m_bcTypeCombo->currentText());
        }
    }
}

void BoundaryPage::onPatchTypeChanged(const QString& newType) {
    if (!m_patchListWidget->currentItem()) return;

    // Update boundary data
    for (auto& patch : m_boundaryPatches) {
        if (patch.name == m_currentPatch) {
            patch.type = newType;
            break;
        }
    }
    onPatchSelected(m_patchListWidget->currentItem()->text());
}

void BoundaryPage::onBcTypeChanged(const QString& bcType) {

    // Remove rows except the first two
    while (m_bcLayout->rowCount() > 2) {
        m_bcLayout->removeRow(m_bcLayout->rowCount() - 1);
    }

    // Safety check
    if (m_currentField.isEmpty() || m_currentPatch.isEmpty()) { return; }

    // Find the matching patch name
    auto& bcs = (*m_cfg)[m_currentField].bcs;

    auto it = std::find_if(bcs.begin(), bcs.end(),
        [this](const std::pair<QString, FlowCompute::BoundaryCondition>& p) {
            return p.first == m_currentPatch;
        });

    // Update the boundary condition type
    it->second.type = bcType;

    // Get list of parameters for the selected boundary condition type
    QStringList params;
    for (const auto& bc : std::as_const(m_boundaryConditions)) {
        if (bc.name == bcType) {
            params = bc.parameters;
            break;
        }
    }

    // Add rows to layout for parameters
    for (const auto& param : std::as_const(params)) {
        QLineEdit* paramEdit = new QLineEdit();

        // Set initial text
        if (it->second.parameters.contains(param)) {
            paramEdit->setText(it->second.parameters[param]);
        }

        m_bcLayout->addRow(param, paramEdit);

        // Update state data
        connect(paramEdit, &QLineEdit::textEdited, this, [this, param](const QString& text) {

            if (m_currentField.isEmpty() || m_currentPatch.isEmpty()) return;
            auto& currentBcs = (*m_cfg)[m_currentField].bcs;
            auto patchIt = std::find_if(currentBcs.begin(), currentBcs.end(),
                [this](const std::pair<QString, FlowCompute::BoundaryCondition>& p) {
                    return p.first == m_currentPatch;
                });

            if (patchIt != currentBcs.end()) {
                patchIt->second.parameters[param] = text;
            }
        });
    }
}