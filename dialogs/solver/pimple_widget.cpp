#include "pimple_widget.h"

PimpleWidget::PimpleWidget(QWidget *parent): QWidget(parent) {

    // Create layout
    QFormLayout* layout = new QFormLayout(this);
    layout->setSpacing(20);
    setLayout(layout);

    // Outer correctors
    m_nOuterCorrectors = new QSpinBox(this);
    m_nOuterCorrectors->setRange(1, 100);
    m_nOuterCorrectors->setValue(1);
    m_nOuterCorrectors->setToolTip(tr("Number of outer loops across the entire system of equations. Set >1 for large Courant numbers."));
    layout->addRow(tr("Number of outer correctors: "), m_nOuterCorrectors);

    // Correctors
    m_nCorrectors = new QSpinBox(this);
    m_nCorrectors->setRange(1, 100);
    m_nCorrectors->setValue(2);
    m_nCorrectors->setToolTip(tr("Number of pressure correction loops per outer iteration. Standard is 2."));
    layout->addRow(tr("Number of correctors: "), m_nCorrectors);

    // Non-orthogonal correctors
    m_nNonOrthogonalCorrectors = new QSpinBox(this);
    m_nNonOrthogonalCorrectors->setRange(0, 50);
    m_nNonOrthogonalCorrectors->setValue(0);
    m_nNonOrthogonalCorrectors->setToolTip(tr("Additional corrections for highly skewed meshes. 0 is standard for hex meshes."));
    layout->addRow(tr("Number of non-orthogonal correctors: "), m_nNonOrthogonalCorrectors);

    // Ref Cell
    m_pRefCell = new QSpinBox(this);
    m_pRefCell->setRange(0, INT_MAX);
    m_pRefCell->setValue(0);
    m_pRefCell->setToolTip(tr("Cell index used to pin the pressure level in fully enclosed domains."));
    layout->addRow(tr("Reference cell index for pressure: "), m_pRefCell);

    // Pressure at reference cell
    m_pRefValue = new QDoubleSpinBox(this);
    m_pRefValue->setRange(-1e9, 1e9);
    m_pRefValue->setDecimals(5);
    m_pRefValue->setValue(0.0);
    m_pRefValue->setToolTip(tr("The fixed pressure value enforced at the reference cell."));
    layout->addRow(tr("Pressure at reference cell: "), m_pRefValue);
}


