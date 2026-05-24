#include "page_30_boundary.h"

#include "wizard_solver.h"

// Introduction page asks for the case name and platform
BoundaryPage::BoundaryPage(const QHash<QString, FlowCompute::FieldData>& fieldData,
                           const QList<FlowCompute::BoundaryCondition>& boundaryConditions,
                           QWidget *parent): QWizardPage(parent), m_fieldData(fieldData),
    m_boundaryConditions(boundaryConditions) {

    // Set title and style
    setTitle(tr("Boundary Configuration"));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    setLayout(layout);

    // Create group box
    QGroupBox* groupBox = new QGroupBox(tr("Initial Field Configuration"), this);
    layout->addWidget(groupBox);
    QFormLayout* groupLayout = new QFormLayout(groupBox);
    groupLayout->setSpacing(25);

    // Create widgets
    m_fieldBox = new QComboBox(groupBox);
    groupLayout->addRow(tr("Select field for configuration: "), m_fieldBox);
    connect(m_fieldBox, &QComboBox::currentIndexChanged, this, &BoundaryPage::fieldChanged);
    m_dimensionEdit = new QLineEdit(groupBox);
    groupLayout->addRow(tr("Physical dimensions: "), m_dimensionEdit);
    m_internalFieldEdit = new QLineEdit(groupBox);
    groupLayout->addRow(tr("Initial field value: "), m_internalFieldEdit);

    // Create table to set surface extraction features
    m_boundaryTable = new QTableWidget(this);
    m_boundaryTable->setColumnCount(3);
    m_boundaryTable->setHorizontalHeaderLabels({tr("Boundary"), tr("Boundary Condition"), tr("Parameter Configuration")});
    m_boundaryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_boundaryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_boundaryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_boundaryTable->verticalHeader()->setVisible(false);
    m_boundaryTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_boundaryTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    groupLayout->addRow(m_boundaryTable);
}

QString getBaseMathType(FlowCompute::FieldClass foamClass) {
    switch (foamClass) {
    case FlowCompute::FieldClass::VolScalarField:
    case FlowCompute::FieldClass::SurfaceScalarField:
    case FlowCompute::FieldClass::PointScalarField:
        return "scalar";

    case FlowCompute::FieldClass::VolVectorField:
    case FlowCompute::FieldClass::SurfaceVectorField:
    case FlowCompute::FieldClass::PointVectorField:
        return "vector";

    case FlowCompute::FieldClass::VolSymmTensorField:
    case FlowCompute::FieldClass::SurfaceSymmTensorField:
    case FlowCompute::FieldClass::PointSymmTensorField:
        return "symmTensor";

    case FlowCompute::FieldClass::VolTensorField:
    case FlowCompute::FieldClass::SurfaceTensorField:
    case FlowCompute::FieldClass::PointTensorField:
        return "tensor";

    case FlowCompute::FieldClass::VolSphericalTensorField:
    case FlowCompute::FieldClass::SurfaceSphericalTensorField:
    case FlowCompute::FieldClass::PointSphericalTensorField:
        return "sphericalTensor";

    default:
        return "unknown";
    }
}

void BoundaryPage::initializePage() {
    solverWizard = qobject_cast<SolverWizard*>(this->wizard());
    if (!solverWizard) return;

    m_solverFields = solverWizard->getSolverFields();
    m_turbFields = solverWizard->getTurbulenceFields();
    m_boundaryPatches = solverWizard->getBoundaries();

    // 1. Combine and deduplicate fields
    QSet<QString> combinedFields;
    for (const QString& f : std::as_const(m_solverFields)) combinedFields.insert(f);
    for (const QString& f : std::as_const(m_turbFields)) combinedFields.insert(f);
    QStringList fieldsList = combinedFields.values();
    fieldsList.sort();

    // 2. Initialize the backend state map with defaults
    m_fieldEditorMap.clear();
    for (const QString& f : std::as_const(fieldsList)) {
        FieldEditor editor;
        editor.dimension = m_fieldData[f].dimensions;
        editor.internalField = m_fieldData[f].defaultValue;

        for (const auto& patch : std::as_const(m_boundaryPatches)) {
            editor.patchStates[patch.name] = PatchState();
        }
        m_fieldEditorMap.insert(f, editor);
    }

    // Initialize table
    m_boundaryTable->setRowCount(m_boundaryPatches.size());
    for (int i = 0; i < m_boundaryPatches.size(); ++i) {
        QString patchName = m_boundaryPatches[i].name;

        // Column 0: Boundary name
        QTableWidgetItem* nameItem = new QTableWidgetItem(patchName);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_boundaryTable->setItem(i, 0, nameItem);

        // Column 1: List of boundary conditions
        QComboBox* bcBox = new QComboBox(m_boundaryTable);
        connect(bcBox, &QComboBox::currentTextChanged, this, [this, i](const QString& text){
            bcChanged(i, text);
        });
        m_boundaryTable->setCellWidget(i, 1, bcBox);

        // Column 2: Push-button for parameters
        QPushButton* paramButton = new QPushButton(tr("Set parameters"), m_boundaryTable);
        paramButton->setEnabled(true);
        m_boundaryTable->setCellWidget(i, 2, paramButton);

        // Set event-handling for the button
        connect(paramButton, &QPushButton::clicked, this, [this, i]() {
            if (m_currentField.isEmpty()) return;

            QString patchName = m_boundaryPatches[i].name;
            FieldEditor& currentEditor = m_fieldEditorMap[m_currentField];
            QString currentBc = currentEditor.patchStates[patchName].bcName;

            // Pass the parameters map by reference so the dialog can modify it
            if (setParams(currentBc, currentEditor.patchStates[patchName].parameters)) {
                QPushButton* btn = qobject_cast<QPushButton*>(m_boundaryTable->cellWidget(i, 2));
                if (btn) btn->setText(tr("Parameters Set ✓"));
            }
        });
    }

    // 4. Populate the top field selector and trigger the first UI update
    m_fieldBox->blockSignals(true);
    m_fieldBox->clear();
    m_fieldBox->addItems(fieldsList);
    m_fieldBox->blockSignals(false);

    if (!fieldsList.isEmpty()) {
        m_currentField = ""; // Force a full refresh
        fieldChanged();
    }
}

bool BoundaryPage::validatePage() {

    /*
    if (validationFailed) {
        // ... (show error dialog) ...
        return false;
    }
    */

    // Validation passed! Save the final list of fields to the Master Wizard
    QStringList finalFields = m_fieldEditorMap.keys();

    if (SolverWizard* wizard = qobject_cast<SolverWizard*>(this->wizard())) {
        wizard->setConfiguredFields(finalFields);
    }

    return true;
}

void BoundaryPage::fieldChanged() {
    QString newField = m_fieldBox->currentText();
    if (newField.isEmpty() || newField == m_currentField) return;

    // Save the state of the previous field
    if (!m_currentField.isEmpty()) {
        m_fieldEditorMap[m_currentField].dimension = m_dimensionEdit->text();
        m_fieldEditorMap[m_currentField].internalField = m_internalFieldEdit->text();
    }

    // Update tracker
    m_currentField = newField;
    QString fieldClassName = getBaseMathType(m_fieldData[m_currentField].fieldClass);
    FieldEditor& currentEditor = m_fieldEditorMap[m_currentField];

    // Update text for new field
    m_dimensionEdit->setText(currentEditor.dimension);
    m_internalFieldEdit->setText(currentEditor.internalField);

    // Populate lists of boundary conditions
    for (int i = 0; i < m_boundaryPatches.size(); ++i) {
        QString patchName = m_boundaryPatches[i].name;
        QString patchType = m_boundaryPatches[i].type;

        QComboBox* bcBox = qobject_cast<QComboBox*>(m_boundaryTable->cellWidget(i, 1));
        if (!bcBox) continue;

        // Filter valid BCs
        QStringList validBcNames;
        for (const auto& bc : std::as_const(m_boundaryConditions)) {
            if (bc.types.contains(fieldClassName) && bc.patchTypes.contains(patchType)) {
                validBcNames.append(bc.name);
            }
        }

        // Block signals and add items
        bcBox->blockSignals(true);
        bcBox->clear();
        bcBox->addItems(validBcNames);

        // Restore saved selection
        QString savedBc = currentEditor.patchStates[patchName].bcName;
        int index = bcBox->findText(savedBc);

        if (index >= 0) {
            bcBox->setCurrentIndex(index);
        } else if (bcBox->count() > 0) {
            bcBox->setCurrentIndex(0);
            currentEditor.patchStates[patchName].bcName = bcBox->currentText();
            currentEditor.patchStates[patchName].parameters.clear();
        }
        bcBox->blockSignals(false);
    }
}

void BoundaryPage::bcChanged(int row, const QString& text) {
    if (m_currentField.isEmpty() || row < 0 || row >= m_boundaryPatches.size()) return;

    QString patchName = m_boundaryPatches[row].name;
    FieldEditor& currentEditor = m_fieldEditorMap[m_currentField];

    // Only wipe data if the BC actually changed
    if (currentEditor.patchStates[patchName].bcName != text) {
        currentEditor.patchStates[patchName].bcName = text;
        currentEditor.patchStates[patchName].parameters.clear();

        QPushButton* paramButton = qobject_cast<QPushButton*>(m_boundaryTable->cellWidget(row, 2));
        bool hasParams = false;

        for (auto const& bc: std::as_const(m_boundaryConditions)) {
            if (bc.name == text) {
                if (bc.parameters.size() > 0) {
                    hasParams = true;
                    for (auto const& paramName: std::as_const(bc.parameters)) {
                        currentEditor.patchStates[patchName].parameters[paramName] = "";
                    }
                }
                break;
            }
        }

        // Toggle the button state safely
        if (hasParams) {
            paramButton->setText(tr("Set parameters..."));
            paramButton->setEnabled(true);
        } else {
            paramButton->setText(tr("No parameters"));
            paramButton->setEnabled(false);
        }
    }
}

bool BoundaryPage::setParams(const QString& bcName, QHash<QString, QString>& params) {
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Parameter Settings"));
    dialog.setMinimumWidth(300);

    QFormLayout* formLayout = new QFormLayout(&dialog);
    formLayout->setSpacing(15);
    formLayout->addRow(new QLabel(tr("Configure the '%1' boundary condition:").arg(bcName)));

    // Track the pointers so we can read them after the dialog finishes
    QHash<QString, QLineEdit*> editMap;

    // Create a label and edit box for each pointer
    for(const QString& param : params.keys()) {
        QLineEdit* lineEdit = new QLineEdit(&dialog);
        lineEdit->setText(params.value(param));
        formLayout->addRow(param + ":", lineEdit);
        editMap.insert(param, lineEdit);
    }

    // Connect standard dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    formLayout->addRow(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // Block until the dialog is closed
    if (dialog.exec() == QDialog::Accepted) {
        for(auto it = editMap.cbegin(); it != editMap.cend(); ++it) {
            params[it.key()] = it.value()->text();
        }
        return true;
    }
    return false;
}