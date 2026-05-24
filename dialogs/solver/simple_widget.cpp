#include "simple_widget.h"

SimpleWidget::SimpleWidget(QWidget *parent): QWidget(parent) {

    // Create layout
    QFormLayout* layout = new QFormLayout(this);
    layout->setSpacing(20);
    setLayout(layout);

    // Non-orthogonal correctors
    m_nNonOrthogonalCorrectors = new QSpinBox(this);
    m_nNonOrthogonalCorrectors->setRange(0, 50);
    m_nNonOrthogonalCorrectors->setValue(0);
    m_nNonOrthogonalCorrectors->setToolTip(tr("Additional corrections for highly skewed meshes. 0 is standard for hex meshes."));
    layout->addRow(tr("Number of non-orthogonal correctors: "), m_nNonOrthogonalCorrectors);

    // Consistency
    m_consistentCheck = new QCheckBox(this);
    layout->addRow(tr("Enable consistent SIMPLE algorithm: "), m_consistentCheck);
    m_consistentCheck->setToolTip(tr("Uses a modified version of the SIMPLE algorithm that improves the coupling between pressure and velocity."));

    // Reference cell index
    m_pRefCell = new QSpinBox(this);
    m_pRefCell->setRange(0, INT_MAX);
    m_pRefCell->setValue(0);
    m_pRefCell->setToolTip(tr("Cell index used to set the pressure level in fully enclosed domains."));
    layout->addRow(tr("Reference cell index for pressure: "), m_pRefCell);

    // Pressure at reference cell
    m_pRefValue = new QDoubleSpinBox(this);
    m_pRefValue->setRange(-1e9, 1e9);
    m_pRefValue->setDecimals(5);
    m_pRefValue->setValue(0.0);
    m_pRefValue->setToolTip(tr("Fixed pressure value enforced at the reference cell."));
    layout->addRow(tr("Pressure at reference cell: "), m_pRefValue);
}


