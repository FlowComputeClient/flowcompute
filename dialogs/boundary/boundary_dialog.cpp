#include "boundary_dialog.h"

#include <QDoubleSpinBox>
#include <QLineEdit>

BoundaryDialog::BoundaryDialog(QString patchName, QString patchType, QStringList fields,
                               const QHash<QString, FlowCompute::FieldDef>& fieldData,
                               const std::vector<FlowCompute::BoundaryConditionDef>& boundaryConditions,
                               QWidget* parent):
    m_patchName(patchName), m_patchType(patchType), m_fields(fields), m_fieldData(fieldData),
    m_boundaryConditions(boundaryConditions), QDialog(parent) {

    setWindowTitle(tr("Boundary Conditions for Patch %1").arg(m_patchName));

    // Increased minimum size to comfortably accommodate the Master-Detail split
    setMinimumSize(550, 400);

    for(const auto& field: fields) {
        qDebug() << "Field: " << field;
    }

    // Populate map of boundary condition names and parameters
    for(const auto& bc: std::as_const(m_boundaryConditions)) {
        m_paramMap.insert(bc.name, bc.parameters);
    }

    // Base layout: Stacks the main content area and the dialog buttons vertically
    QVBoxLayout* baseLayout = new QVBoxLayout(this);
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(10);

    // The left sidebar
    m_fieldListWidget = new QListWidget(this);
    m_fieldListWidget->addItems(m_fields);
    m_fieldListWidget->setMaximumWidth(150);
    contentLayout->addWidget(m_fieldListWidget);

    // The right side
    m_stackedWidget = new QStackedWidget(this);

    // Build a page for each field
    QStringList boundaryTypes;
    for (const QString& field : std::as_const(m_fields)) {
        QWidget* pageWidget = new QWidget();
        QFormLayout* pageLayout = new QFormLayout(pageWidget);
        pageLayout->setSpacing(15);

        // Boundary type selection for this specific field
        QComboBox* boundaryTypeCombo = new QComboBox(pageWidget);
        pageLayout->addRow(tr("Boundary Condition Type"), boundaryTypeCombo);

        // Determine the data type
        QString dataType = "scalar";
        if (m_fieldData.contains(field)) {
            dataType = getBaseMathType(m_fieldData[field].fieldClass);
        }

        // Find boundary types
        boundaryTypes.clear();
        for(const auto& bc: std::as_const(m_boundaryConditions)) {
            if(bc.types.contains(dataType) && bc.patchTypes.contains(m_patchType)) {
                boundaryTypes.append(bc.name);
            }
        }
        boundaryTypeCombo->addItems(boundaryTypes);

        // Group box for parameters for this field
        QGroupBox* paramGroup = new QGroupBox(tr("Parameter Configuration"), pageWidget);
        QFormLayout* paramLayout = new QFormLayout(paramGroup);
        paramLayout->setSpacing(15);
        paramGroup->setVisible(false);

        pageLayout->addRow(paramGroup);

        // Push a spacer at the bottom so the form elements stay anchored to the top
        QSpacerItem* spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
        pageLayout->addItem(spacer);

        // Add the completed page to the stack
        m_stackedWidget->addWidget(pageWidget);

        // Store pointers in your class maps
        m_boundaryTypeCombos[field] = boundaryTypeCombo;
        m_paramGroups[field] = paramGroup;
        m_paramLayouts[field] = paramLayout;

        // Connect the combo box using a lambda to capture the specific field name
        connect(boundaryTypeCombo, &QComboBox::currentTextChanged, this, [this, field](const QString& bcType) {
            bcChanged(field, bcType);
        });
    }

    contentLayout->addWidget(m_stackedWidget);
    baseLayout->addLayout(contentLayout);

    // Standard OK/Cancel buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    baseLayout->addWidget(buttonBox);

    // Dialog Connections
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        saveCurrentState();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Wire the list selection to swap the active page in the stack
    connect(m_fieldListWidget, &QListWidget::currentRowChanged, m_stackedWidget, &QStackedWidget::setCurrentIndex);

    // Initialize the default state
    if (!m_fields.isEmpty()) {
        m_fieldListWidget->setCurrentRow(0); // This automatically triggers setCurrentIndex(0)
    }
}

void BoundaryDialog::saveCurrentState() {
    m_patchStates.clear();

    // Iterate through every field page we built in the constructor
    for (const QString& field : std::as_const(m_fields)) {
        // Safety check to ensure the UI elements exist in our hash maps
        if (!m_boundaryTypeCombos.contains(field) || !m_paramLayouts.contains(field)) {
            continue;
        }

        FlowCompute::BoundaryCondition state;

        // Grab the boundary type from this specific field's combo box
        state.type = m_boundaryTypeCombos[field]->currentText();

        QFormLayout* paramLayout = m_paramLayouts[field];

        // Extract parameters from the dynamic form layout for this specific field
        for (int i = 0; i < paramLayout->rowCount(); ++i) {
            QLayoutItem* labelItem = paramLayout->itemAt(i, QFormLayout::LabelRole);
            QLayoutItem* fieldItem = paramLayout->itemAt(i, QFormLayout::FieldRole);

            if (labelItem && fieldItem) {
                QLabel* label = qobject_cast<QLabel*>(labelItem->widget());
                QLineEdit* lineEdit = qobject_cast<QLineEdit*>(fieldItem->widget());

                if (label && lineEdit) {
                    QString paramName = label->text();

                    // QFormLayout sometimes visually appends colons depending on the OS/Style.
                    // Instead of chop(2), replacing the colon and trimming is slightly
                    // more robust across different Windows UI scaling environments.
                    paramName = paramName.remove(':').trimmed();

                    state.parameters[paramName] = lineEdit->text();
                }
            }
        }

        // Store the fully extracted state into the map for this field
        m_patchStates[field] = state;
    }
}

void BoundaryDialog::bcChanged(const QString& fieldName, const QString& bcType) {

    // 1. Verify we have the UI elements for this field
    if (!m_paramLayouts.contains(fieldName) || !m_paramGroups.contains(fieldName)) {
        return;
    }

    QFormLayout* paramLayout = m_paramLayouts[fieldName];
    QGroupBox* paramGroup = m_paramGroups[fieldName];

    // Remove widgets from the layout
    while (QLayoutItem* item = paramLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    // Check if the boundary condition type is in the map
    if (!m_paramMap.contains(bcType)) {
        paramGroup->setVisible(false);
        return;
    }

    // Access parameter names for the boundary condition type
    const QStringList& parameters = m_paramMap.value(bcType);
    if (parameters.empty()) {
        paramGroup->setVisible(false);
    } else {
        for (const auto& param : parameters) {
            QLineEdit* inputField = new QLineEdit(paramGroup);
            paramLayout->addRow(param, inputField);
        }
        paramGroup->setVisible(true);
    }
}